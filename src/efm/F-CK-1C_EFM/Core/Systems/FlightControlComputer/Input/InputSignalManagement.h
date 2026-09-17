#pragma once

#include "../ControlLaws/ControlLawConfig.h"
#include "../ControlLaws/ControlLawSignals.h"
#include "../../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
struct RawFlightControlInput
{
	double dt_s = 0.0;
	FlightControlObservation observation;
	PilotControlSignal pilot;
	LandingGearData landing_gear;
	FlightControlActuatorState actuator;
};

struct InputSignalManagementStepInput
{
	RawFlightControlInput raw;
	::Systems::PilotInputShapingConfig pilot_shaping;
	bool update_lateral_directional_shaping = false;
	double lateral_directional_shaping_dt_s = 0.0;
};

class InputSignalManagement
{
public:
	explicit InputSignalManagement(
		const ::Systems::InputSignalManagementConfig& config);
	const ::Systems::ManagedFlightControlSignals& update(
		const InputSignalManagementStepInput& input);

private:
	struct FilterInput
	{
		double current = 0.0;
		double target = 0.0;
		double time_constant_s = 0.0;
		double dt_s = 0.0;
	};
	struct ShapeAxisInput
	{
		double current = 0.0;
		double target = 0.0;
		const ::Systems::PilotInputShapingConfig& config;
		double dt_s = 0.0;
	};

	double filter(const FilterInput& input) const;
	double filter_signed_angle(const FilterInput& input) const;
	double filter_heading_deg(const FilterInput& input) const;
	double shape_stick(double value, double cubic_weight) const;
	void validate(const RawFlightControlInput& raw) const;
	void update_observation(const RawFlightControlInput& raw);
	void initialize_observation(const FlightControlObservation& source);
	void update_navigation_observation(
		const FlightControlObservation& source,
		double dt_s);
	void update_motion_observation(
		const FlightControlObservation& source,
		double dt_s);
	void update_air_data_observation(
		const FlightControlObservation& source,
		double dt_s);
	void update_pilot_raw(const PilotControlSignal& pilot);
	void update_pitch_shaping(
		const ::Systems::PilotInputShapingConfig& config,
		double dt_s);
	void update_lateral_directional_shaping(
		const ::Systems::PilotInputShapingConfig& config,
		double dt_s);
	double shape_axis(const ShapeAxisInput& input) const;
	void update_actuator_feedback(const RawFlightControlInput& raw);

	const ::Systems::InputSignalManagementConfig config_;
	::Systems::ManagedFlightControlSignals state_;
	bool observation_initialized_ = false;
};
}
}
