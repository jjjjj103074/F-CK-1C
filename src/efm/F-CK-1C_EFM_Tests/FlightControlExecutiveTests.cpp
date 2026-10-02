#include "SystemPipelineTestFixture.h"

#include "Core/Systems/FlightControlComputer/Components/Executive/Executive.h"
#include "Core/Systems/FlightControlComputer/FlightControlComputer.h"

#include <array>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace Core::Systems::Flcc;
using namespace std::chrono;

constexpr double kTolerance = 1.0e-12;

static_assert(!std::is_copy_constructible<FlightControlComputer>::value,
    "FLCC must preserve database and task target addresses.");
static_assert(!std::is_move_constructible<FlightControlComputer>::value,
    "Moving FLCC would invalidate constructor-bound references.");
static_assert(std::is_const<decltype(std::declval<ExecutiveTask &>().definition)>::value,
    "A stored task definition must remain immutable.");
static_assert(std::is_same<decltype(std::declval<const ExecutiveTables &>().tasks()),
    const std::vector<ExecutiveTask> &>::value,
    "Observers must receive read-only task rows.");

/// @brief 記錄任務實例收到的名稱與預定時間，供排程行為驗證。
struct Call
{
    std::string task;
    TaskScheduledTime scheduled_time;
    double dt_s;
};

/// @brief 測試用元件；只提供入口，不宣告排程頻率或順序。
struct Recorder
{
    std::string id;
    std::vector<Call> &calls;

    /// @brief 記錄主要入口的呼叫。
    void step(TaskScheduledTime scheduled_time, double dt_s)
    {
        calls.push_back({id, scheduled_time, dt_s});
    }

    /// @brief 記錄相同實例的第二個入口，驗證成員函式綁定。
    void secondary_step(TaskScheduledTime scheduled_time, double dt_s)
    {
        calls.push_back({id + ".secondary", scheduled_time, dt_s});
    }
};

/// @brief 判斷操作是否拋出指定型別的例外。
template <typename Exception, typename Action>
bool throws(const Action &action)
{
    try
    {
        action();
    }
    catch (const Exception &)
    {
        return true;
    }
    return false;
}

/// @brief 驗證明確填表可綁定同一實例的不同入口。
void test_multiple_member_functions(Tests::Context &context)
{
    std::vector<Call> calls;
    Recorder recorder{"bound_object", calls};
    ExecutiveTables tables;
    Executive executive(tables, {
        {"primary", TaskFrequency(4), recorder, &Recorder::step},
        {"secondary", TaskFrequency(2), recorder, &Recorder::secondary_step}});
    executive.step(seconds(1));
    TEST_EXPECT(context, calls.size() == 6);
    if (calls.size() != 6) return;
    TEST_EXPECT(context, calls[0].task == "bound_object");
    TEST_EXPECT(context, calls[0].scheduled_time == milliseconds(250));
    TEST_EXPECT(context, calls[2].task == "bound_object.secondary");
    TEST_EXPECT(context, calls[2].scheduled_time == milliseconds(500));
    TEST_EXPECT_NEAR(context, calls[2].dt_s, 0.5, kTolerance);
}

