#pragma once

#include "GunAmmunitionTypes.h"
#include "../Aircraft/AircraftBuildProfiles.h"
#include <cstddef>
#include <cstdint>

namespace DcsBridge::Internal
{
/// @brief 記錄原生程式庫的版本指紋與 payload 入口。
struct NativeGunHost
{
	std::uint32_t timestamp = 0;
	std::uint32_t image_size = 0;
	std::uint64_t fingerprint = 0;
	std::uintptr_t payload_function = 0;
};

const NativeGunProfile* native_gun_profile(std::uint32_t timestamp, std::uint32_t image_size) noexcept;

/// @brief 將記憶體讀取與三種原生呼叫交給執行環境。
struct NativeGunAccess
{
	void* context;
	bool (*read)(void*, std::uintptr_t, void*, std::size_t);
	bool (*payload)(void*, std::uintptr_t, std::uintptr_t&);
	bool (*mount)(void*, std::uintptr_t, std::uintptr_t, unsigned char, std::uintptr_t&);
	bool (*count)(void*, std::uintptr_t, std::uintptr_t, unsigned&);
};

/// @brief 核對版本後讀取原生機砲存量，回傳讀值或具體診斷狀態。
NativeGunResult read_native_gun(const NativeGunHost& host, const NativeGunAccess& access);
}
