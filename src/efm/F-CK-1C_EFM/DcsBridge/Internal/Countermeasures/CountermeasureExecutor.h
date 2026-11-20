#pragma once
#include "../Aircraft/DcsAircraftInterface.h"
#include "../EventLog.h"
#include <functional>
#include "../../../Core/Contracts/CockpitContracts.h"
namespace DcsBridge::Internal
{
/// @brief 於 Bridge 原生 I/O 階段執行每輪反制要求，保存呼叫結果與即時庫存。
class CountermeasureExecutor final
{
public:
	CountermeasureExecutor(DcsAircraftInterface& aircraft, std::function<void(const EventRecord&)> report);
	void reset_flight();
	Core::CountermeasureObservation observe();
	void execute(const Core::CountermeasureSnapshot& snapshot, double time_s);

private:
	void dispense(AircraftCountermeasure kind, std::uint64_t sequence, double time_s, bool& returned);
	DcsAircraftInterface& aircraft_;
	std::function<void(const EventRecord&)> report_;
	Core::CountermeasureExecutionFeedback feedback_;
	std::uint64_t accepted_program_ = 0;
};
}
