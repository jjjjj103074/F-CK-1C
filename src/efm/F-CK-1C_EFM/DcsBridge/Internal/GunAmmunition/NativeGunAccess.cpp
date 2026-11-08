#include "NativeGunAccess.h"

#include <limits>

namespace
{
constexpr std::size_t kPayloadGunMountMethodOffset = 560;
constexpr std::size_t kMountGunPointerOffset = 8;
constexpr std::size_t kGunCountMethodOffset = 88;
}

namespace DcsBridge::Internal
{
const NativeGunProfile* native_gun_profile(std::uint32_t timestamp, std::uint32_t image_size) noexcept
{
	for (const auto& profile : kNativeGunProfiles)
		if (profile.timestamp == timestamp && profile.image_size == image_size)
			return &profile;
	return nullptr;
}

std::uint64_t native_gun_fingerprint(const unsigned char* bytes, std::size_t size) noexcept
{
	if (bytes == nullptr || size != kNativeGunFingerprintSize)
		return 0;
	std::uint64_t hash = 14695981039346656037ULL;
	for (std::size_t i = 0; i < size; ++i)
		hash = (hash ^ bytes[i]) * 1099511628211ULL;
	return hash;
}

const char* native_gun_status_name(NativeGunStatus status) noexcept
{
	switch (status)
	{
	case NativeGunStatus::WaitingForModule:
		return "waiting_for_module";
	case NativeGunStatus::UnsupportedBuild:
		return "unsupported_build";
	case NativeGunStatus::MissingPayloadExport:
		return "missing_payload_export";
	case NativeGunStatus::WaitingForPayload:
		return "waiting_for_payload";
	case NativeGunStatus::UnreadableObject:
		return "unreadable_object";
	case NativeGunStatus::CallFailed:
		return "native_call_failed";
	case NativeGunStatus::NoGunMount:
		return "no_gun_mount";
	case NativeGunStatus::InvalidCount:
		return "invalid_count";
	case NativeGunStatus::TooManyMounts:
		return "too_many_mounts";
	case NativeGunStatus::Available:
		return "available";
	}
	return "unknown";
}

NativeGunResult read_native_gun(const NativeGunHost& host, const NativeGunAccess& access)
{
	NativeGunResult result;
	const auto* profile = native_gun_profile(host.timestamp, host.image_size);
	if (profile == nullptr || host.fingerprint != profile->fingerprint)
	{
		result.status = NativeGunStatus::UnsupportedBuild;
		return result;
	}
	if (host.payload_function == 0)
	{
		result.status = NativeGunStatus::MissingPayloadExport;
		return result;
	}
	std::uintptr_t payload = 0;
	if (!access.payload(access.context, host.payload_function, payload))
	{
		result.status = NativeGunStatus::CallFailed;
		return result;
	}
	if (payload == 0)
	{
		result.status = NativeGunStatus::WaitingForPayload;
		return result;
	}
	// 配置依據公開 payload／gun 互通研究；所有位址經讀取介面核對。
	const auto read_pointer = [&](std::uintptr_t object, std::size_t offset, std::uintptr_t& value)
	{
		return object != 0 && object <= (std::numeric_limits<std::uintptr_t>::max)() - offset &&
		       access.read(access.context, object + offset, &value, sizeof(value)) && value != 0;
	};
	std::uintptr_t payload_table = 0, mount_function = 0;
	if (!read_pointer(payload, 0, payload_table) ||
	    !read_pointer(payload_table, kPayloadGunMountMethodOffset, mount_function))
	{
		result.status = NativeGunStatus::UnreadableObject;
		return result;
	}
	for (unsigned char index = 0; index < 32; ++index)
	{
		std::uintptr_t mount = 0;
		if (!access.mount(access.context, mount_function, payload, index, mount))
		{
			result.status = NativeGunStatus::CallFailed;
			return result;
		}
		if (mount == 0)
		{
			result.status = index == 0 ? NativeGunStatus::NoGunMount : NativeGunStatus::Available;
			return result;
		}
		std::uintptr_t gun = 0, gun_table = 0, count_function = 0;
		if (!read_pointer(mount, kMountGunPointerOffset, gun) || !read_pointer(gun, 0, gun_table) ||
		    !read_pointer(gun_table, kGunCountMethodOffset, count_function))
		{
			result.status = NativeGunStatus::UnreadableObject;
			return result;
		}
		unsigned count = 0;
		if (!access.count(access.context, count_function, gun, count))
		{
			result.status = NativeGunStatus::CallFailed;
			return result;
		}
		if (count > 9999 || result.count > 9999 - count)
		{
			result.status = NativeGunStatus::InvalidCount;
			return result;
		}
		result.count += count;
		++result.mounts;
	}
	result.status = NativeGunStatus::TooManyMounts;
	return result;
}
}
