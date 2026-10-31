#include "Targeting.h"

#include "Common/CommandValue.h"

#include <algorithm>

namespace Core::Systems::Mc::Internal
{
Targeting::Targeting(const MissionInputs& input,
                     const ModeData& mode,
                     const StoresData& stores,
                     TargetingData& targeting,
                     ExternalActionQueue& actions,
                     StoresManagement& stores_management)
    : input_(input),
      mode_(mode),
      stores_(stores),
      targeting_(targeting),
      actions_(actions),
      stores_management_(stores_management)
{
}

void Targeting::ensure_radar_on()
{
	if (input_.cockpit.mission_adapter.radar_power_on)
	{
		radar_on_requested_ = false;
	}
	else if (!radar_on_requested_)
	{
		actions_.append(MissionActionKind::EnsureRadarOn);
		radar_on_requested_ = true;
	}
}

void Targeting::apply_submode(AirToAirSubmode mode)
{
	targeting_.submode = mode;
	switch (mode)
	{
	case AirToAirSubmode::Helmet:
		actions_.append(MissionActionKind::ModeHelmet);
		break;
	case AirToAirSubmode::VerticalScan:
		actions_.append(MissionActionKind::ModeVerticalScan);
		break;
	case AirToAirSubmode::BeyondVisualRange:
		actions_.append(MissionActionKind::ModeBeyondVisualRange);
		actions_.append(MissionActionKind::ModeFi0);
		break;
	default:
		break;
	}
}

void Targeting::begin_lock(bool pulse)
{
	if (!input_.cockpit.mission_adapter.available)
	{
		return;
	}
	prepare_release();
	actions_.append(MissionActionKind::LockStart);
	lock_.begin(actions_.append(MissionActionKind::ChangeLock));
	if (pulse)
	{
		lock_.request_release();
	}
	targeting_.designation_requested = true;
}

void Targeting::finish_lock()
{
	actions_.append(MissionActionKind::LockFinish);
	actions_.append(MissionActionKind::ChangeLockUp);
	lock_.clear();
}

void Targeting::unlock()
{
	if (lock_.active())
	{
		lock_.request_release();
	}
	else
	{
		finish_lock();
	}
	targeting_.designation_requested = false;
}

void Targeting::prepare_release()
{
	if (mode_.air_to_air())
	{
		apply_submode(targeting_.submode);
	}
}

void Targeting::select_mode(bool cycle_weapon)
{
	next_auto_lock_ = {};
	if (mode_.operating_mode == OperatingMode::Navigation)
	{
		targeting_.submode = AirToAirSubmode::None;
		unlock();
		actions_.append(MissionActionKind::ModeCannon);
		return;
	}
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	if (!dogfight)
	{
		unlock();
	}
	ensure_radar_on();
	if (cycle_weapon)
	{
		stores_management_.request_weapon_cycle();
	}
	apply_submode(dogfight ? AirToAirSubmode::Helmet : AirToAirSubmode::BeyondVisualRange);
	if (dogfight)
	{
		begin_lock(true);
	}
}

void Targeting::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	if (command.id == CommandId::SetMissileUncage)
	{
		targeting_.uncage_held = pressed;
		if (pressed && mode_.air_to_air())
		{
			ensure_radar_on();
			prepare_release();
		}
		else if (!pressed && dogfight)
		{
			unlock();
		}
		return;
	}
	if (command.id == CommandId::SetTmsUp)
	{
		if (pressed)
		{
			if (dogfight)
				apply_submode(AirToAirSubmode::Helmet);
			begin_lock(dogfight);
		}
		else if (!dogfight && lock_.active())
		{
			lock_.request_release();
		}
		return;
	}
	if (!pressed)
		return;
	if (command.id == CommandId::PressTmsDown)
	{
		if (dogfight)
		{
			apply_submode(AirToAirSubmode::VerticalScan);
			begin_lock(true);
		}
		else
			unlock();
	}
	else if (command.id == CommandId::PressTmsRight)
	{
		if (dogfight)
			apply_submode(AirToAirSubmode::Hud);
		else if (mode_.operating_mode == OperatingMode::MissileOverride)
		{
			actions_.append(MissionActionKind::ChangeLock);
			targeting_.designation_requested = true;
		}
	}
}

void Targeting::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at);
	if (input_.cockpit.mission_adapter.radar_power_on)
		radar_on_requested_ = false;
	if (lock_.release_due(time, actions_))
	{
		finish_lock();
	}
	if (mode_.operating_mode == OperatingMode::Dogfight && mode_.master_arm == MasterArmMode::On &&
	    targeting_.uncage_held && stores_.missile_count > 0)
	{
		if (time >= next_auto_lock_)
		{
			begin_lock(true);
			next_auto_lock_ = time + std::chrono::seconds(1);
		}
	}
	else
	{
		next_auto_lock_ = time + std::chrono::seconds(1);
	}
}
}
