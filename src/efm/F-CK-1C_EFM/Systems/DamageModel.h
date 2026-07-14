#pragma once

#include <cstddef>

namespace Systems
{
constexpr std::size_t kWingDamageSegmentCount = 3;
constexpr std::size_t kTailDamageSegmentCount = 5;
constexpr std::size_t kEngineDamageSegmentCount = 3;

struct DamageModel
{
	double left_wing_segments[kWingDamageSegmentCount] = { 1.0, 1.0, 1.0 };
	double right_wing_segments[kWingDamageSegmentCount] = { 1.0, 1.0, 1.0 };
	double tail_segments[kTailDamageSegmentCount] = { 1.0, 1.0, 1.0, 1.0, 1.0 };
	double left_engine_segments[kEngineDamageSegmentCount] = { 1.0, 1.0, 1.0 };
	double right_engine_segments[kEngineDamageSegmentCount] = { 1.0, 1.0, 1.0 };
	double left_wing_integrity = 1.0;
	double right_wing_integrity = 1.0;
	double tail_integrity = 1.0;
	double left_engine_integrity = 1.0;
	double right_engine_integrity = 1.0;
	double total_damage = 0.0;
};

template <std::size_t Size>
inline void reset_damage_segments(double (&segments)[Size])
{
	for (std::size_t index = 0; index < Size; ++index)
	{
		segments[index] = 1.0;
	}
}

template <std::size_t Size>
inline double combined_integrity(const double (&segments)[Size])
{
	double integrity = 1.0;
	for (std::size_t index = 0; index < Size; ++index)
	{
		integrity *= segments[index];
	}
	return integrity;
}

inline void refresh_damage_integrity(DamageModel& damage)
{
	damage.left_wing_integrity = combined_integrity(damage.left_wing_segments);
	damage.right_wing_integrity = combined_integrity(damage.right_wing_segments);
	damage.tail_integrity = combined_integrity(damage.tail_segments);
	damage.left_engine_integrity = combined_integrity(damage.left_engine_segments);
	damage.right_engine_integrity = combined_integrity(damage.right_engine_segments);
}

inline void reset_damage_model(DamageModel& damage)
{
	reset_damage_segments(damage.left_wing_segments);
	reset_damage_segments(damage.right_wing_segments);
	reset_damage_segments(damage.tail_segments);
	reset_damage_segments(damage.left_engine_segments);
	reset_damage_segments(damage.right_engine_segments);
	refresh_damage_integrity(damage);
	damage.total_damage = 0.0;
}

template <std::size_t Size>
inline void apply_damage_segment(
	double (&segments)[Size],
	std::size_t segment,
	double integrity)
{
	if (segment < Size)
	{
		segments[segment] = integrity;
	}
}
}
