#pragma once

#include "../WeaponControl/WeaponControl.h"

namespace Core::Systems::Mc
{
/// @brief Owns master-arm/mode transitions and coordinates their effects on peer components.
class ModeManagement final
{
public:
	ModeManagement(ModeData& mode, SensorCoordination& sensors, WeaponControl& weapons, ExternalActionQueue& actions);
	void handle(const Command& command);

private:
	ModeData& mode_;
	SensorCoordination& sensors_;
	WeaponControl& weapons_;
	ExternalActionQueue& actions_;
};
}
