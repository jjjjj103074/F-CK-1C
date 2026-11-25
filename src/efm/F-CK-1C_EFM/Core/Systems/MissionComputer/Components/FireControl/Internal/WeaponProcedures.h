#pragma once
#include "../../../Database/MissionData.h"

namespace Core::Systems::Mc::Internal
{
/// @brief 依共用程序、目標觀測及 UNCAGE 按鈕判斷本次發射意圖。
WeaponReleaseState evaluate_weapon_release(const ModeData& mode,
                                           const SelectedWeaponState& selected,
                                           const RadarSnapshot& radar,
                                           bool uncage_button_held);
bool permits_release(WeaponReleaseState state) noexcept;
}
