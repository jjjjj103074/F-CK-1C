#pragma once

#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class PropulsionDiagnostics final : public System
{
public:
	void setup(SystemSetup& setup) override;
	void step(
		const AircraftDataView& aircraft,
		SystemResult& result) override;
	void handle_command(const Command& command);
	const PropulsionTestIntent& intent() const;

private:
	PropulsionTestIntent intent_;
};
}
}
