#pragma once

#include "Core/Contracts/FrameContracts.h"
#include "Common/Vec3.h"

namespace DcsBridge
{
namespace Internal
{
inline Core::BodyAngularKinematicsInput adapt_dcs_body_angular_kinematics(
	const Common::Vec3& angular_acceleration_dcs_rad_s2,
	const Common::Vec3& angular_velocity_dcs_rad_s)
{
	// Preserve the DCS local-body right-handed sign convention. Core names the
	// rotations, but does not silently change handedness: +x is right roll,
	// +z is nose-up pitch, and +y is nose-left yaw.
	return {
		angular_acceleration_dcs_rad_s2.x,
		angular_acceleration_dcs_rad_s2.z,
		angular_acceleration_dcs_rad_s2.y,
		angular_velocity_dcs_rad_s.x,
		angular_velocity_dcs_rad_s.z,
		angular_velocity_dcs_rad_s.y
	};
}
}
}
