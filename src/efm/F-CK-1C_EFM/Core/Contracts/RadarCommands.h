#pragma once
#include <cstdint>
#include <vector>
namespace Core
{
/// @brief 目標取得策略選擇的捕獲操作方式。
enum class TargetAcquisitionMode
{
	None = 0,
	Helmet = 1,
	VerticalScan = 2,
	Hud = 3,
	BeyondVisualRange = 4,
	Boresight = 5,
	WideScan = 6
};
/// @brief 火控要求的雷達工作方式；實際追蹤結果由原生觀測回報。
enum class RadarSearchMode
{
	RangeWhileSearch,
	TrackWhileScan,
	AirCombat,
	GroundMap
};

enum class RadarOperation
{
	EnsureOn,
	SetAcquisitionMode,
	BeginAcquisition,
	ReleaseAcquisition,
	CancelAcquisition,
	SelectNextTarget,
	SelectNavigation,
	BeginSpotlight,
	EndSpotlight,
	SetSearchMode,
	ExpandTargetData,
	StepPriorityTarget,
	CycleSightingPoint,
	DesignateGroundTarget,
	RejectGroundTarget
};
enum class AcquisitionInput
{
	Pulse,
	Hold
};
/// @brief Targeting 提出的設備操作；模式重送由明確的 SetAcquisitionMode 要求表達。
struct RadarCommand
{
	RadarOperation operation = RadarOperation::EnsureOn;
	TargetAcquisitionMode mode = TargetAcquisitionMode::None;
	AcquisitionInput input = AcquisitionInput::Pulse;
	RadarSearchMode search_mode = RadarSearchMode::RangeWhileSearch;
};
struct RadarRequest
{
	std::uint64_t sequence = 0;
	RadarCommand command;
};
/// @brief 由 MC 發布、由 RadarSystem 依序消費的未完成雷達要求。
struct RadarRequests
{
	std::uint64_t acknowledged_through = 0;
	std::vector<RadarRequest> items;
};
/// @brief 設備要求的實作方式，供操作回報區分原生、簡化與預留功能。
enum class RadarImplementation
{
	Native,
	Simplified,
	Reserved
};

/// @brief 一筆雷達要求的接受結果及對應設備動作範圍。
struct RadarCommandResult
{
	std::uint64_t request_sequence = 0;
	bool accepted = true;
	std::uint64_t first_action_sequence = 0;
	std::uint64_t last_action_sequence = 0;
	RadarOperation operation = RadarOperation::EnsureOn;
	RadarImplementation implementation = RadarImplementation::Native;
};
}
