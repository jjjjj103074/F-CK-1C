#pragma once

#include <cstddef>

namespace Core
{
inline constexpr std::size_t kWingDamageSegmentCount = 3;
inline constexpr std::size_t kTailDamageSegmentCount = 5;
inline constexpr std::size_t kEngineDamageSegmentCount = 3;

enum class DamageArea
{
	LeftWing,
	RightWing,
	Tail,
	LeftEngine,
	RightEngine
};

struct DamageEvent
{
	DamageArea area = DamageArea::LeftWing;
	std::size_t segment = 0;
	double integrity = 1.0;
};

struct DamageApplyResult
{
	bool invincible = false;
};

struct RepairEvent
{
};
}
