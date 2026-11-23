#include "TestHarness.h"
#include "FakeCockpitParameters.h"
#include "Core/Systems/DisplaySystem/Internal/RadarPresentation.h"
#include "Core/Systems/RadarSystem/Internal/SearchProfiles.h"
#include "DcsBridge/Internal/RadarDisplayExporter.h"

#include <chrono>
#include <limits>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace std::chrono;

RadarSnapshot source(bool bore)
{
	RadarSnapshot radar;
	radar.status = { true, 1 };
	radar.observation.status = { true, 1, ObservationInvalidReason::None };
	radar.observation.mode = RadarMode::Search;
	radar.active_search_mode = bore ? RadarSearchMode::AirCombat : RadarSearchMode::RangeWhileSearch;
	radar.scan = bore ? RadarProfiles::boresight() : RadarProfiles::range_while_search();
	return radar;
}

void test_bore_corners_flash_only_during_acquisition(Tests::Context& c)
{
	auto radar = source(true);
	auto display = DisplayPresentation::radar(radar, OperatingMode::Dogfight, milliseconds(350));
	TEST_EXPECT(c, display.visible && display.sensor_available && display.cue_visible);
	TEST_EXPECT(c, display.cue == RadarCueKind::BoresightReady);
	TEST_EXPECT_NEAR(c, display.bore_half_width_mrad, Common::rad(3.3) * 500.0, 1e-9);
	radar.automatic_acquisition_requested = true;
	display = DisplayPresentation::radar(radar, OperatingMode::Dogfight, milliseconds(100));
	TEST_EXPECT(c, display.cue == RadarCueKind::BoresightAcquiring && display.cue_visible);
	display = DisplayPresentation::radar(radar, OperatingMode::Dogfight, milliseconds(350));
	TEST_EXPECT(c, !display.cue_visible && !display.estimated_sweep_visible);
	radar.observation.status.available = false;
	display = DisplayPresentation::radar(radar, OperatingMode::Dogfight, milliseconds(100));
	TEST_EXPECT(c, display.visible && !display.sensor_available && display.cue == RadarCueKind::Hidden);
}

void test_stt_uses_source_line_of_sight_and_retains_tracking_beyond_bore_range(Tests::Context& c)
{
	auto radar = source(true);
	radar.observation.mode = RadarMode::SingleTargetTrack;
	radar.observation.stt_azimuth_rad = Common::rad(20.0);
	radar.observation.stt_elevation_rad = Common::rad(5.0);
	radar.observation.stt_range_m = 22000.0;
	radar.observation.search_returns.push_back({ 10, 0, 0, 1000, 0 });
	const auto display = DisplayPresentation::radar(radar, OperatingMode::Dogfight, milliseconds(350));
	TEST_EXPECT(c, display.cue == RadarCueKind::SingleTargetTrack && display.cue_visible);
	TEST_EXPECT_NEAR(c, display.target_x_mrad, std::tan(Common::rad(20.0)) * 1000.0, 1e-9);
	TEST_EXPECT_NEAR(c, display.target_y_mrad, std::tan(Common::rad(5.0)) / std::cos(Common::rad(20.0)) * 1000.0, 1e-9);
	TEST_EXPECT(c, display.target_visible && !display.estimated_sweep_visible);
	TEST_EXPECT_NEAR(c, display.range_scale_m, 20.0 * 1852.0, 1e-9);
	TEST_EXPECT(c, display.plots.empty());
}

