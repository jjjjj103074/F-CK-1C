#include "SystemPipelineTestFixture.h"
#include "ExternalActionTestSupport.h"
#include "Core/Systems/MissionComputer/Components/SensorOfInterest/SensorOfInterest.h"

#include <algorithm>
#include <set>

namespace Core::Systems::Catalog::DisplaySystem
{
SystemEntry create_entry();
}
namespace Core::Systems::Catalog::MissionComputer
{
SystemEntry create_entry();
}
namespace Core::Systems::Catalog::RadarSystem
{
SystemEntry create_entry();
}

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace Core::Systems::Mc;
using namespace std::chrono;
constexpr nanoseconds kPeriod(31'250'000);

struct Fixture
{
	Tests::RecordingDebugTelemetry telemetry;
	SystemPipeline pipeline{ { StartMode::HotGround, {}, {}, telemetry },
		                     { Catalog::MissionComputer::create_entry(),
		                       Catalog::RadarSystem::create_entry(),
		                       Catalog::DisplaySystem::create_entry() } };
	FrameInput frame;
	DcsBridge::Internal::ExternalActionTransport transport;
	std::vector<RadarRequest> requests;
	std::set<std::uint64_t> recorded;
	CombatAvionicsSnapshot mission;
	DisplaySnapshot display;
	RadarSnapshot radar;

	Fixture()
	{
		frame.cockpit.mission_adapter.available = true;
		frame.cockpit.mission_adapter.radar_power_on = true;
		frame.cockpit.weapon_stations.status = { true, 2, ObservationInvalidReason::None };
		frame.cockpit.radar.status = { true, 2, ObservationInvalidReason::None };
		frame.cockpit.radar.mode = RadarMode::Search;
	}

	void advance(int ticks = 1)
	{
		for (int i = 0; i < ticks; ++i)
		{
			frame.cockpit.external_action_feedback = transport.feedback();
			const auto snapshot = std::make_unique<AircraftDataSnapshot>(
			    SystemPipelineTest::step_pipeline_to(pipeline, frame, {}, pipeline.advanced_through() + kPeriod));
			mission = snapshot->read(AircraftDataKeys::kMissionComputerSnapshot);
			display = snapshot->read(AircraftDataKeys::kDisplaySnapshot);
			radar = snapshot->read(AircraftDataKeys::kRadarSnapshot);
			for (const auto& request : snapshot->read(AircraftDataKeys::kRadarRequests).items)
				if (recorded.insert(request.sequence).second)
					requests.push_back(request);
			transport.collect(mission.action_requests, radar);
			const auto batch = transport.batch();
			if (batch.count)
				transport.acknowledge(batch.first_sequence + batch.count - 1);
		}
	}

	void send(CommandId id, double value = 1)
	{
		pipeline.send({ id, value });
	}
	void select(CommandId id)
	{
		send(id);
		advance(8);
	}
	void tap(CommandId id)
	{
		send(id);
		advance();
		send(id, 0);
		advance(6);
	}
	std::size_t count(RadarOperation operation) const
	{
		return static_cast<std::size_t>(std::count_if(requests.begin(),
		                                              requests.end(),
		                                              [operation](const RadarRequest& request)
		                                              {
			                                              return request.command.operation == operation;
		                                              }));
	}
	std::string text(const std::string& name) const
	{
		for (std::size_t i = 0; i < telemetry.channels().size(); ++i)
			if (telemetry.channels()[i].name == name)
				for (auto update = telemetry.updates().rbegin(); update != telemetry.updates().rend(); ++update)
					if (update->id == i)
						return std::get<std::string>(update->value);
		return {};
	}
};

void test_soi_uses_display_sources_and_follows_the_sensor(Tests::Context& c)
{
	ModeData mode;
	mode.master_mode = MasterMode::AirToAir;
	DisplaySnapshot display;
	display.status = { true, 1 };
	display.operating_mode = OperatingMode::AirToAir;
	display.left = { DisplayFunction::FireControlRadar, SensorControlSource::Radar };
	display.right = { DisplayFunction::StoresManagement, SensorControlSource::None };
	SensorOfInterestSnapshot selected;
	SensorOfInterest soi(mode, display, selected);
	soi.refresh();
	TEST_EXPECT(c, selected.source == SensorControlSource::Radar && selected.location == DisplayLocation::LeftMfd);
	soi.handle({ CommandId::PressDmsDown, 1 });
	soi.handle({ CommandId::PressDmsUp, 1 });
	TEST_EXPECT(c, selected.source == SensorControlSource::Radar && selected.location == DisplayLocation::LeftMfd);
	std::swap(display.left, display.right);
	soi.refresh();
	TEST_EXPECT(c, selected.source == SensorControlSource::Radar && selected.location == DisplayLocation::RightMfd);
	display.status.available = false;
	soi.refresh();
	TEST_EXPECT(c, selected.source == SensorControlSource::None && !selected.display_context_ready);
	display.status.available = true;
	soi.refresh();
	TEST_EXPECT(c, selected.source == SensorControlSource::Radar && selected.location == DisplayLocation::RightMfd);
}

