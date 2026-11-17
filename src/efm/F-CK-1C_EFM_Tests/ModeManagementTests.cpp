#include "ExternalActionTestSupport.h"
#include "TestHarness.h"

#include "Core/Systems/MissionComputer/Components/CountermeasureControl/CountermeasureControl.h"
#include "Core/Systems/MissionComputer/Components/ModeManagement/ModeManagement.h"

#include <type_traits>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems;
using namespace Core::Systems::Mc;

static_assert(!std::is_move_constructible<ModeEvents>::value, "Channels must retain their MC's address.");

void test_master_arm_is_independent_from_both_mode_selections(Tests::Context& context)
{
	for (MasterMode base : { MasterMode::Navigation, MasterMode::AirToAir, MasterMode::AirToGround })
		for (OverrideMode override : { OverrideMode::None, OverrideMode::Dogfight, OverrideMode::Missile })
		{
			ModeData data;
			data.master_mode = base;
			data.override_mode = override;
			ModeEvents events;
			ModeManagement modes(data, events);
			int arm_events = 0, mode_events = 0;
			sigslot::scoped_connection arm = events.master_arm_selected.connect(
			    [&](const MasterArmSelected& event)
			    {
				    ++arm_events;
				    TEST_EXPECT(context, data.master_arm == event.selected.master_arm);
				    TEST_EXPECT(context, event.selected.master_mode == base);
				    TEST_EXPECT(context, event.selected.override_mode == override);
			    });
			sigslot::scoped_connection mode = events.operating_mode_selected.connect(
			    [&](const OperatingModeSelected&)
			    {
				    ++mode_events;
			    });
			for (CommandId id : { CommandId::SetMasterArmOff, CommandId::SetMasterArmSim, CommandId::SetMasterArmOn })
				modes.handle({ id, 1.0 });
			TEST_EXPECT(context, arm_events == 3 && mode_events == 0);
			TEST_EXPECT(context, data.master_mode == base && data.override_mode == override);
		}
}

void test_override_restores_each_saved_master_mode(Tests::Context& context)
{
	for (MasterMode base : { MasterMode::Navigation, MasterMode::AirToAir, MasterMode::AirToGround })
		for (MasterArmMode arm : { MasterArmMode::Off, MasterArmMode::Simulation, MasterArmMode::On })
		{
			ModeData data;
			data.master_mode = base;
			data.master_arm = arm;
			const auto initial = data.operating_mode();
			ModeEvents events;
			ModeManagement modes(data, events);
			std::vector<OperatingModeSelected> selections;
			sigslot::scoped_connection connection = events.operating_mode_selected.connect(
			    [&](const OperatingModeSelected& event)
			    {
				    selections.push_back(event);
				    TEST_EXPECT(context, data.operating_mode() == event.selected.operating_mode());
			    });
			modes.handle({ CommandId::SelectDogfightMode, 1.0 });
			TEST_EXPECT(context, data.operating_mode() == OperatingMode::Dogfight);
			modes.handle({ CommandId::SelectAirToAirMode, 1.0 });
			modes.handle({ CommandId::SelectAirToGroundMode, 1.0 });
			modes.handle({ CommandId::SelectNavigationMode, 1.0 });
			TEST_EXPECT(context, data.master_mode == base);
			modes.handle({ CommandId::SelectMissileOverride, 1.0 });
			TEST_EXPECT(context, data.operating_mode() == OperatingMode::MissileOverride);
			modes.handle({ CommandId::SelectOverrideCenter, 1.0 });
			TEST_EXPECT(context, data.master_mode == base && data.override_mode == OverrideMode::None);
			TEST_EXPECT(context, data.operating_mode() == initial);
			TEST_EXPECT(context, data.master_arm == arm && selections.size() == 3);
			TEST_EXPECT(context, data.air_to_air() == (base == MasterMode::AirToAir));
			if (selections.size() == 3)
			{
				TEST_EXPECT(context, selections[0].previous.override_mode == OverrideMode::None);
				TEST_EXPECT(context, selections[0].selected.override_mode == OverrideMode::Dogfight);
				TEST_EXPECT(context, selections[1].previous.override_mode == OverrideMode::Dogfight);
				TEST_EXPECT(context, selections[2].previous.override_mode == OverrideMode::Missile);
				TEST_EXPECT(context, selections[2].selected.master_mode == base);
			}
		}
}

