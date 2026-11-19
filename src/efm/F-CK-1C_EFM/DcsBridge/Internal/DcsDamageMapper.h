#pragma once

#include "../../Core/Contracts/Events.h"

namespace DcsBridge
{
struct DcsDamageMapping
{
	bool mapped = false;
	Core::DamageEvent event;
};

DcsDamageMapping map_damage(int element, double integrity);
}
