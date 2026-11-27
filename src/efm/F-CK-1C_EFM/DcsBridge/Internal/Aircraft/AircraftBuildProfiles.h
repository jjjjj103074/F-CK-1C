#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace DcsBridge::Internal
{
/// @brief 保存機砲存量存取所用的建置識別與程式指紋。
struct NativeGunProfile
{
	std::uint32_t timestamp;
	std::uint32_t image_size;
	std::uint32_t fingerprint_rva;
	std::uint64_t fingerprint;
};
inline constexpr std::size_t kNativeGunFingerprintSize = 49;

/// @brief 定義原生函式的匯出名稱與指紋範圍。
struct AircraftFunctionSignature
{
	const char* symbol;
	std::size_t byte_count;
};
inline constexpr std::array<AircraftFunctionSignature, 5> kCountermeasureFunctions{ {
	{ "?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ", 29 },
	{ "?l_drop_chaff@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 202 },
	{ "?l_drop_flare@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 202 },
	{ "?l_get_chaff_count@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 199 },
	{ "?l_get_flare_count@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 199 },
} };

inline constexpr std::array<AircraftFunctionSignature, 9> kIrSeekerFunctions{ {
	{ "?get_wide_device@ccCockpitContext@cockpit@@QEAAPEAVavDevice@2@I@Z", 32 },
	{ "?update_ir_seeker@avSimpleWeaponSystem@cockpit@@AEAAXXZ", 32 },
	{ "??0avSidewinderSeeker@cockpit@@QEAA@XZ", 64 },
	{ "?setConsentToTrack@avSidewinderSeeker@cockpit@@QEAAXM@Z", 26 },
	{ "?getStatusID@avIRSensor@cockpit@@QEBAIXZ", 7 },
	{ "?cage@avSidewinderSeeker@cockpit@@MEAAXXZ", 5 },
	{ "?uncage@avIRSensor@cockpit@@IEAAXXZ", 34 },
	{ "?slaveToDirection@avSidewinderSeeker@cockpit@@UEAAXAEBVVec3d@osg@@@Z", 48 },
	{ "?toSlave@avSidewinderSeeker@cockpit@@IEAAXXZ", 32 },
} };

/// @brief 集中保存經 DLL 核對的函式指紋，供各原生介面選取。
struct AircraftBuildProfile
{
	NativeGunProfile gun;
	std::array<std::uint64_t, 5> countermeasure_hashes;
	std::array<std::uint64_t, 9> infrared_hashes;
};
inline constexpr AircraftBuildProfile kAircraftBuildProfiles[] = {
	{ { 0x6adbad09, 0x68e000, 0x2fe650, 0xdbd7d715791c239fULL },
	  { { 0xe4153ccf2d04e7d6ULL,
	      0x46228e87c2536c9fULL,
	      0x18198bf9ab4a388aULL,
	      0x872314f529cfb306ULL,
	      0xf4663cb30bfdfa3ULL } },
	  { { 0x81cc11171e1ba916ULL,
	      0x98f33ac9cf396e65ULL,
	      0x5d5008a754d68227ULL,
	      0xdf74a7ec2826fc8ULL,
	      0xec4c08ae667a8ea4ULL,
	      0x7503bb3a0ff66589ULL,
	      0xc117d6212af01575ULL,
	      0x447f2c8a1a851472ULL,
	      0x5a587ffbf47695b9ULL } } },
	{ { 0x6afec04f, 0x68e000, 0x2fe400, 0x51418c51962d3af8ULL },
	  { { 0x46c1a7daa0c3383fULL,
	      0x5166f4e210af3452ULL,
	      0x83981d0b957ef8abULL,
	      0x2e3707e3ac6b86c4ULL,
	      0x45d000871e765ccdULL } },
	  { { 0x41d2d25f18107729ULL,
	      0x98f33ac9cf396e65ULL,
	      0x1a2079ded3a1ce88ULL,
	      0xdf74a7ec2826fc8ULL,
	      0xec4c08ae667a8ea4ULL,
	      0x7503bb3a0ff66589ULL,
	      0xc117d6212af01575ULL,
	      0x181082bbc1acedb8ULL,
	      0x6b730569969a099eULL } } },
};

/// @brief 依 PE 建置識別取得已核對的相容資料。
inline const AircraftBuildProfile* aircraft_build_profile(std::uint32_t timestamp, std::uint32_t image_size) noexcept
{
	for (const auto& profile : kAircraftBuildProfiles)
		if (profile.gun.timestamp == timestamp && profile.gun.image_size == image_size)
			return &profile;
	return nullptr;
}
}