/// @brief 驗證同一列可觀察設定、次數、下次時間與衍生剩餘時間。
void test_task_row_and_first_tick(Tests::Context &context)
{
    std::vector<Call> calls;
    Recorder recorder{"first", calls};
    ExecutiveTables tables;
    Executive executive(tables, {
        {"first.step", TaskFrequency(64), recorder, &Recorder::step}});
    const ExecutiveTask &task = tables.tasks().front();
    TEST_EXPECT(context, task.definition.id == "first.step");
    TEST_EXPECT(context, task.definition.frequency.numerator() == 64);
    TEST_EXPECT(context, task.state.completed_ticks == 0);
    TEST_EXPECT(context, task.state.next_due == nanoseconds(15'625'000));

    executive.step(nanoseconds(0));
    executive.step(nanoseconds(15'624'999));
    TEST_EXPECT(context, calls.empty());
    executive.step(nanoseconds(15'625'000));
    executive.step(milliseconds(20));
    executive.step(milliseconds(20));
    TEST_EXPECT(context, calls.size() == 1);
    TEST_EXPECT(context, &task == &tables.tasks().front());
    TEST_EXPECT(context, task.state.completed_ticks == 1);
    TEST_EXPECT(context, task.state.next_due == nanoseconds(31'250'000));
    TEST_EXPECT(context, tables.scheduler_state().advanced_through == milliseconds(20));
    TEST_EXPECT(context, task.time_until_next(tables.scheduler_state().advanced_through) ==
        nanoseconds(11'250'000));
    TEST_EXPECT(context, task.time_until_next(milliseconds(32)) == nanoseconds(-750'000));
    TEST_EXPECT(context, throws<std::invalid_argument>([&]
        { task.time_until_next(nanoseconds(-1)); }));
    TEST_EXPECT(context, throws<std::invalid_argument>([&]
        { executive.step(nanoseconds(-1)); }));
    TEST_EXPECT(context, throws<std::invalid_argument>([&]
        { executive.step(nanoseconds(15'624'999)); }));
    if (!calls.empty())
    {
        TEST_EXPECT(context, calls.front().scheduled_time == nanoseconds(15'625'000));
        TEST_EXPECT_NEAR(context, calls.front().dt_s, 1.0 / 64.0, kTolerance);
    }
}

/// @brief 使用指定的外層呼叫時間，執行相同的多頻率固定任務表。
std::vector<Call> run_schedule(const std::vector<nanoseconds> &targets)
{
    std::vector<Call> calls;
    Recorder fast{"fast", calls}, medium{"medium", calls};
    Recorder slow{"slow", calls}, fractional{"fractional", calls};
    ExecutiveTables tables;
    Executive executive(tables, {
        {"slow.step", TaskFrequency(3), slow, &Recorder::step},
        {"fast.step", TaskFrequency(64), fast, &Recorder::step},
        {"fractional.step", TaskFrequency(5, 2), fractional, &Recorder::step},
        {"medium.step", TaskFrequency(32), medium, &Recorder::step}});
    for (const auto target : targets) executive.step(target);
    return calls;
}

/// @brief 驗證多頻率與欠期補跑不依賴外層呼叫次數，並保持時間先後關係。
void test_hz_and_catch_up(Tests::Context &context)
{
    const auto expected = run_schedule({seconds(1)});
    TEST_EXPECT(context, expected.size() == 64 + 32 + 3 + 2);
    std::size_t fast_count = 0, medium_count = 0, slow_count = 0, fractional_count = 0;
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        const auto &call = expected[i];
        if (i != 0) TEST_EXPECT(context,
            expected[i - 1].scheduled_time <= call.scheduled_time);
        if (call.task == "fast")
        {
            ++fast_count;
            TEST_EXPECT_NEAR(context, call.dt_s, 1.0 / 64.0, kTolerance);
        }
        else if (call.task == "medium") ++medium_count;
        else if (call.task == "slow")
        {
            ++slow_count;
            TEST_EXPECT_NEAR(context, call.dt_s, 1.0 / 3.0, kTolerance);
        }
        else if (call.task == "fractional")
        {
            ++fractional_count;
            TEST_EXPECT(context, call.scheduled_time ==
                milliseconds(400 * fractional_count));
        }
    }
    TEST_EXPECT(context, fast_count == 64 && medium_count == 32);
    TEST_EXPECT(context, slow_count == 3 && fractional_count == 2);

    for (const int host_rate : {30, 60, 144})
    {
        std::vector<nanoseconds> targets;
        for (int tick = 1; tick <= host_rate; ++tick)
            targets.push_back(nanoseconds(1'000'000'000LL * tick / host_rate));
        const auto actual = run_schedule(targets);
        TEST_EXPECT(context, actual.size() == expected.size());
        for (std::size_t i = 0; i < actual.size() && i < expected.size(); ++i)
        {
            TEST_EXPECT(context, actual[i].task == expected[i].task);
            TEST_EXPECT(context, actual[i].scheduled_time ==
                expected[i].scheduled_time);
        }
    }
    const auto irregular = run_schedule({
        nanoseconds(0), milliseconds(17), milliseconds(193),
        milliseconds(800), milliseconds(800), seconds(1)});
    TEST_EXPECT(context, irregular.size() == expected.size());
    for (std::size_t i = 0; i < irregular.size() && i < expected.size(); ++i)
    {
        TEST_EXPECT(context, irregular[i].task == expected[i].task);
        TEST_EXPECT(context, irregular[i].scheduled_time ==
            expected[i].scheduled_time);
    }
}

/// @brief 驗證同時到期直接使用填表順序，不依名稱或元件種類重新排序。
void test_same_time_uses_row_order(Tests::Context &context)
{
    std::vector<Call> calls;
    Recorder control{"control", calls}, input{"input", calls}, monitor{"monitor", calls};
    ExecutiveTables tables;
    Executive executive(tables, {
        {"z_control", TaskFrequency(4), control, &Recorder::step},
        {"a_input", TaskFrequency(4), input, &Recorder::step},
        {"m_monitor", TaskFrequency(4), monitor, &Recorder::step}});
    executive.step(milliseconds(250));
    const std::array<const char *, 3> expected{"control", "input", "monitor"};
    TEST_EXPECT(context, calls.size() == expected.size());
    for (std::size_t i = 0; i < calls.size() && i < expected.size(); ++i)
        TEST_EXPECT(context, calls[i].task == expected[i]);
}

/// @brief 驗證 3 Hz 在一小時內不累積週期捨入誤差，進度可由同一列查看。
void test_no_accumulated_rounding(Tests::Context &context)
{
    std::vector<Call> calls;
    Recorder recorder{"thirds", calls};
    ExecutiveTables tables;
    Executive executive(tables, {
        {"thirds", TaskFrequency(3), recorder, &Recorder::step}});
    executive.step(hours(1) - nanoseconds(1));
    TEST_EXPECT(context, calls.size() == 10'799);
    executive.step(hours(1));
    TEST_EXPECT(context, calls.size() == 10'800);
    TEST_EXPECT(context, tables.tasks().front().state.completed_ticks == 10'800);
    TEST_EXPECT(context, tables.tasks().front().state.next_due == nanoseconds(3'600'333'333'333));
    if (!calls.empty()) TEST_EXPECT(context, calls.back().scheduled_time == hours(1));
}

/// @brief 驗證分數 Hz 的整數精度、約分、範圍與中間乘法不溢位。
void test_exact_frequency_arithmetic(Tests::Context &context)
{
    const TaskFrequency normalized(64, 2);
    TEST_EXPECT(context, normalized.numerator() == 32);
    TEST_EXPECT(context, normalized.denominator() == 1);
    TEST_EXPECT(context, normalized.tick_time(1) == nanoseconds(31'250'000));
    TEST_EXPECT(context, TaskFrequency(5, 2).tick_time(1) == milliseconds(400));
    TEST_EXPECT(context, TaskFrequency(3).tick_time(0) == nanoseconds(0));
    TEST_EXPECT(context, TaskFrequency(3).tick_time(27'000'000'001ULL) ==
        nanoseconds(9'000'000'000'333'333'333LL));
    TEST_EXPECT(context, TaskFrequency(4'000'000'000U, 5).tick_time(
        6'000'000'000'000'000'000ULL) == nanoseconds(7'500'000'000'000'000'000LL));

    const auto maximum = (std::numeric_limits<nanoseconds::rep>::max)();
    TEST_EXPECT(context, TaskFrequency(1'000'000'000).tick_time(
        static_cast<std::uint64_t>(maximum)) == nanoseconds(maximum));
    TEST_EXPECT(context, throws<std::overflow_error>([&]
        { TaskFrequency(1'000'000'000).tick_time(static_cast<std::uint64_t>(maximum) + 1); }));
    TEST_EXPECT(context, TaskFrequency(3).tick_time(27'670'116'110ULL) ==
        nanoseconds(9'223'372'036'666'666'666LL));
    TEST_EXPECT(context, throws<std::overflow_error>([]
        { TaskFrequency(3).tick_time(27'670'116'111ULL); }));
    TEST_EXPECT(context, throws<std::invalid_argument>([] { TaskFrequency invalid(0); }));
    TEST_EXPECT(context, throws<std::invalid_argument>([] { TaskFrequency invalid(1, 0); }));
    TEST_EXPECT(context, throws<std::invalid_argument>([]
        { TaskFrequency invalid(1'000'000'001); }));
}

/// @brief 驗證無效設定被拒絕、初始化只有一次，且初始化失敗不留下部分任務。
void test_definition_validation_and_initialization(Tests::Context &context)
{
    std::vector<Call> calls;
    Recorder recorder{"validation", calls};
    TEST_EXPECT(context, throws<std::invalid_argument>([&]
        { TaskDefinition invalid("", TaskFrequency(4), recorder, &Recorder::step); }));
    TEST_EXPECT(context, throws<std::invalid_argument>([&]
    {
        void (Recorder::*missing)(TaskScheduledTime, double) = nullptr;
        TaskDefinition invalid("missing", TaskFrequency(4), recorder, missing);
    }));

    ExecutiveTables tables;
    TEST_EXPECT(context, throws<std::invalid_argument>([&]
    {
        Executive invalid(tables, {
            {"duplicate", TaskFrequency(4), recorder, &Recorder::step},
            {"duplicate", TaskFrequency(2), recorder, &Recorder::step}});
    }));
    TEST_EXPECT(context, tables.tasks().empty());
    Executive executive(tables, {
        {"valid", TaskFrequency(4), recorder, &Recorder::step}});
    TEST_EXPECT(context, throws<std::logic_error>([&] { Executive duplicate(tables, {}); }));
    executive.step(seconds(1));
    TEST_EXPECT(context, calls.size() == 4);
    TEST_EXPECT(context, tables.tasks().front().state.completed_ticks == 4);

    ExecutiveTables empty;
    Executive empty_executive(empty, {});
    empty_executive.step(seconds(1));
    TEST_EXPECT(context, empty.tasks().empty());
    TEST_EXPECT(context, empty.scheduler_state().advanced_through == seconds(1));
}

/// @brief 驗證 FLCC 骨架可完成 Pipeline 排程，但不宣稱控制功能可用。
void test_pipeline_skeleton(Tests::Context &context)
{
    using namespace SystemPipelineTest;
    const SystemDefinition inputs{
        "flcc_test_inputs",
        [](SystemSetup &setup)
        {
            // 飛行觀測由 Pipeline 發布；此來源只提供其他四個元件輸入。
            setup.publish(AircraftDataKeys::kPilotControlSignal, PilotControlSignal{});
            setup.publish(AircraftDataKeys::kThrottleLeverSignal, ThrottleLeverSignal{0.8, 0.9});
            setup.publish(AircraftDataKeys::kLandingGearData, LandingGearData{});
            setup.publish(AircraftDataKeys::kFlightControlActuatorState, FlightControlActuatorState{});
        }, no_step()};
    auto setup = flight_setup();
    SystemPipeline pipeline(setup, {entry(inputs), {
        "flight_control_computer",
        [](const FlightSetupContext &)
        {
            return std::make_unique<FlightControlComputer>(ThrottleLeverSignal{0.2, 0.3});
        }}});
    const auto after = step_pipeline_to(pipeline, {}, {}, seconds(1));
    TEST_EXPECT(context, pipeline.advanced_through() == seconds(1));
    TEST_EXPECT(context, !after.read(AircraftDataKeys::kFlightControlComputerSnapshot).status.available);
    TEST_EXPECT(context, !after.read(AircraftDataKeys::kAutomaticFlightControlSnapshot).status.available);
    TEST_EXPECT_NEAR(context,
        after.read(AircraftDataKeys::kFlightControlActuatorCommand).symmetric_stabilator_demand_rad,
        0.0, kTolerance);
    TEST_EXPECT_NEAR(context,
        after.read(AircraftDataKeys::kEngineThrottleCommand).left_normalized, 0.2, kTolerance);
    TEST_EXPECT_NEAR(context,
        after.read(AircraftDataKeys::kEngineThrottleCommand).right_normalized, 0.3, kTolerance);
}
}

/// @brief 執行 FLCC 組裝邊界、排程資料與 Executive 時間行為測試。
void run_flight_control_executive_tests(Tests::Context &context)
{
    test_multiple_member_functions(context);
    test_task_row_and_first_tick(context);
    test_hz_and_catch_up(context);
    test_same_time_uses_row_order(context);
    test_no_accumulated_rounding(context);
    test_exact_frequency_arithmetic(context);
    test_definition_validation_and_initialization(context);
    test_pipeline_skeleton(context);
}
