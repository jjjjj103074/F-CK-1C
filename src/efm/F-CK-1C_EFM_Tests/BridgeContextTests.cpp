#include "TestFileUtils.h"
#include "TestHarness.h"

#include "../F-CK-1C_EFM/DcsBridge/Internal/BridgeContext.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr std::size_t kConcurrentCallbackCount = 8;
constexpr int kCoreActionsPerCallback = 100;
constexpr double kPreparedStepS = 0.006;
constexpr double kPreparedAltitudeAslM = 1234.0;
constexpr double kPreparedTemperatureK = 275.0;
constexpr double kPreparedSpeedOfSoundMps = 340.0;
constexpr double kPreparedDensityKgM3 = 1.2;
constexpr double kPreparedPressurePa = 101325.0;
constexpr double kPreparedSurfaceHeightM = 100.0;
constexpr unsigned kPreparedSurfaceType = 0;
constexpr double kPreparedSurfaceNormalY = 1.0;
constexpr double kPreparedPositionWorldZM = 321.0;
constexpr double kPreparedMassKg = 10000.0;
constexpr double kPreparedCenterOfMassXM = -0.5;
constexpr double kPreparedInertiaX = 1000.0;
constexpr double kPreparedInertiaY = 2000.0;
constexpr double kPreparedInertiaZ = 3000.0;
constexpr int kPreparedSuspensionIndex = 0;
constexpr double kPreparedSuspensionForceY = 1000.0;
constexpr double kPreparedSuspensionIntegrity = 1.0;
constexpr double kPreparedSuspensionCompressionM = 0.1;
constexpr double kPreviousFlightAltitudeAslM = 999.0;
constexpr double kPreparedInternalFuelKg = 300.0;
constexpr int kPreparedExternalFuelStation = 2;
constexpr double kPreparedExternalFuelKg = 40.0;
constexpr double kFirstQueuedMassKg = 1.25;
constexpr double kSecondQueuedMassKg = 2.5;
constexpr auto kInputConcurrencyTimeout = std::chrono::seconds(1);
constexpr const char* kRepeatedStartWarning =
	"lifecycle_warning=repeated_start_without_release";
std::atomic<int> g_cockpit_api_requests = 0;

void* get_parameter_handle(const char* name)
{
	return const_cast<char*>(name);
}

void update_string(void*, const char*)
{
}

void update_number(void*, double)
{
}

bool read_number(const void*, double& value, bool)
{
	value = 0.0;
	return true;
}

bool read_string(const void*, char*, unsigned)
{
	return true;
}

int compare_parameters(void*, void*)
{
	return 0;
}

cockpit_param_api provide_cockpit_api()
{
	++g_cockpit_api_requests;
	return {
		get_parameter_handle,
		update_string,
		update_number,
		read_number,
		read_string,
		compare_parameters
	};
}

std::string create_config_path(const std::filesystem::path& module_root)
{
	const std::filesystem::path fm_directory = module_root / "FM";
	std::filesystem::create_directories(fm_directory);
	const std::filesystem::path config_path = fm_directory / "config.lua";
	TestFiles::write_text(config_path, "-- bridge context test\n");
	return config_path.string();
}

DcsBridge::Internal::BridgeContextEnvironment make_environment()
{
	static const int module_address_anchor = 0;
	return {
		provide_cockpit_api,
		[]() { return std::make_unique<Core::Fck1cEfm>(); },
		&module_address_anchor
	};
}

Core::AtmosphereInput make_prepared_atmosphere(double altitude_asl_m)
{
	return {
		altitude_asl_m,
		kPreparedTemperatureK,
		kPreparedSpeedOfSoundMps,
		kPreparedDensityKgM3,
		kPreparedPressurePa,
		{}
	};
}

void start_flight(
	DcsBridge::Internal::BridgeContext& context,
	Core::StartMode mode)
{
	const Core::FrameOutput output = context.start_flight(mode);
	context.event_reporter().log_start(mode, output.simulation_time_s);
}

void release_flight(DcsBridge::Internal::BridgeContext& context)
{
	const std::lock_guard<std::mutex> lock(context.execution_mutex());
	context.core().release();
	context.input_collector().reset();
	context.output_store().mark_released();
}

