#include "CountermeasureSystem.h"
#include "../SystemPipeline.h"
#include <stdexcept>
namespace Core::Systems
{
void CountermeasureSystem::setup(SystemSetup& setup)
{
	setup.update_rate_hz(32);
	setup.read(AircraftDataKeys::kFrameInput);
	setup.read(AircraftDataKeys::kCountermeasureRequests);
	setup.publish(AircraftDataKeys::kCountermeasureSnapshot, output_);
}
void CountermeasureSystem::step(const SystemStepContext& context,
                                const AircraftDataView& aircraft,
                                SystemResult& result)
{
	const auto& observation = aircraft.read(AircraftDataKeys::kFrameInput).cockpit.countermeasures;
	output_.observation = observation;
	if (output_.dispense && observation.execution.sequence == output_.dispense->sequence)
	{
		const auto& request = *output_.dispense;
		const auto& execution = observation.execution;
		const bool returned =
		    (!request.flare || execution.flare_returned) && (!request.chaff || execution.chaff_returned);
		output_.dispense.reset();
		if (returned)
		{
			++output_.completed_rounds;
			--output_.remaining_rounds;
			output_.state =
			    output_.remaining_rounds ? CountermeasureProgramState::Running : CountermeasureProgramState::Completed;
			next_due_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
			                std::chrono::duration<double>(execution.completed_at_s)) +
			            interval_;
		}
		else
			output_.state = CountermeasureProgramState::Failed;
	}
	for (const auto& request : aircraft.read(AircraftDataKeys::kCountermeasureRequests).items)
	{
		if (request.sequence <= output_.accepted_sequence)
			continue;
		output_.accepted_sequence = request.sequence;
		output_.program = request.program;
		output_.completed_rounds = 0;
		output_.remaining_rounds = request.program == CountermeasureProgram::Forward ? 10 : 7;
		interval_ = request.program == CountermeasureProgram::Forward ? std::chrono::milliseconds(1000)
		                                                              : std::chrono::milliseconds(500);
		output_.state = CountermeasureProgramState::Running;
		output_.dispense.reset();
		next_due_ = context.input_time;
	}
	// 以目前輸入時間排程，每輪等待 Bridge 回報後才安排下一輪。
	if (output_.state == CountermeasureProgramState::Running && !output_.dispense && context.input_time >= next_due_)
	{
		if (last_dispense_ == kMaximumExactCockpitInteger)
			throw std::overflow_error("Countermeasure dispense sequence exhausted.");
		output_.dispense =
		    CountermeasureDispenseRequest{ ++last_dispense_, output_.program == CountermeasureProgram::Forward, true };
	}
	output_.status = { true, output_.status.revision + 1 };
	result.publish(AircraftDataKeys::kCountermeasureSnapshot, output_);
}
}
