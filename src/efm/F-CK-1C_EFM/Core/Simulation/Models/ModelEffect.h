#pragma once

#include "../../../Common/Vec3.h"

namespace Core
{
namespace Simulation
{
enum class ModelEffectType
{
	LocalForce,
	LocalMoment
};

struct ModelEffect
{
	ModelEffectType type = ModelEffectType::LocalForce;
	Common::Vec3 force_body_n;
	Common::Vec3 moment_body_nm;
	Common::Vec3 application_position_body_m;
};

inline ModelEffect make_local_force_effect(
	const Common::Vec3& force_body_n,
	const Common::Vec3& application_position_body_m)
{
	return {
		ModelEffectType::LocalForce,
		force_body_n,
		{},
		application_position_body_m
	};
}

inline ModelEffect make_local_moment_effect(
	const Common::Vec3& moment_body_nm)
{
	return { ModelEffectType::LocalMoment, {}, moment_body_nm, {} };
}
}
}
