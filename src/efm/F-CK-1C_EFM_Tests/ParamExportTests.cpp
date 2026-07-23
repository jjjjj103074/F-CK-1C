#include "TestFileUtils.h"
#include "TestHarness.h"

#include "DcsBridge/Internal/EfmEventReporter.h"
#include "DcsBridge/Internal/ParamExport.h"
#include "DcsBridge/Internal/ParamExporter.h"

namespace
{
constexpr double kTolerance = 1e-9;

struct ParamExpectation
{
	unsigned index;
	double value;
};

struct ParamExporterFixture
{
	ParamExporterFixture()
		: root("pex"),
		root_path(root.path().string()),
		event_log(root_path.c_str()),
		event_reporter(event_log, output_store),
		exporter(event_reporter)
	{
	}

	TestFiles::TemporaryDirectory root;
	std::string root_path;
	DcsBridge::Internal::OutputStore output_store;
	DcsBridge::Internal::EventLog event_log;
	DcsBridge::Internal::EfmEventReporter event_reporter;
	DcsBridge::Internal::ParamExporter exporter;
};

DcsBridge::ParamExportState make_state()
{
	DcsBridge::ParamExportState state = {};
	state.suspension_feedback_available = true;
	state.atmosphere_available = true;
	state.any_weight_on_wheels = true;
	state.gear_pos = 1.0;
	state.nose_wheel_steering = -0.25;
	state.wheel_spin[0] = 1.0;
	state.wheel_spin[1] = 2.0;
	state.wheel_spin[2] = 3.0;
	state.wheel_brake_left = 0.6;
	state.wheel_brake_right = 0.7;
	state.pitch_input = 0.2;
	state.roll_input = -0.3;
	state.yaw_input = 0.4;
	state.left_engine_switch = true;
	state.left_throttle_input = 0.8;
	state.left_throttle_output = 0.9;
	state.left_engine_power_readout = 0.5;
	state.left_thrust_force = 12000.0;
	state.atmosphere_temperature = 288.0;
	state.internal_fuel = 900.0;
	state.total_fuel = 1100.0;
	state.total_fuel_flow = 4.0;
	return state;
}

double require_param(
	Tests::Context& context,
	unsigned index,
	const DcsBridge::ParamExportState& state)
{
	const std::optional<double> value = DcsBridge::get_param(index, state);
	TEST_EXPECT(context, value.has_value());
	return value.value_or(0.0);
}

void test_wheel_and_control_params(Tests::Context& context)
{
	using namespace DcsIds::Params;
	const DcsBridge::ParamExportState state = make_state();
	TEST_EXPECT_NEAR(context, require_param(context, NoseWheelYaw, state), -0.25, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, RightWheelSpin, state), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, LeftBrakeMoment, state), 0.6, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, StickPitch, state), 0.2, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, StickRoll, state), -0.3, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, RudderPedals, state), -0.4, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, ThrottleLeft, state), 0.8, kTolerance);
}

void test_service_and_engine_params(Tests::Context& context)
{
	using namespace DcsIds::Params;
	const DcsBridge::ParamExportState state = make_state();
	TEST_EXPECT_NEAR(context, require_param(context, InternalFuel, state), 900.0, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, TotalFuel, state), 1100.0, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, ApuRelatedRpm, state), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, LeftEngineCoreRpm, state), 9929.25, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, LeftEngineRpm, state), 5545.125, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, LeftEngineCombustion, state), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, require_param(context, LeftEngineThrust, state), 12000.0, kTolerance);
	TEST_EXPECT(context, !DcsBridge::get_param(999999, state).has_value());
}

