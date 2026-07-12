#pragma once

#include "../Common/Vec3.h"

namespace Core
{
inline void reset_force_moment(Common::Vec3& force, Common::Vec3& moment)
{
	force = Common::Vec3();
	moment = Common::Vec3();
}

inline void add_local_force(
	Common::Vec3& force_accum,
	Common::Vec3& moment_accum,
	const Common::Vec3& center_of_mass,
	const Common::Vec3& force,
	const Common::Vec3& force_pos)
{
	force_accum.x += force.x;
	force_accum.y += force.y;
	force_accum.z += force.z;

	const Common::Vec3 delta_pos(
		force_pos.x - center_of_mass.x,
		force_pos.y - center_of_mass.y,
		force_pos.z - center_of_mass.z);

	const Common::Vec3 delta_moment = Common::cross(delta_pos, force);

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
