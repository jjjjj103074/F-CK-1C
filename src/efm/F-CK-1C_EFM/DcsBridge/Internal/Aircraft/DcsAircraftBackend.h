#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <array>

namespace DcsBridge::Internal
{
/// @brief 描述目前載入的座艙程式庫。
struct AircraftModule
{
	std::uintptr_t base = 0;
	bool valid = false;
	std::uint32_t timestamp = 0, image_size = 0;
};
/// @brief 提供原生環境與各種已驗證呼叫的執行介面，供平台實作與測試替換。
struct DcsAircraftBackend
{
	std::function<bool(std::uintptr_t, std::uintptr_t, unsigned, std::uintptr_t&, std::uint32_t&)> device;
	std::function<bool(std::uintptr_t, std::uintptr_t, std::uint32_t&)> object_action;
	std::function<bool(std::uintptr_t, std::uintptr_t, float, std::uint32_t&)> set_float;
	std::function<bool(std::uintptr_t, std::uintptr_t, const std::array<double, 3>&, std::uint32_t&)> direction;
	std::function<AircraftModule()> module;
	std::function<std::uintptr_t(std::uintptr_t, const char*)> symbol;
	std::function<bool(std::uintptr_t, void*, std::size_t)> read;
	std::function<bool(std::uintptr_t)> executable;
	std::function<std::uint64_t(std::uintptr_t, std::size_t)> fingerprint;
	std::function<bool(std::uintptr_t, std::uintptr_t&, std::uint32_t&)> payload;
	std::function<bool(std::uintptr_t, std::uintptr_t, std::uintptr_t&, std::uint32_t&)> human_payload;
	std::function<bool(std::uintptr_t, std::uintptr_t, int&, std::uint32_t&)> countermeasure_count;
	std::function<bool(std::uintptr_t, std::uintptr_t, std::uint32_t&)> dispense;
	std::function<bool(std::uintptr_t, std::uintptr_t, unsigned char, std::uintptr_t&, std::uint32_t&)> gun_mount;
	std::function<bool(std::uintptr_t, std::uintptr_t, unsigned&, std::uint32_t&)> gun_count;
};
std::uint64_t aircraft_code_fingerprint(const unsigned char* code, std::size_t size);
DcsAircraftBackend make_dcs_aircraft_backend();
}
