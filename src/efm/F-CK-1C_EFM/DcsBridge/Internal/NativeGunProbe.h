#pragma once

#include "../../Core/Contracts/CockpitContracts.h"
#include "NativeGunAccess.h"

#include <functional>

#include <memory>
#include <string>

namespace DcsBridge::Internal
{
class EventLog;

/// @brief 由測試包旗標啟用原生機砲觀測，記錄原始讀值與 MC 快照。
class NativeGunProbe final
{
public:
	using Reader = std::function<NativeGunResult()>;
	NativeGunProbe(const std::string& module_root, EventLog& log, Reader reader = {});
	~NativeGunProbe();
	NativeGunProbe(const NativeGunProbe&) = delete;
	NativeGunProbe& operator=(const NativeGunProbe&) = delete;
	bool enabled() const noexcept;
	void reset_flight();
	Core::GunAmmunitionObservation observe(double time_s);
	void record_mc(double time_s, const Core::CombatAvionicsSnapshot& snapshot);

private:
	struct Implementation;
	std::unique_ptr<Implementation> implementation_;
};
}
