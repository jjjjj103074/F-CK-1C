#pragma once

#include "../../../Database/MissionData.h"

namespace Core::Systems::Mc::Internal
{
/// @brief FC 解讀完成的目標指定要求。
enum class TargetDesignationRequest
{
	Designate,
	SelectNext,
	Cancel
};

/// @brief 保存機載目標指定意圖，整理雷達回報的追蹤結果。
class TargetDesignation final
{
public:
	explicit TargetDesignation(TargetDesignationData& data);
	/// @brief 接受指定、切換或取消要求，更新指定意圖。
	void request(TargetDesignationRequest request);
	/// @brief 更新雷達觀測有效性、追蹤狀態與目標幾何資料。
	void observe(const RadarObservation& radar);

private:
	TargetDesignationData& data_;
};
}
