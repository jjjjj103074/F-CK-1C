#pragma once

#include <memory>
#include <string>

namespace Core
{
enum class WeaponCategory
{
	Other,
	Missile,
	Bomb,
	FuelTank,
	Pod,
	Rack,
	Unknown
};
enum class WeaponRole
{
	Other,
	AirToAir,
	AirToGround,
	Support,
	Unknown
};
enum class WeaponGuidance
{
	None,
	Infrared,
	SemiActiveRadar,
	ActiveRadar,
	Laser,
	Satellite,
	Unknown
};
enum class WeaponProcedure
{
	Unsupported,
	Sidewinder,
	Sparrow,
	Amraam
};

/// @brief 共用武器清單解析後的身分、分類及本機操作支援。
struct WeaponDefinition
{
	std::string id, name, family, selection_group;
	WeaponCategory category = WeaponCategory::Unknown;
	WeaponRole role = WeaponRole::Unknown;
	WeaponGuidance guidance = WeaponGuidance::Unknown;
	WeaponProcedure procedure = WeaponProcedure::Unsupported;
	bool native_ir = false;
	bool radar_cue = false;
};

/// @brief 快照共同持有的唯讀武器定義。
using WeaponDefinitionRef = std::shared_ptr<const WeaponDefinition>;

/// @brief SM 依有效觀測與選站意圖發布的使用對象；pending 保存執行確認狀態。
struct SelectedWeaponState
{
	bool available = false;
	bool pending = false;
	int station = -1;
	WeaponDefinitionRef weapon;
};

/// @brief FC 的使用程序判定，供操作及顯示共用。
enum class WeaponReleaseState
{
	NoWeapon,
	Inhibited,
	Unsupported,
	Infrared,
	Targeted,
	VisualConsentRequired,
	Visual,
	SparrowTrackRequired,
	SelectionPending,
	ObservationUnavailable
};
}
