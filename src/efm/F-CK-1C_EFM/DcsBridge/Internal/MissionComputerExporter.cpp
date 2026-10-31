#include "MissionComputerExporter.h"

#include "../../DcsIds/CockpitParams.g.h"

#include <stdexcept>

namespace DcsBridge::Internal
{
MissionComputerExporter::MissionComputerExporter(cockpit_param_api api)
    : api_(api),
      slots_({ { cockpit_parameter_writer(DcsIds::CockpitParams::McGeneration),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McAvailable),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McActionFirstSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McActionCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::HmcsMasterMode),
                 cockpit_parameter_writer(DcsIds::CockpitParams::HmcsDogfightMode),
                 cockpit_parameter_writer(DcsIds::CockpitParams::HmcsFireControlMode),
                 cockpit_parameter_writer(DcsIds::CockpitParams::HmcsAamSubmode),
                 cockpit_parameter_writer(DcsIds::CockpitParams::HmcsGunFiring),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9UncageHeld),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9TargetDesignated),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9MissileStatus),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9MissileCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::WeaponRequestedStation),
                 cockpit_parameter_writer(DcsIds::CockpitParams::WeaponSelectionRequestRevision),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9SeekerState),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9ToneState),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9SeekerContact),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9SeekerLock),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9WeaponActive),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9SeekerAzimuth),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9SeekerElevation),
                 cockpit_parameter_writer(DcsIds::CockpitParams::Aim9LockRange),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresGunCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McTriggerSecondStageHeld),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McMasterArmPressSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McNavigationPressSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McDogfightPressSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McMissileOverridePressSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McMissileUncagePressSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McWeaponReleasePressSequence) } }),
      action_slots_({ { cockpit_parameter_writer(DcsIds::CockpitParams::McAction01),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction02),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction03),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction04),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction05),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction06),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction07),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction08),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction09),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction10),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction11),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction12),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction13),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction14),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction15),
                        cockpit_parameter_writer(DcsIds::CockpitParams::McAction16) } })
{
}

void MissionComputerExporter::reset()
{
	for (auto& slot : slots_)
		slot.reset();
	for (auto& slot : action_slots_)
		slot.reset();
}

bool MissionComputerExporter::write(Parameter parameter, double value, CockpitParameterEvents& events)
{
	const auto result = slots_[static_cast<std::size_t>(parameter)].write_number(api_, value);
	if (result.event)
		events = events.with_event(*result.event);
	return result.success;
}

CockpitParameterEvents MissionComputerExporter::export_snapshot(const Core::CombatAvionicsSnapshot& snapshot)
{
	CockpitParameterEvents events;
	if (!snapshot.status.available)
		return events;
	if (generation_ > Core::kMaximumExactCockpitInteger - 2)
	{
		throw std::overflow_error("MC cockpit generation exhausted its exact numeric range.");
	}
	const auto& batch = snapshot.external_actions.batch;
	if (batch.count > action_slots_.size())
		throw std::logic_error("Invalid MC action window size.");
	if (!write(Parameter::Generation, static_cast<double>(++generation_), events))
	{
		++generation_;
		return events;
	}
	bool complete = true;
#define WRITE_MC(parameter, value) complete = write(Parameter::parameter, value, events) && complete
	WRITE_MC(Available, 1.0);
	WRITE_MC(FirstSequence, static_cast<double>(batch.first_sequence));
	WRITE_MC(ActionCount, static_cast<double>(batch.count));
	// 將 ModeSnapshot 映射至 Lua/HMCS 座艙參數。
	WRITE_MC(MasterArm, static_cast<double>(snapshot.mode.master_arm_mode));
	WRITE_MC(Dogfight, snapshot.mode.dogfight_mode ? 1.0 : 0.0);
	WRITE_MC(OperatingMode, static_cast<double>(snapshot.mode.operating_mode));
	WRITE_MC(Submode, static_cast<double>(snapshot.fire_control.air_to_air_submode));
	WRITE_MC(GunFiring, snapshot.fire_control.gun_firing_indication ? 1.0 : 0.0);
	WRITE_MC(Uncage, snapshot.infrared_missile.uncage_held ? 1.0 : 0.0);
	WRITE_MC(Designated, snapshot.infrared_missile.target_designated ? 1.0 : 0.0);
	WRITE_MC(MissileStatus, static_cast<double>(snapshot.infrared_missile.readiness));
	WRITE_MC(MissileCount, static_cast<double>(snapshot.stores.air_to_air_missile_count));
	WRITE_MC(RequestedStation, static_cast<double>(snapshot.stores.requested_station.value_or(-1)));
	WRITE_MC(SelectionRequestRevision, static_cast<double>(snapshot.stores.selection_request_revision));
	WRITE_MC(SeekerState, static_cast<double>(snapshot.infrared_missile.seeker_state));
	WRITE_MC(Tone, static_cast<double>(snapshot.infrared_missile.requested_tone));
	WRITE_MC(Contact, snapshot.infrared_missile.contact ? 1.0 : 0.0);
	WRITE_MC(Lock, snapshot.infrared_missile.locked ? 1.0 : 0.0);
	WRITE_MC(Active, snapshot.infrared_missile.active ? 1.0 : 0.0);
	WRITE_MC(Azimuth, snapshot.infrared_missile.seeker_azimuth_rad);
	WRITE_MC(Elevation, snapshot.infrared_missile.seeker_elevation_rad);
	WRITE_MC(LockRange, snapshot.infrared_missile.lock_range_m);
	WRITE_MC(GunCount, snapshot.stores.gun_count);
	WRITE_MC(TriggerHeld, snapshot.control_inputs.trigger_second_stage_held ? 1.0 : 0.0);
	WRITE_MC(MasterArmPressSequence, static_cast<double>(snapshot.control_inputs.master_arm_press_sequence));
	WRITE_MC(NavigationPressSequence, static_cast<double>(snapshot.control_inputs.navigation_press_sequence));
	WRITE_MC(DogfightPressSequence, static_cast<double>(snapshot.control_inputs.dogfight_press_sequence));
	WRITE_MC(MissileOverridePressSequence,
	         static_cast<double>(snapshot.control_inputs.missile_override_press_sequence));
	WRITE_MC(MissileUncagePressSequence, static_cast<double>(snapshot.control_inputs.missile_uncage_press_sequence));
	WRITE_MC(WeaponReleasePressSequence, static_cast<double>(snapshot.control_inputs.weapon_release_press_sequence));
#undef WRITE_MC
	for (std::size_t i = 0; i < action_slots_.size(); ++i)
	{
		const auto result =
		    action_slots_[i].write_number(api_, i < batch.count ? static_cast<double>(batch.actions[i]) : 0.0);
		if (result.event)
			events = events.with_event(*result.event);
		complete = result.success && complete;
	}
	++generation_;
	if (complete)
		write(Parameter::Generation, static_cast<double>(generation_), events);
	return events;
}
}