void test_scope_uses_fresh_returns_and_actual_scan_geometry(Tests::Context& c)
{
	auto radar = source(false);
	auto& observation = radar.observation;
	observation.scan_available = true;
	observation.scan_azimuth_center_rad = Common::rad(10.0);
	observation.scan_azimuth_width_rad = Common::rad(60.0);
	observation.tdc_available = true;
	observation.tdc_azimuth_rad = Common::rad(15.0);
	observation.tdc_range_m = 10000;
	observation.search_returns = { { 101, Common::rad(10.0), 0, 5000, 0 },
		                           { 102, Common::rad(10.0), 0, 5000, 1 },
		                           { 160, Common::rad(-15.0), 0, 10000, 2 },
		                           { 170, 0, 0, 1000, 6 },
		                           { 171, 0, 0, 1000, std::numeric_limits<double>::quiet_NaN() } };
	const auto display = DisplayPresentation::radar(radar, OperatingMode::AirToAir, milliseconds(100));
	TEST_EXPECT(c, display.visible && display.estimated_sweep_visible && display.cursor_visible);
	TEST_EXPECT_NEAR(c, display.scan_left_normalized, -20.0 / 60.0, 1e-9);
	TEST_EXPECT_NEAR(c, display.scan_right_normalized, 40.0 / 60.0, 1e-9);
	TEST_EXPECT(c, display.plots.size() == 2);
	if (display.plots.size() != 2)
		return;
	TEST_EXPECT(c, display.plots[0].source_slot == 101 && display.plots[1].source_slot == 160);
	TEST_EXPECT_NEAR(c, display.plots[1].opacity_0_1, 0.6, 1e-9);
	TEST_EXPECT(c, display.plots.size() == 2);
	TEST_EXPECT_NEAR(c, display.cursor_x_normalized, 0.25, 1e-9);
	TEST_EXPECT(c, !DisplayPresentation::radar(radar, OperatingMode::Navigation, {}).visible);
	TEST_EXPECT(c, !DisplayPresentation::radar(radar, OperatingMode::Dogfight, {}).visible);
}

void test_scope_caps_the_number_of_drawn_returns(Tests::Context& c)
{
	auto radar = source(false);
	for (unsigned index = 0; index < 50; ++index)
		radar.observation.search_returns.push_back({ index + 1, Common::rad(-59.0 + index * 2.0), 0.0, 5000.0, 0.0 });
	const auto display = DisplayPresentation::radar(radar, OperatingMode::AirToAir, {});
	TEST_EXPECT(c, display.plots.size() == kRadarDisplayPlotCount);
	if (display.plots.size() != kRadarDisplayPlotCount)
		return;
	TEST_EXPECT(c, display.plots.front().source_slot == 1 && display.plots.back().source_slot == 32);
}

