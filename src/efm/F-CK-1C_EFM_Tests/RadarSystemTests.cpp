#include "SystemPipelineTestFixture.h"
#include "Core/Fck1cEfm.h"
#include "ExternalActionTestSupport.h"
#include "Core/Systems/RadarSystem/RadarSystem.h"
#include "Core/Systems/MissionComputer/Components/FireControl/Internal/Targeting.h"
#include <chrono>
namespace Core::Systems::Catalog::RadarSystem
{
SystemEntry create_entry();
}
namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace std::chrono;
constexpr nanoseconds period{ 31'250'000 };
struct RadarFixture
{
	RadarRequests requests;
	SystemPipeline pipeline;
	FrameInput frame;
	RadarFixture()
	    : pipeline(SystemPipelineTest::flight_setup(),
	               { Catalog::RadarSystem::create_entry(),
	                 SystemPipelineTest::entry({ "radar_test_producer",
	                                             [this](SystemSetup& setup)
	                                             {
		                                             setup.publish(AircraftDataKeys::kRadarRequests, requests);
	                                             },
	                                             [this](const AircraftDataView&, SystemResult& result)
	                                             {
		                                             result.publish(AircraftDataKeys::kRadarRequests, requests);
	                                             },
	                                             32 }) })
	{
		frame.cockpit.mission_adapter.available = true;
		frame.cockpit.mission_adapter.radar_power_on = true;
	}
	RadarSnapshot step(nanoseconds at)
	{
		return SystemPipelineTest::step_pipeline_to(pipeline, frame, {}, at).read(AircraftDataKeys::kRadarSnapshot);
	}
};
void test_radar_requests_survive_snapshots_and_execute_once(Tests::Context& c)
{
	RadarFixture f;
	f.requests.items = {
		{ 4, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::BeyondVisualRange } },
		{ 7, { RadarOperation::BeginAcquisition, TargetAcquisitionMode::BeyondVisualRange, AcquisitionInput::Pulse } },
		{ 9, { RadarOperation::ReleaseAcquisition } }
	};
	auto output = f.step(period);
	TEST_EXPECT(c, output.command_results.empty());
	output = f.step(period * 2);
	TEST_EXPECT(c, output.command_results.size() == 3);
	TEST_EXPECT(c, output.action_requests.items.size() == 3);
	TEST_EXPECT(c, output.action_requests.items[0].action.kind == MissionActionKind::ModeBeyondVisualRange);
	TEST_EXPECT(c, output.action_requests.items[1].action.kind == MissionActionKind::ModeFi0);
	TEST_EXPECT(c, output.action_requests.items[2].action.kind == MissionActionKind::LockStart);
	TEST_EXPECT(c, output.action_requests.items[2].parent_request == 7);
	output = f.step(period * 3);
	TEST_EXPECT(c, output.command_results.size() == 3 && output.action_requests.items.size() == 3);
	f.frame.cockpit.external_action_feedback.radar_acknowledged_through = 3;
	output = f.step(period * 10);
	TEST_EXPECT(c, output.action_requests.items.empty());
	output = f.step(period * 12);
	TEST_EXPECT(c, output.action_requests.items.empty());
	output = f.step(period * 13);
	TEST_EXPECT(c, output.action_requests.items.size() == 1);
	TEST_EXPECT(c, output.action_requests.items[0].action.kind == MissionActionKind::LockFinish);
	TEST_EXPECT(c, output.action_requests.items[0].parent_request == 0);
}
void test_radar_observations_and_unavailable_capture_remain_distinct(Tests::Context& c)
{
	RadarFixture f;
	f.frame.cockpit.mission_adapter.available = false;
	f.frame.cockpit.radar.status = { true, 11, ObservationInvalidReason::None };
	f.frame.cockpit.radar.mode = RadarMode::SingleTargetTrack;
	f.frame.cockpit.radar.stt_range_m = 3000;
	f.requests.items = { { 1, { RadarOperation::BeginAcquisition } } };
	f.step(period);
	auto output = f.step(period * 2);
	TEST_EXPECT(c, !output.adapter_available && !output.command_results[0].accepted);
	TEST_EXPECT(c, output.action_requests.items.empty());
	TEST_EXPECT(c, output.observation.status.available && output.observation.mode == RadarMode::SingleTargetTrack);
	f.frame.cockpit.radar.status = { false, 12, ObservationInvalidReason::ParameterUnavailable };
	output = f.step(period * 3);
	TEST_EXPECT(c, !output.observation.status.available && output.observation.status.revision == 12);
	f.frame.cockpit.mission_adapter.available = true;
	f.requests.items.push_back({ 2, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::BeyondVisualRange } });
	f.requests.items.push_back(
	    { 3, { RadarOperation::BeginAcquisition, TargetAcquisitionMode::BeyondVisualRange, AcquisitionInput::Hold } });
	f.step(period * 4);
	output = f.step(period * 5);
	TEST_EXPECT(c, output.command_results.back().accepted && output.action_requests.items.size() == 3);
}
void test_bridge_preserves_radar_dependency_and_maps_partial_ack(Tests::Context& c)
{
	ExternalActionOutbox mission;
	mission.append({ MissionActionKind::SelectStation, 3, 9 });
	const auto radar_request = mission.reserve_radar_operation();
	mission.append(MissionActionKind::PickleOn);
	DcsBridge::Internal::ExternalActionTransport tx;
	tx.collect(mission.pending(), {});
	auto batch = tx.batch();
	TEST_EXPECT(c, batch.count == 1 && batch.actions[0].kind == MissionActionKind::SelectStation);
	RadarSnapshot radar;
	radar.command_results.push_back({ radar_request, true, 1, 2 });
	radar.action_requests.items = {
		{ 1, ExternalRequestKind::Action, { MissionActionKind::ModeHelmet }, radar_request },
		{ 2, ExternalRequestKind::Action, { MissionActionKind::LockStart }, radar_request }
	};
	tx.collect(mission.pending(), radar);
	batch = tx.batch();
	TEST_EXPECT(c, batch.count == 4);
	TEST_EXPECT(c, batch.actions[1].kind == MissionActionKind::ModeHelmet);
	TEST_EXPECT(c, batch.actions[2].kind == MissionActionKind::LockStart);
	TEST_EXPECT(c, batch.actions[3].kind == MissionActionKind::PickleOn);
	tx.acknowledge(2);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == 1);
	TEST_EXPECT(c, tx.feedback().radar_acknowledged_through == 1);
	tx.acknowledge(3);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == radar_request);
	TEST_EXPECT(c, tx.feedback().radar_acknowledged_through == 2);
	mission.cancel_pending(
	    [](const MissionAction& action)
	    {
		    return is_weapon_start(action.kind);
	    });
	tx.collect(mission.pending(), radar);
	batch = tx.batch();
	TEST_EXPECT(c, batch.count == 1 && batch.actions[0].kind == MissionActionKind::None);
	tx.acknowledge(4);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == 3);
	tx.collect(mission.pending(), radar);
	TEST_EXPECT(c, tx.batch().count == 0);
	tx.reset();
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == 0 && tx.feedback().radar_acknowledged_through == 0);
}
void test_bridge_places_stop_before_radar_background_release(Tests::Context& c)
{
	ExternalActionOutbox mission;
	mission.append(MissionActionKind::PickleOff);
	RadarSnapshot radar;
	radar.action_requests.items = { { 1, ExternalRequestKind::Action, { MissionActionKind::LockFinish }, 0 },
		                            { 2, ExternalRequestKind::Action, { MissionActionKind::ChangeLockUp }, 0 } };
	DcsBridge::Internal::ExternalActionTransport tx;
	tx.collect(mission.pending(), radar);
	auto batch = tx.batch();
	TEST_EXPECT(c, batch.count == 3);
	TEST_EXPECT(c, batch.actions[0].kind == MissionActionKind::PickleOff);
	TEST_EXPECT(c, batch.actions[1].kind == MissionActionKind::LockFinish);
	TEST_EXPECT(c, batch.actions[2].kind == MissionActionKind::ChangeLockUp);
	tx.acknowledge(4);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == 0);
	tx.acknowledge(3);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == 1 && tx.feedback().radar_acknowledged_through == 2);
}
void test_core_projects_radar_and_resets_flight_requests(Tests::Context& c)
{
	Core::Fck1cEfm core(Tests::disabled_debug_telemetry());
	DcsBridge::Internal::ExternalActionTransport transport;
	auto output = core.start(StartMode::HotAir);
	TEST_EXPECT(c, !output.cockpit.radar.status.available);
	FrameInput frame;
	frame.dt_s = 0.03125;
	frame.cockpit.mission_adapter.available = true;
	frame.cockpit.mission_adapter.radar_power_on = true;
	frame.cockpit.weapon_stations.status = { true, 2, ObservationInvalidReason::None };
	for (auto& station : frame.cockpit.weapon_stations.stations)
		station.configuration = StoreConfiguration::Empty;
	core.handle_command({ CommandId::SelectDogfightMode, 1 });
	for (int i = 0; i < 5; ++i)
	{
		frame.cockpit.external_action_feedback = transport.feedback();
		output = core.step(frame);
		transport.collect(output.cockpit.combat_avionics.action_requests, output.cockpit.radar);
		const auto batch = transport.batch();
		if (batch.count)
			transport.acknowledge(batch.first_sequence + batch.count - 1);
		frame.cockpit.weapon_stations.selection_ack_revision =
		    output.cockpit.combat_avionics.stores.selection.request_revision;
	}
	TEST_EXPECT(c, output.cockpit.radar.status.available);
	TEST_EXPECT(c, output.cockpit.combat_avionics.mode.operating_mode == OperatingMode::Dogfight);
	TEST_EXPECT(c, transport.feedback().radar_acknowledged_through != 0);
	core.release();
	transport.reset();
	output = core.start(StartMode::HotAir);
	TEST_EXPECT(c, output.cockpit.combat_avionics.action_requests.items.empty());
	TEST_EXPECT(c, !output.cockpit.radar.status.available && output.cockpit.radar.action_requests.items.empty());
	TEST_EXPECT(c, transport.feedback().radar_acknowledged_through == 0);
	core.release();
}

