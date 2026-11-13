#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>

namespace DcsBridge::Internal
{
enum class NativeCmMethod
{
	Digital = 1,
	Analog = 2
};
enum class NativeCmKind
{
	Flare = 1,
	Chaff = 2
};
enum class NativeCmStatus
{
	Disabled = 0,
	Ready = 1,
	Queued = 2,
	Called = 3,
	Released = 4,
	NotArmed = 5,
	MissingExport = 6,
	CallFailed = 7,
	Canceled = 8,
	QueueFull = 9
};

/// @brief 一次原生派送，保存飛行世代、要求編號及類比輸入邊緣。
struct NativeCmCall
{
	std::uint64_t epoch = 0;
	std::uint64_t request = 0;
	NativeCmMethod method = NativeCmMethod::Digital;
	NativeCmKind kind = NativeCmKind::Flare;
	int command = 0;
	float value = 1.0F;
	std::uint32_t request_thread = 0;
};
struct NativeCmCallResult
{
	bool returned = false;
	std::uint32_t exception = 0;
};
struct NativeCmProbeSnapshot
{
	bool enabled = false;
	unsigned api_mask = 0;
	NativeCmStatus status = NativeCmStatus::Disabled;
	std::uint64_t requests = 0;
	std::uint64_t calls = 0;
	std::uint64_t last_request = 0;
	NativeCmMethod method = NativeCmMethod::Digital;
	NativeCmKind kind = NativeCmKind::Flare;
	std::uint32_t request_thread = 0;
	std::uint32_t execute_thread = 0;
	std::uint32_t exception = 0;
	double time_s = 0;
};
/// @brief 提供具名匯出可用性、受保護派送與目前執行緒識別。
struct NativeCmHost
{
	std::function<unsigned()> availability;
	std::function<NativeCmCallResult(const NativeCmCall&)> dispatch;
	std::function<std::uint32_t()> thread_id;
};
NativeCmHost make_native_countermeasure_host();

/// @brief 測試包啟用的原生反制驗證，將按下要求排至 EFM 更新回呼執行。
class NativeCountermeasureProbe final
{
public:
	using Reporter = std::function<void(const NativeCmProbeSnapshot&, const char*)>;
	NativeCountermeasureProbe(NativeCmHost host, Reporter reporter = {});
	NativeCountermeasureProbe(const NativeCountermeasureProbe&) = delete;
	NativeCountermeasureProbe& operator=(const NativeCountermeasureProbe&) = delete;
	void begin_flight(bool enabled);
	void end_flight();
	bool handle_command(int command, float value);
	/// @brief 執行至多一筆派送；類比輸入至少保持 40 ms 後送出放開。
	void step(double time_s, bool armed);
	NativeCmProbeSnapshot snapshot() const;

private:
	void report(const char* reason);
	NativeCmHost host_;
	Reporter reporter_;
	mutable std::mutex mutex_;
	NativeCmProbeSnapshot state_;
	std::uint64_t epoch_ = 0;
	bool active_ = false;
	bool calling_ = false;
	bool failed_ = false;
	std::array<bool, 4> held_ = {};
	std::deque<NativeCmCall> pending_;
	std::optional<NativeCmCall> analog_;
	double release_at_ = 0;
};
}
