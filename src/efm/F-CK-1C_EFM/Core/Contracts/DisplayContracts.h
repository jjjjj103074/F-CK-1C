#pragma once

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
}
