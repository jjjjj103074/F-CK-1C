#include "NativeCountermeasureProbe.h"
#include "../../../DcsIds/CustomCommands.g.h"

#include <cmath>
#include <iterator>
#include <utility>

namespace
{
struct ProbeBinding
{
	int id;
	DcsBridge::Internal::NativeCmMethod method;
	DcsBridge::Internal::NativeCmKind kind;
};
using DcsBridge::Internal::NativeCmKind;
using DcsBridge::Internal::NativeCmMethod;
constexpr ProbeBinding kBindings[] = {
	{ DcsIds::Commands::NativeCmFlareDigital, NativeCmMethod::Digital, NativeCmKind::Flare },
	{ DcsIds::Commands::NativeCmChaffDigital, NativeCmMethod::Digital, NativeCmKind::Chaff },
	{ DcsIds::Commands::NativeCmFlareAnalog, NativeCmMethod::Analog, NativeCmKind::Flare },
	{ DcsIds::Commands::NativeCmChaffAnalog, NativeCmMethod::Analog, NativeCmKind::Chaff }
};
}
namespace DcsBridge::Internal
{
NativeCountermeasureProbe::NativeCountermeasureProbe(NativeCmHost host, Reporter reporter)
    : host_(std::move(host)),
      reporter_(std::move(reporter))
{
}
void NativeCountermeasureProbe::begin_flight(bool enabled)
{
	bool emit = enabled;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		emit = emit || state_.enabled;
		++epoch_;
		state_ = {};
		state_.enabled = enabled;
		state_.api_mask = enabled && host_.availability ? host_.availability() : 0;
		state_.status = enabled ? NativeCmStatus::Ready : NativeCmStatus::Disabled;
		active_ = enabled;
		failed_ = false;
		calling_ = false;
		held_ = {};
		pending_.clear();
		analog_.reset();
	}
	if (emit)
		report("begin_flight");
}
void NativeCountermeasureProbe::end_flight()
{
	bool emit = false;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		emit = state_.enabled;
		++epoch_;
		active_ = false;
		calling_ = false;
		pending_.clear();
		analog_.reset();
		held_ = {};
		state_.status = NativeCmStatus::Canceled;
	}
	if (emit)
		report("end_flight");
}
bool NativeCountermeasureProbe::handle_command(int command, float value)
{
	std::size_t index = 0;
	while (index < std::size(kBindings) && kBindings[index].id != command)
		++index;
	if (index == std::size(kBindings))
		return false;
	bool emit = false;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		if (!active_ || !std::isfinite(value))
			return true;
		const bool pressed = value > 0;
		const bool rising = pressed && !held_[index];
		held_[index] = pressed;
		if (!rising)
			return true;
		emit = true;
		if (pending_.size() >= 16 || failed_)
			state_.status = failed_ ? NativeCmStatus::CallFailed : NativeCmStatus::QueueFull;
		else
		{
			const auto& binding = kBindings[index];
			const int native = binding.kind == NativeCmKind::Flare ? DcsIds::DcsCommands::iCommandPlaneDropFlareOnce
			                                                       : DcsIds::DcsCommands::iCommandPlaneDropChaffOnce;
			pending_.push_back({ epoch_,
			                     ++state_.requests,
			                     binding.method,
			                     binding.kind,
			                     native,
			                     1.0F,
			                     host_.thread_id ? host_.thread_id() : 0 });
			state_.status = NativeCmStatus::Queued;
		}
	}
	if (emit)
		report("input");
	return true;
}
void NativeCountermeasureProbe::step(double time_s, bool armed)
{
	std::optional<NativeCmCall> work;
	bool rejected = false;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		if (!active_ || calling_ || failed_ || !std::isfinite(time_s))
			return;
		state_.time_s = time_s;
		if (!armed && analog_)
			pending_.clear();
		if (analog_)
		{
			if (armed && time_s < release_at_)
				return;
			work = analog_;
			work->value = 0;
			analog_.reset();
		}
		else if (!pending_.empty())
		{
			work = pending_.front();
			pending_.pop_front();
			state_.last_request = work->request;
			state_.method = work->method;
			state_.kind = work->kind;
			state_.request_thread = work->request_thread;
			state_.api_mask = host_.availability ? host_.availability() : 0;
			if (!armed || !(state_.api_mask & static_cast<unsigned>(work->method)))
			{
				state_.status = armed ? NativeCmStatus::MissingExport : NativeCmStatus::NotArmed;
				if (!armed)
					pending_.clear();
				work.reset();
				rejected = true;
			}
		}
		if (work)
			calling_ = true;
	}
	if (rejected)
		report("rejected");
	if (!work)
		return;
	// 原生派送在本工具及 Bridge 執行鎖以外進行，供 DCS 回送輸入回呼。
	NativeCmCallResult result;
	try
	{
		result = host_.dispatch ? host_.dispatch(*work) : NativeCmCallResult{};
	}
	catch (...)
	{
		result = {};
	}
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		if (work->epoch != epoch_ || !active_)
			return;
		calling_ = false;
		state_.execute_thread = host_.thread_id ? host_.thread_id() : 0;
		state_.exception = result.exception;
		if (!result.returned)
		{
			state_.status = NativeCmStatus::CallFailed;
			failed_ = true;
			pending_.clear();
		}
		else
		{
			++state_.calls;
			state_.status = work->value == 0 ? NativeCmStatus::Released : NativeCmStatus::Called;
			if (work->method == NativeCmMethod::Analog && work->value > 0)
			{
				analog_ = work;
				release_at_ = time_s + 0.04;
			}
		}
	}
	report(work->value == 0 ? "release" : "native_call");
}
NativeCmProbeSnapshot NativeCountermeasureProbe::snapshot() const
{
	const std::lock_guard<std::mutex> lock(mutex_);
	return state_;
}
void NativeCountermeasureProbe::report(const char* reason)
{
	if (reporter_)
		reporter_(snapshot(), reason);
}
}
