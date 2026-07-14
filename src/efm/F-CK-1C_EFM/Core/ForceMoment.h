#pragma once

#include "../Common/Vec3.h"

namespace Core
{
struct LocalForceApplication
{
	Common::Vec3 center_of_mass;
	Common::Vec3 force;
	Common::Vec3 position;
};

inline void reset_force_moment(Common::Vec3& force, Common::Vec3& moment)
{
	force = Common::Vec3();
	moment = Common::Vec3();
}

inline void add_local_force(
	Common::Vec3& force_accum,
	Common::Vec3& moment_accum,
	const LocalForceApplication& application)
{
	force_accum.x += application.force.x;
	force_accum.y += application.force.y;
	force_accum.z += application.force.z;

	const Common::Vec3 delta_pos(
		application.position.x - application.center_of_mass.x,
		application.position.y - application.center_of_mass.y,
		application.position.z - application.center_of_mass.z);

	const Common::Vec3 delta_moment = Common::cross(delta_pos, application.force);

	moment_accum.x += delta_moment.x;
	moment_accum.y += delta_moment.y;
	moment_accum.z += delta_moment.z;
}

inline void add_local_moment(Common::Vec3& moment_accum, const Common::Vec3& moment)
{
	moment_accum.x += moment.x;
	moment_accum.y += moment.y;
	moment_accum.z += moment.z;
}
}
