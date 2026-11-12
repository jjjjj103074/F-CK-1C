#pragma once
#include "../../../Database/MissionData.h"
#include "../../../Database/RadarRequestQueue.h"
#include "../../../ModeEvents.h"
#include "../../../../../Contracts/Commands.h"
namespace Core::Systems::Mc::Internal
{
/// @brief 規劃目標取得操作，保存指定意圖並整理雷達追蹤回報。
class Targeting final
{
public:
	Targeting(const ModeData& mode, const RadarSnapshot& radar, TargetingData& data, RadarRequestQueue& requests);
	/// @brief 將 TMS 與解籠操作轉成目標取得要求。
	void handle(const Command& command);
	/// @brief 套用 NAV／DGFT／MSL 的既有目標操作策略。
	void on_operating_mode_selected(const OperatingModeSelected& event);
	/// @brief 投放操作前重送目前捕獲方式。
	void before_weapon_release();
	/// @brief 正常投放輸入結束後取消指定。
	void on_weapon_release_ended();
	/// @brief 接收設備要求結果，整理指定意圖與雷達追蹤觀測。
	void step();

private:
	void select_mode(TargetAcquisitionMode mode);
	void reapply_mode();
	void acquire(AcquisitionInput input);
	void cancel();
	const ModeData& mode_;
	const RadarSnapshot& radar_;
	TargetingData& data_;
	RadarRequestQueue& requests_;
	std::uint64_t latest_designation_request_ = 0;
};
}
