#pragma once
#include "DcsAircraftInterface.h"
#include "DcsAircraftBackend.h"
namespace DcsBridge::Internal
{
/// @brief 保存已核對的原生尋標器函式，並於每次操作重新取得選中武器物件。
class IrSeekerAccess final
{
public:
	void reset();
	AircraftIrReading apply(const DcsAircraftBackend& backend,
	                        const AircraftModule& module,
	                        bool uncaged,
	                        double azimuth_rad,
	                        double elevation_rad,
	                        bool reset);

private:
	bool resolve(const DcsAircraftBackend& backend, const AircraftModule& module);
	bool resolved_ = false;
	AircraftAccessStatus status_ = AircraftAccessStatus::WaitingForModule;
	std::uintptr_t contexts_ = 0, device_ = 0, weapon_table_ = 0, sensor_table_ = 0;
	std::uintptr_t cage_ = 0, uncage_ = 0, consent_ = 0, slave_ = 0, direction_ = 0;
	std::uintptr_t last_sensor_ = 0;
	bool last_uncaged_ = false;
	std::uint32_t fault_ = 0;
};
}
