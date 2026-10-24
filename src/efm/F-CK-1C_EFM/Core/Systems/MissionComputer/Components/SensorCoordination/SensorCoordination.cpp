#include "SensorCoordination.h"

#include "Common/CommandValue.h"

#include <algorithm>

namespace Core::Systems::Mc
{
SensorCoordination::SensorCoordination(const MissionInputs& input,
                                       const ModeData& mode,
                                       const StoresData& stores,
                                       SensorData& sensor,
                                       ExternalActionQueue& actions)
    : input_(input),
      mode_(mode),
      stores_(stores),
      sensor_(sensor),
      actions_(actions)
{
}

void SensorCoordination::ensure_radar_on()
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

void SensorCoordination::apply_submode(AirToAirSubmode mode)
{
	sensor_.submode = mode;
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
	default: // HUD scan and IFF remain reserved, as in the Lua implementation.
		break;
	}
}

void SensorCoordination::begin_lock(bool pulse)
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
	sensor_.target_designated = true;
}

void SensorCoordination::finish_lock()
{
	actions_.append(MissionActionKind::LockFinish);
	actions_.append(MissionActionKind::ChangeLockUp);
	lock_.clear();
}

void SensorCoordination::unlock()
{
	if (lock_.active())
	{
		lock_.request_release();
	}
	else
	{
		finish_lock();
	}
	sensor_.target_designated = false;
}

void SensorCoordination::prepare_release()
{
	if (mode_.air_to_air())
	{
		apply_submode(sensor_.submode);
	}
}

void SensorCoordination::select_mode(bool cycle_weapon)
{
	next_auto_lock_ = {};
	if (mode_.fire_control == FireControlMode::Navigation)
	{
		sensor_.submode = AirToAirSubmode::None;
		unlock();
		actions_.append(MissionActionKind::ModeCannon);
		return;
	}
	const bool dogfight = mode_.fire_control == FireControlMode::Dogfight;
	if (!dogfight)
	{
		unlock();
	}
	ensure_radar_on();
	if (cycle_weapon)
	{
		actions_.append(MissionActionKind::CycleWeapon);
	}
	apply_submode(dogfight ? AirToAirSubmode::Helmet : AirToAirSubmode::BeyondVisualRange);
	if (dogfight)
	{
		begin_lock(true);
	}
}

void SensorCoordination::handle(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	const bool dogfight = mode_.fire_control == FireControlMode::Dogfight;
	if (command.id == CommandId::SetMissileUncage)
	{
		sensor_.uncage_held = pressed;
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
		else if (mode_.fire_control == FireControlMode::MissileOverride)
		{
			actions_.append(MissionActionKind::ChangeLock);
			sensor_.target_designated = true;
		}
	}
}

void SensorCoordination::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at);
	if (input_.cockpit.mission_adapter.radar_power_on)
		radar_on_requested_ = false;
	if (lock_.release_due(time, actions_))
	{
		finish_lock();
	}
	if (mode_.fire_control == FireControlMode::Dogfight && mode_.master_arm == MasterArmMode::On &&
	    sensor_.uncage_held && stores_.missile_count > 0)
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
