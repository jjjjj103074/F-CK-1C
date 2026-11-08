#pragma once

#include "NativeGunAccess.h"
#include "../../../Core/Contracts/CockpitContracts.h"

#include <functional>
#include <memory>

namespace DcsBridge::Internal
{
class EventLog;

/// @brief 每 0.1 秒提供經版本核對的原生機砲存量觀測。
class GunAmmunitionSource final
{
public:
	using Reader = std::function<NativeGunResult()>;
	explicit GunAmmunitionSource(EventLog& log, Reader reader = {});
	~GunAmmunitionSource();
	GunAmmunitionSource(const GunAmmunitionSource&) = delete;
	GunAmmunitionSource& operator=(const GunAmmunitionSource&) = delete;
	/// @brief 清除飛行取樣狀態並重新核對原生環境。
	void reset_flight();
	/// @brief 回傳最近完整觀測，並在取樣時刻讀取原生存量。
	Core::GunAmmunitionObservation observe(double time_s);

private:
	struct Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
