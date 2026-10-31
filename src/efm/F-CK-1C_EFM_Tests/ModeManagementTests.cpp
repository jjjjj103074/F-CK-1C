#include "TestHarness.h"

#include "Core/Systems/MissionComputer/Components/CountermeasureControl/CountermeasureControl.h"
#include "Core/Systems/MissionComputer/Components/ModeManagement/ModeManagement.h"

#include <type_traits>
#include <vector>

namespace
{
using namespace Core;
using namespace Core::Systems::Mc;

static_assert(!std::is_move_constructible<ModeEvents>::value, "Channels must retain their MC's address.");

void test_selection_events_keep_each_command_stage(Tests::Context& context)
{
	ModeData data;
	ModeEvents events;
	ModeManagement modes(data, events);
	std::vector<MasterArmSelected> arm_selections;
	std::vector<OperatingModeSelected> mode_selections;
	std::vector<char> order;
	sigslot::scoped_connection arm_connection = events.master_arm_selected.connect(
	    [&](const MasterArmSelected& event)
	    {
		    arm_selections.push_back(event);
		    order.push_back('A');
		    TEST_EXPECT(context, data.master_arm == event.selected.master_arm);
		    TEST_EXPECT(context, data.operating_mode == event.selected.operating_mode);
	    });
	sigslot::scoped_connection mode_connection = events.operating_mode_selected.connect(
	    [&](const OperatingModeSelected& event)
	    {
		    mode_selections.push_back(event);
		    order.push_back('M');
		    TEST_EXPECT(context, data.operating_mode == event.selected.operating_mode);
	    });

	modes.handle({ CommandId::SelectDogfightMode, 1.0 });
	modes.handle({ CommandId::SetMasterArmOff, 1.0 });
	modes.handle({ CommandId::SetMasterArmOn, 1.0 });
	TEST_EXPECT(context, order == std::vector<char>({ 'A', 'M', 'A', 'M', 'A', 'M' }));
	TEST_EXPECT(context, arm_selections.size() == 3);
	TEST_EXPECT(context, mode_selections.size() == 3);
	if (arm_selections.size() != 3 || mode_selections.size() != 3)
		return;
	TEST_EXPECT(context, arm_selections[1].previous.master_arm == MasterArmMode::On);
	TEST_EXPECT(context, arm_selections[1].selected.master_arm == MasterArmMode::Off);
	TEST_EXPECT(context, arm_selections[1].selected.operating_mode == OperatingMode::Dogfight);
	TEST_EXPECT(context, mode_selections[1].previous.master_arm == MasterArmMode::Off);
	TEST_EXPECT(context, mode_selections[1].previous.operating_mode == OperatingMode::Dogfight);
	TEST_EXPECT(context, mode_selections[1].selected.operating_mode == OperatingMode::Navigation);
	TEST_EXPECT(context, mode_selections[0].cycle_weapon);
	TEST_EXPECT(context, !mode_selections[1].cycle_weapon);
	TEST_EXPECT(context, !mode_selections[2].cycle_weapon);
	TEST_EXPECT(context, data.master_arm == MasterArmMode::On);
	TEST_EXPECT(context, data.operating_mode == OperatingMode::Navigation);
}

void test_repeated_selection_and_ignored_commands(Tests::Context& context)
{
	ModeData data;
	ModeEvents events;
	ModeManagement modes(data, events);
	std::vector<OperatingModeSelected> selections;
	sigslot::scoped_connection connection = events.operating_mode_selected.connect(
	    [&](const OperatingModeSelected& event)
	    {
		    selections.push_back(event);
	    });
	modes.handle({ CommandId::SelectMissileOverride, 1.0 });
	modes.handle({ CommandId::SelectMissileOverride, 1.0 });
	modes.handle({ CommandId::SelectNavigationMode, 0.0 });
	modes.handle({ CommandId::StartCmsForward, 1.0 });
	TEST_EXPECT(context, selections.size() == 2);
	if (selections.size() == 2)
	{
		TEST_EXPECT(context, selections[1].previous.operating_mode == OperatingMode::MissileOverride);
		TEST_EXPECT(context, selections[1].selected.operating_mode == OperatingMode::MissileOverride);
		TEST_EXPECT(context, selections[1].cycle_weapon);
	}
	TEST_EXPECT(context, data.operating_mode == OperatingMode::MissileOverride);
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
	ExternalActionQueue actions;
	CountermeasureControl cms(current, actions);
	ModeData off;
	off.master_arm = MasterArmMode::Off;
	// 在資料庫為 ON 時，驗證 OFF 事件仍產生中止要求。
	cms.on_master_arm_selected({ current, off });
	cms.handle({ CommandId::StartCmsForward, 1.0 });
	const auto batch = actions.batch();
	TEST_EXPECT(context, batch.count == 2);
	if (batch.count == 2)
	{
		TEST_EXPECT(context, batch.actions[0] == MissionActionKind::AbortCms);
		TEST_EXPECT(context, batch.actions[1] == MissionActionKind::StartCmsForward);
	}
}
}

void run_mode_management_tests(Tests::Context& context)
{
	test_selection_events_keep_each_command_stage(context);
	test_repeated_selection_and_ignored_commands(context);
	test_ordered_scoped_subscriptions_and_instance_isolation(context);
	test_countermeasure_reactions_use_selection_payload(context);
}