void test_targeting_ignores_old_rejections_after_cancel_and_new_request(Tests::Context& c)
{
	Core::Systems::Mc::ModeData mode;
	mode.master_mode = MasterMode::AirToAir;
	Core::Systems::Mc::TargetingData data;
	RadarSnapshot radar;
	ExternalActionOutbox actions;
	Core::Systems::Mc::RadarRequestQueue requests(actions);
	Core::Systems::Mc::Internal::Targeting targeting(mode, radar, data, requests);
	targeting.handle({ CommandId::SetTmsUp, 1 });
	targeting.handle({ CommandId::SetTmsUp, 0 });
	const auto first = requests.pending().items.back().sequence;
	targeting.handle({ CommandId::PressTmsDown, 1 });
	targeting.handle({ CommandId::SetTmsUp, 1 });
	targeting.handle({ CommandId::SetTmsUp, 0 });
	const auto latest = requests.pending().items.back().sequence;
	TEST_EXPECT(c, latest > first && data.designation.requested);
	radar.command_results = { { first, false, 0, 0 } };
	targeting.step();
	TEST_EXPECT(c, data.designation.requested);
	radar.command_results.push_back({ latest, false, 0, 0 });
	targeting.step();
	TEST_EXPECT(c, !data.designation.requested);
}
void test_bridge_completes_empty_radar_requests_without_device_ack(Tests::Context& c)
{
	ExternalActionOutbox source;
	const auto request = source.reserve_radar_operation();
	source.append(MissionActionKind::GunOff);
	DcsBridge::Internal::ExternalActionTransport tx;
	tx.collect(source.pending(), {});
	TEST_EXPECT(c, tx.batch().count == 0);
	RadarSnapshot radar;
	radar.command_results = { { request, false, 0, 0 } };
	tx.collect(source.pending(), radar);
	const auto batch = tx.batch();
	TEST_EXPECT(c, batch.count == 2 && batch.actions[0].kind == MissionActionKind::None);
	TEST_EXPECT(c, batch.actions[1].kind == MissionActionKind::GunOff);
	tx.acknowledge(1);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == request);
	TEST_EXPECT(c, tx.feedback().radar_acknowledged_through == 0);
	tx.acknowledge(2);
	TEST_EXPECT(c, tx.feedback().mission_acknowledged_through == 2);
}

