#pragma once

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

struct NativeGunProfile
{
	std::uint32_t timestamp;
	std::uint32_t image_size;
	std::uint32_t fingerprint_rva;
	std::uint64_t fingerprint;
};

/// @brief 以公開互通研究記錄的 PE 與程式指紋選擇物件配置。
inline constexpr NativeGunProfile kNativeGunProfiles[] = { { 0x6adbad09, 0x68e000, 0x2fe650, 0xdbd7d715791c239fULL } };
inline constexpr std::size_t kNativeGunFingerprintSize = 49;

const NativeGunProfile* native_gun_profile(std::uint32_t timestamp, std::uint32_t image_size) noexcept;
std::uint64_t native_gun_fingerprint(const unsigned char* bytes, std::size_t size) noexcept;

enum class NativeGunStatus
{
	WaitingForModule,
	UnsupportedBuild,
	MissingPayloadExport,
	WaitingForPayload,
	UnreadableObject,
	CallFailed,
	NoGunMount,
	InvalidCount,
	TooManyMounts,
	Available
};

const char* native_gun_status_name(NativeGunStatus status) noexcept;

/// @brief 將記憶體讀取與三種原生呼叫交給執行環境。
struct NativeGunAccess
{
	void* context;
	bool (*read)(void*, std::uintptr_t, void*, std::size_t);
	bool (*payload)(void*, std::uintptr_t, std::uintptr_t&);
	bool (*mount)(void*, std::uintptr_t, std::uintptr_t, unsigned char, std::uintptr_t&);
	bool (*count)(void*, std::uintptr_t, std::uintptr_t, unsigned&);
};

struct NativeGunResult
{
	NativeGunStatus status = NativeGunStatus::WaitingForModule;
	unsigned count = 0;
	unsigned mounts = 0;
};

/// @brief 核對版本後讀取原生機砲存量，回傳讀值或具體診斷狀態。
NativeGunResult read_native_gun(const NativeGunHost& host, const NativeGunAccess& access);
}
