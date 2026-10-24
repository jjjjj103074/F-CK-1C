#pragma once

#include "../../../Contracts/MissionActions.h"

#include <deque>

namespace Core::Systems::Mc
{
/// @brief Ordered MC-to-Lua delivery state; this queue never replaces edges with a latest-value flag.
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
