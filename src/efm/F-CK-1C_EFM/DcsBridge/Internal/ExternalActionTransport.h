#pragma once
#include "../../Core/Contracts/CockpitContracts.h"
#include <deque>
namespace DcsBridge::Internal
{
/// @brief 合併各生產端操作，保存共用 DCS 序號、傳送視窗與執行確認。
class ExternalActionTransport final
{
public:
	ExternalActionTransport() = default;
	ExternalActionTransport(const ExternalActionTransport&) = delete;
	ExternalActionTransport& operator=(const ExternalActionTransport&) = delete;
	ExternalActionTransport(ExternalActionTransport&&) = delete;
	ExternalActionTransport& operator=(ExternalActionTransport&&) = delete;
	void reset();
	/// @brief 將已發布的成功前綴對應回各生產端要求。
	void acknowledge(std::uint64_t sequence);
	/// @brief 依 MC 來源順序展開設備結果，接續雷達週期操作。
	void collect(const Core::ExternalActionRequests& mission, const Core::RadarSnapshot& radar);
	/// @brief 提供下一個連續 16 項 DCS 動作視窗。
	Core::MissionActionBatch batch();
	Core::ExternalActionFeedback feedback() const noexcept;

private:
	struct Delivery
	{
		std::uint64_t wire_sequence;
		Core::MissionAction action;
		std::uint64_t mission_source = 0;
		std::uint64_t mission_complete = 0;
		std::uint64_t radar_source = 0;
	};
	void append(Core::MissionAction action,
	            std::uint64_t mission_source,
	            std::uint64_t mission_complete,
	            std::uint64_t radar_source);
	const Core::ExternalActionRequest* next_radar_action(const Core::RadarSnapshot& radar) const;
	void collect_background(const Core::RadarSnapshot& radar);
	bool collect_radar_result(std::uint64_t request, const Core::RadarSnapshot& radar);
	std::deque<Delivery> pending_;
	Core::ExternalActionFeedback feedback_;
	std::uint64_t last_wire_sequence_ = 0;
	std::uint64_t acknowledged_wire_ = 0;
	std::uint64_t published_through_ = 0;
	std::uint64_t collected_mission_ = 0;
	std::uint64_t collected_radar_ = 0;
};
}
