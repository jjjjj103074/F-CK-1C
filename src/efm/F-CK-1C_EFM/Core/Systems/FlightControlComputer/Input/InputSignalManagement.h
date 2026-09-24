#pragma once

#include "InputSignalManagementConfig.h"
#include "../ModeAndGainScheduling/ModeAndGainSchedulingConfig.h"
#include "../ControlLaws/ControlLawSignals.h"
#include "../../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
// 「Raw」表示尚未經 FLCC 濾波與塑形；所有欄位已是 Core 約定的單位，
// 不是 DCS 原始軸值或座標。這也是 Executive 每週期的飛控部分輸入。
struct RawFlightControlInput
{
	// 本次 FLCC 更新相隔的秒數。
	double dt_s = 0.0;
	// 已換算的飛行感測值：高度 ft、速度 m/s 或 ft/s、角度 rad 等。
	FlightControlObservation observation;
	// 已校準的飛行員三軸與配平要求，範圍採 Core 正規化約定。
	PilotControlSignal pilot;
	// 起落架資料；FLCC 目前使用 handle_down 與 any_weight_on_wheels。
	LandingGearData landing_gear;
	// 致動器實際位置、速率與限制狀態；不是 FLCC 的輸出命令。
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
