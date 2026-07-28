#include "TestFileUtils.h"
#include "TestHarness.h"

#include "DcsBridge/Internal/BoundaryValidator.h"
#include "DcsBridge/Internal/EfmEventReporter.h"
#include "DcsBridge/Internal/EventLog.h"
#include "DcsBridge/Internal/FrameInputCollector.h"
#include "DcsBridge/Internal/OutputStore.h"

#include <array>
#include <limits>
#include <vector>

namespace
{
constexpr double kFrameDt = 0.001;
constexpr double kOriginalDensity = 1.225;
constexpr double kOriginalCompression = 0.2;
constexpr double kBelowMinimumDamageIntegrity = -0.1;
constexpr double kAboveMaximumDamageIntegrity = 1.1;
constexpr float kCarrierReadyEventPhase = 1.0F;

class ValidationFixture final
{
public:
	ValidationFixture()
		: event_log(root.path().string().c_str()),
		reporter(event_log, output_store)
	{
	}

	TestFiles::TemporaryDirectory root = TestFiles::TemporaryDirectory("bdv");
	DcsBridge::Internal::EventLog event_log;
	DcsBridge::Internal::OutputStore output_store;
	DcsBridge::Internal::EfmEventReporter reporter;
};

void test_invalid_sample_preserves_latest(Tests::Context& context)
{
	ValidationFixture fixture;
	TEST_EXPECT(context, fixture.root.valid());
	DcsBridge::Internal::FrameInputCollector collector;
	Core::AtmosphereInput valid = {};
	valid.density = kOriginalDensity;
	TEST_EXPECT(context, DcsBridge::Internal::validate_atmosphere_input(
		valid, fixture.reporter));
	collector.publish_atmosphere(valid);
	Core::AtmosphereInput invalid = valid;
	invalid.density = std::numeric_limits<double>::quiet_NaN();
	if (DcsBridge::Internal::validate_atmosphere_input(invalid, fixture.reporter))
	{
		collector.publish_atmosphere(invalid);
	}
	const Core::FrameInput snapshot = collector.snapshot(kFrameDt);
	TEST_EXPECT_NEAR(context, snapshot.atmosphere.density, kOriginalDensity, 0.0);
	const std::string log = TestFiles::read_text_while_open(
		fixture.root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(context, log.find(
		"callback=ed_fm_set_atmosphere field=density invalid numeric value=nan") !=
		std::string::npos);
}

void test_frame_dt_contract(Tests::Context& context)
{
	TEST_EXPECT(context, DcsBridge::Internal::is_valid_frame_dt(kFrameDt));
	TEST_EXPECT(context, !DcsBridge::Internal::is_valid_frame_dt(0.0));
	TEST_EXPECT(context, !DcsBridge::Internal::is_valid_frame_dt(-kFrameDt));
	TEST_EXPECT(context, !DcsBridge::Internal::is_valid_frame_dt(
		std::numeric_limits<double>::quiet_NaN()));
	TEST_EXPECT(context, !DcsBridge::Internal::is_valid_frame_dt(
		std::numeric_limits<double>::infinity()));
}

void test_invalid_typed_inputs_are_rejected(Tests::Context& context)
{
	ValidationFixture fixture;
	const double infinity = std::numeric_limits<double>::infinity();
	Core::SurfaceInput surface = {};
	surface.normal.x = infinity;
	TEST_EXPECT(context, !DcsBridge::Internal::validate_surface_input(
		surface, fixture.reporter));
	Core::MassStateInput mass = {};
	mass.mass = infinity;
	TEST_EXPECT(context, !DcsBridge::Internal::validate_mass_input(
		mass, fixture.reporter));
	Core::WorldKinematicsInput world = {};
	world.orientation.w = infinity;
	TEST_EXPECT(context, !DcsBridge::Internal::validate_world_kinematics_input(
		world, fixture.reporter));
	Core::BodyKinematicsInput body = {};
	body.angle_of_attack = infinity;
	TEST_EXPECT(context, !DcsBridge::Internal::validate_body_kinematics_input(
		body, fixture.reporter));
	Core::ExternalFuelInput external = {};
	external.position.z = infinity;
	TEST_EXPECT(context, !DcsBridge::Internal::validate_external_fuel_input(
		external, fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_internal_fuel_input(
		infinity, fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_refueling_fuel_input(
		infinity, fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_damage_input(
		infinity, fixture.reporter));
}

void test_damage_integrity_range(Tests::Context& context)
{
	ValidationFixture fixture;
	TEST_EXPECT(context, DcsBridge::Internal::validate_damage_input(
		0.0, fixture.reporter));
	TEST_EXPECT(context, DcsBridge::Internal::validate_damage_input(
		1.0, fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_damage_input(
		kBelowMinimumDamageIntegrity, fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_damage_input(
		kAboveMaximumDamageIntegrity, fixture.reporter));
	const std::string log = TestFiles::read_text_while_open(
		fixture.root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(context, log.find(
		"callback=ed_fm_on_damage field=integrity out_of_range "
		"value=-0.10000000000000001 expected=[0,1]") != std::string::npos);
	TEST_EXPECT(context, log.find(
		"callback=ed_fm_on_damage field=integrity out_of_range "
		"value=1.1000000000000001 expected=[0,1]") != std::string::npos);
}

ed_fm_suspension_info make_suspension_info()
{
	ed_fm_suspension_info info = {};
	info.acting_force[1] = 1000.0;
	info.struct_compression = kOriginalCompression;
	return info;
}

void test_suspension_rejection_preserves_latest(Tests::Context& context)
{
	ValidationFixture fixture;
	DcsBridge::Internal::FrameInputCollector collector;
	ed_fm_suspension_info info = make_suspension_info();
	TEST_EXPECT(context, DcsBridge::Internal::validate_suspension_feedback(
		0, &info, fixture.reporter));
	collector.publish_suspension({
		0,
		{ info.acting_force[0], info.acting_force[1], info.acting_force[2] },
		{},
		info.integrity_factor,
		info.struct_compression,
		info.wheel_speed_X
	});
	info.struct_compression = std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT(context, !DcsBridge::Internal::validate_suspension_feedback(
		0, &info, fixture.reporter));
	const Core::FrameInput snapshot = collector.snapshot(kFrameDt);
	TEST_EXPECT_NEAR(
		context,
		snapshot.suspension[0].compression,
		kOriginalCompression,
		0.0);
}

void test_invalid_pointer_and_index_are_rejected(Tests::Context& context)
{
	ValidationFixture fixture;
	ed_fm_suspension_info info = make_suspension_info();
	TEST_EXPECT(context, !DcsBridge::Internal::validate_suspension_feedback(
		-1, &info, fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_suspension_feedback(
		0, nullptr, fixture.reporter));
	std::array<EdDrawArgument, 1> draw_args = {};
	TEST_EXPECT(context, !DcsBridge::Internal::validate_draw_args_buffer(
		draw_args.data(), draw_args.size(), fixture.reporter));
	TEST_EXPECT(context, !DcsBridge::Internal::validate_draw_args_buffer(
		nullptr,
		DcsBridge::Internal::required_draw_arg_count(),
		fixture.reporter));
	std::vector<EdDrawArgument> valid(
		DcsBridge::Internal::required_draw_arg_count());
	TEST_EXPECT(context, DcsBridge::Internal::validate_draw_args_buffer(
		valid.data(), valid.size(), fixture.reporter));
}

void test_simulation_event_numeric_input(Tests::Context& context)
{
	ValidationFixture fixture;
	ed_fm_simulation_event event = {};
	event.event_type = ED_FM_EVENT_CARRIER_CATAPULT;
	event.event_params[0] = std::numeric_limits<float>::infinity();
	TEST_EXPECT(context, !DcsBridge::Internal::validate_simulation_event_input(
		event, fixture.reporter));
	event.event_params[0] = kCarrierReadyEventPhase;
	TEST_EXPECT(context, DcsBridge::Internal::validate_simulation_event_input(
		event, fixture.reporter));
	const std::string log = TestFiles::read_text_while_open(
		fixture.root.path() / "log" / "fck1c_efm.log");
	TEST_EXPECT(context, log.find(
		"callback=ed_fm_push_simulation_event field=event_params[0] ") !=
		std::string::npos);
}
}

void run_boundary_validator_tests(Tests::Context& context)
{
	test_invalid_sample_preserves_latest(context);
	test_frame_dt_contract(context);
	test_invalid_typed_inputs_are_rejected(context);
	test_damage_integrity_range(context);
	test_suspension_rejection_preserves_latest(context);
	test_invalid_pointer_and_index_are_rejected(context);
	test_simulation_event_numeric_input(context);
}
