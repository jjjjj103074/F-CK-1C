#include "TestHarness.h"
#include "Core/Systems/MissionComputer/Components/FireControl/Internal/WeaponProcedures.h"
#include "Core/Systems/MissionComputer/Components/FireControl/Internal/WeaponOperation.h"
#include <limits>

void run_weapon_procedures_tests(Tests::Context& c)
{
	using namespace Core;
	using namespace Core::Systems;
	using namespace Core::Systems::Mc;
	using namespace Core::Systems::Mc::Internal;
	ModeData mode;
	mode.master_mode = MasterMode::AirToAir;
	mode.master_arm = MasterArmMode::On;
	SelectedWeaponState selected;
	selected.available = true;
	auto definition = std::make_shared<WeaponDefinition>();
	selected.weapon = definition;
	definition->role = WeaponRole::AirToAir;
	RadarSnapshot radar;
	const auto evaluate = [&](bool held = false)
	{
		return evaluate_weapon_release(mode, selected, radar, held);
	};
	definition->procedure = WeaponProcedure::Sidewinder;
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::Infrared && permits_release(evaluate()));
	definition->procedure = WeaponProcedure::Sparrow;
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::SparrowTrackRequired && !permits_release(evaluate(true)));
	definition->procedure = WeaponProcedure::Amraam;
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::VisualConsentRequired && !permits_release(evaluate()));
	TEST_EXPECT(c, evaluate(true) == WeaponReleaseState::Visual && permits_release(evaluate(true)));
	radar.observation.status.available = true;
	radar.observation.mode = RadarMode::SingleTargetTrack;
	radar.observation.stt_range_m = 5000;
	for (const auto procedure : { WeaponProcedure::Sparrow, WeaponProcedure::Amraam })
	{
		definition->procedure = procedure;
		TEST_EXPECT(c, evaluate() == WeaponReleaseState::Targeted && permits_release(evaluate()));
	}
	radar.observation.stt_range_m = std::numeric_limits<double>::quiet_NaN();
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::VisualConsentRequired);
	definition->procedure = WeaponProcedure::Unsupported;
	TEST_EXPECT(c, evaluate(true) == WeaponReleaseState::Unsupported);
	definition->procedure = WeaponProcedure::Sidewinder;
	selected.weapon.reset();
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::Unsupported);
	selected.weapon = definition;
	selected.available = false;
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::NoWeapon);
	selected.pending = true;
	TEST_EXPECT(c, evaluate() == WeaponReleaseState::SelectionPending);
	selected.available = true;
	TEST_EXPECT(c, permits_release(evaluate()));
	for (const auto safety : { MasterArmMode::Off, MasterArmMode::Simulation })
	{
		mode.master_arm = safety;
		TEST_EXPECT(c, evaluate(true) == WeaponReleaseState::Inhibited);
	}
	mode.master_arm = MasterArmMode::On;
	mode.master_mode = MasterMode::Navigation;
	TEST_EXPECT(c, evaluate(true) == WeaponReleaseState::Inhibited);
	mode.master_mode = MasterMode::AirToAir;
	WeaponOperationData data;
	WeaponUseData execution;
	ExternalActionOutbox actions;
	StoresCommandQueue commands(actions);
	WeaponOperation operation(mode, radar, selected, execution, data, commands);
	definition->procedure = WeaponProcedure::Amraam;
	operation.handle({ CommandId::SetMissileUncage, 1 }, {});
	TEST_EXPECT(c, data.uncage_button_held);
	operation.handle({ CommandId::SetMissileUncage, 0 }, {});
	TEST_EXPECT(c, !data.uncage_button_held);
	operation.handle({ CommandId::ToggleMissileUncage, 1 }, {});
	TEST_EXPECT(c, !data.uncage_button_held && !commands.take_next());
	operation.handle({ CommandId::SetMissileUncage, 1 }, {});
	operation.on_mode_changed();
	TEST_EXPECT(c, !data.uncage_button_held);
}
