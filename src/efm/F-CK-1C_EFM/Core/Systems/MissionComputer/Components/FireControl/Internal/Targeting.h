#pragma once

#include "../../../Database/MissionData.h"
#include "../../../Database/RadarRequestQueue.h"
#include "../../../ModeEvents.h"
#include "../../../../../Contracts/Commands.h"

#include <chrono>

namespace Core::Systems::Mc::Internal
{
/// @brief 依雷達情境解讀目標操作，保存指定意圖並整理追蹤回報。
class Targeting final
{
public:
	Targeting(const ModeData& mode, const RadarSnapshot& radar, TargetingData& data, RadarRequestQueue& requests);
	/// @brief 接收已分派的 TMS 與解籠操作，依觀測時間處理按住及放開。
	void handle(const Command& command, std::chrono::nanoseconds input_time = {});
	/// @brief 依解籠選擇接續既有的雷達設定與狗鬥取消要求。
	void on_missile_uncage_selected(bool selected);
	void on_operating_mode_selected(const OperatingModeSelected& event);
	void before_weapon_release();
	void on_weapon_release_ended();
	/// @brief 推進 TMS 長按、接收設備要求結果並整理追蹤觀測。
	void step(std::chrono::nanoseconds time = {});

private:
	void select_mode(TargetAcquisitionMode mode);
	void reapply_mode();
	void acquire(AcquisitionInput input);
	void cancel();
	void submit_tms(const RadarCommand& command);
	void handle_tms_up(bool pressed);
	void handle_tms_down();
	void handle_tms_right(bool pressed, std::chrono::nanoseconds time);
	void apply_search_toggle();
	bool tracking() const;

	const ModeData& mode_;
	const RadarSnapshot& radar_;
	TargetingData& data_;
	RadarRequestQueue& requests_;
	std::uint64_t latest_designation_request_ = 0;
	bool acm_radiating_requested_ = false;
	bool tms_up_held_ = false;
	bool tms_right_held_ = false;
	bool tms_right_long_ = false;
	std::chrono::nanoseconds tms_right_pressed_at_ = {};
};
}
