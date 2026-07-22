#pragma once

#include "../../Core/Fck1cEfm.h"
#include "../../include/FM/wHumanCustomPhysicsAPI.h"
#include "../DcsCommandRouter.h"

#include <cstddef>

namespace DcsBridge
{
namespace Internal
{
class EfmEventReporter;

struct CommandCallbackSample
{
	int command_id = 0;
	float value = 0.0F;
	const DcsCommandMapping& mapping;
};

bool validate_atmosphere_input(
	const Core::AtmosphereInput& input,
	EfmEventReporter& reporter);
bool validate_surface_input(
	const Core::SurfaceInput& input,
	EfmEventReporter& reporter);
bool validate_mass_input(
	const Core::MassStateInput& input,
	EfmEventReporter& reporter);
bool validate_world_kinematics_input(
	const Core::WorldKinematicsInput& input,
	EfmEventReporter& reporter);
bool validate_body_kinematics_input(
	const Core::BodyKinematicsInput& input,
	EfmEventReporter& reporter);
bool validate_internal_fuel_input(double fuel, EfmEventReporter& reporter);
bool validate_external_fuel_input(
	const Core::ExternalFuelInput& input,
	EfmEventReporter& reporter);
bool validate_refueling_fuel_input(double fuel, EfmEventReporter& reporter);
bool validate_damage_input(double integrity, EfmEventReporter& reporter);
bool validate_command_mapping(
	const CommandCallbackSample& sample,
	EfmEventReporter& reporter);
bool validate_simulation_event_input(
	const ed_fm_simulation_event& event,
	EfmEventReporter& reporter);
bool validate_suspension_feedback(
	int index,
	const ed_fm_suspension_info* info,
	EfmEventReporter& reporter);
bool validate_draw_args_buffer(
	const EdDrawArgument* draw_args,
	std::size_t size,
	EfmEventReporter& reporter);
std::size_t required_draw_arg_count();
}
}
