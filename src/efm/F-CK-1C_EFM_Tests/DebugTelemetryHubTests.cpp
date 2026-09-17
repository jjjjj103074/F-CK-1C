#include "TestHarness.h"
#include "DebugTelemetryTestSupport.h"
#include "Fck1cEfmTestFixture.h"

#include "Common/Units.h"
#include "DcsBridge/Internal/DebugTelemetry/DebugTelemetryHub.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
using DcsBridge::Internal::DebugTelemetryHub;
using namespace std::chrono_literals;
constexpr auto kIsolatedPublishErrors =
	Core::DebugTelemetryPublishErrorPolicy::IsolateAndReport;

Core::DebugTelemetryChannelDescriptor channel(const char* name)
{
	return {
		name,
		std::string("Label ") + name,
		Core::DebugTelemetryValueType::Double,
		"",
		"test channel"
	};
}

template <typename Action>
bool throws(Action action)
{
	try
	{
		action();
	}
	catch (const std::exception&)
	{
		return true;
	}
	return false;
}

void test_contract_supports_all_value_types(Tests::Context& context)
{
	Tests::RecordingDebugTelemetry telemetry;
	auto number = telemetry.declare_channel<double>(channel("number"));
	auto integer = telemetry.declare_channel<std::int64_t>(channel("integer"));
	auto boolean = telemetry.declare_channel<bool>(channel("boolean"));
	auto text = telemetry.declare_channel<std::string>(channel("text"));
	number.publish(1ns, 1.25);
	integer.publish(2ns, -7);
	boolean.publish(3ns, true);
	text.publish(4ns, std::string("alpha,beta\n\"quoted\""));
	TEST_EXPECT(context, telemetry.channels().size() == 4);
	TEST_EXPECT(context, telemetry.updates().size() == 4);
	TEST_EXPECT(context,
		std::get<std::string>(telemetry.updates()[3].value) ==
			"alpha,beta\n\"quoted\"");
}

void test_publication_is_not_commit_bound(Tests::Context& context)
{
	Tests::RecordingDebugTelemetry telemetry;
	auto value = telemetry.declare_channel<double>(channel("operation"));
	value.publish(1ns, 4.0);
	TEST_EXPECT(context, throws([]() { throw std::runtime_error("failed"); }));
	TEST_EXPECT(context, telemetry.updates().size() == 1);
	TEST_EXPECT(context, std::get<double>(telemetry.updates()[0].value) == 4.0);
	Core::DebugTelemetryChannel<double> undeclared;
	TEST_EXPECT(context, throws([&undeclared]()
	{
		undeclared.publish(0ns, 1.0);
	}));
}

void test_latest_values_and_64_hz_resampling(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto number = hub.declare_channel<double>(channel("number"));
	auto text = hub.declare_channel<std::string>(channel("text"));
	number.publish(0ns, 1.0);
	text.publish(0ns, std::string("initial"));
	hub.seal_schema();
	number.publish(10ms, 2.0);
	number.publish(20ms, 3.0);
	const auto samples = hub.sample_through(40ms);
	TEST_EXPECT(context, samples.size() == 3);
	TEST_EXPECT(context, samples[0].simulation_time == 0ns);
	TEST_EXPECT(context, samples[1].simulation_time == 15'625'000ns);
	TEST_EXPECT(context, samples[2].simulation_time == 31'250'000ns);
	TEST_EXPECT(context, std::get<double>(*samples[0].values[0]) == 1.0);
	TEST_EXPECT(context, std::get<double>(*samples[1].values[0]) == 2.0);
	TEST_EXPECT(context, std::get<double>(*samples[2].values[0]) == 3.0);
	const auto latest = hub.latest_snapshot();
	TEST_EXPECT(context, std::get<double>(*latest.values[0]) == 3.0);
	TEST_EXPECT(context, std::get<std::string>(*latest.values[1]) == "initial");
}

void test_schema_is_stable_between_flights(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	(void)hub.declare_channel<double>(channel("stable"));
	TEST_EXPECT(context, throws([&hub]()
	{
		(void)hub.declare_channel<double>(channel("stable"));
	}));
	hub.seal_schema();
	hub.release_flight();
	hub.begin_flight();
	(void)hub.declare_channel<double>(channel("stable"));
	hub.seal_schema();
	hub.release_flight();
	hub.begin_flight();
	TEST_EXPECT(context, throws([&hub]() { hub.seal_schema(); }));
	hub.release_flight();
	hub.begin_flight();
	auto changed = channel("stable");
	changed.label = "Changed Label";
	TEST_EXPECT(context, throws([&hub, changed]() mutable
	{
		(void)hub.declare_channel<double>(std::move(changed));
	}));
}

