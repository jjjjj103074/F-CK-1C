#include "TestFileUtils.h"
#include "TestHarness.h"

#include "../F-CK-1C_EFM/Data/AircraftConfig.h"
#include "../F-CK-1C_EFM/DcsBridge/Internal/BridgeContext.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr std::size_t kConcurrentCallbackCount = 8;
constexpr int kCoreActionsPerCallback = 100;
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
		Data::fck1c_aircraft_config(),
		&module_address_anchor
	};
}

void start_flight(
	DcsBridge::Internal::BridgeContext& context,
	Core::StartMode mode)
{
	const std::lock_guard<std::mutex> lock(context.execution_mutex());
	context.input_collector().reset();
	context.param_exporter().reset();
	const Core::FrameOutput output = context.core().start(mode);
	context.output_store().publish(output);
	context.param_exporter().observe(output);
	context.state_csv_writer().publish_start(output);
}

void release_flight(DcsBridge::Internal::BridgeContext& context)
{
	const std::lock_guard<std::mutex> lock(context.execution_mutex());
	context.core().release();
	context.output_store().mark_released();
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
}

void test_concurrent_first_callbacks_share_context(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcc");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	g_cockpit_api_requests.store(0);
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
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
	TEST_EXPECT(tests, g_cockpit_api_requests.load() == 1);
}

void test_execution_mutex_serializes_core_actions(Tests::Context& tests)
{
	TestFiles::TemporaryDirectory root("bcm");
	TEST_EXPECT(tests, root.valid());
	const std::string config_path = create_config_path(root.path());
	DcsBridge::Internal::BridgeContextOwner owner(make_environment());
	DcsBridge::Internal::BridgeContext& context = owner.get(config_path.c_str());
	start_flight(context, Core::StartMode::HotGround);
	std::atomic<int> active_actions = 0;
	std::atomic<int> peak_actions = 0;
	std::atomic<int> completed_actions = 0;
	std::vector<std::thread> callbacks;
	for (std::size_t index = 0; index < kConcurrentCallbackCount; ++index)
	{
		callbacks.emplace_back([&context, &active_actions, &peak_actions, &completed_actions]()
		{
			for (int action = 0; action < kCoreActionsPerCallback; ++action)
			{
				(void)context.perform_core_action(
					{ "test_execution_mutex_serializes_core_actions" },
					[&active_actions, &peak_actions, &completed_actions, action](
						Core::Fck1cEfm& core)
					{
						const int active = ++active_actions;
						peak_actions.store((std::max)(peak_actions.load(), active));
						core.set_easy_flight((action % 2) == 0);
						++completed_actions;
						--active_actions;
					});
			}
		});
	}
	for (std::thread& callback : callbacks)
	{
		callback.join();
	}
	TEST_EXPECT(tests, peak_actions.load() == 1);
	TEST_EXPECT(
		tests,
		completed_actions.load() ==
			static_cast<int>(kConcurrentCallbackCount) * kCoreActionsPerCallback);
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
}

void run_bridge_context_tests(Tests::Context& context)
{
	test_first_callback_initializes_once(context);
	test_files_ready_before_flight(context);
	test_release_then_start_reuses_context(context);
	test_concurrent_first_callbacks_share_context(context);
	test_execution_mutex_serializes_core_actions(context);
	test_core_action_rejects_released_flight(context);
}
