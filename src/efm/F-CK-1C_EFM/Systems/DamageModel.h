#pragma once

#include "../DcsIds/DamageIds.h"

namespace Systems
{
struct DamageModel
{
	double element_integrity[DcsIds::Damage::ElementCount] = {};
	double left_wing_integrity = 1.0;
	double right_wing_integrity = 1.0;
	double tail_integrity = 1.0;
	double left_engine_integrity = 1.0;
	double right_engine_integrity = 1.0;
	double total_damage = 0.0;
};

inline void reset_damage_model(DamageModel& damage)
{
	for (int i = 0; i < DcsIds::Damage::ElementCount; ++i)
	{
		damage.element_integrity[i] = 1.0;
	}

	damage.left_wing_integrity = 1.0;
	damage.right_wing_integrity = 1.0;
	damage.tail_integrity = 1.0;
	damage.left_engine_integrity = 1.0;
	damage.right_engine_integrity = 1.0;
	damage.total_damage = 0.0;
}

inline void apply_damage(DamageModel& damage, int element, double element_integrity_factor, bool invincible)
{
	if (element >= 0 && element < DcsIds::Damage::ElementCount)
	{
		damage.element_integrity[element] = element_integrity_factor;
	}

	// Element integrity uses 0.0 = destroyed and 1.0 = intact.
	if (invincible == false)
	{
		// Left wing
		damage.left_wing_integrity =
			damage.element_integrity[DcsIds::Damage::LeftWing[0]] *
			damage.element_integrity[DcsIds::Damage::LeftWing[1]] *
			damage.element_integrity[DcsIds::Damage::LeftWing[2]];

		// Right wing
		damage.right_wing_integrity =
			damage.element_integrity[DcsIds::Damage::RightWing[0]] *
			damage.element_integrity[DcsIds::Damage::RightWing[1]] *
			damage.element_integrity[DcsIds::Damage::RightWing[2]];

		// Tail
		damage.tail_integrity =
			damage.element_integrity[DcsIds::Damage::Tail[0]] *
			damage.element_integrity[DcsIds::Damage::Tail[1]] *
			damage.element_integrity[DcsIds::Damage::Tail[2]] *
			damage.element_integrity[DcsIds::Damage::Tail[3]] *
			damage.element_integrity[DcsIds::Damage::Tail[4]];

		// Left engine
		damage.left_engine_integrity =
			damage.element_integrity[DcsIds::Damage::LeftEngine[0]] *
			damage.element_integrity[DcsIds::Damage::LeftEngine[1]] *
			damage.element_integrity[DcsIds::Damage::LeftEngine[2]];

		// Right engine
		damage.right_engine_integrity =
			damage.element_integrity[DcsIds::Damage::RightEngine[0]] *
			damage.element_integrity[DcsIds::Damage::RightEngine[1]] *
			damage.element_integrity[DcsIds::Damage::RightEngine[2]];
	}
}
}
