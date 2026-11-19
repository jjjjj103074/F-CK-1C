#include "TestHarness.h"
#include "DcsBridge/Internal/GunAmmunition/NativeGunAccess.h"

#include <cstring>
#include <map>

namespace
{
using namespace DcsBridge::Internal;

struct Fixture
{
	NativeGunHost host{ kNativeGunProfiles[0].timestamp,
		                kNativeGunProfiles[0].image_size,
		                kNativeGunProfiles[0].fingerprint,
		                501 };
	std::map<std::uintptr_t, std::uintptr_t> memory{ { 0x1000, 0x4000 }, { 0x4000 + 560, 502 }, { 0x2000 + 8, 0x3000 },
		                                             { 0x3000, 0x5000 }, { 0x5000 + 88, 503 },  { 0x2100 + 8, 0x3100 },
		                                             { 0x3100, 0x5000 } };
	unsigned mounts = 1;
	unsigned count = 523;
	unsigned calls = 0;
	bool payload_present = true;
	bool count_failure = false;
	bool repeat_mount = false;

	NativeGunAccess access()
	{
		return { this,
			     [](void* object, std::uintptr_t address, void* destination, std::size_t size)
			     {
			         auto& f = *static_cast<Fixture*>(object);
			         const auto found = f.memory.find(address);
			         if (found == f.memory.end() || size != sizeof(std::uintptr_t))
				         return false;
			         std::memcpy(destination, &found->second, size);
			         return true;
			     },
			     [](void* object, std::uintptr_t function, std::uintptr_t& value)
			     {
			         auto& f = *static_cast<Fixture*>(object);
			         ++f.calls;
			         if (function != 501)
				         return false;
			         value = f.payload_present ? 0x1000 : 0;
			         return true;
			     },
			     [](void* object,
			        std::uintptr_t function,
			        std::uintptr_t payload,
			        unsigned char index,
			        std::uintptr_t& value)
			     {
			         auto& f = *static_cast<Fixture*>(object);
			         ++f.calls;
			         if (function != 502 || payload != 0x1000)
				         return false;
			         value = index < f.mounts ? 0x2000 + (f.repeat_mount ? 0 : index * 0x100) : 0;
			         return true;
			     },
			     [](void* object, std::uintptr_t function, std::uintptr_t gun, unsigned& value)
			     {
			         auto& f = *static_cast<Fixture*>(object);
			         ++f.calls;
			         if (f.count_failure || function != 503 || (gun != 0x3000 && gun != 0x3100))
				         return false;
			         value = f.count;
			         return true;
			     } };
	}

	NativeGunResult read()
	{
		return read_native_gun(host, access());
	}
};
}

void run_native_gun_access_tests(Tests::Context& context)
{
	Fixture f;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::Available);
	TEST_EXPECT(context, f.read().count == 523);
	for (unsigned count : { 261u, 216u, 0u, 523u })
	{
		f.count = count;
		const auto result = f.read();
		TEST_EXPECT(context, result.status == NativeGunStatus::Available);
		TEST_EXPECT(context, result.count == count && result.mounts == 1);
	}
	f.mounts = 0;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::NoGunMount);
	f.mounts = 2;
	f.count = 100;
	TEST_EXPECT(context, f.read().count == 200);
	f.count = 5000;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::InvalidCount);
	f.mounts = 1;
	f.count = 10000;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::InvalidCount);
	f.count = 9999;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::Available);
	f.count_failure = true;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::CallFailed);
	f.count_failure = false;
	f.payload_present = false;
	TEST_EXPECT(context, f.read().status == NativeGunStatus::WaitingForPayload);
	f.payload_present = true;
	f.memory.erase(0x5000 + 88);
	TEST_EXPECT(context, f.read().status == NativeGunStatus::UnreadableObject);
	Fixture bounded;
	bounded.repeat_mount = true;
	bounded.mounts = 32;
	bounded.count = 0;
	TEST_EXPECT(context, bounded.read().status == NativeGunStatus::TooManyMounts);
	Fixture rejected;
	rejected.host.timestamp ^= 1;
	TEST_EXPECT(context, rejected.read().status == NativeGunStatus::UnsupportedBuild);
	TEST_EXPECT(context, rejected.calls == 0);
	rejected.host = f.host;
	rejected.host.fingerprint ^= 1;
	TEST_EXPECT(context, rejected.read().status == NativeGunStatus::UnsupportedBuild);
	TEST_EXPECT(context, rejected.calls == 0);
	rejected.host = f.host;
	rejected.host.payload_function = 0;
	TEST_EXPECT(context, rejected.read().status == NativeGunStatus::MissingPayloadExport);
	TEST_EXPECT(context, rejected.calls == 0);
}
