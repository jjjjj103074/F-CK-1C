#pragma once

#include "../../../Contracts/RadarCommands.h"
#include "../../../../Common/Units.h"

namespace Core::Systems::RadarProfiles
{
// 距離與 STT 限制沿用 FCK1C_Radar.lua 的既有感測器校準。
inline constexpr double kDetectionModelRangeM = 90000.0;
// BORE 角窗參考 Lockheed Martin 16PR14341（1998 MLU），印刷頁 134–135。
inline constexpr double kBoresightWidthRad = Common::rad(3.3);
inline constexpr double kBoresightHeightRad = Common::rad(4.6);
// 捕獲上限採手冊 ACM 10 NM 量程作本輪工程設定。
inline constexpr double kBoresightAcquisitionRangeM = 10.0 * 1852.0;

/// @brief 建立 RWS 寬區搜尋、穩定姿態與游標捕獲設定。
inline RadarScanControl range_while_search()
{
	RadarScanControl result;
	result.enabled = true;
	// ±60°、四條帶參考早期 F-16 APG-66；條帶高度依原生介面作工程映射。
	result.azimuth_width_rad = Common::rad(120.0);
	result.elevation_height_rad = 4.0 * kBoresightHeightRad;
	result.maximum_search_range_m = kDetectionModelRangeM;
	result.display_range_m = 40.0 * 1852.0;
	result.acquisition_azimuth_width_rad = kBoresightWidthRad;
	result.acquisition_range_window_m = 6000.0;
	return result;
}

/// @brief 建立沿機體參考視線的 BORE 近距離捕獲設定。
inline RadarScanControl boresight()
{
	RadarScanControl result;
	result.enabled = true;
	result.azimuth_width_rad = kBoresightWidthRad;
	result.elevation_height_rad = kBoresightHeightRad;
	result.maximum_search_range_m = kBoresightAcquisitionRangeM;
	result.display_range_m = kBoresightAcquisitionRangeM;
	result.attitude_stabilized = false;
	result.override_tdc = true;
	result.tdc_range_m = kBoresightAcquisitionRangeM / 2.0;
	result.acquisition_azimuth_width_rad = kBoresightWidthRad;
	// 完整距離門涵蓋 BORE 捕獲區域；搜尋距離上限負責裁切。
	result.acquisition_range_window_m = kBoresightAcquisitionRangeM * 2.0;
	return result;
}
/// @brief 建立 F-16 參考的 HUD 寬區與向上垂直 ACM 掃描窗口。
inline RadarScanControl air_combat(TargetAcquisitionMode mode)
{
	auto result = boresight();
	if (mode == TargetAcquisitionMode::WideScan)
	{
		result.azimuth_width_rad = Common::rad(30.0);
		result.elevation_height_rad = Common::rad(20.0);
	}
	else if (mode == TargetAcquisitionMode::VerticalScan)
	{
		result.azimuth_width_rad = Common::rad(10.0);
		result.elevation_height_rad = Common::rad(60.0);
		result.elevation_center_rad = Common::rad(30.0);
	}
	result.acquisition_azimuth_width_rad = result.azimuth_width_rad;
	return result;
}

}