void test_rws_and_bore_publish_different_scan_geometry(Tests::Context& c)
{
	RadarFixture f;
	f.frame.cockpit.radar.tdc_available = true;
	f.frame.cockpit.radar.tdc_range_m = 10000.0;
	f.requests.items = { { 1,
		                   { RadarOperation::SetSearchMode,
		                     TargetAcquisitionMode::None,
		                     AcquisitionInput::Pulse,
		                     RadarSearchMode::RangeWhileSearch } } };
	f.step(period);
	auto output = f.step(period * 2);
	TEST_EXPECT(c, output.scan.enabled && output.scan.attitude_stabilized && !output.scan.override_tdc);
	TEST_EXPECT_NEAR(c, output.scan.azimuth_width_rad, Common::rad(120.0), 1e-9);
	TEST_EXPECT_NEAR(c, output.scan.maximum_search_range_m, 90000.0, 1e-9);
	const auto revision = output.scan.revision;
	f.requests.items.push_back({ 2, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::BeyondVisualRange } });
	f.step(period * 3);
	output = f.step(period * 4);
	TEST_EXPECT(c, output.scan.revision == revision);
	f.requests.items.push_back({ 3,
	                             { RadarOperation::SetSearchMode,
	                               TargetAcquisitionMode::None,
	                               AcquisitionInput::Pulse,
	                               RadarSearchMode::AirCombat } });
	f.step(period * 5);
	output = f.step(period * 6);
	TEST_EXPECT(c, output.active_search_mode == RadarSearchMode::AirCombat);
	TEST_EXPECT(c, !output.scan.attitude_stabilized && output.scan.override_tdc);
	TEST_EXPECT_NEAR(c, output.scan.azimuth_width_rad, Common::rad(3.3), 1e-9);
	TEST_EXPECT_NEAR(c, output.scan.elevation_height_rad, Common::rad(4.6), 1e-9);
	TEST_EXPECT_NEAR(c, output.scan.maximum_search_range_m, 18520.0, 1e-9);
	TEST_EXPECT(c, !output.automatic_acquisition_requested);
}

