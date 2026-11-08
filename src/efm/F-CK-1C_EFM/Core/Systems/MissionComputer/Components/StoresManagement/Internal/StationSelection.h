#pragma once

#include "../../../../../Contracts/CockpitContracts.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 依模式意圖與有效庫存決定掛點，管理要求版本及相符的轉接層執行確認。
class StationSelection final
{
public:
	StationSelection(const StoresInventorySnapshot& inventory, StationSelectionState& state);
	/// @brief 保存切換模式與同模式重選的循環次數，供更新庫存後套用。
	void select_mode(OperatingMode mode, bool cycle_weapon);
	/// @brief 套用選站意圖、掛載變化及確認，回報對外選站狀態是否改變。
	bool update(const WeaponStationObservation& observation);

private:
	void request(std::optional<int> station, bool force = false);
	const StoresInventorySnapshot& inventory_;
	StationSelectionState& state_;
	OperatingMode mode_ = OperatingMode::Navigation;
	bool reset_selection_ = false;
	std::uint64_t pending_cycles_ = 0;
	std::uint64_t rearm_revision_ = 0;
	std::uint64_t requested_load_revision_ = 0;
};
}
