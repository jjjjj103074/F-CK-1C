#pragma once
#include "DcsBridge/Internal/Aircraft/DcsAircraftBackend.h"
#include <cstring>
#include <map>
#include <string>

namespace Tests::Aircraft
{
using namespace DcsBridge::Internal;
struct Fixture
{
	AircraftModule module{ 0x10000000, true, 0x6adbad09, 0x68e000 };
	std::map<std::string, std::uintptr_t> symbols{
		{ "?c_payload@cockpit@@YAPEAVIwHumanPayload@@XZ", 0x5000 },
		{ "?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A", 0x6000 },
		{ "?l_drop_chaff@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 0x5100 },
		{ "?l_drop_flare@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 0x5200 },
		{ "?l_get_chaff_count@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 0x5300 },
		{ "?l_get_flare_count@avSimpleWeaponSystem@cockpit@@CAHPEAUlua_State@@@Z", 0x5400 }
	};
	std::map<std::uintptr_t, std::uint64_t> hashes{
		{ 0x5000, 0xe4153ccf2d04e7d6ULL }, { 0x5100, 0x46228e87c2536c9fULL }, { 0x5200, 0x18198bf9ab4a388aULL },
		{ 0x5300, 0x872314f529cfb306ULL }, { 0x5400, 0x0f4663cb30bfdfa3ULL }, { 0x102fe650, 0xdbd7d715791c239fULL }
	};
	std::map<std::uintptr_t, std::uintptr_t> memory{ { 0x6000, 0x1000 }, { 0x1010, 0x2000 }, { 0x2000, 0x7000 },
		                                             { 0x7120, 0x8000 }, { 0x7178, 0x8100 }, { 0x7180, 0x8200 },
		                                             { 0x3000, 0x9000 }, { 0x91a8, 0xa000 }, { 0x91b0, 0xa100 },
		                                             { 0x9230, 0xa200 }, { 0xb008, 0xc000 }, { 0xc000, 0xd000 },
		                                             { 0xd058, 0xa300 } };
	int flare = 90, chaff = 90;
	unsigned gun = 523, calls = 0, drops = 0, fingerprints = 0, human_reads = 0;
	bool mismatch = false, fail_count = false, fail_drop = false, change_human = false, non_executable = false;
	DcsAircraftBackend backend()
	{
		DcsAircraftBackend b;
		b.module = [this]
		{
			return module;
		};
		b.symbol = [this](std::uintptr_t, const char* name)
		{
			const auto i = symbols.find(name);
			return i == symbols.end() ? 0 : i->second;
		};
		b.read = [this](std::uintptr_t address, void* target, std::size_t size)
		{
			const auto i = memory.find(address);
			if (i == memory.end() || size != sizeof(std::uintptr_t))
				return false;
			auto value = i->second;
			if (address == 0x1010 && ++human_reads == 2 && change_human)
				value = 0x2010;
			std::memcpy(target, &value, size);
			return true;
		};
		b.executable = [this](std::uintptr_t f)
		{
			return f != 0 && !non_executable;
		};
		b.fingerprint = [this](std::uintptr_t address, std::size_t)
		{
			++fingerprints;
			return hashes[address];
		};
		b.payload = [this](std::uintptr_t f, std::uintptr_t& value, std::uint32_t&)
		{
			++calls;
			value = 0x3000;
			return f == 0x5000;
		};
		b.human_payload = [this](std::uintptr_t f, std::uintptr_t h, std::uintptr_t& value, std::uint32_t&)
		{
			++calls;
			value = mismatch ? 0x3010 : 0x3000;
			return f == 0x8000 && h == 0x2000;
		};
		b.countermeasure_count = [this](std::uintptr_t f, std::uintptr_t p, int& value, std::uint32_t& error)
		{
			++calls;
			if (fail_count)
			{
				error = 0xc0000005;
				return false;
			}
			value = f == 0xa000 ? flare : chaff;
			return p == 0x3000 && (f == 0xa000 || f == 0xa100);
		};
		b.dispense = [this](std::uintptr_t f, std::uintptr_t h, std::uint32_t& error)
		{
			++calls;
			++drops;
			if (fail_drop)
			{
				error = 0xc0000005;
				return false;
			}
			if (h != 0x2000)
				return false;
			if (f == 0x8200)
			{
				--flare;
				return true;
			}
			if (f == 0x8100)
			{
				--chaff;
				return true;
			}
			return false;
		};
		b.gun_mount = [this](std::uintptr_t f, std::uintptr_t p, unsigned char i, std::uintptr_t& value, std::uint32_t&)
		{
			++calls;
			value = i == 0 ? 0xb000 : 0;
			return f == 0xa200 && p == 0x3000;
		};
		b.gun_count = [this](std::uintptr_t f, std::uintptr_t p, unsigned& value, std::uint32_t&)
		{
			++calls;
			value = gun;
			return f == 0xa300 && p == 0xc000;
		};
		return b;
	}
};
}
