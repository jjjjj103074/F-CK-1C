#pragma once
#include "../Contracts/ExternalActions.h"
#include <deque>
namespace Core::Systems
{
/// @brief 保存功能提出的操作與執行回覆；DcsBridge 負責實際傳送視窗。
class ExternalActionOutbox final
{
public:
	ExternalActionOutbox() = default;
	ExternalActionOutbox(const ExternalActionOutbox&) = delete;
	ExternalActionOutbox& operator=(const ExternalActionOutbox&) = delete;
	ExternalActionOutbox(ExternalActionOutbox&&) = delete;
	ExternalActionOutbox& operator=(ExternalActionOutbox&&) = delete;
	std::uint64_t append(MissionActionKind action, std::uint64_t parent_request = 0);
	std::uint64_t append(const MissionAction& action, std::uint64_t parent_request = 0);
	std::uint64_t reserve_radar_operation();
	/// @brief 將符合呼叫端條件的待執行動作保留為空操作。
	template <typename Predicate> void cancel_pending(Predicate predicate)
	{
		for (auto& request : pending_)
			if (request.kind == ExternalRequestKind::Action && predicate(request.action))
				request.action = {};
	}
	/// @brief 查詢尚待執行的指定語意動作。
	bool contains(MissionActionKind action) const noexcept;
	void acknowledge(std::uint64_t sequence);
	bool acknowledged(std::uint64_t sequence) const noexcept;
	ExternalActionRequests pending();
	/// @brief 最近提出的功能要求編號。
	std::uint64_t latest_sequence() const noexcept
	{
		return last_sequence_;
	}

private:
	std::uint64_t append_request(ExternalActionRequest request);
	std::deque<ExternalActionRequest> pending_;
	std::uint64_t acknowledged_through_ = 0;
	std::uint64_t last_sequence_ = 0;
	std::uint64_t published_through_ = 0;
};
}
