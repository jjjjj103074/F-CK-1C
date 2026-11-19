#pragma once
#include "MissionActions.h"
#include <vector>
namespace Core
{
enum class ExternalRequestKind
{
	Action,
	RadarOperation
};
/// @brief 生產端要求編號；雷達佔位保存與其他 MC 操作的先後關係。
struct ExternalActionRequest
{
	std::uint64_t sequence = 0;
	ExternalRequestKind kind = ExternalRequestKind::Action;
	MissionAction action;
	std::uint64_t parent_request = 0; ///< RadarSystem 產生動作時對應的 MC 雷達要求。
};
/// @brief 生產端保存的未完成要求，隨 Pipeline 快照完整發布。
struct ExternalActionRequests
{
	std::uint64_t acknowledged_through = 0;
	std::vector<ExternalActionRequest> items;
};
/// @brief DcsBridge 將共用傳送確認轉回各生產端的要求編號。
struct ExternalActionFeedback
{
	std::uint64_t mission_acknowledged_through = 0;
	std::uint64_t radar_acknowledged_through = 0;
};
}
