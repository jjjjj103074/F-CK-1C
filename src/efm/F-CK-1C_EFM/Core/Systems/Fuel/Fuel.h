#pragma once

#include "FuelModel.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class Fuel final : public System
{
public:
	explicit Fuel(const FlightFuelState& initial = {});

	void setup(SystemSetup& setup) override;
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;

	const FuelData& step(const FuelDemand& demand, double dt_s);
	void begin_frame(bool suppress_consumption);
	void set_internal_fuel(double fuel_kg);
	void set_external_fuel(const ::Systems::ExternalFuelState& fuel);
	double internal_fuel_kg() const;
	double external_fuel_kg() const;
	const ::Systems::FuelSystem& state() const;
	const FuelData& data() const;

private:
	FlightFuelState management_state() const;
	const FuelData& update(const FuelDemand& demand, double dt_s);
	void refresh_data();

	::Systems::FuelSystem fuel_;
	FuelData data_;
	bool consumption_suppressed_ = false;
};
}
}
