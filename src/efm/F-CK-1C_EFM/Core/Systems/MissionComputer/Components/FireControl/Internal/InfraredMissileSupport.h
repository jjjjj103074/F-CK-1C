#pragma once

#include "../../../Database/InputOutputData.h"
#include "../../../Database/MissionData.h"
#include "../../../Database/StoresData.h"

#include <chrono>

namespace Core::Systems::Mc::Internal
{
/// @brief 整理飛彈尋標觀測、準備狀態與音調要求，沿用既有 AIM-9 推導。
class InfraredMissileSupport final
{
public:
	InfraredMissileSupport(const MissionInputs& input,
	                       const ModeData& mode,
	                       const StoresData& stores,
	                       const TargetDesignationData& designation,
	                       const WeaponOperationData& weapon,
	                       InfraredMissileData& missile);
	void step(std::chrono::nanoseconds time, double dt_s);

private:
	void resolve_tracking();
	bool telemetry_active() const;
	const MissionInputs& input_;
	const ModeData& mode_;
	const StoresData& stores_;
	const TargetDesignationData& designation_;
	const WeaponOperationData& weapon_;
	InfraredMissileData& missile_;
};
}