void test_primary_buttons_toggle_to_navigation(Tests::Context& context)
{
	ModeData data;
	ModeEvents events;
	ModeManagement modes(data, events);
	modes.handle({ CommandId::SelectAirToAirMode, 1.0 });
	TEST_EXPECT(context, data.master_mode == MasterMode::AirToAir && data.air_to_air());
	modes.handle({ CommandId::SelectAirToGroundMode, 1.0 });
	TEST_EXPECT(context, data.master_mode == MasterMode::AirToGround && !data.air_to_air());
	modes.handle({ CommandId::SelectAirToGroundMode, 1.0 });
	TEST_EXPECT(context, data.master_mode == MasterMode::Navigation);
	modes.handle({ CommandId::SelectAirToAirMode, 1.0 });
	modes.handle({ CommandId::SelectAirToAirMode, 1.0 });
	TEST_EXPECT(context, data.master_mode == MasterMode::Navigation);
	modes.handle({ CommandId::SelectAirToGroundMode, 1.0 });
	modes.handle({ CommandId::SelectNavigationMode, 1.0 });
	TEST_EXPECT(context, data.master_mode == MasterMode::Navigation);
}

void test_override_positions_are_idempotent_and_ignore_releases(Tests::Context& context)
{
	ModeData data;
	ModeEvents events;
	ModeManagement modes(data, events);
	int selections = 0;
	sigslot::scoped_connection connection = events.operating_mode_selected.connect(
	    [&](const OperatingModeSelected&)
	    {
		    ++selections;
	    });
	modes.handle({ CommandId::SelectDogfightMode, 1.0 });
	modes.handle({ CommandId::SelectDogfightMode, 1.0 });
	modes.handle({ CommandId::SelectMissileOverride, 1.0 });
	modes.handle({ CommandId::SelectMissileOverride, 1.0 });
	modes.handle({ CommandId::SelectOverrideCenter, 0.0 });
	modes.handle({ CommandId::StartCmsForward, 1.0 });
	TEST_EXPECT(context, selections == 2 && data.override_mode == OverrideMode::Missile);
	modes.handle({ CommandId::SelectOverrideCenter, 1.0 });
	modes.handle({ CommandId::SelectOverrideCenter, 1.0 });
	TEST_EXPECT(context, selections == 3 && data.override_mode == OverrideMode::None);
}

void test_ordered_scoped_subscriptions_and_instance_isolation(Tests::Context& context)
{
	ModeData data;
	ModeData other_data;
	ModeEvents events;
	ModeEvents other_events;
	ModeManagement modes(data, events);
	ModeManagement other_modes(other_data, other_events);
	std::vector<int> calls;
	// 反向登記監聽者，驗證依 group 順序通知。
	sigslot::scoped_connection late = events.master_arm_selected.connect(
	    [&](const MasterArmSelected&)
	    {
		    calls.push_back(2);
	    },
	    20);
	{
		sigslot::scoped_connection early = events.master_arm_selected.connect(
		    [&](const MasterArmSelected&)
		    {
			    calls.push_back(1);
		    },
		    10);
		modes.handle({ CommandId::SetMasterArmOff, 1.0 });
	}
	TEST_EXPECT(context, calls == std::vector<int>({ 1, 2 }));
	modes.handle({ CommandId::SetMasterArmOn, 1.0 });
	TEST_EXPECT(context, calls == std::vector<int>({ 1, 2, 2 }));
	other_modes.handle({ CommandId::SetMasterArmSim, 1.0 });
	TEST_EXPECT(context, calls == std::vector<int>({ 1, 2, 2 }));
	TEST_EXPECT(context, data.master_arm == MasterArmMode::On);
	TEST_EXPECT(context, other_data.master_arm == MasterArmMode::Simulation);
}

void test_countermeasure_reactions_use_selection_payload(Tests::Context& context)
{
	ModeData current;
	ExternalActionOutbox actions;
	CountermeasureControl cms(current, actions);
	ModeData off;
	off.master_arm = MasterArmMode::Off;
	// 在資料庫為 ON 時，驗證 OFF 事件仍產生中止要求。
	cms.on_master_arm_selected({ current, off });
	cms.handle({ CommandId::StartCmsForward, 1.0 });
	const auto batch = Tests::pending_actions(actions);
	TEST_EXPECT(context, batch.count == 2);
	if (batch.count == 2)
	{
		TEST_EXPECT(context, batch.actions[0].kind == MissionActionKind::AbortCms);
		TEST_EXPECT(context, batch.actions[1].kind == MissionActionKind::StartCmsForward);
	}
}
}

void run_mode_management_tests(Tests::Context& context)
{
	test_master_arm_is_independent_from_both_mode_selections(context);
	test_override_restores_each_saved_master_mode(context);
	test_primary_buttons_toggle_to_navigation(context);
	test_override_positions_are_idempotent_and_ignore_releases(context);
	test_ordered_scoped_subscriptions_and_instance_isolation(context);
	test_countermeasure_reactions_use_selection_payload(context);
}
