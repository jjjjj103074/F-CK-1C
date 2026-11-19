#include "GunAmmunitionSource.h"

#include "../EventLog.h"
#include "../AbiBoundary.h"
#include <cstdio>

#include "../Aircraft/DcsAircraftInterface.h"
#include <optional>
#include <utility>

namespace DcsBridge::Internal
{
struct GunAmmunitionSource::Implementation
{
	EventLog& log;
	std::function<AircraftGunReading()> reader;
	bool call_failed = false;
	double next_read_s = 0;
	std::uint32_t exception = 0;
	bool reported_host = false;
	NativeGunResult result;
	Core::GunAmmunitionObservation observation;
	std::optional<NativeGunStatus> reported_status;

	Implementation(EventLog& event_log, std::function<AircraftGunReading()> read)
	    : log(event_log),
	      reader(std::move(read))
	{
	}

	void write(double time_s, const char* message, EventLevel level = EventLevel::Info) noexcept
	try
	{
		(void)log.write({ level, time_s, message });
	}
	catch (...)
	{
	}

	void report_host(double time_s, const AircraftBuildInfo& build)
	{
		if (reported_host || !build.present)
			return;
		reported_host = true;
		if (!build.header_valid)
		{
			write(time_s, "gun_ammunition event=host_header_invalid");
			return;
		}
		char message[256]{};
		std::snprintf(
		    message,
		    sizeof(message),
		    "gun_ammunition event=host pe_timestamp=0x%08x image_size=0x%x fingerprint=0x%016llx payload_export=%u matched=%u",
		    static_cast<unsigned>(build.timestamp),
		    static_cast<unsigned>(build.image_size),
		    static_cast<unsigned long long>(build.gun_fingerprint),
		    build.payload_export ? 1U : 0U,
		    build.gun_profile_matched ? 1U : 0U);
		write(time_s, message);
	}

	void report_status(double time_s)
	{
		if (reported_status == result.status)
			return;
		reported_status = result.status;
		char message[192]{};
		std::snprintf(message,
		              sizeof(message),
		              "gun_ammunition event=status status=%s exception_code=0x%08lx",
		              native_gun_status_name(result.status),
		              static_cast<unsigned long>(exception));
		write(time_s, message, result.status == NativeGunStatus::CallFailed ? EventLevel::Error : EventLevel::Info);
	}
};

GunAmmunitionSource::GunAmmunitionSource(EventLog& log, Reader reader)
    : implementation_(std::make_unique<Implementation>(log,
                                                       [read = std::move(reader)]
                                                       {
	                                                       AircraftGunReading result;
	                                                       if (read)
		                                                       result.ammunition = read();
	                                                       return result;
                                                       }))
{
}
GunAmmunitionSource::GunAmmunitionSource(EventLog& log, DcsAircraftInterface& aircraft)
    : implementation_(std::make_unique<Implementation>(log,
                                                       [&aircraft]
                                                       {
	                                                       return aircraft.read_gun_ammunition();
                                                       }))
{
}
GunAmmunitionSource::~GunAmmunitionSource() = default;

void GunAmmunitionSource::reset_flight()
{
	auto& state = *implementation_;
	state.next_read_s = 0;
	state.call_failed = false;
	state.exception = 0;
	state.observation = {};
	state.result = {};
	state.reported_host = false;
	state.reported_status.reset();
}

Core::GunAmmunitionObservation GunAmmunitionSource::observe(double time_s)
{
	auto& state = *implementation_;
	if (time_s < state.next_read_s)
		return state.observation;
	state.next_read_s = time_s + 0.1;
	try
	{
		if (!state.call_failed)
		{
			const auto reading = state.reader();
			state.result = reading.ammunition;
			state.exception = reading.exception;
			state.report_host(time_s, reading.build);
			state.call_failed = state.result.status == NativeGunStatus::CallFailed;
		}
	}
	catch (...)
	{
		report_abi_exception("GunAmmunitionSource::observe", std::current_exception(), &state.log);
		state.call_failed = true;
		state.result.status = NativeGunStatus::CallFailed;
	}
	if (state.result.status == NativeGunStatus::Available && state.result.count > 9999)
		state.result.status = NativeGunStatus::InvalidCount;
	const bool available = state.result.status == NativeGunStatus::Available;
	const auto reason = available ? Core::ObservationInvalidReason::None
	                    : state.result.status == NativeGunStatus::InvalidCount
	                        ? Core::ObservationInvalidReason::InvalidNumeric
	                        : Core::ObservationInvalidReason::NotProvided;
	state.observation.status = { available, state.observation.status.revision + 1, reason };
	state.observation.count = available ? static_cast<int>(state.result.count) : 0;
	// 觀測先完成，再記錄來源狀態。
	state.report_status(time_s);
	return state.observation;
}
}
