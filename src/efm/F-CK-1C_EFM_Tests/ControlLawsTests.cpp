#include "TestFileUtils.h"
#include "TestHarness.h"

#include "Common/Configuration/Configuration.h"
#include "Core/Systems/FlightControlComputer/Components/ControlLaws/ControlLaws.h"
#include "Core/Systems/FlightControlComputer/Database/ControlLawData.h"
#include "Core/Systems/FlightControlComputer/Database/InputOutputData.h"
#include "Core/Systems/FlightControlComputer/Database/SystemStatusTables.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems::Flcc;

constexpr double kTolerance = 1.0e-12;
constexpr char kIdentityAxis[] =
    R"({"gain":1,"curve":[[-1,-1],[0,0],[1,1]]})";

/// @brief 在暫存 FM 目錄提供控制律設定，並收集載入診斷。
struct ControlLawFixture
{
    TestFiles::TemporaryDirectory root{"clw"};
    std::vector<Configuration::Diagnostic> diagnostics;

    ControlLawFixture()
    {
        if (!root.valid())
            return;
        std::filesystem::create_directories(root.path() / "FM" / "FLCC");
        Configuration::initialize(root.path() / "FM",
            [this](const Configuration::Diagnostic& diagnostic)
            {
                diagnostics.push_back(diagnostic);
            });
    }

    ~ControlLawFixture() { Configuration::shutdown(); }

    void write(const char* pitch, const char* roll = kIdentityAxis,
               const char* yaw = kIdentityAxis)
    {
        const std::string source =
            std::string("{\"direct_mapping\":{\"pitch\":") + pitch +
            ",\"roll\":" + roll + ",\"yaw\":" + yaw + "}}";
        TestFiles::write_text(root.path() / "FM" / "FLCC" / "ControlLaws.jsonc",
                              source.c_str());
    }
};

void test_identity_mapping_and_axis_signs(Tests::Context& tests)
{
    ControlLawFixture fixture;
    TEST_EXPECT(tests, fixture.root.valid());
    if (!fixture.root.valid())
        return;
    fixture.write(kIdentityAxis);
    ControlLawData data;
    InputOutputData io;
    SystemStatusTables status;
    ControlLaws laws(io, data, status);

    const std::array<std::array<double, 3>, 4> inputs = {{
        {{0.0, 0.0, 0.0}}, {{1.0, 1.0, 1.0}},
        {{-1.0, -1.0, -1.0}}, {{0.25, -0.5, 0.75}}
    }};
    for (const auto& input : inputs)
    {
        io.input.pilot_control.pitch_axis_normalized = input[0];
        io.input.pilot_control.roll_axis_normalized = input[1];
        io.input.pilot_control.yaw_axis_normalized = input[2];
        laws.step(std::chrono::nanoseconds{0}, 1.0 / 64.0);
        const auto& command = io.output.actuator_command;
        TEST_EXPECT_NEAR(tests, command.symmetric_stabilator_demand_normalized,
                         input[0], kTolerance);
        TEST_EXPECT_NEAR(tests, command.differential_flaperon_demand_normalized,
                         input[1], kTolerance);
        TEST_EXPECT_NEAR(tests, command.rudder_demand_normalized, input[2], kTolerance);
        TEST_EXPECT(tests, !data.state.direct_mapping.pitch.limited);
        TEST_EXPECT(tests, !data.state.direct_mapping.roll.limited);
        TEST_EXPECT(tests, !data.state.direct_mapping.yaw.limited);
    }
    TEST_EXPECT(tests, fixture.diagnostics.empty());
}

void test_curve_gain_and_output_limit(Tests::Context& tests)
{
    ControlLawFixture fixture;
    TEST_EXPECT(tests, fixture.root.valid());
    if (!fixture.root.valid())
        return;
    fixture.write(
        R"({"gain":2,"curve":[[-1,-1],[-0.5,-0.25],[0,0],[0.5,0.25],[1,1]]})",
        R"({"gain":0.5,"curve":[[-1,-1],[0,0],[1,1]]})",
        R"({"gain":0,"curve":[[-1,-1],[0,0],[1,1]]})");
    ControlLawData data;
    InputOutputData io;
    SystemStatusTables status;
    ControlLaws laws(io, data, status);
    io.input.pilot_control.pitch_axis_normalized = 0.25;
    io.input.pilot_control.roll_axis_normalized = -0.8;
    io.input.pilot_control.yaw_axis_normalized = 1.0;
    laws.step(std::chrono::nanoseconds{0}, 1.0 / 64.0);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.symmetric_stabilator_demand_normalized,
                     0.25, kTolerance);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.differential_flaperon_demand_normalized,
                     -0.4, kTolerance);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.rudder_demand_normalized, 0.0, kTolerance);
    TEST_EXPECT(tests, !data.state.direct_mapping.pitch.limited);

    for (double input : {-1.0, 1.0})
    {
        io.input.pilot_control.pitch_axis_normalized = input;
        laws.step(std::chrono::nanoseconds{0}, 1.0 / 64.0);
        TEST_EXPECT_NEAR(tests, data.state.direct_mapping.pitch.demand_before_limit_normalized,
                         2.0 * input, kTolerance);
        TEST_EXPECT_NEAR(tests, io.output.actuator_command.symmetric_stabilator_demand_normalized,
                         input, kTolerance);
        TEST_EXPECT(tests, data.state.direct_mapping.pitch.limited);
    }

    // 每次運算完整更新狀態，中立需求不沿用前次的限幅旗標。
    io.input.pilot_control.pitch_axis_normalized = 0.0;
    laws.step(std::chrono::nanoseconds{0}, 1.0 / 64.0);
    TEST_EXPECT_NEAR(tests, data.state.direct_mapping.pitch.demand_before_limit_normalized,
                     0.0, kTolerance);
    TEST_EXPECT(tests, !data.state.direct_mapping.pitch.limited);
}

