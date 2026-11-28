#include "TestHarness.h"
#include "DcsBridge/Internal/Aircraft/DcsAircraftBackend.h"
#include "DcsBridge/Internal/Aircraft/DcsAircraftInterface.h"
#include "DcsBridge/Internal/Weapons/IrSeekerExecutor.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>
namespace
{
using namespace DcsBridge::Internal;
struct IrFixture
{
	std::map<std::string, std::uintptr_t> symbols;
	std::map<std::uintptr_t, std::uint64_t> hashes;
	std::map<std::uintptr_t, std::vector<unsigned char>> memory;
	AircraftModule module{ 0x100000, true, 0x6adbad09, 0x68e000 };
	unsigned calls = 0, clears = 0;
	bool succeed = true;
	std::array<double, 3> direction{};
	template <class T> void put(std::uintptr_t address, T value)
	{
		auto& bytes = memory[address];
		bytes.resize(sizeof(T));
		std::memcpy(bytes.data(), &value, sizeof(T));
	}
	unsigned target() const
	{
		unsigned value = 0;
		std::memcpy(&value, memory.at(0x42c8).data(), sizeof(value));
		return value;
	}
	void add(const char* name, std::uint64_t hash = 0)
	{
		const auto address = 0x100000 + symbols.size() * 256;
		symbols[name] = address;
		hashes[address] = hash;
	}
	explicit IrFixture(bool current_build = false)
	{
		add("?get_wide_device@ccCockpitContext@cockpit@@QEAAPEAVavDevice@2@I@Z", 0x81cc11171e1ba916ULL);
		add("?update_ir_seeker@avSimpleWeaponSystem@cockpit@@AEAAXXZ", 0x98f33ac9cf396e65ULL);
		add("??0avSidewinderSeeker@cockpit@@QEAA@XZ", 0x5d5008a754d68227ULL);
		add("?setConsentToTrack@avSidewinderSeeker@cockpit@@QEAAXM@Z", 0x0df74a7ec2826fc8ULL);
		add("?getStatusID@avIRSensor@cockpit@@QEBAIXZ", 0xec4c08ae667a8ea4ULL);
		add("?cage@avSidewinderSeeker@cockpit@@MEAAXXZ", 0x7503bb3a0ff66589ULL);
		add("?uncage@avIRSensor@cockpit@@IEAAXXZ", 0xc117d6212af01575ULL);
		add("?slaveToDirection@avSidewinderSeeker@cockpit@@UEAAXAEBVVec3d@osg@@@Z", 0x447f2c8a1a851472ULL);
		add("?toSlave@avSidewinderSeeker@cockpit@@IEAAXXZ", 0x5a587ffbf47695b9ULL);
		for (const auto* name : { "?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A",
		                          "??_7avSimpleWeaponSystem@cockpit@@6BccContextRelatedObject@1@@",
		                          "??_7avSimpleWeaponSystem@cockpit@@6BavLuaDevice@1@@",
		                          "??_7avSidewinderSeeker@cockpit@@6BavPlatform@1@@" })
			add(name);
		if (current_build)
		{
			module.timestamp = 0x6afec04f;
			hashes[symbols["?get_wide_device@ccCockpitContext@cockpit@@QEAAPEAVavDevice@2@I@Z"]] =
			    0x41d2d25f18107729ULL;
			hashes[symbols["??0avSidewinderSeeker@cockpit@@QEAA@XZ"]] = 0x1a2079ded3a1ce88ULL;
			hashes[symbols["?slaveToDirection@avSidewinderSeeker@cockpit@@UEAAXAEBVVec3d@osg@@@Z"]] =
			    0x181082bbc1acedb8ULL;
			hashes[symbols["?toSlave@avSidewinderSeeker@cockpit@@IEAAXXZ"]] = 0x6b730569969a099eULL;
		}
		put(symbols["?contexts_ptr@ccCockpitContext@cockpit@@0PAPEAV12@A"], std::uintptr_t{ 0x2000 });
		// get_wide_device 回傳裝置主物件；Lua 子物件位於 +0x98。
		put(0x3000, symbols["??_7avSimpleWeaponSystem@cockpit@@6BccContextRelatedObject@1@@"]);
		put(0x3098, symbols["??_7avSimpleWeaponSystem@cockpit@@6BavLuaDevice@1@@"]);
		put(0x3158, std::uintptr_t{ 0x4000 });
		put(0x4000, symbols["??_7avSidewinderSeeker@cockpit@@6BavPlatform@1@@"]);
		put(0x4284, int{ 5 });
		put(0x42c8, unsigned{ 123 });
		put(0x442d, static_cast<unsigned char>(0));
	}
	DcsAircraftBackend backend()
	{
		DcsAircraftBackend b;
		b.module = [this]
		{
			return module;
		};
		b.symbol = [this](std::uintptr_t, const char* n)
		{
			auto i = symbols.find(n);
			return i == symbols.end() ? 0 : i->second;
		};
		b.executable = [this](std::uintptr_t a)
		{
			return hashes.count(a) != 0;
		};
		b.fingerprint = [this](std::uintptr_t a, std::size_t)
		{
			return hashes[a];
		};
		b.read = [this](std::uintptr_t a, void* p, std::size_t n)
		{
			auto i = memory.find(a);
			if (i == memory.end() || i->second.size() != n)
				return false;
			std::memcpy(p, i->second.data(), n);
			return true;
		};
		b.device = [](std::uintptr_t, std::uintptr_t c, unsigned i, std::uintptr_t& o, std::uint32_t&)
		{
			o = c == 0x2000 && i == 4 ? 0x3000 : 0;
			return true;
		};
		b.set_float = [this](std::uintptr_t, std::uintptr_t, float v, std::uint32_t& e)
		{
			++calls;
			if (!succeed)
			{
				e = 0xc0000005;
				return false;
			}
			put(0x442d, static_cast<unsigned char>(v != 0));
			return true;
		};
		b.object_action = [this](std::uintptr_t f, std::uintptr_t, std::uint32_t&)
		{
			++calls;
			++clears;
			put(0x42c8, unsigned{ 0 });
			const bool slave = f == symbols["?toSlave@avSidewinderSeeker@cockpit@@IEAAXXZ"];
			put(0x4284, slave ? 5 : 0);
			return true;
		};
		b.direction = [this](std::uintptr_t, std::uintptr_t, const std::array<double, 3>& v, std::uint32_t&)
		{
			++calls;
			direction = v;
			return true;
		};
		return b;
	}
};
}
void run_ir_seeker_access_tests(Tests::Context& c)
{
	for (const bool current_build : { false, true })
	{
		IrFixture f(current_build);
		DcsAircraftInterface aircraft(f.backend());
		const auto caged = aircraft.control_ir_seeker(false, .2, .1, true);
		TEST_EXPECT(c, caged.available() && !caged.self_track_enabled && caged.native_mode == 5);
		// 原生捕獲回報後，連續解籠要求保留目標與原生模式。
		f.put(0x42c8, unsigned{ 123 });
		f.put(0x4284, int{ 3 });
		const auto clears = f.clears;
		for (unsigned tick = 0; tick != 32; ++tick)
		{
			const auto tracking = aircraft.control_ir_seeker(true, .3, .2, false);
			TEST_EXPECT(c, tracking.available() && tracking.self_track_enabled && tracking.native_mode == 3);
			TEST_EXPECT(c, f.target() == 123 && f.clears == clears);
		}
		const auto recaged = aircraft.control_ir_seeker(false, 0, 0, false);
		TEST_EXPECT(c, recaged.available() && !recaged.self_track_enabled && recaged.native_mode == 5);
		TEST_EXPECT(c, f.target() == 0 && f.clears == clears + 2);
	}
	{
		IrFixture f(true);
		f.put(0x3000, f.symbols["??_7avSimpleWeaponSystem@cockpit@@6BavLuaDevice@1@@"]);
		DcsAircraftInterface aircraft(f.backend());
		const auto rejected = aircraft.control_ir_seeker(false, 0, 0, false);
		TEST_EXPECT(c, rejected.status == AircraftAccessStatus::UnreadableObject && f.calls == 0);
		TEST_EXPECT(c, std::strcmp(rejected.stage, "weapon_identity") == 0);
	}
	{
		IrFixture f(true);
		DcsAircraftInterface aircraft(f.backend());
		IrSeekerExecutor executor(aircraft, {}, {});
		Core::StoresSnapshot stores;
		stores.status.available = true;
		stores.seeker_control.active = true;
		stores.seeker_control.revision = 1;
		executor.execute(stores, 0);
		Core::IrSeekerObservation observation;
		executor.observe(observation);
		TEST_EXPECT(c, observation.control_available && !observation.self_track_enabled);
		f.put(0x42c8, unsigned{ 123 });
		f.put(0x4284, int{ 3 });
		stores.seeker_control.uncaged = true;
		stores.seeker_control.revision = 2;
		const auto clears = f.clears;
		for (unsigned tick = 1; tick <= 32; ++tick)
		{
			executor.execute(stores, tick * .04);
			executor.observe(observation);
			TEST_EXPECT(c,
			            observation.control_available && observation.self_track_enabled &&
			                observation.native_mode == 3);
			TEST_EXPECT(c, f.target() == 123 && f.clears == clears);
		}
		stores.seeker_control.uncaged = false;
		stores.seeker_control.revision = 3;
		executor.execute(stores, 1.4);
		executor.observe(observation);
		TEST_EXPECT(c,
		            observation.control_available && !observation.self_track_enabled && observation.native_mode == 5);
	}

	{
		IrFixture f(true);
		DcsAircraftInterface aircraft(f.backend());
		const auto caged = aircraft.control_ir_seeker(false, 0.2, 0.1, true);
		TEST_EXPECT(c, caged.available() && !caged.self_track_enabled && caged.native_mode == 5);
		const auto self_track = aircraft.control_ir_seeker(true, 0.2, 0.1, false);
		TEST_EXPECT(c, self_track.available() && self_track.self_track_enabled);
		TEST_EXPECT_NEAR(c, f.direction[2], -std::cos(0.1) * std::sin(0.2), 1e-12);
	}
	{
		IrFixture f(true);
		f.hashes[f.symbols["?get_wide_device@ccCockpitContext@cockpit@@QEAAPEAVavDevice@2@I@Z"]] ^= 1;
		DcsAircraftInterface aircraft(f.backend());
		TEST_EXPECT(c, aircraft.control_ir_seeker(false, 0, 0, false).status == AircraftAccessStatus::UnsupportedBuild);
		TEST_EXPECT(c, f.calls == 0);
	}

	{
		IrFixture f;
		DcsAircraftInterface aircraft(f.backend());
		IrSeekerExecutor executor(aircraft, {}, {});
		Core::StoresSnapshot stores;
		stores.status.available = true;
		stores.seeker_control.active = true;
		stores.seeker_control.azimuth_rad = .2;
		stores.seeker_control.elevation_rad = .1;
		executor.execute(stores, 0.0);
		Core::IrSeekerObservation observation;
		executor.observe(observation);
		TEST_EXPECT(c, observation.control_available);
		TEST_EXPECT_NEAR(c, f.direction[2], std::cos(.1) * std::sin(.2), 1e-12);
	}
	{
		IrFixture f;
		DcsAircraftInterface aircraft(f.backend());
		auto r = aircraft.control_ir_seeker(true, .2, .1, false);
		TEST_EXPECT(c, r.available() && r.self_track_enabled);
		TEST_EXPECT(c, f.clears == 0);
		r = aircraft.control_ir_seeker(false, .2, .1, false);
		TEST_EXPECT(c, r.available() && !r.self_track_enabled && r.native_mode == 5);
		TEST_EXPECT(c, f.clears == 2);
		TEST_EXPECT_NEAR(c, f.direction[0], std::cos(.1) * std::cos(.2), 1e-12);
		TEST_EXPECT_NEAR(c, f.direction[1], std::sin(.1), 1e-12);
		TEST_EXPECT_NEAR(c, f.direction[2], -std::cos(.1) * std::sin(.2), 1e-12);
		(void)aircraft.control_ir_seeker(false, -.3, .1, false);
		TEST_EXPECT_NEAR(c, f.direction[2], std::cos(.1) * std::sin(.3), 1e-12);
		TEST_EXPECT(c, f.clears == 2);
		(void)aircraft.control_ir_seeker(false, 0, 0, true);
		TEST_EXPECT(c, f.clears == 4);
	}
	{
		IrFixture f;
		f.module.timestamp = 0;
		DcsAircraftInterface aircraft(f.backend());
		TEST_EXPECT(c, aircraft.control_ir_seeker(false, 0, 0, false).status == AircraftAccessStatus::UnsupportedBuild);
		TEST_EXPECT(c, f.calls == 0);
	}
	{
		IrFixture f;
		f.put(0x4000, std::uintptr_t{ 0 });
		DcsAircraftInterface aircraft(f.backend());
		TEST_EXPECT(c, aircraft.control_ir_seeker(false, 0, 0, false).status == AircraftAccessStatus::UnreadableObject);
		TEST_EXPECT(c, f.calls == 0);
	}
	{
		IrFixture f;
		f.succeed = false;
		DcsAircraftInterface aircraft(f.backend());
		const auto r = aircraft.control_ir_seeker(true, 0, 0, false);
		TEST_EXPECT(c, !r.available() && r.exception == 0xc0000005);
		const auto calls = f.calls;
		f.succeed = true;
		TEST_EXPECT(c, !aircraft.control_ir_seeker(true, 0, 0, false).available());
		TEST_EXPECT(c, f.calls == calls);
		aircraft.reset_flight();
		TEST_EXPECT(c, aircraft.control_ir_seeker(true, 0, 0, false).available());
	}
	{
		IrFixture f;
		DcsAircraftInterface aircraft(f.backend());
		TEST_EXPECT(c, aircraft.control_ir_seeker(false, 4, 0, false).status == AircraftAccessStatus::InvalidRequest);
		TEST_EXPECT(c, f.calls == 0);
	}
}
