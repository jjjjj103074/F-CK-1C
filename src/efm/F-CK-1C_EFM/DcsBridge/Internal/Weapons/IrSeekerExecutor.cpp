#include "IrSeekerExecutor.h"
#include "../../../DcsIds/CockpitParams.g.h"
#include <cstdio>
#include <utility>
namespace DcsBridge::Internal
{
IrSeekerExecutor::IrSeekerExecutor(DcsAircraftInterface& aircraft,
                                   cockpit_param_api api,
                                   std::function<void(const EventRecord&)> report)
    : aircraft_(aircraft),
      api_(api),
      report_(std::move(report))
{
}
void IrSeekerExecutor::reset_flight()
{
	reading_ = {};
	previous_ = {};
	last_attempt_s_ = -1.0;
}
void IrSeekerExecutor::observe(Core::IrSeekerObservation& observation) const
{
	observation.control_available = previous_.active && reading_.available();
	observation.self_track_enabled = observation.control_available && reading_.self_track_enabled;
	observation.native_mode = observation.control_available ? reading_.native_mode : -1;
	observation.control_status = static_cast<int>(reading_.status);
	observation.control_revision = previous_.revision;
}
void IrSeekerExecutor::execute(const Core::StoresSnapshot& stores, double time_s)
{
	const auto& control = stores.seeker_control;
	const bool changed = control.revision != previous_.revision || control.active != previous_.active;
	if (!stores.status.available || (!changed && last_attempt_s_ >= 0 && time_s - last_attempt_s_ < 1.0 / 32.0))
		return;
	if (last_attempt_s_ < 0 && report_)
		report_({ EventLevel::Info, time_s, "ir_seeker_io source=weapon_device_update received=1" });
	last_attempt_s_ = time_s;
	if (!control.active)
	{
		if (previous_.active)
			(void)aircraft_.control_ir_seeker(false, 0.0, 0.0, true);
		previous_ = control;
		reading_ = {};
		return;
	}
	const bool reset =
	    !previous_.active || control.station != previous_.station || control.load_revision != previous_.load_revision;
	const auto old_status = reading_.status;
	// 原生 IR 要求角以 -Z 為正，雷達與 MC 視線以 +Z 為正。
	const double native_azimuth = -control.azimuth_rad;
	reading_ = aircraft_.control_ir_seeker(control.uncaged, native_azimuth, control.elevation_rad, reset);
	if (reading_.available())
	{
		auto az = cockpit_parameter_writer(DcsIds::RawCockpitParams::IrDesiredAzimuth);
		auto el = cockpit_parameter_writer(DcsIds::RawCockpitParams::IrDesiredElevation);
		(void)az.write_number(api_, native_azimuth);
		(void)el.write_number(api_, control.elevation_rad);
	}
	if ((reset || control.uncaged != previous_.uncaged || control.radar_slaved != previous_.radar_slaved ||
	     old_status != reading_.status) &&
	    report_)
	{
		char message[384]{};
		std::snprintf(
		    message,
		    sizeof(message),
		    "ir_seeker_control revision=%llu station=%d requested_uncaged=%u radar_slaved=%u status=%d native_mode=%d consent=%u exception=0x%08x",
		    static_cast<unsigned long long>(control.revision),
		    control.station,
		    control.uncaged ? 1U : 0U,
		    control.radar_slaved ? 1U : 0U,
		    static_cast<int>(reading_.status),
		    reading_.native_mode,
		    reading_.self_track_enabled ? 1U : 0U,
		    reading_.exception);
		report_({ reading_.available() ? EventLevel::Info : EventLevel::Warning, time_s, message });
	}
	previous_ = control;
}
}
