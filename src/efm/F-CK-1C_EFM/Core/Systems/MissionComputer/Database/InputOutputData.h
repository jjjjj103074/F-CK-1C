#pragma once

#include "../../../Contracts/CockpitContracts.h"
#include "../../../Contracts/Commands.h"

#include <chrono>
#include <deque>

namespace Core::Systems::Mc
{
enum class MissionCommandTarget
{
	Mode,
	Sensor,
	Weapon
};

struct PendingMissionCommand
{
	MissionCommandTarget target = MissionCommandTarget::Mode;
	Command command;
};

/// @brief MC 觀測與有序命令；外殼寫入觀測、保存命令並在 tick 依序消費。
struct MissionInputs
{
	CockpitObservation cockpit;
	std::chrono::nanoseconds observed_at = {};
	std::deque<PendingMissionCommand> pending_commands;
};

/// @brief MC 根據各元件資料分區組裝的對外快照。
struct MissionOutputs
{
	CombatAvionicsSnapshot combat_avionics;
};

/// @brief 由 MC 持有的輸入輸出分區；各功能狀態由對應元件持有寫入責任。
struct InputOutputData
{
	MissionInputs input;
	MissionOutputs output;
};
}