void test_bore_waits_for_a_target_without_repeating_lock_commands(Tests::Context& c)
{
	RadarFixture f;
	f.frame.cockpit.radar.status = { true, 1, ObservationInvalidReason::None };
	f.frame.cockpit.radar.mode = RadarMode::Search;
	f.requests.items = { { 1, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::Boresight } },
		                 { 2, { RadarOperation::BeginAcquisition } } };
	f.step(period);
	auto output = f.step(period * 2);
	TEST_EXPECT(c, output.automatic_acquisition_requested && output.action_requests.items.size() == 2);
	TEST_EXPECT(c, output.action_requests.items[1].action.kind == MissionActionKind::LockStart);
	f.frame.cockpit.external_action_feedback.radar_acknowledged_through = 2;
	f.frame.cockpit.radar.mode = RadarMode::LockAttempt;
	output = f.step(period * 20);
	TEST_EXPECT(c, output.action_requests.items.empty());
	output = f.step(period * 40);
	TEST_EXPECT(c, output.action_requests.items.empty() && output.automatic_acquisition_requested);
	f.frame.cockpit.radar.mode = RadarMode::SingleTargetTrack;
	f.frame.cockpit.radar.stt_range_m = 22000.0;
	output = f.step(period * 41);
	TEST_EXPECT(c, output.observation.mode == RadarMode::SingleTargetTrack);
	TEST_EXPECT_NEAR(c, output.scan.maximum_search_range_m, 90000.0, 1e-9);
	TEST_EXPECT(c, !output.scan.override_tdc);
	TEST_EXPECT(c, output.action_requests.items.size() == 1);
	TEST_EXPECT(c, output.action_requests.items[0].action.kind == MissionActionKind::LockFinish);
	f.frame.cockpit.external_action_feedback.radar_acknowledged_through = 3;
	f.frame.cockpit.radar.mode = RadarMode::Search;
	output = f.step(period * 42);
	TEST_EXPECT(c, output.action_requests.items.size() == 1);
	TEST_EXPECT(c, output.action_requests.items[0].action.kind == MissionActionKind::LockStart);
	f.requests.items.push_back({ 3, { RadarOperation::CancelAcquisition } });
	f.step(period * 43);
	output = f.step(period * 44);
	TEST_EXPECT(c, !output.automatic_acquisition_requested);
	TEST_EXPECT(c, output.action_requests.items.back().action.kind == MissionActionKind::LockFinish);
}

void test_rws_cursor_moves_only_while_its_direction_is_held(Tests::Context& c)
{
	RadarFixture f;
	f.frame.cockpit.radar.tdc_available = true;
	f.frame.cockpit.radar.tdc_range_m = 5000.0;
	f.requests.items = { { 1, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::BeyondVisualRange } } };
	f.step(period);
	f.step(period * 2);
	f.pipeline.send({ CommandId::SetRadarCursorRight, 1.0 });
	auto output = f.step(period * 3);
	TEST_EXPECT(c, output.scan.override_tdc && output.scan.tdc_azimuth_rad > 0.0);
	const double previous = output.scan.tdc_azimuth_rad;
	output = f.step(period * 4);
	TEST_EXPECT(c, output.scan.tdc_azimuth_rad > previous);
	f.frame.cockpit.radar.tdc_azimuth_rad = output.scan.tdc_azimuth_rad;
	f.pipeline.send({ CommandId::SetRadarCursorRight, 0.0 });
	output = f.step(period * 5);
	TEST_EXPECT(c, !output.scan.override_tdc);
	TEST_EXPECT_NEAR(c, output.scan.tdc_azimuth_rad, f.frame.cockpit.radar.tdc_azimuth_rad, 1e-9);
	f.requests.items.push_back({ 2, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::Boresight } });
	f.step(period * 6);
	f.step(period * 7);
	f.pipeline.send({ CommandId::SetRadarCursorRight, 1.0 });
	output = f.step(period * 8);
	TEST_EXPECT_NEAR(c, output.scan.tdc_azimuth_rad, 0.0, 1e-9);
}

