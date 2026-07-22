#include "TestHarness.h"

#include "DcsBridge/Internal/DcsDamageMapper.h"
#include "DcsIds/DamageIds.h"

namespace
{
constexpr double kTolerance = 1e-9;

struct ExpectedDamage
{
	int element = 0;
	Core::DamageArea area = Core::DamageArea::LeftWing;
	std::size_t segment = 0;
};

void expect_damage_mapping(
	Tests::Context& context,
	const ExpectedDamage& expected)
{
	const DcsBridge::DcsDamageMapping mapping =
		DcsBridge::map_damage(expected.element, 0.35);
	TEST_EXPECT(context, mapping.mapped);
	TEST_EXPECT(context, mapping.event.area == expected.area);
	TEST_EXPECT(context, mapping.event.segment == expected.segment);
	TEST_EXPECT_NEAR(context, mapping.event.integrity, 0.35, kTolerance);
}
}

void run_dcs_damage_mapper_tests(Tests::Context& context)
{
	expect_damage_mapping(context,
		{ DcsIds::Damage::LeftWing[2], Core::DamageArea::LeftWing, 2 });
	expect_damage_mapping(context,
		{ DcsIds::Damage::RightWing[1], Core::DamageArea::RightWing, 1 });
	expect_damage_mapping(context,
		{ DcsIds::Damage::Tail[4], Core::DamageArea::Tail, 4 });
	expect_damage_mapping(context,
		{ DcsIds::Damage::LeftEngine[0], Core::DamageArea::LeftEngine, 0 });
	expect_damage_mapping(context,
		{ DcsIds::Damage::RightEngine[2], Core::DamageArea::RightEngine, 2 });
	TEST_EXPECT(context, !DcsBridge::map_damage(-1, 0.5).mapped);
}
