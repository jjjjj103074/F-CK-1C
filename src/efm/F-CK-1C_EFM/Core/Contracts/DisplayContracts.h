#pragma once

#include <array>
#include <cstddef>

namespace Core
{
/// @brief 顯示位置對應的任務功能。
enum class DisplayFunction
{
	None,
	FireControlRadar,
	StoresManagement,
	HorizontalSituation
};

/// @brief 接受共用目標操作的功能來源。
enum class SensorControlSource
{
	None,
	Radar,
	Hud,
	Navigation
};

enum class DisplayLocation
{
	None,
	Hud,
	LeftMfd,
	RightMfd
};

/// @brief 一個 MFD 位置的功能與可供選擇的控制來源。
struct DisplayAssignment
{
	DisplayFunction function = DisplayFunction::None;
	SensorControlSource control_source = SensorControlSource::None;
};

/// @brief MC 選定的控制來源及其顯示位置。
struct SensorOfInterestSnapshot
{
	SensorControlSource source = SensorControlSource::None;
	DisplayLocation location = DisplayLocation::None;
	bool display_context_ready = false;
};
inline constexpr std::size_t kRadarDisplayPlotCount = 32;

enum class RadarCueKind
{
	Hidden = 0,
	BoresightReady = 1,
	BoresightAcquiring = 2,
	SingleTargetTrack = 3
};

/// @brief 雷達圖上的回波座標、淡出程度與原生來源編號。
struct RadarDisplayPlot
{
	double x_normalized = 0.0;
	double y_normalized = 0.0;
	double opacity_0_1 = 0.0;
	unsigned source_slot = 0;
};

/// @brief 前視準直框及方位—距離雷達圖的圖形資料。
struct RadarDisplaySnapshot
{
	bool visible = false;
	bool sensor_available = false;
	RadarCueKind cue = RadarCueKind::Hidden;
	bool cue_visible = false;
	double bore_half_width_mrad = 0.0;
	double bore_half_height_mrad = 0.0;
	double target_x_mrad = 0.0;
	double target_y_mrad = 0.0;
	double range_scale_m = 0.0;
	double scan_left_normalized = -1.0;
	double scan_right_normalized = 1.0;
	bool estimated_sweep_visible = false;
	double estimated_sweep_x_normalized = 0.0;
	bool cursor_visible = false;
	double cursor_x_normalized = 0.0;
	double cursor_y_normalized = 0.0;
	bool target_visible = false;
	double target_x_normalized = 0.0;
	double target_y_normalized = 0.0;
	std::array<RadarDisplayPlot, kRadarDisplayPlotCount> plots;
};

}
