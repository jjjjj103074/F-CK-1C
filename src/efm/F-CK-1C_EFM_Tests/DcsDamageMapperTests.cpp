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

template <std::size_t Size>
void expect_area_mappings(
	Tests::Context& context,
	const int (&elements)[Size],
	Core::DamageArea area)
{
	for (std::size_t segment = 0; segment < Size; ++segment)
	{
		expect_damage_mapping(context, { elements[segment], area, segment });
	}
}
}

void run_dcs_damage_mapper_tests(Tests::Context& context)
{
	expect_area_mappings(
		context, DcsIds::Damage::LeftWing, Core::DamageArea::LeftWing);
	expect_area_mappings(
		context, DcsIds::Damage::RightWing, Core::DamageArea::RightWing);
	expect_area_mappings(
		context, DcsIds::Damage::Tail, Core::DamageArea::Tail);
	expect_area_mappings(
		context, DcsIds::Damage::LeftEngine, Core::DamageArea::LeftEngine);
	expect_area_mappings(
		context, DcsIds::Damage::RightEngine, Core::DamageArea::RightEngine);
	TEST_EXPECT(context, !DcsBridge::map_damage(-1, 0.5).mapped);
}