void publish_prepared_frame_input(DcsBridge::Internal::BridgeContext& context)
{
	DcsBridge::Internal::FrameInputCollector& collector = context.input_collector();
	collector.publish_atmosphere(
		make_prepared_atmosphere(kPreparedAltitudeAslM));
	collector.publish_surface({
		kPreparedSurfaceHeightM,
		kPreparedSurfaceHeightM,
		kPreparedSurfaceType,
		{ 0.0, kPreparedSurfaceNormalY, 0.0 }
	});
	collector.publish_mass({
		kPreparedMassKg, { kPreparedCenterOfMassXM, 0.0, 0.0 },
		{ kPreparedInertiaX, kPreparedInertiaY, kPreparedInertiaZ }
	});
	collector.publish_world_kinematics({
		{}, {}, { 0.0, kPreparedAltitudeAslM, kPreparedPositionWorldZM }
	});
	collector.publish_body_kinematics({});
	(void)collector.publish_suspension({
		kPreparedSuspensionIndex,
		{ 0.0, kPreparedSuspensionForceY, 0.0 },
		{},
		kPreparedSuspensionIntegrity,
		kPreparedSuspensionCompressionM,
		0.0
	});
}

void expect_prepared_frame_input(
	Tests::Context& tests,
	const Core::FrameInput& input)
{
	TEST_EXPECT(tests, input.availability.atmosphere);
	TEST_EXPECT(tests, input.availability.surface);
	TEST_EXPECT(tests, input.availability.mass);
	TEST_EXPECT(tests, input.availability.world_kinematics);
	TEST_EXPECT(tests, input.availability.body_kinematics);
	TEST_EXPECT(
		tests,
		input.availability.suspension[kPreparedSuspensionIndex]);
	TEST_EXPECT_NEAR(
		tests, input.atmosphere.altitude_asl, kPreparedAltitudeAslM, 0.0);
	TEST_EXPECT_NEAR(
		tests, input.world_kinematics.position.z, kPreparedPositionWorldZM, 0.0);
}

void test_first_callback_initializes_once(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory first_root("bc1");
	TestFiles::TemporaryDirectory second_root("bc2");
	TEST_EXPECT(tests, first_root.valid());
	TEST_EXPECT(tests, second_root.valid());
	const std::string first_config = create_config_path(first_root.path());
	const std::string second_config = create_config_path(second_root.path());
	g_cockpit_api_requests.store(0);
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& first = owner.get(first_config.c_str());
	DcsBridge::Internal::BridgeContext& second = owner.get(second_config.c_str());
	TEST_EXPECT(tests, &first == &second);
	TEST_EXPECT(tests, g_cockpit_api_requests.load() == 1);
	TEST_EXPECT(
		tests,
		std::filesystem::path(first.module_paths().mod_root_path) == first_root.path());
}

void test_files_ready_before_flight(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bci");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	const std::filesystem::path log_path = root.path() / "log" / "fck1c_efm.log";
	const std::filesystem::path csv_path = root.path() / "log" / "fck1c_state.csv";
	TEST_EXPECT(tests, context.event_log().is_open());
	TEST_EXPECT(tests, context.state_csv_writer().is_ready());
	TEST_EXPECT(tests, std::filesystem::exists(log_path));
	TEST_EXPECT(tests, std::filesystem::exists(csv_path));
	TEST_EXPECT(tests, !context.output_store().read().has_value());
	TEST_EXPECT(
		tests,
		TestFiles::read_text_while_open(csv_path) ==
			DcsBridge::Internal::state_csv_header());
}

void test_release_then_start_reuses_context(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcl");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	auto* const writer = &context.state_csv_writer();
	start_flight(context, Core::StartMode::ColdGround);
	TEST_EXPECT(tests, context.output_store().read().has_value());
	release_flight(context);
	TEST_EXPECT(tests, context.output_store().is_released());
	TEST_EXPECT(tests, !context.output_store().read().has_value());
	start_flight(context, Core::StartMode::HotAir);
	TEST_EXPECT(tests, &context.state_csv_writer() == writer);
	TEST_EXPECT(tests, !context.output_store().is_released());
	const std::optional<Core::FrameOutput> output = context.output_store().read();
	TEST_EXPECT(tests, output.has_value());
	TEST_EXPECT_NEAR(tests, output->simulation_time_s, 0.0, 0.0);
	const std::string log = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(tests, log.find(kRepeatedStartWarning) == std::string::npos);
}