void test_declared_compatibility_params(Tests::Context& context)
{
	using namespace DcsIds::Params;
	const DcsBridge::ParamExportState state = make_state();
	const ParamExpectation expected[] = {
		{ ApuCoreRelatedRpm, 1.0 },
		{ LeftPropellerPitch, 0.0 }, { RightPropellerPitch, 0.0 },
		{ LeftEngineFuelFlow, 2.0 }, { RightEngineFuelFlow, 2.0 },
		{ LeftEngineFanPhase, 0.0 }, { RightEngineFanPhase, 0.0 },
		{ LeftEngineFlowSpeedCompatibility, 0.0 },
		{ RightEngineFlowSpeedCompatibility, 0.0 },
		{ LeftWheelSpin, 0.0 }, { RightWheelSpin, 0.0 },
		{ PitchForceFactor, 0.0 }, { PitchForceShakeAmplitude, 0.0 },
		{ PitchForceShakeFrequency, 0.0 }, { RollForceCenter, 0.0 },
		{ RollForceFactor, 0.0 }, { RollForceShakeAmplitude, 0.0 },
		{ RollForceShakeFrequency, 0.0 }, { CockpitPressurization, 0.0 },
		{ InterruptRefuel, 0.0 },
		{ UnknownCompatibility2134, 0.0 }, { UnknownCompatibility2135, 0.0 },
		{ UnknownCompatibility2136, 0.0 }, { UnknownCompatibility2137, 0.0 }
	};
	for (const ParamExpectation& item : expected)
	{
		TEST_EXPECT_NEAR(
			context,
			require_param(context, item.index, state),
			item.value,
			kTolerance);
	}
}

void test_missing_required_data_is_identified(Tests::Context& context)
{
	using namespace DcsIds::Params;
	DcsBridge::ParamExportState state = make_state();
	state.suspension_feedback_available = false;
	const std::optional<DcsBridge::ParamDataCategory> suspension =
		DcsBridge::missing_param_data(NoseWheelYaw, state);
	TEST_EXPECT(context, suspension == DcsBridge::ParamDataCategory::Suspension);
	TEST_EXPECT(context, !DcsBridge::missing_param_data(NoseWheelSpin, state));
	state.suspension_feedback_available = true;
	state.atmosphere_available = false;
	const std::optional<DcsBridge::ParamDataCategory> atmosphere =
		DcsBridge::missing_param_data(LeftEngineTemperature, state);
	TEST_EXPECT(context, atmosphere == DcsBridge::ParamDataCategory::Atmosphere);
	TEST_EXPECT(context, !DcsBridge::missing_param_data(InternalFuel, state));
}

void test_param_export_values_and_unknown(Tests::Context& context)
{
	using namespace DcsIds::Params;
	Core::FrameOutput output;
	output.simulation_time_s = 1.0;
	output.availability.atmosphere = true;
	output.availability.suspension[0] = true;
	output.fuel.total_fuel_flow = 4.0;
	const DcsBridge::ParamExportAvailabilityHistory history;
	const DcsBridge::ParamExportResult implemented =
		DcsBridge::resolve_param(LeftEngineFuelFlow, output, history);
	TEST_EXPECT(context, implemented.status == DcsBridge::ParamExportStatus::Value);
	TEST_EXPECT_NEAR(context, implemented.value, 2.0, kTolerance);
	const DcsBridge::ParamExportResult unknown =
		DcsBridge::resolve_param(999999, output, history);
	TEST_EXPECT(context, unknown.status == DcsBridge::ParamExportStatus::Unknown);
	TEST_EXPECT_NEAR(context, unknown.value, 0.0, kTolerance);
}

void test_param_data_availability_classification(Tests::Context& context)
{
	using namespace DcsIds::Params;
	Core::FrameOutput output;
	output.simulation_time_s = 5.0;
	output.availability.atmosphere = false;
	output.availability.suspension[0] = false;
	const DcsBridge::ParamExportAvailabilityHistory initial_history;
	const DcsBridge::ParamExportResult initial =
		DcsBridge::resolve_param(
			LeftEngineTemperature, output, initial_history);
	TEST_EXPECT(
		context,
		initial.status == DcsBridge::ParamExportStatus::StartCompatibility);
	TEST_EXPECT_NEAR(context, initial.value, 0.0, kTolerance);
	const DcsBridge::ParamExportResult initial_suspension =
		DcsBridge::resolve_param(NoseWheelYaw, output, initial_history);
	TEST_EXPECT(
		context,
		initial_suspension.status ==
			DcsBridge::ParamExportStatus::StartCompatibility);
	DcsBridge::ParamExportAvailabilityHistory available_history;
	available_history.atmosphere_available = true;
	available_history.nose_suspension_available = true;
	const DcsBridge::ParamExportResult missing =
		DcsBridge::resolve_param(
			LeftEngineTemperature, output, available_history);
	TEST_EXPECT(
		context,
		missing.status == DcsBridge::ParamExportStatus::MissingRuntimeData);
	TEST_EXPECT(
		context,
		missing.missing_data == DcsBridge::ParamDataCategory::Atmosphere);
	const DcsBridge::ParamExportResult missing_suspension =
		DcsBridge::resolve_param(NoseWheelYaw, output, available_history);
	TEST_EXPECT(
		context,
		missing_suspension.status ==
			DcsBridge::ParamExportStatus::MissingRuntimeData);
	TEST_EXPECT(
		context,
		missing_suspension.missing_data ==
			DcsBridge::ParamDataCategory::Suspension);
}

