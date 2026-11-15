#include "NativeCountermeasureProbe.h"
#include "../../../DcsIds/CustomCommands.g.h"
#include <cmath>
#include <iterator>
#include <optional>
#include <utility>

namespace
{
using namespace DcsBridge::Internal;
struct ProbeBinding
{
	int id;
	NativeCmKind kind;
};
constexpr ProbeBinding kBindings[] = { { DcsIds::Commands::NativeCmFlareDirect, NativeCmKind::Flare },
	                                   { DcsIds::Commands::NativeCmChaffDirect, NativeCmKind::Chaff } };
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
	bool emit;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		emit = enabled || state_.enabled;
		++epoch_;
		state_ = {};
		state_.enabled = enabled;
		state_.status = enabled ? NativeCmStatus::Ready : NativeCmStatus::Disabled;
		active_ = enabled;
		failed_ = false;
		held_ = {};
		pending_.clear();
		next_sample_s_ = next_report_s_ = 0;
	}
	if (emit)
		report("begin_flight");
}
void NativeCountermeasureProbe::end_flight()
{
	bool emit;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		emit = state_.enabled;
		++epoch_;
		active_ = false;
		pending_.clear();
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
	const auto thread = host_.thread_id ? host_.thread_id() : 0;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		if (!active_ || !std::isfinite(value))
			return true;
		const bool pressed = value > 0;
		const bool rising = pressed && !held_[index];
		held_[index] = pressed;
		if (!rising)
			return true;
		if (failed_)
			state_.status = NativeCmStatus::CallFailed;
		else if (pending_.size() >= 16)
			state_.status = NativeCmStatus::QueueFull;
		else
		{
			pending_.push_back({ ++state_.requests, kBindings[index].kind, thread });
			state_.status = NativeCmStatus::Queued;
		}
	}
	report("input");
	return true;
}
void NativeCountermeasureProbe::step(double time_s, bool armed)
{
	std::optional<NativeCmCall> work;
	std::uint64_t epoch;
	bool rejected = false;
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		if (!active_ || calling_ || failed_ || !std::isfinite(time_s))
			return;
		state_.time_s = time_s;
		if (!pending_.empty())
		{
			work = pending_.front();
			pending_.pop_front();
			state_.last_request = work->request;
			state_.kind = work->kind;
			state_.request_thread = work->request_thread;
			if (!armed)
			{
				state_.status = NativeCmStatus::NotArmed;
				pending_.clear();
				work.reset();
				rejected = true;
			}
		}
		if (!work && !rejected && time_s < next_sample_s_)
			return;
		next_sample_s_ = time_s + 0.1;
		epoch = epoch_;
		calling_ = true;
	}
	// 在 EFM 回呼中取樣與施放，將引擎呼叫放在測試工具鎖以外。
	unsigned mask = 0;
	NativeCmInventory inventory;
	NativeCmInventory before;
	NativeCmCallResult result;
	try
	{
		mask = host_.availability ? host_.availability() : 0;
		if ((mask & 1U) && host_.observe)
			inventory = host_.observe();
		else
			inventory.status = NativeCmSourceStatus::UnsupportedBuild;
		before = inventory;
		{
			const std::lock_guard<std::mutex> lock(mutex_);
			if (epoch != epoch_ || !active_)
			{
				calling_ = false;
				return;
			}
		}
		if (work && inventory.available && (mask & 2U) && host_.dispatch)
		{
			result = host_.dispatch(*work);
			before = result.before;
			inventory = result.after;
		}
	}
	catch (...)
	{
		inventory.available = false;
		inventory.flare = inventory.chaff = -1;
		inventory.status = NativeCmSourceStatus::NativeCallFailed;
		inventory.exception = 0xe0000001U;
	}
	const auto thread = host_.thread_id ? host_.thread_id() : 0;
	bool emit;
	const char* reason = rejected ? "rejected" : work ? "direct_call" : "inventory";
	{
		const std::lock_guard<std::mutex> lock(mutex_);
		if (epoch != epoch_ || !active_)
		{
			calling_ = false;
			return;
		}
		calling_ = false;
		const auto& previous = state_.inventory;
		emit = work.has_value() || rejected || time_s >= next_report_s_ || previous.available != inventory.available ||
		       previous.status != inventory.status || previous.flare != inventory.flare ||
		       previous.chaff != inventory.chaff;
		state_.api_mask = mask;
		state_.execute_thread = thread;
		state_.inventory = inventory;
		++state_.samples;
		state_.exception = inventory.exception ? inventory.exception : result.exception;
		if (work)
		{
			state_.before_flare = before.flare;
			state_.before_chaff = before.chaff;
		}
		if (state_.exception)
		{
			failed_ = true;
			pending_.clear();
			state_.status = NativeCmStatus::CallFailed;
			emit = true;
		}
		else if (work)
		{
			if (!(mask & 2U))
				state_.status = NativeCmStatus::MissingExport;
			else if (inventory.status == NativeCmSourceStatus::NoAmmunition)
				state_.status = NativeCmStatus::NoAmmunition;
			else if (!result.returned)
			{
				state_.status = inventory.available ? NativeCmStatus::CallFailed : NativeCmStatus::WaitingForSource;
				if (inventory.available)
				{
					failed_ = true;
					pending_.clear();
				}
			}
			else
			{
				++state_.calls;
				state_.status = NativeCmStatus::Called;
			}
		}
		else if (!rejected &&
		         (state_.status == NativeCmStatus::Ready || state_.status == NativeCmStatus::WaitingForSource))
			state_.status = inventory.available ? NativeCmStatus::Ready : NativeCmStatus::WaitingForSource;
		if (emit)
			next_report_s_ = time_s + 1;
	}
	if (emit)
		report(reason);
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
