#include "FakeCockpitParameters.h"
#include "TestHarness.h"

#include "DcsBridge/Internal/CockpitBridge.h"
#include "DcsIds/CockpitParams.g.h"

namespace
{
constexpr double kTolerance = 1e-9;

void test_temperature_export(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	TEST_EXPECT(context, bridge.export_temperature(15.0).count == 0);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::TemperatureC), 288.0, kTolerance);
}

void test_zero_radar_and_ir_samples_are_available(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const Core::CockpitObservation observation =
		bridge.read_step_input().cockpit;
	TEST_EXPECT(context, observation.radar.status.available);
	TEST_EXPECT(context, observation.radar.status.revision == 1);
	TEST_EXPECT(
		context,
		observation.radar.status.invalid_reason ==
			Core::ObservationInvalidReason::None);
	TEST_EXPECT(context, observation.ir_seeker.status.available);
	TEST_EXPECT(context, observation.ir_seeker.status.revision == 1);
	TEST_EXPECT_NEAR(
		context, observation.radar.stt_range_m, 0.0, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.ir_seeker.target_range_m, 0.0, kTolerance);
}

void test_typed_radar_and_ir_units(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::RawCockpitParams::RadarMode, 3.0);
	cockpit.set(DcsIds::RawCockpitParams::RadarSttAzimuth, 0.25);
	cockpit.set(DcsIds::RawCockpitParams::RadarSttRange, 4200.0);
	cockpit.set(DcsIds::RawCockpitParams::IrLock, 1.0);
	cockpit.set(DcsIds::RawCockpitParams::IrDesiredElevation, -0.15);
	cockpit.set(DcsIds::RawCockpitParams::WeaponTargetRange, 3100.0);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const Core::CockpitObservation observation =
		bridge.read_step_input().cockpit;
	TEST_EXPECT(
		context,
		observation.radar.mode == Core::RadarMode::SingleTargetTrack);
	TEST_EXPECT_NEAR(
		context, observation.radar.stt_azimuth_rad, 0.25, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.radar.stt_range_m, 4200.0, kTolerance);
	TEST_EXPECT(context, observation.ir_seeker.locked);
	TEST_EXPECT_NEAR(
		context, observation.ir_seeker.desired_elevation_rad, -0.15, kTolerance);
	TEST_EXPECT_NEAR(
		context, observation.ir_seeker.target_range_m, 3100.0, kTolerance);
}

void test_missing_radar_parameter_marks_only_radar_unavailable(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set_available(
		DcsIds::RawCockpitParams::RadarSttRange,
		false);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const DcsBridge::Internal::CockpitStepInput input =
		bridge.read_step_input();
	TEST_EXPECT(context, !input.cockpit.radar.status.available);
	TEST_EXPECT(
		context,
		input.cockpit.radar.status.invalid_reason ==
			Core::ObservationInvalidReason::ParameterUnavailable);
	TEST_EXPECT(context, input.cockpit.ir_seeker.status.available);
	TEST_EXPECT(context, input.events.count == 1);
}

void test_unknown_radar_mode_is_invalid(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::RawCockpitParams::RadarMode, 3.0);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const Core::RadarObservation valid =
		bridge.read_step_input().cockpit.radar;
	cockpit.set(DcsIds::RawCockpitParams::RadarMode, 99.0);
	const Core::RadarObservation observation =
		bridge.read_step_input().cockpit.radar;
	TEST_EXPECT(context, !observation.status.available);
	TEST_EXPECT(context, observation.status.revision == valid.status.revision);
	TEST_EXPECT(
		context,
		observation.status.invalid_reason ==
			Core::ObservationInvalidReason::InvalidNumeric);
}

void test_weapon_station_observation_contract(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::WeaponObservationAvailable, 1.0);
	cockpit.set(DcsIds::CockpitParams::WeaponObservationRevision, 7.0);
	cockpit.set(DcsIds::CockpitParams::WeaponObservationInvalidReason, 0.0);
	cockpit.set(DcsIds::CockpitParams::WeaponObservationAim9Count, 2.0);
	cockpit.set(DcsIds::CockpitParams::WeaponObservationSelectedStation, 3.0);
	cockpit.set(
		DcsIds::CockpitParams::WeaponObservationScannedStationCount,
		7.0);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const Core::WeaponStationObservation observation =
		bridge.read_step_input().cockpit.weapon_stations;
	TEST_EXPECT(context, observation.status.available);
	TEST_EXPECT(context, observation.status.revision == 7);
	TEST_EXPECT(context, observation.aim9_count == 2);
	TEST_EXPECT(context, observation.selected_station == 3);
	TEST_EXPECT(context, observation.scanned_station_count == 7);
}

void test_weapon_station_invalid_revision_is_explicit(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::WeaponObservationAvailable, 1.0);
	cockpit.set(DcsIds::CockpitParams::WeaponObservationRevision, 1.5);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const Core::WeaponStationObservation observation =
		bridge.read_step_input().cockpit.weapon_stations;
	TEST_EXPECT(context, !observation.status.available);
	TEST_EXPECT(
		context,
		observation.status.invalid_reason ==
			Core::ObservationInvalidReason::InvalidRevision);
}

void test_weapon_station_unavailable_reason_is_preserved(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::WeaponObservationRevision, 9.0);
	cockpit.set(
		DcsIds::CockpitParams::WeaponObservationInvalidReason,
		static_cast<double>(Core::ObservationInvalidReason::StationApiError));
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const Core::WeaponStationObservation observation =
		bridge.read_step_input().cockpit.weapon_stations;
	TEST_EXPECT(context, !observation.status.available);
	TEST_EXPECT(context, observation.status.revision == 9);
	TEST_EXPECT(
		context,
		observation.status.invalid_reason ==
			Core::ObservationInvalidReason::StationApiError);
}
}

void run_cockpit_bridge_tests(Tests::Context& context)
{
	test_temperature_export(context);
	test_zero_radar_and_ir_samples_are_available(context);
	test_typed_radar_and_ir_units(context);
	test_missing_radar_parameter_marks_only_radar_unavailable(context);
	test_unknown_radar_mode_is_invalid(context);
	test_weapon_station_observation_contract(context);
	test_weapon_station_invalid_revision_is_explicit(context);
	test_weapon_station_unavailable_reason_is_preserved(context);
}