void test_navigation_and_ground_shells_are_explicit(Tests::Context& c)
{
	Fixture f;
	f.advance(4);
	TEST_EXPECT(c, f.display.left.function == DisplayFunction::HorizontalSituation);
	TEST_EXPECT(c, f.display.right.function == DisplayFunction::StoresManagement);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Hud);
	f.tap(CommandId::PressDmsDown);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Navigation);
	f.tap(CommandId::SetTmsUp);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == 0);
	TEST_EXPECT(c, f.text("mc_tms_receiver") == "HSD" && f.text("mc_tms_result") == "RESERVED");
	f.select(CommandId::SelectAirToGroundMode);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Radar);
	f.tap(CommandId::SetTmsUp);
	TEST_EXPECT(c, f.count(RadarOperation::DesignateGroundTarget) == 1);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == 0);
	TEST_EXPECT(c, f.text("mc_tms_result") == "RESERVED");
	f.tap(CommandId::PressDmsUp);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Hud);
}

void test_mode_and_display_snapshots_settle_through_pipeline(Tests::Context& c)
{
	Fixture f;
	f.advance(4);
	f.send(CommandId::SelectAirToAirMode);
	f.advance();
	TEST_EXPECT(c, !f.mission.sensor_of_interest.display_context_ready);
	f.advance(3);
	TEST_EXPECT(c, f.mission.sensor_of_interest.display_context_ready);
	TEST_EXPECT(c, f.display.operating_mode == OperatingMode::AirToAir);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Radar);
	f.tap(CommandId::PressDmsUp);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Radar);
	f.tap(CommandId::PressDmsLeft);
	TEST_EXPECT(c, f.display.last_format_step == DisplayLocation::LeftMfd);
	TEST_EXPECT(c, f.display.left.function == DisplayFunction::FireControlRadar);
	f.tap(CommandId::PressDmsRight);
	TEST_EXPECT(c, f.display.last_format_step == DisplayLocation::RightMfd);
	TEST_EXPECT(c, f.display.right.function == DisplayFunction::StoresManagement);
}

void test_crm_designates_on_release_and_preserves_observation_authority(Tests::Context& c)
{
	Fixture f;
	f.select(CommandId::SelectAirToAirMode);
	f.send(CommandId::SetMasterArmOff);
	f.send(CommandId::SetTmsUp);
	f.advance(6);
	TEST_EXPECT(c, f.count(RadarOperation::BeginSpotlight) == 1);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == 0);
	TEST_EXPECT(c, !f.mission.fire_control.target_designation.requested);
	f.send(CommandId::SetTmsUp, 0);
	f.advance(6);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == 1);
	TEST_EXPECT(c, f.mission.fire_control.target_designation.requested);
	TEST_EXPECT(c, !f.mission.fire_control.target_designation.radar_tracking);
	f.frame.cockpit.radar.mode = RadarMode::SingleTargetTrack;
	f.frame.cockpit.radar.stt_range_m = 3000;
	f.advance(3);
	TEST_EXPECT(c, f.mission.fire_control.target_designation.radar_tracking);
	f.tap(CommandId::PressTmsDown);
	TEST_EXPECT(c, !f.mission.fire_control.target_designation.requested);
	TEST_EXPECT(c, f.mission.fire_control.target_designation.radar_tracking);
	TEST_EXPECT(c, f.mission.mode.master_arm_mode == MasterArmMode::Off);
}

