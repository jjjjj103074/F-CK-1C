#pragma once

#include "FlightControlActuationModel.h"
#include "FlightControlActuationSystemConfig.h"
#include "../System.h"
#include "../../Contracts/AircraftData.h"

namespace Core
{
namespace Systems
{
class FlightControlActuationSystem final : public System
{
public:
	explicit FlightControlActuationSystem(
		const FlightControlActuationSystemConfig& config);

	void setup(SystemSetup& setup) override;
	void step(
		const SystemStepContext& context,
		const AircraftDataView& aircraft,
		SystemResult& result) override;
	/// @brief 將 [-1, 1] 位置需求依固定行程轉成弧度，推進致動器模型。
	/// @param command 三軸正規化舵面位置需求。
	/// @param dt_s 本次致動器積分的時間間隔，單位為秒。
	/// @return 含實際角度、角速度與限制資訊的目前狀態。
	const FlightControlActuatorState& update(
		const FlightControlActuatorCommand& command,
		double dt_s);
	const FlightControlActuatorState& state() const;

private:
	const FlightControlActuationSystemConfig config_;
	::Systems::FlightControlAxisModelState symmetric_stabilator_;
	::Systems::FlightControlAxisModelState differential_flaperon_;
	::Systems::FlightControlAxisModelState rudder_;
	FlightControlActuatorState state_;
};

SystemEntry make_flight_control_actuation_system_entry(
	const FlightControlActuationSystemConfig& config);
}
}
