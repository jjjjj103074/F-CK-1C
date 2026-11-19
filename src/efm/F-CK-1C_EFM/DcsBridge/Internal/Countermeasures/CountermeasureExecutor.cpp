#include "CountermeasureExecutor.h"
#include "../EventLog.h"
#include <cstdio>
#include <utility>
namespace DcsBridge::Internal
{
CountermeasureExecutor::CountermeasureExecutor(DcsAircraftInterface& aircraft,
                                               std::function<void(const EventRecord&)> report)
    : aircraft_(aircraft),
      report_(std::move(report))
{
}
void CountermeasureExecutor::reset_flight()
{
	feedback_ = {};
	accepted_program_ = 0;
}
Core::CountermeasureObservation CountermeasureExecutor::observe()
{
	const auto stock = aircraft_.read_countermeasures();
	return { { stock.available(), stock.flare, stock.chaff }, feedback_ };
}
void CountermeasureExecutor::dispense(AircraftCountermeasure kind,
                                      std::uint64_t sequence,
                                      double time_s,
                                      bool& returned)
{
	const auto result = aircraft_.dispense_countermeasure(kind);
	returned = result.returned;
	char message[512] = {};
	std::snprintf(
	    message,
	    sizeof(message),
	    "countermeasure_dispense sequence=%llu kind=%s returned=%u before_status=%d after_status=%d before_flare=%d before_chaff=%d after_flare=%d after_chaff=%d exception=0x%08x",
	    static_cast<unsigned long long>(sequence),
	    kind == AircraftCountermeasure::Flare ? "flare" : "chaff",
	    returned ? 1U : 0U,
	    static_cast<int>(result.before.status),
	    static_cast<int>(result.after.status),
	    result.before.flare,
	    result.before.chaff,
	    result.after.flare,
	    result.after.chaff,
	    result.after.exception);
	if (report_)
		report_({ returned ? EventLevel::Info : EventLevel::Warning, time_s, message });
}
void CountermeasureExecutor::execute(const Core::CountermeasureSnapshot& snapshot, double time_s)
{
	if (!snapshot.status.available)
		return;
	if (snapshot.accepted_sequence != accepted_program_)
	{
		accepted_program_ = snapshot.accepted_sequence;
		char message[192] = {};
		std::snprintf(message,
		              sizeof(message),
		              "countermeasure_program accepted=%llu program=%s rounds=%d",
		              static_cast<unsigned long long>(accepted_program_),
		              snapshot.program == Core::CountermeasureProgram::Forward ? "forward" : "program6",
		              snapshot.remaining_rounds);
		if (report_)
			report_({ EventLevel::Info, time_s, message });
	}
	if (!snapshot.dispense || snapshot.dispense->sequence <= feedback_.sequence)
		return;
	const auto request = *snapshot.dispense;
	// 先保存已消費序號，後續 frame 持有相同快照時沿用同一筆回覆。
	feedback_ = { request.sequence, false, false, time_s };
	if (request.flare)
		dispense(AircraftCountermeasure::Flare, request.sequence, time_s, feedback_.flare_returned);
	if (request.chaff)
		dispense(AircraftCountermeasure::Chaff, request.sequence, time_s, feedback_.chaff_returned);
}
}