void test_graphical_export_preserves_named_fields_and_plot_boundaries(Tests::Context& c)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::RadarDisplayExporter exporter(cockpit.api());
	RadarDisplaySnapshot display;
	display.visible = true;
	display.sensor_available = true;
	display.cue = RadarCueKind::SingleTargetTrack;
	display.cue_visible = true;
	display.bore_half_width_mrad = 28.8;
	display.bore_half_height_mrad = 40.1;
	display.target_x_mrad = 12.0;
	display.target_y_mrad = -9.0;
	display.range_scale_m = 40.0 * 1852.0;
	display.scan_left_normalized = -0.7;
	display.scan_right_normalized = 0.6;
	display.estimated_sweep_visible = true;
	display.estimated_sweep_x_normalized = -0.4;
	display.cursor_visible = true;
	display.cursor_x_normalized = 0.3;
	display.cursor_y_normalized = 0.8;
	display.target_visible = true;
	display.target_x_normalized = -0.2;
	display.target_y_normalized = 0.9;
	for (std::size_t i = 0; i < kRadarDisplayPlotCount; ++i)
		display.plots.push_back({ static_cast<double>(i) / 32.0, 0.25, 0.75, static_cast<unsigned>(101 + i) });
	TEST_EXPECT(c, exporter.export_snapshot(display).count == 0);
	const auto expect = [&](const char* name, double value)
	{
		TEST_EXPECT_NEAR(c, cockpit.value(name), value, 1e-9);
	};
	using namespace DcsIds::CockpitParams;
	expect(RadarDisplayAvailable, 1.0);
	expect(RadarDisplayVisible, 1.0);
	expect(RadarDisplaySensorAvailable, 1.0);
	expect(RadarDisplayCueKind, 3.0);
	expect(RadarDisplayCueVisible, 1.0);
	expect(RadarDisplayBoreHalfWidthMrad, 28.8);
	expect(RadarDisplayBoreHalfHeightMrad, 40.1);
	expect(RadarDisplayTargetXMrad, 12.0);
	expect(RadarDisplayTargetYMrad, -9.0);
	expect(RadarDisplayRangeScaleM, 40.0 * 1852.0);
	expect(RadarDisplayRangeScaleNm, 40.0);
	expect(RadarDisplayScanLeft, -0.7);
	expect(RadarDisplayScanRight, 0.6);
	expect(RadarDisplaySweepVisible, 1.0);
	expect(RadarDisplaySweepX, -0.4);
	expect(RadarDisplayCursorVisible, 1.0);
	expect(RadarDisplayCursorX, 0.3);
	expect(RadarDisplayCursorY, 0.8);
	expect(RadarDisplayTargetVisible, 1.0);
	expect(RadarDisplayTargetX, -0.2);
	expect(RadarDisplayTargetY, 0.9);
	expect(RadarDisplayPlot01X, 0.0);
	expect(RadarDisplayPlot01Y, 0.25);
	expect(RadarDisplayPlot01Opacity, 0.75);
	expect(RadarDisplayPlot01SourceSlot, 101.0);
	expect(RadarDisplayPlot16X, 15.0 / 32.0);
	expect(RadarDisplayPlot16Y, 0.25);
	expect(RadarDisplayPlot16Opacity, 0.75);
	expect(RadarDisplayPlot16SourceSlot, 116.0);
	expect(RadarDisplayPlot32X, 31.0 / 32.0);
	expect(RadarDisplayPlot32Y, 0.25);
	expect(RadarDisplayPlot32Opacity, 0.75);
	expect(RadarDisplayPlot32SourceSlot, 132.0);
	display.plots.resize(1);
	exporter.reset();
	TEST_EXPECT(c, exporter.export_snapshot(display).count == 0);
	expect(RadarDisplayPlot01SourceSlot, 101.0);
	expect(RadarDisplayPlot32X, 0.0);
	expect(RadarDisplayPlot32Y, 0.0);
	expect(RadarDisplayPlot32Opacity, 0.0);
	expect(RadarDisplayPlot32SourceSlot, 0.0);
}

void test_graphical_export_commits_complete_snapshots_and_recovers(Tests::Context& c)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(DcsIds::CockpitParams::RadarDisplayPlot01X, false);
	DcsBridge::Internal::RadarDisplayExporter exporter(cockpit.api());
	auto display = DisplayPresentation::radar(source(true), OperatingMode::Dogfight, {});
	TEST_EXPECT(c, exporter.export_snapshot(display).count == 1);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayAvailable), 0.0, 1e-9);
	TEST_EXPECT(c, exporter.export_snapshot(display).count == 0);
	cockpit.set_available(DcsIds::CockpitParams::RadarDisplayPlot01X, true);
	TEST_EXPECT(c, exporter.export_snapshot(display).count == 1);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayAvailable), 1.0, 1e-9);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayCueKind), 1.0, 1e-9);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayRangeScaleNm), 10.0, 1e-9);
	display.plots.push_back({ 0.2, 0.3, 0.8, 101 });
	TEST_EXPECT(c, exporter.export_snapshot(display).count == 0);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayPlot01Opacity), 0.8, 1e-9);
	TEST_EXPECT(c, exporter.export_snapshot({}).count == 0);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayPlot01Opacity), 0.0, 1e-9);
	TEST_EXPECT_NEAR(c, cockpit.value(DcsIds::CockpitParams::RadarDisplayVisible), 0.0, 1e-9);
}
}

void run_radar_display_tests(Tests::Context& context)
{
	test_bore_corners_flash_only_during_acquisition(context);
	test_stt_uses_source_line_of_sight_and_retains_tracking_beyond_bore_range(context);
	test_scope_uses_fresh_returns_and_actual_scan_geometry(context);
	test_scope_caps_the_number_of_drawn_returns(context);
	test_graphical_export_commits_complete_snapshots_and_recovers(context);
	test_graphical_export_preserves_named_fields_and_plot_boundaries(context);
}
