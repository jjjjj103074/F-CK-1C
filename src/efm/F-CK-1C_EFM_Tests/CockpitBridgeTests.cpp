#include "FakeCockpitParameters.h"
#include "TestHarness.h"

#include "Common/Units.h"
#include "DcsBridge/Internal/CockpitBridge.h"
#include "DcsIds/CockpitParams.g.h"

#include <limits>

namespace
{
constexpr double kTolerance = 1e-9;

void expect_invalid_numeric(
	Tests::Context& context,
	const Core::ObservationStatus& status)
{
	TEST_EXPECT(context, !status.available);
	TEST_EXPECT(context, status.revision == 0);
	TEST_EXPECT(context, status.invalid_reason ==
		Core::ObservationInvalidReason::InvalidNumeric);
}

void test_temperature_export(Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	TEST_EXPECT(context, bridge.export_temperature(15.0).count == 0);
	TEST_EXPECT_NEAR(context, cockpit.value(
		DcsIds::CockpitParams::TemperatureC), 288.0, kTolerance);
}

void test_pressure_altitude_observation_is_typed_in_feet(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::PressureAltitudeAvailable, 1.0);
	cockpit.set(DcsIds::CockpitParams::PressureAltitudeM, 1200.0);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto altitude = bridge.read_step_input().cockpit.pressure_altitude;
	TEST_EXPECT(context, altitude.status.available);
	TEST_EXPECT(context, altitude.status.revision == 1);
	TEST_EXPECT_NEAR(
		context, altitude.pressure_altitude_ft,
		Common::feet(1200.0), kTolerance);
}

void test_pressure_altitude_unavailability_is_explicit(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto altitude = bridge.read_step_input().cockpit.pressure_altitude;
	TEST_EXPECT(context, !altitude.status.available);
	TEST_EXPECT(
		context, altitude.status.invalid_reason ==
			Core::ObservationInvalidReason::NotProvided);
}

void test_non_finite_pressure_altitude_is_rejected(Tests::Context& context)
{
	for (const double value : {
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::infinity() })
	{
		Tests::FakeCockpitParameters cockpit;
		cockpit.set(DcsIds::CockpitParams::PressureAltitudeAvailable, 1.0);
		cockpit.set(DcsIds::CockpitParams::PressureAltitudeM, value);
		DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
		expect_invalid_numeric(context,
			bridge.read_step_input().cockpit.pressure_altitude.status);
	}
}

void test_magnetic_heading_observation_is_typed_and_normalized(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::MagneticHeadingAvailable, 1.0);
	cockpit.set(
		DcsIds::CockpitParams::MagneticHeadingRad,
		Common::rad(370.0));
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto heading = bridge.read_step_input().cockpit.magnetic_heading;
	TEST_EXPECT(context, heading.status.available);
	TEST_EXPECT(context, heading.status.revision == 1);
	TEST_EXPECT_NEAR(
		context, heading.magnetic_heading_deg,
		10.0, kTolerance);
}

void test_magnetic_heading_unavailability_is_explicit(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto heading = bridge.read_step_input().cockpit.magnetic_heading;
	TEST_EXPECT(context, !heading.status.available);
	TEST_EXPECT(context, heading.status.revision == 0);
	TEST_EXPECT(
		context,
		heading.status.invalid_reason ==
			Core::ObservationInvalidReason::NotProvided);
}

void test_missing_magnetic_heading_parameter_is_explicit(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::MagneticHeadingAvailable, 1.0);
	cockpit.set_available(DcsIds::CockpitParams::MagneticHeadingRad, false);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto input = bridge.read_step_input();
	TEST_EXPECT(context, !input.cockpit.magnetic_heading.status.available);
	TEST_EXPECT(
		context,
		input.cockpit.magnetic_heading.status.invalid_reason ==
			Core::ObservationInvalidReason::ParameterUnavailable);
	TEST_EXPECT(context, input.events.count == 1);
}

void expect_invalid_magnetic_heading(
	Tests::Context& context,
	double value)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::MagneticHeadingAvailable, 1.0);
	cockpit.set(DcsIds::CockpitParams::MagneticHeadingRad, value);
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto heading = bridge.read_step_input().cockpit.magnetic_heading;
	expect_invalid_numeric(context, heading.status);
}

void test_non_finite_magnetic_heading_is_rejected(Tests::Context& context)
{
	expect_invalid_magnetic_heading(
		context, std::numeric_limits<double>::quiet_NaN());
	expect_invalid_magnetic_heading(
		context, std::numeric_limits<double>::infinity());
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

void test_non_finite_radar_and_ir_values_are_rejected(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::RawCockpitParams::RadarSttRange,
		std::numeric_limits<double>::quiet_NaN());
	cockpit.set(DcsIds::RawCockpitParams::WeaponTargetRange,
		std::numeric_limits<double>::infinity());
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	const auto observation = bridge.read_step_input().cockpit;
	expect_invalid_numeric(context, observation.radar.status);
	expect_invalid_numeric(context, observation.ir_seeker.status);
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

void test_non_finite_weapon_station_value_is_rejected(
	Tests::Context& context)
{
	Tests::FakeCockpitParameters cockpit;
	cockpit.set(DcsIds::CockpitParams::WeaponObservationAvailable, 1.0);
	cockpit.set(DcsIds::CockpitParams::WeaponObservationRevision,
		std::numeric_limits<double>::quiet_NaN());
	DcsBridge::Internal::CockpitBridge bridge(cockpit.api());
	expect_invalid_numeric(context,
		bridge.read_step_input().cockpit.weapon_stations.status);
}
}

void run_cockpit_bridge_tests(Tests::Context& context)
{
	test_temperature_export(context);
	test_pressure_altitude_observation_is_typed_in_feet(context);
	test_pressure_altitude_unavailability_is_explicit(context);
	test_non_finite_pressure_altitude_is_rejected(context);
	test_magnetic_heading_observation_is_typed_and_normalized(context);
	test_magnetic_heading_unavailability_is_explicit(context);
	test_missing_magnetic_heading_parameter_is_explicit(context);
	test_non_finite_magnetic_heading_is_rejected(context);
	test_zero_radar_and_ir_samples_are_available(context);
	test_typed_radar_and_ir_units(context);
	test_missing_radar_parameter_marks_only_radar_unavailable(context);
	test_non_finite_radar_and_ir_values_are_rejected(context);
	test_unknown_radar_mode_is_invalid(context);
	test_weapon_station_observation_contract(context);
	test_weapon_station_invalid_revision_is_explicit(context);
	test_weapon_station_unavailable_reason_is_preserved(context);
	test_non_finite_weapon_station_value_is_rejected(context);
}