void test_input_limits_and_nonfinite_neutral(Tests::Context& tests)
{
    ControlLawFixture fixture;
    TEST_EXPECT(tests, fixture.root.valid());
    if (!fixture.root.valid())
        return;
    fixture.write(kIdentityAxis);
    ControlLawData data;
    InputOutputData io;
    SystemStatusTables status;
    ControlLaws laws(io, data, status);
    io.input.pilot_control.pitch_axis_normalized = 2.0;
    io.input.pilot_control.roll_axis_normalized = -2.0;
    io.input.pilot_control.yaw_axis_normalized = std::numeric_limits<double>::quiet_NaN();
    laws.step(std::chrono::nanoseconds{0}, 1.0 / 64.0);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.symmetric_stabilator_demand_normalized,
                     1.0, kTolerance);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.differential_flaperon_demand_normalized,
                     -1.0, kTolerance);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.rudder_demand_normalized, 0.0, kTolerance);

    io.input.pilot_control.pitch_axis_normalized = std::numeric_limits<double>::infinity();
    io.input.pilot_control.roll_axis_normalized = -std::numeric_limits<double>::infinity();
    laws.step(std::chrono::nanoseconds{0}, 1.0 / 64.0);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.symmetric_stabilator_demand_normalized,
                     0.0, kTolerance);
    TEST_EXPECT_NEAR(tests, io.output.actuator_command.differential_flaperon_demand_normalized,
                     0.0, kTolerance);
}

void test_invalid_axis_configuration_reports_field(Tests::Context& tests)
{
    ControlLawFixture fixture;
    TEST_EXPECT(tests, fixture.root.valid());
    if (!fixture.root.valid())
        return;
    struct InvalidAxis
    {
        const char* source;
        const char* field;
    };
    const InvalidAxis invalid_axes[] = {
        {R"({"curve":[[-1,-1],[0,0],[1,1]]})", "/gain"},
        {R"({"gain":-1,"curve":[[-1,-1],[0,0],[1,1]]})", "/gain"},
        {R"({"gain":1,"curve":[[-1,-1],[0,0],[1,1]],"extra":0})", "/extra"},
        {R"({"gain":1,"curve":[[-1,-1],[1,1]]})", "/curve"},
        {R"({"gain":1,"curve":[[-1,-1],[0],[1,1]]})", "/curve/1"},
        {R"({"gain":1,"curve":[[-0.5,-0.5],[0,0],[1,1]]})", "/curve/0/0"},
        {R"({"gain":1,"curve":[[-1,-1],[0,0],[0.5,0.5]]})", "/curve/2/0"},
        {R"({"gain":1,"curve":[[-1,-1],[0.5,0.5],[0,0],[1,1]]})", "/curve/2/0"},
        {R"({"gain":1,"curve":[[-1,-1],[-0.5,-0.5],[1,1]]})", "/curve"},
        {R"({"gain":1,"curve":[[-1,-1],[0,0.1],[1,1]]})", "/curve/1/1"},
        {R"({"gain":1,"curve":[[-1,1],[0,0],[1,1]]})", "/curve/0/1"},
        {R"({"gain":1,"curve":[[-1,-1],[0,0],[1,1.1]]})", "/curve/2/1"}
    };
    for (const auto& invalid : invalid_axes)
    {
        fixture.write(invalid.source);
        fixture.diagnostics.clear();
        const auto result = Configuration::load<ControlLawConfiguration>("FLCC/ControlLaws.jsonc");
        TEST_EXPECT(tests, !result.succeeded());
        TEST_EXPECT(tests, fixture.diagnostics.size() == 1);
        if (!result.succeeded())
        {
            TEST_EXPECT(tests, result.error().field ==
                         std::string("/direct_mapping/pitch") + invalid.field);
            TEST_EXPECT(tests, result.error().file.filename() == "ControlLaws.jsonc");
            if (fixture.diagnostics.size() == 1)
                TEST_EXPECT(tests, result.error().message() == fixture.diagnostics[0].message());
        }
    }
}
}

void run_control_laws_tests(Tests::Context& tests)
{
    test_identity_mapping_and_axis_signs(tests);
    test_curve_gain_and_output_limit(tests);
    test_input_limits_and_nonfinite_neutral(tests);
    test_invalid_axis_configuration_reports_field(tests);
}