void test_priority_step_data_and_tws_have_explicit_backend_results(Tests::Context& c)
{
	Fixture f;
	f.select(CommandId::SelectMissileOverride);
	f.tap(CommandId::PressTmsLeft);
	TEST_EXPECT(c, f.count(RadarOperation::ExpandTargetData) == 1);
	TEST_EXPECT(c, f.text("mc_tms_result") == "RESERVED");
	f.tap(CommandId::PressTmsRight);
	TEST_EXPECT(c, f.count(RadarOperation::StepPriorityTarget) == 1);
	f.send(CommandId::PressTmsRight);
	f.advance(40);
	f.send(CommandId::PressTmsRight, 0);
	f.advance(6);
	TEST_EXPECT(c, f.radar.requested_search_mode == RadarSearchMode::TrackWhileScan);
	TEST_EXPECT(c, f.radar.observation.mode == RadarMode::Search);
	TEST_EXPECT(c, !f.mission.fire_control.target_designation.radar_tracking);
	TEST_EXPECT(c, f.text("mc_tms_result") == "RESERVED");
	const auto steps = f.count(RadarOperation::StepPriorityTarget);
	f.send(CommandId::PressTmsRight);
	f.advance(40);
	f.send(CommandId::PressTmsRight, 0);
	f.advance(6);
	TEST_EXPECT(c, f.radar.requested_search_mode == RadarSearchMode::RangeWhileSearch);
	TEST_EXPECT(c, f.count(RadarOperation::StepPriorityTarget) == steps);
}

void test_tms_right_one_second_boundary_and_mode_change(Tests::Context& c)
{
	Fixture f;
	f.select(CommandId::SelectAirToAirMode);
	f.send(CommandId::PressTmsRight);
	f.advance(32);
	f.send(CommandId::PressTmsRight, 0);
	f.advance(6);
	TEST_EXPECT(c, f.count(RadarOperation::StepPriorityTarget) == 1);
	TEST_EXPECT(c, f.radar.requested_search_mode == RadarSearchMode::RangeWhileSearch);
	f.send(CommandId::PressTmsRight);
	f.advance(33);
	f.send(CommandId::PressTmsRight, 0);
	f.advance(6);
	TEST_EXPECT(c, f.count(RadarOperation::StepPriorityTarget) == 1);
	TEST_EXPECT(c, f.radar.requested_search_mode == RadarSearchMode::TrackWhileScan);
	TEST_EXPECT(c, f.text("mc_tms_request").find("SEARCH MODE #") == 0);
	TEST_EXPECT(c, f.text("mc_tms_result") == "RESERVED");
	f.send(CommandId::SetTmsUp);
	f.advance();
	f.select(CommandId::SelectDogfightMode);
	const auto captures = f.count(RadarOperation::BeginAcquisition);
	f.send(CommandId::SetTmsUp);
	f.advance();
	f.send(CommandId::SetTmsUp, 0);
	f.advance(6);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == captures);
	f.tap(CommandId::SetTmsUp);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == captures + 1);
}

void test_acm_inputs_and_mode_change_clear_held_operations(Tests::Context& c)
{
	Fixture f;
	f.select(CommandId::SelectDogfightMode);
	TEST_EXPECT(c, f.radar.requested_acquisition_mode == TargetAcquisitionMode::WideScan);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == 0);
	f.tap(CommandId::SetTmsUp);
	TEST_EXPECT(c, f.radar.requested_acquisition_mode == TargetAcquisitionMode::Boresight);
	f.tap(CommandId::PressTmsDown);
	TEST_EXPECT(c, f.radar.requested_acquisition_mode == TargetAcquisitionMode::WideScan);
	f.tap(CommandId::PressTmsDown);
	TEST_EXPECT(c, f.radar.requested_acquisition_mode == TargetAcquisitionMode::VerticalScan);
	f.tap(CommandId::PressTmsRight);
	TEST_EXPECT(c, f.radar.requested_acquisition_mode == TargetAcquisitionMode::WideScan);
	f.send(CommandId::SetTmsUp);
	f.advance();
	f.select(CommandId::SelectOverrideCenter);
	const auto captures = f.count(RadarOperation::BeginAcquisition);
	f.send(CommandId::SetTmsUp, 0);
	f.advance(6);
	TEST_EXPECT(c, f.count(RadarOperation::BeginAcquisition) == captures);
	TEST_EXPECT(c, f.mission.sensor_of_interest.source == SensorControlSource::Hud);
	TEST_EXPECT(c, f.text("mc_mfd_left") == "HSD (SHELL) / HSD");
	TEST_EXPECT(c, f.text("mc_mfd_right") == "SMS / NONE");
	TEST_EXPECT(c, f.text("mc_soi_location") == "HUD");
}
}

void run_sensor_control_tests(Tests::Context& context)
{
	test_soi_uses_display_sources_and_follows_the_sensor(context);
	test_navigation_and_ground_shells_are_explicit(context);
	test_mode_and_display_snapshots_settle_through_pipeline(context);
	test_crm_designates_on_release_and_preserves_observation_authority(context);
	test_priority_step_data_and_tws_have_explicit_backend_results(context);
	test_acm_inputs_and_mode_change_clear_held_operations(context);
	test_tms_right_one_second_boundary_and_mode_change(context);
}