void test_repeated_start_warns_without_resetting_input(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcw");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	context.input_collector().publish_atmosphere(
		make_prepared_atmosphere(kPreparedAltitudeAslM));
	start_flight(context, Core::StartMode::HotAir);
	const Core::FrameInput retained =
		context.input_collector().snapshot(kPreparedStepS);
	TEST_EXPECT(tests, retained.availability.atmosphere);
	TEST_EXPECT_NEAR(
		tests, retained.atmosphere.altitude_asl, kPreparedAltitudeAslM, 0.0);
	const std::string log = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	const std::size_t warning = log.find(kRepeatedStartWarning);
	TEST_EXPECT(tests, warning != std::string::npos);
	TEST_EXPECT(
		tests,
		log.find(kRepeatedStartWarning, warning + 1) == std::string::npos);
	TEST_EXPECT(tests, log.find("flight release") == std::string::npos);
}

void test_mass_delivery_drains_output_queue(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcq");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	Core::FrameOutput first;
	first.mass_effect = { true, { kFirstQueuedMassKg, {}, {} } };
	Core::FrameOutput second;
	second.mass_effect = { true, { kSecondQueuedMassKg, {}, {} } };
	context.output_store().publish(first);
	context.output_store().publish(second);
	const Core::MassDeltaResult first_result =
		context.take_flight_mass_delta();
	const Core::MassDeltaResult second_result =
		context.take_flight_mass_delta();
	TEST_EXPECT(tests, first_result.available);
	TEST_EXPECT(tests, second_result.available);
	TEST_EXPECT_NEAR(
		tests, first_result.delta.mass, kFirstQueuedMassKg, 0.0);
	TEST_EXPECT_NEAR(
		tests, second_result.delta.mass, kSecondQueuedMassKg, 0.0);
	TEST_EXPECT(tests, !context.take_flight_mass_delta().available);
}

void test_concurrent_first_callbacks_share_context(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcc");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	g_cockpit_api_requests.store(0);
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	TEST_EXPECT(tests, owner.try_get() == nullptr);
	std::array<DcsBridge::Internal::BridgeContext*, kConcurrentCallbackCount> contexts = {};
	std::vector<std::thread> callbacks;
	for (std::size_t index = 0; index < contexts.size(); ++index)
	{
		callbacks.emplace_back([&owner, &config_path, &contexts, index]()
			{ contexts[index] = &owner.get(config_path.c_str()); });
	}
	for (std::thread& callback : callbacks)
	{
		callback.join();
	}
	for (const auto* context : contexts)
	{
		TEST_EXPECT(tests, context == contexts[0]);
	}
	TEST_EXPECT(tests, owner.try_get() == contexts[0]);
	TEST_EXPECT(tests, g_cockpit_api_requests.load() == 1);
}

struct CoreActionCounters
{
	std::atomic<int> active = 0;
	std::atomic<int> peak = 0;
	std::atomic<int> completed = 0;
};

void perform_test_core_action(
	DcsBridge::Internal::BridgeContext& context,
	CoreActionCounters& counters,
	int action)
{
	(void)context.perform_core_action(
		{ "test_execution_mutex_serializes_core_actions" },
		[&counters, action](Core::Fck1cEfm& core)
		{
			const int active = ++counters.active;
			counters.peak.store((std::max)(counters.peak.load(), active));
			core.set_easy_flight((action % 2) == 0);
			++counters.completed;
			--counters.active;
		});
}

void perform_test_core_actions(
	DcsBridge::Internal::BridgeContext& context,
	CoreActionCounters& counters)
{
	for (int action = 0; action < kCoreActionsPerCallback; ++action)
	{
		perform_test_core_action(context, counters, action);
	}
}

void test_execution_mutex_serializes_core_actions(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcm");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	CoreActionCounters counters;
	std::vector<std::thread> callbacks;
	for (std::size_t index = 0; index < kConcurrentCallbackCount; ++index)
	{
		callbacks.emplace_back(
			[&context, &counters]()
			{
				perform_test_core_actions(context, counters);
			});
	}
	for (std::thread& callback : callbacks)
	{
		callback.join();
	}
	TEST_EXPECT(tests, counters.peak.load() == 1);
	TEST_EXPECT(
		tests,
		counters.completed.load() ==
			static_cast<int>(kConcurrentCallbackCount) * kCoreActionsPerCallback);
}

void test_input_collection_does_not_wait_for_core(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcu");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	std::unique_lock<std::mutex> core_lock(context.execution_mutex());
	auto input_update = std::async(std::launch::async, [&context]()
		{
			context.input_collector().publish_atmosphere(
				make_prepared_atmosphere(kPreparedAltitudeAslM));
		});
	const bool completed_while_core_locked =
		input_update.wait_for(kInputConcurrencyTimeout) == std::future_status::ready;
	core_lock.unlock();
	input_update.get();
	TEST_EXPECT(tests, completed_while_core_locked);
	TEST_EXPECT(
		tests,
		context.input_collector().snapshot(kPreparedStepS).availability.atmosphere);
}

