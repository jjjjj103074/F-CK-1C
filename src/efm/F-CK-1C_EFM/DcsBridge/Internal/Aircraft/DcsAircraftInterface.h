#pragma once
#include "../GunAmmunition/GunAmmunitionTypes.h"
#include <cstdint>
#include <memory>

namespace DcsBridge::Internal
{
struct DcsAircraftBackend;
struct AircraftBuildInfo
{
	bool present = false;
	std::uint32_t timestamp = 0, image_size = 0;
	std::uint64_t gun_fingerprint = 0;
	bool payload_export = false, gun_profile_matched = false;
	bool header_valid = false;
};
struct AircraftGunReading
{
	NativeGunResult ammunition;
	AircraftBuildInfo build;
	std::uint32_t exception = 0;
};
enum class AircraftAccessStatus
{
	Available,
	WaitingForModule,
	UnsupportedBuild,
	MissingInterface,
	WaitingForContext,
	WaitingForHuman,
	WaitingForPayload,
	UnreadableObject,
	PayloadMismatch,
	ContextChanged,
	CallFailed,
	InvalidCount,
	NoAmmunition,
	InvalidRequest
};
enum class AircraftCountermeasure
{
	Flare,
	Chaff
};
/// @brief 保存本次反制庫存及原生來源狀態。
struct AircraftCountermeasureReading
{
	AircraftAccessStatus status = AircraftAccessStatus::WaitingForModule;
	int flare = -1, chaff = -1;
	std::uint32_t exception = 0;
	bool available() const noexcept
	{
		return status == AircraftAccessStatus::Available;
	}
};
struct AircraftCountermeasureResult
{
	bool returned = false; ///< 原生施放呼叫已返回；after 保存呼叫後的庫存觀測。
	AircraftCountermeasureReading before, after;
};
/// @brief 提供玩家飛機的機砲觀測與反制操作，集中原生版本及物件核對。
/// 由 Bridge 持有，於原生 I/O 階段使用；各次呼叫重新取得飛機物件。
class DcsAircraftInterface final
{
public:
	DcsAircraftInterface();
	explicit DcsAircraftInterface(DcsAircraftBackend backend);
	~DcsAircraftInterface();
	DcsAircraftInterface(const DcsAircraftInterface&) = delete;
	DcsAircraftInterface& operator=(const DcsAircraftInterface&) = delete;
	/// @brief 清除本次飛行的原生解析結果。
	void reset_flight();
	AircraftGunReading read_gun_ammunition();
	AircraftCountermeasureReading read_countermeasures();
	/// @brief 取得即時庫存後施放一發，並保存前後觀測。
	AircraftCountermeasureResult dispense_countermeasure(AircraftCountermeasure kind);

private:
	struct Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
