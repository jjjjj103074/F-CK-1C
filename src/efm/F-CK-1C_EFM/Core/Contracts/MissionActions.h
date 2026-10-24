#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Core
{
// Stable semantic wire values shared with the Lua DCS adapter. These are not DCS command IDs.
enum class MissionActionKind
{
	None = 0,
	GunOn = 1,
	GunOff = 2,
	LaunchPermissionOverride = 3,
	PickleOn = 4,
	PickleOff = 5,
	EnsureRadarOn = 6,
	CycleWeapon = 7,
	ModeHelmet = 8,
	ModeVerticalScan = 9,
	ModeBeyondVisualRange = 10,
	ModeFi0 = 11,
	ModeCannon = 12,
	LockStart = 13,
	ChangeLock = 14,
	LockFinish = 15,
	ChangeLockUp = 16,
	StartCmsForward = 17,
	StartCmsAft = 18,
	StartCmsLeft = 19,
	AbortCms = 20
};

inline constexpr std::size_t kMissionActionWindowSize = 16;
inline constexpr std::uint64_t kMaximumExactCockpitInteger = 9007199254740991ULL;

struct MissionActionBatch
{
	// Consecutive sequence numbers: first_sequence + [0, count). Pending actions remain until acknowledged.
	std::uint64_t first_sequence = 0;
	std::size_t count = 0;
	std::array<MissionActionKind, kMissionActionWindowSize> actions = {};
};

struct MissionAdapterObservation
{
	bool available = false;
	bool radar_power_on = false;
	std::uint64_t acknowledged_sequence = 0;
	bool missile_count_available = false;
	int missile_count = 0;
	bool gun_count_available = false;
	double gun_count = 0.0;
};
}
