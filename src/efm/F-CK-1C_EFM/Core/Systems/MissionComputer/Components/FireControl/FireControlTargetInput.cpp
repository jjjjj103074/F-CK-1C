#include "FireControl.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
void FireControl::handle_target_input(const Command& command)
{
	const bool pressed = Common::command_value_is_pressed(command.value_normalized);
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	switch (command.id)
	{
	case CommandId::SetMissileUncage:
		weapon_operation_.handle(command);
		handle_uncage(pressed);
		break;
	case CommandId::SetTmsUp:
		handle_tms_up(pressed);
		break;
	case CommandId::PressTmsDown:
		if (pressed)
		{
			if (dogfight)
			{
				apply_acquisition_mode(AirToAirSubmode::VerticalScan);
				acquire_target(AcquisitionInput::Pulse);
			}
			else
				cancel_target_designation();
		}
		break;
	case CommandId::PressTmsRight:
		if (pressed)
		{
			if (dogfight)
				apply_acquisition_mode(AirToAirSubmode::Hud);
			else if (mode_.operating_mode == OperatingMode::MissileOverride)
				select_next_target();
		}
		break;
	default:
		break;
	}
}

void FireControl::handle_uncage(bool pressed)
{
	if (pressed && mode_.air_to_air())
	{
		request_radar_on();
		reapply_acquisition_mode();
	}
	else if (!pressed && mode_.operating_mode == OperatingMode::Dogfight)
		cancel_target_designation();
}

void FireControl::handle_tms_up(bool pressed)
{
	const bool dogfight = mode_.operating_mode == OperatingMode::Dogfight;
	if (pressed)
	{
		if (dogfight)
			apply_acquisition_mode(AirToAirSubmode::Helmet);
		acquire_target(dogfight ? AcquisitionInput::Pulse : AcquisitionInput::Hold);
	}
	else if (!dogfight && acquisition_input_.active())
		acquisition_input_.request_release();
}

void FireControl::request_radar_on()
{
	if (input_.cockpit.mission_adapter.radar_power_on)
		radar_on_requested_ = false;
	else if (!radar_on_requested_)
	{
		actions_.append(MissionActionKind::EnsureRadarOn);
		radar_on_requested_ = true;
	}
}

void FireControl::apply_acquisition_mode(AirToAirSubmode mode)
{
	data_.acquisition_mode = mode;
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

void FireControl::reapply_acquisition_mode()
{
	if (mode_.air_to_air())
		apply_acquisition_mode(data_.acquisition_mode);
}

void FireControl::acquire_target(AcquisitionInput input)
{
	if (!input_.cockpit.mission_adapter.available)
		return;
	reapply_acquisition_mode();
	actions_.append(MissionActionKind::LockStart);
	acquisition_input_.begin(actions_.append(MissionActionKind::ChangeLock));
	if (input == AcquisitionInput::Pulse)
		acquisition_input_.request_release();
	target_designation_.request(Internal::TargetDesignationRequest::Designate);
}

void FireControl::select_next_target()
{
	actions_.append(MissionActionKind::ChangeLock);
	target_designation_.request(Internal::TargetDesignationRequest::SelectNext);
}

void FireControl::cancel_target_designation()
{
	target_designation_.request(Internal::TargetDesignationRequest::Cancel);
	if (acquisition_input_.active())
		acquisition_input_.request_release();
	else
		finish_acquisition_input();
}

void FireControl::finish_acquisition_input()
{
	actions_.append(MissionActionKind::LockFinish);
	actions_.append(MissionActionKind::ChangeLockUp);
	acquisition_input_.clear();
}
}
