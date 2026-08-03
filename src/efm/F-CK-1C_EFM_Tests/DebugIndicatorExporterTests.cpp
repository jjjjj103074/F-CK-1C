#include "FakeCockpitParameters.h"
#include "TestHarness.h"

#include "DcsBridge/Internal/DebugTelemetry/DebugIndicatorExporter.h"
#include "DcsIds/CockpitParams.g.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>

namespace
{
using DcsBridge::Internal::DebugCsvStatus;
using DcsBridge::Internal::DebugIndicatorExporter;
using DcsBridge::Internal::DebugTelemetryHub;
using namespace std::chrono_literals;

constexpr auto kIsolatedPublishErrors =
	Core::DebugTelemetryPublishErrorPolicy::IsolateAndReport;
constexpr std::array<const char*,
	DcsBridge::Internal::kDebugIndicatorTextParameterCount> kTextParameters = {
	DcsIds::CockpitParams::DebugIndicatorText1,
	DcsIds::CockpitParams::DebugIndicatorText2,
	DcsIds::CockpitParams::DebugIndicatorText3,
	DcsIds::CockpitParams::DebugIndicatorText4,
	DcsIds::CockpitParams::DebugIndicatorText5,
	DcsIds::CockpitParams::DebugIndicatorText6,
	DcsIds::CockpitParams::DebugIndicatorStatus
};

Core::DebugTelemetryChannelDescriptor descriptor(
	const std::string& name,
	const std::string& label,
	const std::string& unit = "")
{
	return {
		name,
		label,
		Core::DebugTelemetryValueType::Double,
		unit,
		"indicator test channel"
	};
}

void begin_visible(
	DebugIndicatorExporter& exporter,
	Tests::Context& context)
{
	TEST_EXPECT(context, exporter.begin_flight().count == 0);
	TEST_EXPECT(context, exporter.toggle().count == 0);
}

void expect_all_text_empty(
	Tests::Context& context,
	Tests::FakeCockpitParameters& cockpit)
{
	for (const char* parameter : kTextParameters)
	{
		TEST_EXPECT(context, cockpit.text(parameter).empty());
	}
}

void test_lifecycle_is_hidden_and_clears_text(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	TEST_EXPECT(context, exporter.begin_flight().count == 0);
	TEST_EXPECT(context, !exporter.visible());
	TEST_EXPECT(context, cockpit.value(
		DcsIds::CockpitParams::DebugIndicatorVisible) == 0.0);
	expect_all_text_empty(context, cockpit);
	TEST_EXPECT(context, exporter.release_flight().count == 0);
	expect_all_text_empty(context, cockpit);
}

void test_hidden_indicator_does_not_write_frames(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto channel = hub.declare_channel<double>(
		descriptor("hidden", "Hidden"));
	hub.seal_schema();
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	exporter.begin_flight();
	std::array<std::size_t, kTextParameters.size()> initial_writes = {};
	for (std::size_t index = 0; index < kTextParameters.size(); ++index)
	{
		initial_writes[index] = cockpit.string_write_count(kTextParameters[index]);
	}
	channel.publish(1ms, 4.0);
	TEST_EXPECT(context, exporter.export_latest({ true }).count == 0);
	for (std::size_t index = 0; index < kTextParameters.size(); ++index)
	{
		TEST_EXPECT(context, cockpit.string_write_count(kTextParameters[index]) ==
			initial_writes[index]);
	}
}

void test_snapshot_uses_indicator_precision_and_escaping(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto number = hub.declare_channel<double>(descriptor("number", "Number", "rad"));
	auto integer = hub.declare_channel<std::int64_t>(descriptor("integer", "Integer"));
	auto flag = hub.declare_channel<bool>(descriptor("flag", "Flag"));
	auto text = hub.declare_channel<std::string>(descriptor("text", "Text"));
	hub.seal_schema();
	number.publish(1ms, 1.23456789);
	integer.publish(1ms, 42);
	flag.publish(1ms, true);
	text.publish(1ms, std::string("line one\nline two\tend"));
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	begin_visible(exporter, context);
	TEST_EXPECT(context, exporter.export_latest({ true }).count == 0);
	const std::string expected =
		"NUMBER: 1.23457 RAD\nINTEGER: 42\nFLAG: TRUE\n"
		"TEXT: LINE ONE\\NLINE TWO\\TEND\n";
	TEST_EXPECT(context, cockpit.text(kTextParameters[0]) == expected);
}

void declare_numbered_channels(DebugTelemetryHub& hub, std::size_t count)
{
	for (std::size_t index = 0; index < count; ++index)
	{
		const std::string suffix = std::to_string(index);
		(void)hub.declare_channel<double>(
			descriptor("channel_" + suffix, "Channel " + suffix));
	}
}

void test_channels_pack_into_six_blocks_with_explicit_overflow(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	declare_numbered_channels(hub, 61);
	hub.seal_schema();
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	begin_visible(exporter, context);
	exporter.export_latest({ true });
	for (std::size_t block = 0;
		block < DcsBridge::Internal::kDebugIndicatorBlockCount;
		++block)
	{
		const std::string& output = cockpit.text(kTextParameters[block]);
		TEST_EXPECT(context, std::count(output.begin(), output.end(), '\n') == 10);
	}
	TEST_EXPECT(context, cockpit.text(kTextParameters[5]).find("CHANNEL 59: -") !=
		std::string::npos);
	TEST_EXPECT(context, cockpit.text(kTextParameters[5]).find("CHANNEL 60") ==
		std::string::npos);
	TEST_EXPECT(context, cockpit.text(DcsIds::CockpitParams::DebugIndicatorStatus) ==
		"+1 CHANNELS NOT SHOWN - SEE DEBUG.CSV");
}

void test_unchanged_blocks_are_not_rewritten(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto channel = hub.declare_channel<double>(descriptor("changed", "Changed"));
	hub.seal_schema();
	channel.publish(1ms, 1.0);
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	begin_visible(exporter, context);
	exporter.export_latest({ true });
	std::array<std::size_t, kTextParameters.size()> first_writes = {};
	for (std::size_t index = 0; index < kTextParameters.size(); ++index)
	{
		first_writes[index] = cockpit.string_write_count(kTextParameters[index]);
	}
	exporter.export_latest({ true });
	channel.publish(2ms, 2.0);
	exporter.export_latest({ true });
	TEST_EXPECT(context, cockpit.string_write_count(kTextParameters[0]) ==
		first_writes[0] + 1);
	for (std::size_t index = 1; index < kTextParameters.size(); ++index)
	{
		TEST_EXPECT(context, cockpit.string_write_count(kTextParameters[index]) ==
			first_writes[index]);
	}
}

void test_parameter_failure_keeps_background_visible_and_recovers(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(DcsIds::CockpitParams::DebugIndicatorText1, false);
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	hub.seal_schema();
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	TEST_EXPECT(context, exporter.begin_flight().count == 1);
	TEST_EXPECT(context, exporter.toggle().count == 0);
	TEST_EXPECT(context, exporter.export_latest({ true }).count == 0);
	TEST_EXPECT(context, exporter.visible());
	TEST_EXPECT(context, cockpit.value(
		DcsIds::CockpitParams::DebugIndicatorVisible) == 1.0);
	cockpit.set_available(DcsIds::CockpitParams::DebugIndicatorText1, true);
	TEST_EXPECT(context, exporter.export_latest({ true }).count == 1);
	TEST_EXPECT(context, exporter.visible());
}

void test_csv_failure_is_in_status_row(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	hub.seal_schema();
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	begin_visible(exporter, context);
	exporter.export_latest({ false, 5, "write" });
	TEST_EXPECT(context, cockpit.text(DcsIds::CockpitParams::DebugIndicatorStatus) ==
		"[DEBUG.CSV ERROR] WRITE CODE 5");
}

void test_publication_failure_is_in_status_row(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DebugTelemetryHub hub(kIsolatedPublishErrors);
	hub.begin_flight();
	auto channel = hub.declare_channel<double>(descriptor("ordered", "Ordered"));
	channel.publish(2ms, 1.0);
	channel.publish(1ms, 2.0);
	channel.publish(0ms, 3.0);
	hub.seal_schema();
	DebugIndicatorExporter exporter(cockpit.api(), hub);
	begin_visible(exporter, context);
	exporter.export_latest({ true });
	const std::string& output =
		cockpit.text(DcsIds::CockpitParams::DebugIndicatorStatus);
	TEST_EXPECT(context, output.find("[TELEMETRY ERROR] CHANNEL 0:") !=
		std::string::npos);
	TEST_EXPECT(context, output.find("(COUNT 2)") != std::string::npos);
}
}

void run_debug_indicator_exporter_tests(Tests::Context& context)
{
	test_lifecycle_is_hidden_and_clears_text(context);
	test_hidden_indicator_does_not_write_frames(context);
	test_snapshot_uses_indicator_precision_and_escaping(context);
	test_channels_pack_into_six_blocks_with_explicit_overflow(context);
	test_unchanged_blocks_are_not_rewritten(context);
	test_parameter_failure_keeps_background_visible_and_recovers(context);
	test_csv_failure_is_in_status_row(context);
	test_publication_failure_is_in_status_row(context);
}
