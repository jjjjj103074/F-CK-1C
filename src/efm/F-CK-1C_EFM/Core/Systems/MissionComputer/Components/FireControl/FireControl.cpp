#include "FireControl.h"

#include "../../Common/StoresQueries.h"

namespace Core::Systems::Mc
{
FireControl::FireControl(const MissionInputs& input,
                         const ModeData& mode,
                         const StoresData& stores,
                         FireControlData& data,
                         ExternalActionQueue& actions,
                         StoresManagement& stores_management)
    : mode_(mode),
      stores_(stores),
      stores_management_(stores_management),
      targeting_(input, mode, stores, data.targeting, actions, stores_management),
      weapon_operation_(input, mode, data.targeting, data.weapon, actions, targeting_, stores_management),
      infrared_missile_support_(input, mode, stores, data.targeting, data.infrared_missile)
{
}

void FireControl::handle(const Command& command)
{
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
	case CommandId::SetTmsUp:
	case CommandId::PressTmsDown:
	case CommandId::PressTmsLeft:
	case CommandId::PressTmsRight:
		targeting_.handle(command);
		break;
	default:
		weapon_operation_.handle(command);
		break;
	}
}

void FireControl::on_operating_mode_selected(const OperatingModeSelected& event)
{
	targeting_.select_mode(event.cycle_weapon);
}

void FireControl::on_master_arm_selected(const MasterArmSelected& event)
{
	if (event.selected.master_arm != MasterArmMode::On)
		weapon_operation_.stop();
}

void FireControl::update_station_selection()
{
	if (!mode_.air_to_air())
	{
		stores_management_.request_station(std::nullopt);
		selection_mode_ = mode_.operating_mode;
	}
	else if (stores_.inventory.observation.available)
	{
		const auto stations = find_missile_stations(stores_.inventory, { WeaponRole::AirToAir, {} });
		const std::optional<int> first = stations.empty() ? std::nullopt : std::optional<int>(stations.front());
		stores_management_.request_station(first,
		                                   selection_mode_ != mode_.operating_mode ||
		                                       rearm_revision_ != stores_.inventory.rearm_revision);
		selection_mode_ = mode_.operating_mode;
		rearm_revision_ = stores_.inventory.rearm_revision;
	}
}

void FireControl::step(std::chrono::nanoseconds time, double dt_s)
{
	update_station_selection();
	// 保留原本同時到期的順序：先完成射擊／投放程序，再處理目標操作計時。
	weapon_operation_.step(time, dt_s);
	targeting_.step(time, dt_s);
	infrared_missile_support_.step(time, dt_s);
}
}
