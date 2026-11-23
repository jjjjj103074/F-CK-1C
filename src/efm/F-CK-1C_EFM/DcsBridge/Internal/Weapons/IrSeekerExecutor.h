#pragma once
#include "../Aircraft/DcsAircraftInterface.h"
#include "../CockpitParameterEndpoint.h"
#include "../EventLog.h"
#include "../../../Core/Contracts/CockpitContracts.h"
#include <functional>
namespace DcsBridge::Internal
{
class BridgeContext;
bool service_ir_seeker(BridgeContext& context, int command, float value);
/// @brief 將 SM 尋標器要求送入原生武器介面，保存控制讀回供下一個 frame 觀測。
class IrSeekerExecutor final
{
public:
	IrSeekerExecutor(DcsAircraftInterface& aircraft,
	                 cockpit_param_api api,
	                 std::function<void(const EventRecord&)> report);
	void reset_flight();
	void observe(Core::IrSeekerObservation& observation) const;
	void execute(const Core::StoresSnapshot& stores, double time_s);

private:
	DcsAircraftInterface& aircraft_;
	cockpit_param_api api_;
	std::function<void(const EventRecord&)> report_;
	AircraftIrReading reading_;
	Core::IrSeekerControl previous_;
	double last_attempt_s_ = -1.0;
};
}
