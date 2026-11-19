#pragma once
#include <cstdint>
#include <optional>
#include <vector>
namespace Core
{
enum class CountermeasureProgram
{
	Forward,
	Program6
};
enum class CountermeasureProgramState
{
	Idle,
	Running,
	Completed,
	Failed
};
/// @brief 駕駛要求的反制程序，序號保存跨更新週期的先後順序。
struct CountermeasureProgramRequest
{
	std::uint64_t sequence = 0;
	CountermeasureProgram program = CountermeasureProgram::Forward;
};
struct CountermeasureRequests
{
	std::vector<CountermeasureProgramRequest> items;
};
/// @brief 原生飛機介面觀測的反制庫存。
struct CountermeasureInventory
{
	bool available = false;
	int flare = -1, chaff = -1;
};
/// @brief 一輪施放的種類；每個選取的種類施放一發。
struct CountermeasureDispenseRequest
{
	std::uint64_t sequence = 0;
	bool flare = false, chaff = false;
};
/// @brief Bridge 已處理的一輪要求及原生呼叫結果，庫存另由即時觀測回報。
struct CountermeasureExecutionFeedback
{
	std::uint64_t sequence = 0;
	bool flare_returned = false, chaff_returned = false;
	double completed_at_s = 0.0;
};
struct CountermeasureObservation
{
	CountermeasureInventory inventory;
	CountermeasureExecutionFeedback execution;
};
}