void test_descriptor_and_time_errors_are_explicit(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	TEST_EXPECT(context, throws([&hub]()
	{
		(void)hub.declare_channel<double>(channel(""));
	}));
	auto value = hub.declare_channel<double>(channel("value"));
	value.publish(5ms, 1.0);
	value.publish(4ms, 2.0);
	value.publish(3ms, 3.0);
	const auto status = hub.status();
	TEST_EXPECT(context, status.publication_failed);
	TEST_EXPECT(context,
		std::string(status.message.data()).find("monotonic") !=
			std::string::npos);
	TEST_EXPECT(context, status.channel_id == 0);
	TEST_EXPECT(context, status.simulation_time_ns == 4'000'000);
	TEST_EXPECT(context, status.flight_failure_count == 2);
	hub.seal_schema();
	TEST_EXPECT(context, throws([&hub]()
	{
		(void)hub.declare_channel<bool>(channel("late"));
	}));
}

void test_strict_publication_policy_rethrows(Tests::Context& context)
{
	DebugTelemetryHub hub(
		Core::DebugTelemetryPublishErrorPolicy::ReportAndRethrow);
	hub.begin_flight();
	auto value = hub.declare_channel<double>(channel("strict"));
	value.publish(2ms, 1.0);
	TEST_EXPECT(context, throws([&value]() { value.publish(1ms, 2.0); }));
	TEST_EXPECT(context, hub.status().publication_failed);
}

void test_publication_failure_does_not_interrupt_core(
	Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto future = hub.declare_channel<double>(channel("future"));
	Core::Fck1cEfm efm(Tests::Fck1c::make_test_config(), hub);
	(void)efm.start(Core::StartMode::HotAir);
	future.publish(10s, 1.0);
	hub.seal_schema();
	const Core::FrameOutput output =
		efm.step(Tests::Fck1c::make_frame_input());
	TEST_EXPECT(context, output.simulation_time_s > 0.0);
	TEST_EXPECT(context, hub.status().publication_failed);
}

void test_flight_reset_restarts_sampling_sequence(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto first = hub.declare_channel<double>(channel("value"));
	first.publish(0ns, 1.0);
	hub.seal_schema();
	TEST_EXPECT(context, hub.sample_through(0ns)[0].sequence == 0);
	hub.release_flight();
	hub.begin_flight();
	auto second = hub.declare_channel<double>(channel("value"));
	second.publish(0ns, 2.0);
	hub.seal_schema();
	const auto sample = hub.sample_through(0ns)[0];
	TEST_EXPECT(context, sample.sequence == 0);
	TEST_EXPECT(context, std::get<double>(*sample.values[0]) == 2.0);
}

void test_concurrent_latest_snapshot_is_complete(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto value = hub.declare_channel<std::int64_t>(channel("counter"));
	hub.seal_schema();
	std::atomic<bool> invalid_snapshot = false;
	std::thread reader([&hub, &invalid_snapshot]()
	{
		for (int index = 0; index < 100; ++index)
		{
			const auto snapshot = hub.latest_snapshot();
			if (snapshot.channels.size() != snapshot.values.size())
			{
				invalid_snapshot.store(true);
			}
		}
	});
	std::vector<std::thread> publishers;
	for (std::int64_t index = 0; index < 8; ++index)
	{
		publishers.emplace_back([value, index]() { value.publish(1ms, index); });
	}
	for (std::thread& publisher : publishers) publisher.join();
	reader.join();
	const auto snapshot = hub.latest_snapshot();
	TEST_EXPECT(context, !invalid_snapshot.load());
	TEST_EXPECT(context, snapshot.channels.size() == 1);
	TEST_EXPECT(context, snapshot.values.size() == 1);
	TEST_EXPECT(context, snapshot.values[0].has_value());
}

std::size_t channel_index(
	const Core::DebugTelemetrySnapshot& snapshot,
	const char* name)
{
	for (std::size_t index = 0; index < snapshot.channels.size(); ++index)
	{
		if (snapshot.channels[index].name == name) return index;
	}
	throw std::logic_error("Expected debug telemetry channel was not declared.");
}

