#pragma once

namespace DcsBridge::Internal
{
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

struct NativeGunResult
{
	NativeGunStatus status = NativeGunStatus::WaitingForModule;
	unsigned count = 0;
	unsigned mounts = 0;
};

}
