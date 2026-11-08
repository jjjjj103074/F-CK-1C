#pragma once

#include "../../../Contracts/MissionActions.h"

#include <deque>

namespace Core::Systems::Mc
{
/// @brief 保存全體 MC 操作順序，以執行確認推進 Lua 傳送視窗。
class ExternalActionQueue final
{
public:
	std::uint64_t append(MissionActionKind action);
	std::uint64_t append(const MissionAction& action);
	/// @brief 將尚待確認的選站及武器啟動動作轉為取消項目。
	void cancel_weapon_starts();
	/// @brief 回報傳送佇列是否仍包含尚待確認的選站動作。
	bool has_pending_selection() const noexcept;
	void acknowledge(std::uint64_t sequence);
	bool acknowledged(std::uint64_t sequence) const noexcept;
	MissionActionBatch batch();

private:
	std::deque<MissionAction> pending_;
	std::uint64_t acknowledged_sequence_ = 0;
	std::uint64_t last_sequence_ = 0;
	std::uint64_t published_through_ = 0;
};
}