void verify_flight_control_debug_values(
	Tests::Context& context,
	const Core::DebugTelemetrySnapshot& snapshot)
{
	const auto heading_available = channel_index(
		snapshot, "flight_actual_magnetic_heading_available");
	const auto heading = channel_index(
		snapshot, "flight_actual_magnetic_heading_deg");
	const auto electronic_saturation = channel_index(
		snapshot, "flight_control_electronic_command_saturated");
	const auto physical_saturation = channel_index(
		snapshot, "flight_control_actuator_saturated");
	const auto control_authority = channel_index(
		snapshot, "flight_control_control_authority_limited");
	const auto auto_throttle_available = channel_index(
		snapshot, "afcs_experimental_auto_throttle_available");
	TEST_EXPECT(context,
		std::get<bool>(*snapshot.values[heading_available]));
	TEST_EXPECT_NEAR(context, std::get<double>(*snapshot.values[heading]),
		Common::deg(0.3), 1e-9);
	TEST_EXPECT(context,
		!std::get<bool>(*snapshot.values[electronic_saturation]));
	TEST_EXPECT(context,
		!std::get<bool>(*snapshot.values[physical_saturation]));
	TEST_EXPECT(context,
		!std::get<bool>(*snapshot.values[control_authority]));
	TEST_EXPECT(context,
		!std::get<bool>(*snapshot.values[auto_throttle_available]));
}

void test_real_core_registers_and_pushes_diagnostics(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	Core::Fck1cEfm efm(Tests::Fck1c::make_test_config(), hub);
	(void)efm.start(Core::StartMode::HotAir);
	hub.seal_schema();
	const auto initial = hub.latest_snapshot();
	const auto master = channel_index(initial, "afcs_master_engaged");
	const auto thrust_cut = channel_index(
		initial, "propulsion_test_thrust_cut_requested");
	TEST_EXPECT(context, !std::get<bool>(*initial.values[master]));
	TEST_EXPECT(context, !std::get<bool>(*initial.values[thrust_cut]));
	(void)efm.step(Tests::Fck1c::make_frame_input());
	const auto stepped = hub.latest_snapshot();
	verify_flight_control_debug_values(context, stepped);
	efm.handle_command({ Core::CommandId::EnableThrustCutTest, 1.0 });
	const auto commanded = hub.latest_snapshot();
	TEST_EXPECT(context, std::get<bool>(*commanded.values[thrust_cut]));
}

void test_real_core_publishes_subrate_diagnostics(Tests::Context& context)
{
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	Core::Fck1cEfm efm(Tests::Fck1c::make_test_config(), hub);
	(void)efm.start(Core::StartMode::HotAir);
	hub.seal_schema();
	(void)efm.step(Tests::Fck1c::make_frame_input());
	const auto snapshot = hub.latest_snapshot();
	const auto pilot_last = channel_index(
		snapshot, "flight_control_pilot_shaping_last_update_tick");
	const auto gain_last = channel_index(
		snapshot, "flight_control_gain_schedule_last_update_tick");
	const auto pitch = channel_index(
		snapshot, "flight_control_conditioned_pitch_input_normalized");
	const auto command_gain = channel_index(
		snapshot, "flight_control_active_command_gain");
	TEST_EXPECT(context,
		std::get<std::int64_t>(*snapshot.values[pilot_last]) == 0);
	TEST_EXPECT(context,
		std::get<std::int64_t>(*snapshot.values[gain_last]) == 0);
	TEST_EXPECT(context, std::isfinite(std::get<double>(*snapshot.values[pitch])));
	TEST_EXPECT(context,
		std::get<double>(*snapshot.values[command_gain]) > 0.0);
}
}

void run_debug_telemetry_hub_tests(Tests::Context& context)
{
	test_contract_supports_all_value_types(context);
	test_publication_is_not_commit_bound(context);
	test_latest_values_and_64_hz_resampling(context);
	test_schema_is_stable_between_flights(context);
	test_descriptor_and_time_errors_are_explicit(context);
	test_publication_failure_does_not_interrupt_core(context);
	test_strict_publication_policy_rethrows(context);
	test_flight_reset_restarts_sampling_sequence(context);
	test_concurrent_latest_snapshot_is_complete(context);
	test_real_core_registers_and_pushes_diagnostics(context);
	test_real_core_publishes_subrate_diagnostics(context);
}
