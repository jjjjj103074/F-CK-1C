#include "WeaponExecution.h"

namespace Core::Systems::Mc::Internal
{
WeaponExecution::WeaponExecution(WeaponUseExecutionState& state, const StoresCommandQueue& commands)
    : state_(state),
      commands_(commands)
{
}

bool WeaponExecution::begin(std::uint64_t request_id, bool available)
{
	if (!available)
	{
		state_.rejected_request_id = request_id;
		return false;
	}
	state_.request_id = request_id;
	state_.phase = WeaponUsePhase::StartPending;
	stop_required_ = true;
	started_at_.reset();
	end_requested_ = false;
	return true;
}

void WeaponExecution::request_end(std::uint64_t request_id)
{
	if (stop_required_ && state_.request_id == request_id)
		end_requested_ = true;
}

bool WeaponExecution::end_due(std::chrono::nanoseconds time)
{
	if (!stop_required_)
		return false;
	if (!started_at_ && commands_.acknowledged(state_.request_id))
	{
		started_at_ = time;
		if (state_.in_progress())
			state_.phase = WeaponUsePhase::StartAcknowledged;
	}
	return end_requested_ && started_at_ && time - *started_at_ >= std::chrono::milliseconds(80);
}

void WeaponExecution::stop_scheduled(std::uint64_t request_id)
{
	if (state_.request_id != request_id)
		return;
	stop_required_ = false;
	finish(state_.phase == WeaponUsePhase::Canceled ? WeaponUsePhase::Canceled : WeaponUsePhase::StopScheduled);
}

void WeaponExecution::cancel_pending()
{
	if (state_.in_progress() && !commands_.acknowledged(state_.request_id))
		state_.phase = WeaponUsePhase::Canceled;
}

bool WeaponExecution::abort()
{
	const bool stop_required = stop_required_;
	stop_required_ = false;
	if (stop_required)
		finish(WeaponUsePhase::Canceled);
	return stop_required;
}

void WeaponExecution::finish(WeaponUsePhase phase)
{
	state_.phase = phase;
	started_at_.reset();
	end_requested_ = false;
}
}