void test_navigation_ends_held_bore_capture(Tests::Context& c)
{
	RadarFixture f;
	f.frame.cockpit.radar.status = { true, 1, ObservationInvalidReason::None };
	f.frame.cockpit.radar.mode = RadarMode::Search;
	f.requests.items = { { 1, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::Boresight } },
		                 { 2, { RadarOperation::BeginAcquisition } } };
	f.step(period);
	f.step(period * 2);
	f.requests.items.push_back({ 3, { RadarOperation::SelectNavigation } });
	f.step(period * 3);
	const auto output = f.step(period * 4);
	TEST_EXPECT(c, !output.scan.enabled && !output.automatic_acquisition_requested);
	TEST_EXPECT(c,
	            output.action_requests.items[output.action_requests.items.size() - 2].action.kind ==
	                MissionActionKind::LockFinish);
	TEST_EXPECT(c, output.action_requests.items.back().action.kind == MissionActionKind::ModeCannon);
}

void test_rejecting_stt_uses_one_native_toggle_and_preserves_observation(Tests::Context& c)
{
	RadarFixture f;
	f.frame.cockpit.radar.status = { true, 1, ObservationInvalidReason::None };
	f.frame.cockpit.radar.mode = RadarMode::SingleTargetTrack;
	f.requests.items = { { 1, { RadarOperation::CancelAcquisition } }, { 2, { RadarOperation::CancelAcquisition } } };
	f.step(period);
	const auto output = f.step(period * 2);
	const auto starts = std::count_if(output.action_requests.items.begin(),
	                                  output.action_requests.items.end(),
	                                  [](const auto& request)
	                                  {
		                                  return request.action.kind == MissionActionKind::LockStart;
	                                  });
	TEST_EXPECT(c, starts == 1);
	TEST_EXPECT(c, output.observation.mode == RadarMode::SingleTargetTrack);
	TEST_EXPECT(c, output.action_requests.items.back().action.kind == MissionActionKind::LockFinish);
}

void test_reserved_patterns_leave_the_supported_mode_running(Tests::Context& c)
{
	RadarFixture f;
	f.requests.items = { { 1, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::BeyondVisualRange } },
		                 { 2,
		                   { RadarOperation::SetSearchMode,
		                     TargetAcquisitionMode::None,
		                     AcquisitionInput::Pulse,
		                     RadarSearchMode::TrackWhileScan } },
		                 { 3, { RadarOperation::SetAcquisitionMode, TargetAcquisitionMode::VerticalScan } } };
	f.step(period);
	auto output = f.step(period * 2);
	TEST_EXPECT(c, output.requested_search_mode == RadarSearchMode::TrackWhileScan);
	TEST_EXPECT(c, output.active_search_mode == RadarSearchMode::RangeWhileSearch);
	TEST_EXPECT(c, output.requested_acquisition_mode == TargetAcquisitionMode::BeyondVisualRange);
	TEST_EXPECT(c, output.command_results[1].implementation == RadarImplementation::Reserved);
	TEST_EXPECT(c, output.command_results[2].implementation == RadarImplementation::Reserved);
	f.requests.items.push_back({ 4, { RadarOperation::SelectNavigation } });
	f.step(period * 3);
	output = f.step(period * 4);
	TEST_EXPECT(c, !output.scan.enabled && !output.automatic_acquisition_requested);
}

}
void run_radar_system_tests(Tests::Context& context)
{
	test_rws_cursor_moves_only_while_its_direction_is_held(context);
	test_navigation_ends_held_bore_capture(context);
	test_rws_and_bore_publish_different_scan_geometry(context);
	test_bore_waits_for_a_target_without_repeating_lock_commands(context);
	test_rejecting_stt_uses_one_native_toggle_and_preserves_observation(context);
	test_reserved_patterns_leave_the_supported_mode_running(context);
	test_targeting_ignores_old_rejections_after_cancel_and_new_request(context);
	test_bridge_completes_empty_radar_requests_without_device_ack(context);
	test_core_projects_radar_and_resets_flight_requests(context);
	test_radar_requests_survive_snapshots_and_execute_once(context);
	test_radar_observations_and_unavailable_capture_remain_distinct(context);
	test_bridge_preserves_radar_dependency_and_maps_partial_ack(context);
	test_bridge_places_stop_before_radar_background_release(context);
}