void test_core_action_rejects_released_flight(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcr");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	release_flight(context);
	bool action_executed = false;
	const bool completed = context.perform_core_action(
		{ "test_released_action" },
		[&action_executed](Core::Fck1cEfm&) { action_executed = true; });
	TEST_EXPECT(tests, !completed);
	TEST_EXPECT(tests, !action_executed);
	const std::string log = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(tests, log.find(
		"callback=test_released_action invalid lifecycle state=released") !=
		std::string::npos);
}

void test_release_clears_previous_frame_input(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcp");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	context.input_collector().publish_atmosphere(
		make_prepared_atmosphere(kPreviousFlightAltitudeAslM));
	release_flight(context);
	TEST_EXPECT(
		tests,
		!context.input_collector().snapshot(kPreparedStepS).availability.atmosphere);
}

void prepare_next_flight_core(DcsBridge::Internal::BridgeContext& context)
{
	context.perform_core_preparation([](Core::Fck1cEfm& core)
		{
			core.set_internal_fuel(kPreparedInternalFuelKg);
			core.set_external_fuel({
				kPreparedExternalFuelStation, kPreparedExternalFuelKg, {}
			});
			core.set_infinite_fuel(true);
			core.set_easy_flight(true);
			core.set_invincible(true);
		});
}

void expect_first_step_uses_prepared_input(
	Tests::Context& tests,
	DcsBridge::Internal::BridgeContext& context)
{
	const Core::FrameInput prepared =
		context.input_collector().snapshot(kPreparedStepS);
	expect_prepared_frame_input(tests, prepared);
	Core::FrameOutput first_step;
	TEST_EXPECT(tests, context.perform_core_action(
		{ "test_next_flight_first_step" },
		[&prepared, &first_step](Core::Fck1cEfm& core)
		{
			first_step = core.step(prepared);
		}));
	TEST_EXPECT_NEAR(
		tests, first_step.flight.altitude_asl_m, kPreparedAltitudeAslM, 0.0);
	TEST_EXPECT_NEAR(
		tests,
		first_step.flight.position_world_z_m,
		kPreparedPositionWorldZM,
		0.0);
	TEST_EXPECT_NEAR(
		tests,
		first_step.force_moment.center_of_mass.x,
		kPreparedCenterOfMassXM,
		0.0);
}

void test_release_allows_next_flight_preparation(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bnp");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	release_flight(context);
	prepare_next_flight_core(context);
	publish_prepared_frame_input(context);
	TEST_EXPECT_NEAR(
		tests,
		context.query_core_preparation(
			[](const Core::Fck1cEfm& core) { return core.internal_fuel(); }),
		kPreparedInternalFuelKg,
		0.0);
	TEST_EXPECT(tests, !context.take_flight_mass_delta().available);
	TEST_EXPECT(tests, !context.output_store().read().has_value());
	const std::string preparation_log = TestFiles::read_text_while_open(
		root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(
		tests,
		preparation_log.find("invalid lifecycle state=released") ==
			std::string::npos);
	start_flight(context, Core::StartMode::ColdGround);
	const std::optional<Core::FrameOutput> output = context.output_store().read();
	TEST_EXPECT(tests, output.has_value());
	TEST_EXPECT_NEAR(
		tests, output->fuel.internal_fuel, kPreparedInternalFuelKg, 0.0);
	TEST_EXPECT_NEAR(
		tests, output->fuel.external_fuel, kPreparedExternalFuelKg, 0.0);
	expect_first_step_uses_prepared_input(tests, context);
}

}

void run_bridge_context_tests(Tests::Context& context)
{
	test_first_callback_initializes_once(context);
	test_files_ready_before_flight(context);
	test_release_then_start_reuses_context(context);
	test_repeated_start_warns_without_resetting_input(context);
	test_mass_delivery_drains_output_queue(context);
	test_concurrent_first_callbacks_share_context(context);
	test_execution_mutex_serializes_core_actions(context);
	test_input_collection_does_not_wait_for_core(context);
	test_core_action_rejects_released_flight(context);
	test_release_clears_previous_frame_input(context);
	test_release_allows_next_flight_preparation(context);
}