void test_param_exporter_known_and_unknown_logging(Tests::Context& context)
{
	using namespace DcsIds::Params;
	ParamExporterFixture fixture;
	TEST_EXPECT(context, fixture.root.valid());
	Core::FrameOutput output;
	output.simulation_time_s = 5.0;
	fixture.output_store.publish(output);
	fixture.exporter.observe(output);
	TEST_EXPECT_NEAR(
		context,
		fixture.exporter.read(LeftEngineTemperature, output),
		0.0,
		kTolerance);
	TEST_EXPECT_NEAR(
		context, fixture.exporter.read(ApuCoreRelatedRpm, output), 1.0, kTolerance);
	TEST_EXPECT_NEAR(context, fixture.exporter.read(999999, output), 0.0, kTolerance);
	TEST_EXPECT_NEAR(context, fixture.exporter.read(999999, output), 0.0, kTolerance);
	fixture.event_reporter.log_release(output.simulation_time_s);
	const std::string content = TestFiles::read_text_while_open(
		fixture.root.path() / "log" / "fck1c_efm.log");
	const std::string unknown =
		"callback=ed_fm_get_param index=999999 missing mapping";
	const std::size_t first_unknown = content.find(unknown);
	TEST_EXPECT(context, content.find("index=112 missing") == std::string::npos);
	TEST_EXPECT(context, content.find("index=3 missing") == std::string::npos);
	TEST_EXPECT(context, first_unknown != std::string::npos);
	TEST_EXPECT(
		context,
		content.find(unknown, first_unknown + 1) == std::string::npos);
	TEST_EXPECT(context, content.find(
		"kind=unknown_param id=999999 total=2 flight_release_summary") !=
		std::string::npos);
}

void test_param_exporter_runtime_data_error(Tests::Context& context)
{
	using namespace DcsIds::Params;
	ParamExporterFixture fixture;
	TEST_EXPECT(context, fixture.root.valid());
	Core::FrameOutput available;
	available.simulation_time_s = 6.0;
	available.availability.atmosphere = true;
	available.engines[0].power_readout = 0.5;
	available.flight.atmosphere_temperature_k = 288.0;
	fixture.output_store.publish(available);
	fixture.exporter.observe(available);
	TEST_EXPECT(
		context,
		fixture.exporter.read(LeftEngineTemperature, available) > 288.0);
	Core::FrameOutput missing = available;
	missing.simulation_time_s = 7.0;
	missing.availability.atmosphere = false;
	fixture.output_store.publish(missing);
	fixture.exporter.observe(missing);
	TEST_EXPECT_NEAR(
		context,
		fixture.exporter.read(LeftEngineTemperature, missing),
		0.0,
		kTolerance);
	const std::string content = TestFiles::read_text_while_open(
		fixture.root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(context, content.find(
		"][ERROR] callback=ed_fm_get_param index=112 missing data=atmosphere") !=
		std::string::npos);
}
}

void run_param_export_tests(Tests::Context& context)
{
	test_wheel_and_control_params(context);
	test_service_and_engine_params(context);
	test_declared_compatibility_params(context);
	test_missing_required_data_is_identified(context);
	test_param_export_values_and_unknown(context);
	test_param_data_availability_classification(context);
	test_param_exporter_known_and_unknown_logging(context);
	test_param_exporter_runtime_data_error(context);
}
