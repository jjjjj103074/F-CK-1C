#pragma once

#include "../../../Contracts/MissionActions.h"

#include <deque>

namespace Core::Systems::Mc
{
/// @brief 依序保存 MC 傳往 Lua 的動作，並以確認序號推進傳送視窗。
class ExternalActionQueue final
{
public:
	std::uint64_t append(MissionActionKind action);
	void acknowledge(std::uint64_t sequence);
	bool acknowledged(std::uint64_t sequence) const noexcept;
	MissionActionBatch batch();

private:
	std::deque<MissionActionKind> pending_;
	std::uint64_t acknowledged_sequence_ = 0;
	std::uint64_t last_sequence_ = 0;
	std::uint64_t published_through_ = 0;
};
}
