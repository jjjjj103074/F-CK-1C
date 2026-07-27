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
	Common::Vec3 value;
	Common::Vec3 position;
};

inline ModelEffect make_local_force_effect(
	const Common::Vec3& force,
	const Common::Vec3& position)
{
	return { ModelEffectType::LocalForce, force, position };
}

inline ModelEffect make_local_moment_effect(const Common::Vec3& moment)
{
	return { ModelEffectType::LocalMoment, moment, {} };
}
}
}
