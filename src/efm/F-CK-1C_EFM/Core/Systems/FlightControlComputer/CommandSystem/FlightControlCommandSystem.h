#pragma once

#include "AutomaticFlightControlTypes.h"
#include "GuidanceCoordinationConfig.h"
#include "../ControlLaws/ControlLawConfig.h"
#include "../ControlLaws/ControlLawSignals.h"
#include "FlightControlCommandBinding.h"
#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

#include <memory>
#include <vector>

namespace Core::Systems
{
/// @brief 建立命令計算模組所需的設定切片與初始狀態。
/// 只在建構期間使用，不形成另一份完整 FLCC 設定。
struct FlightControlCommandSystemConstruction
{
	const ::Systems::LongitudinalControlConfig& longitudinal;  // 飛行員縱向目標換算設定。
	const ::Systems::GuidanceCoordinationConfig& coordination;  // 三軸指引協調設定。
	const AutomaticFlightControlConfig& automatic_flight;  // 自動飛行設定。
	AutomaticFlightGuidanceLimits automatic_limits;  // 從共用飛行包線投影的橫向限制。
	bool initial_weight_on_wheels = false;  // 建立時是否位於地面。
};

struct FlightControlCommandSystemInput
{
	::Systems::ComputedFlightState flight;
	::Systems::ManagedFlightControlSignals signals;
	::Systems::ActiveFlightControlConfiguration configuration;
};

struct FlightControlCommandSystemResult
{
	SelectedFlightReference selected;
	GuidanceCoordinationResult coordinated;
	AutomaticFlightGuidanceReference automatic;
};

struct FlightControlCommandMonitorInput
{
	double dt_s = 0.0;
	bool vertical_active = false;
	bool lateral_active = false;
	VerticalGuidanceReferenceType vertical_type =
		VerticalGuidanceReferenceType::None;
	double pitch_tracking_error_rad = 0.0;
	double vertical_speed_tracking_error_ft_s = 0.0;
	double lateral_tracking_error_rad = 0.0;
	GuidanceConstraint constraint;
	bool control_path_saturated = false;
	ConstraintReason hard_protection_reason = ConstraintReason::None;
};

// Public command-and-guidance boundary of the FLCC. Selection, pilot mapping,
// AP implementation and cross-axis coordination stay private to this module.
class FlightControlCommandSystem final
{
public:
	explicit FlightControlCommandSystem(
		const FlightControlCommandSystemConstruction& construction);
	~FlightControlCommandSystem();
	FlightControlCommandSystem(const FlightControlCommandSystem&) = delete;
	FlightControlCommandSystem& operator=(
		const FlightControlCommandSystem&) = delete;

	/// @brief 從自動飛行模組取得本實例的指令 ID 與交付函式。
	/// @return 只包含本實例已開放指令的綁定，不操作 Pipeline。
	std::vector<FlightControlCommandBinding> command_bindings();
	const FlightControlCommandSystemResult& update(
		const FlightControlCommandSystemInput& input);
	void observe_control_result(
		const FlightControlCommandMonitorInput& observation);
	const AutomaticFlightControlSnapshot& automatic_snapshot() const;

private:
	class Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
