#include "DcsDamageMapper.h"

#include "../../DcsIds/DamageIds.h"

#include <cstddef>

namespace
{
struct DamageBinding
{
	int dcs_element;
	Core::DamageArea area;
	std::size_t segment;
};

constexpr DamageBinding kDamageBindings[] = {
	{ DcsIds::Damage::LeftWing[0], Core::DamageArea::LeftWing, 0 },
	{ DcsIds::Damage::LeftWing[1], Core::DamageArea::LeftWing, 1 },
	{ DcsIds::Damage::LeftWing[2], Core::DamageArea::LeftWing, 2 },
	{ DcsIds::Damage::RightWing[0], Core::DamageArea::RightWing, 0 },
	{ DcsIds::Damage::RightWing[1], Core::DamageArea::RightWing, 1 },
	{ DcsIds::Damage::RightWing[2], Core::DamageArea::RightWing, 2 },
	{ DcsIds::Damage::Tail[0], Core::DamageArea::Tail, 0 },
	{ DcsIds::Damage::Tail[1], Core::DamageArea::Tail, 1 },
	{ DcsIds::Damage::Tail[2], Core::DamageArea::Tail, 2 },
	{ DcsIds::Damage::Tail[3], Core::DamageArea::Tail, 3 },
	{ DcsIds::Damage::Tail[4], Core::DamageArea::Tail, 4 },
	{ DcsIds::Damage::LeftEngine[0], Core::DamageArea::LeftEngine, 0 },
	{ DcsIds::Damage::LeftEngine[1], Core::DamageArea::LeftEngine, 1 },
	{ DcsIds::Damage::LeftEngine[2], Core::DamageArea::LeftEngine, 2 },
	{ DcsIds::Damage::RightEngine[0], Core::DamageArea::RightEngine, 0 },
	{ DcsIds::Damage::RightEngine[1], Core::DamageArea::RightEngine, 1 },
	{ DcsIds::Damage::RightEngine[2], Core::DamageArea::RightEngine, 2 }
};
}

namespace DcsBridge
{
DcsDamageMapping map_damage(int element, double integrity)
{
	for (const DamageBinding& binding : kDamageBindings)
	{
		if (binding.dcs_element == element)
		{
			return { true, { binding.area, binding.segment, integrity } };
		}
	}
	return {};
}
}
