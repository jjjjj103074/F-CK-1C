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
}
