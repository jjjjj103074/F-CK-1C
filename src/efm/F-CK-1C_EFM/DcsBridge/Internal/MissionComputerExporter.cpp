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
                 cockpit_parameter_writer(DcsIds::CockpitParams::McMissileUncageHeld),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrSeekerAvailable),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrSeekerRevision),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrSeekerInvalidReason),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrSeekerLock),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrDesiredAzimuth),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrDesiredElevation),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrTargetAzimuth),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresIrTargetElevation),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresAirToAirMissileCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresMissileCountAvailable),
                 cockpit_parameter_writer(DcsIds::CockpitParams::WeaponRequestedStation),
                 cockpit_parameter_writer(DcsIds::CockpitParams::WeaponSelectionRequestRevision),
                 cockpit_parameter_writer(DcsIds::CockpitParams::WeaponSelectionActionSequence),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresGunCount),
                 cockpit_parameter_writer(DcsIds::CockpitParams::McStoresGunCountAvailable),
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
	{
		write(Parameter::Available, 0.0, events);
		return events;
	}
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
	WRITE_MC(Uncage, snapshot.fire_control.missile_uncage_held ? 1.0 : 0.0);
	const auto& ir = snapshot.stores.ir_seeker;
	WRITE_MC(IrAvailable, ir.status.available ? 1.0 : 0.0);
	WRITE_MC(IrRevision, static_cast<double>(ir.status.revision));
	WRITE_MC(IrInvalidReason, static_cast<double>(ir.status.invalid_reason));
	WRITE_MC(IrLock, ir.status.available && ir.locked ? 1.0 : 0.0);
	WRITE_MC(IrDesiredAzimuth, ir.status.available ? ir.desired_azimuth_rad : 0.0);
	WRITE_MC(IrDesiredElevation, ir.status.available ? ir.desired_elevation_rad : 0.0);
	WRITE_MC(IrTargetAzimuth, ir.status.available ? ir.target_azimuth_rad : 0.0);
	WRITE_MC(IrTargetElevation, ir.status.available ? ir.target_elevation_rad : 0.0);
	WRITE_MC(MissileCount, static_cast<double>(snapshot.stores.air_to_air_missile_count));
	WRITE_MC(MissileCountAvailable, snapshot.stores.air_to_air_missile_count_complete ? 1.0 : 0.0);
	// 僅發布佇列首項的選站資料，讓前一筆操作先完成。
	const bool selecting = batch.count != 0 && batch.actions[0].kind == Core::MissionActionKind::SelectStation;
	WRITE_MC(RequestedStation, selecting ? static_cast<double>(batch.actions[0].station.value_or(-1)) : -1.0);
	WRITE_MC(SelectionRequestRevision, selecting ? static_cast<double>(batch.actions[0].selection_revision) : 0.0);
	WRITE_MC(SelectionActionSequence, selecting ? static_cast<double>(batch.first_sequence) : 0.0);
	WRITE_MC(GunCount, snapshot.stores.gun_count);
	WRITE_MC(GunCountAvailable, snapshot.stores.gun_ammunition.observation.available ? 1.0 : 0.0);
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
		    action_slots_[i].write_number(api_, i < batch.count ? static_cast<double>(batch.actions[i].kind) : 0.0);
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
