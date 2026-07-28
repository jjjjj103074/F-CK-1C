#include "AirframeStructure.h"

#include "../SystemPipeline.h"

namespace Core
{
namespace Systems
{
void AirframeStructure::setup(SystemSetup& setup)
{
	setup.publish(AircraftDataKeys::kAirframeIntegrity, integrity_);
	setup.register_damage_handler(
		DamageArea::LeftWing,
		[this](const DamageEvent& event) { apply_damage(event); });
	setup.register_damage_handler(
		DamageArea::RightWing,
		[this](const DamageEvent& event) { apply_damage(event); });
	setup.register_damage_handler(
		DamageArea::Tail,
		[this](const DamageEvent& event) { apply_damage(event); });
	setup.register_repair_handler(
		[this](const RepairEvent& event) { repair(event); });
}

void AirframeStructure::step(
	const AircraftDataView& aircraft,
	SystemResult& result)
{
	(void)aircraft;
	result.publish(AircraftDataKeys::kAirframeIntegrity, integrity_);
}

void AirframeStructure::apply_damage(const DamageEvent& event)
{
	switch (event.area)
	{
	case DamageArea::LeftWing:
		left_wing_.apply(event.segment, event.integrity); break;
	case DamageArea::RightWing:
		right_wing_.apply(event.segment, event.integrity); break;
	case DamageArea::Tail:
		tail_.apply(event.segment, event.integrity); break;
	default:
		return;
	}
	refresh_integrity();
}

void AirframeStructure::repair(const RepairEvent& event)
{
	(void)event;
	left_wing_.reset();
	right_wing_.reset();
	tail_.reset();
	refresh_integrity();
}

void AirframeStructure::refresh_integrity()
{
	integrity_ = {
		left_wing_.value(),
		right_wing_.value(),
		tail_.value()
	};
}

const AirframeIntegrity& AirframeStructure::integrity() const
{
	return integrity_;
}
}
}
