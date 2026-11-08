#pragma once

#include "../../../../../Contracts/CockpitContracts.h"

#include <deque>

namespace Core::Systems::Mc::Internal
{
/// @brief 依模式意圖與有效庫存決定掛點，管理要求版本及相符的轉接層執行確認。
class StationSelection final
{
public:
	StationSelection(const StoresInventorySnapshot& inventory, StationSelectionState& state);
	/// @brief 保存本筆模式選擇與同模式重選意圖。
	void select_mode(OperatingMode mode, bool cycle_weapon);
	/// @brief 核對已發出的要求，保存最近完成的選站。
	bool acknowledge(const WeaponStationObservation& observation);
	/// @brief 套用選站意圖、掛載變化及確認，回報對外選站狀態是否改變。
	bool update(const WeaponStationObservation& observation);

private:
	void request(std::optional<int> station, bool force = false);
	struct Request
	{
		std::uint64_t revision;
		std::optional<int> station;
	};
	std::deque<Request> requests_;
	const StoresInventorySnapshot& inventory_;
	StationSelectionState& state_;
	OperatingMode mode_ = OperatingMode::Navigation;
	bool reset_selection_ = false;
	bool cycle_requested_ = false;
	std::uint64_t rearm_revision_ = 0;
	std::uint64_t requested_load_revision_ = 0;
};
}
