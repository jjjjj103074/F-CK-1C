#pragma once

#include "../../Common/Vec3.h"

namespace Core
{
struct LocalForceApplication
{
	Common::Vec3 center_of_mass_body_m;
	Common::Vec3 force_body_n;
	Common::Vec3 application_position_body_m;
};

inline void reset_force_moment(
	Common::Vec3& force_body_n,
	Common::Vec3& moment_body_nm)
{
	force_body_n = Common::Vec3();
	moment_body_nm = Common::Vec3();
}

inline void add_local_force(
	Common::Vec3& force_accum_body_n,
	Common::Vec3& moment_accum_body_nm,
	const LocalForceApplication& application)
{
	force_accum_body_n.x += application.force_body_n.x;
	force_accum_body_n.y += application.force_body_n.y;
	force_accum_body_n.z += application.force_body_n.z;

	const Common::Vec3 lever_arm_body_m(
		application.application_position_body_m.x -
			application.center_of_mass_body_m.x,
		application.application_position_body_m.y -
			application.center_of_mass_body_m.y,
		application.application_position_body_m.z -
			application.center_of_mass_body_m.z);

	const Common::Vec3 delta_moment_body_nm =
		Common::cross(lever_arm_body_m, application.force_body_n);

	moment_accum_body_nm.x += delta_moment_body_nm.x;
	moment_accum_body_nm.y += delta_moment_body_nm.y;
	moment_accum_body_nm.z += delta_moment_body_nm.z;
}

inline void add_local_moment(
	Common::Vec3& moment_accum_body_nm,
	const Common::Vec3& moment_body_nm)
{
	moment_accum_body_nm.x += moment_body_nm.x;
	moment_accum_body_nm.y += moment_body_nm.y;
	moment_accum_body_nm.z += moment_body_nm.z;
}
}
