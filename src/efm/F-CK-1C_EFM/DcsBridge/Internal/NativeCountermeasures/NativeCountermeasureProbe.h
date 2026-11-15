#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>

namespace DcsBridge::Internal
{
enum class NativeCmKind
{
	None = 0,
	Flare = 1,
	Chaff = 2
};
enum class NativeCmStatus
{
	Disabled = 0,
	Ready = 1,
	Queued = 2,
	Called = 3,
	NotArmed = 5,
	MissingExport = 6,
	CallFailed = 7,
	Canceled = 8,
	QueueFull = 9,
	WaitingForSource = 10,
	NoAmmunition = 11
};
enum class NativeCmSourceStatus
{
	Available = 0,
	WaitingForModule = 1,
	UnsupportedBuild = 2,
	WaitingForContext = 3,
	WaitingForHuman = 4,
	WaitingForPayload = 5,
	UnreadableObject = 6,
	NativeCallFailed = 7,
	InvalidCount = 8,
	PayloadMismatch = 9,
	ContextChanged = 10,
	NoAmmunition = 11
};
/// @brief 保存這次原生取樣的玩家物件、庫存與來源狀態。
struct NativeCmInventory
{
	bool available = false, payload_matches = false;
	int flare = -1, chaff = -1;
	NativeCmSourceStatus status = NativeCmSourceStatus::WaitingForModule;
	std::uint32_t exception = 0;
	std::uintptr_t context = 0, human = 0, payload = 0, human_payload = 0;
	std::uintptr_t flare_function = 0, chaff_function = 0;
};
struct NativeCmCall
{
	std::uint64_t request = 0;
	NativeCmKind kind = NativeCmKind::None;
	std::uint32_t request_thread = 0;
};
/// @brief 記錄單發原生呼叫及其前後取樣。
struct NativeCmCallResult
{
	bool returned = false;
	std::uint32_t exception = 0;
	NativeCmInventory before, after;
};
struct NativeCmProbeSnapshot
{
	bool enabled = false;
	unsigned api_mask = 0;
	NativeCmStatus status = NativeCmStatus::Disabled;
	std::uint64_t requests = 0, calls = 0, last_request = 0, samples = 0;
	NativeCmKind kind = NativeCmKind::None;
	std::uint32_t request_thread = 0, execute_thread = 0, exception = 0;
	NativeCmInventory inventory;
	int before_flare = -1, before_chaff = -1;
	double time_s = 0;
};
/// @brief 提供版本核對、原生取樣、單發施放與執行緒識別。
struct NativeCmHost
{
	std::function<unsigned()> availability;
	std::function<NativeCmCallResult(const NativeCmCall&)> dispatch;
	std::function<std::uint32_t()> thread_id;
	std::function<NativeCmInventory()> observe;
};
NativeCmHost make_native_countermeasure_host();
/// @brief 將測試鍵排至 EFM 回呼，並以 10 Hz 保存原生庫存。
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
	void step(double time_s, bool armed);
	NativeCmProbeSnapshot snapshot() const;

private:
	void report(const char* reason);
	NativeCmHost host_;
	Reporter reporter_;
	mutable std::mutex mutex_;
	NativeCmProbeSnapshot state_;
	std::uint64_t epoch_ = 0;
	bool active_ = false, calling_ = false, failed_ = false;
	std::array<bool, 2> held_ = {};
	std::deque<NativeCmCall> pending_;
	double next_sample_s_ = 0, next_report_s_ = 0;
};
}
