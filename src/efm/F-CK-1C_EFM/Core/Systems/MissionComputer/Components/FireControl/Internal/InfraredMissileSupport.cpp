#include "InfraredMissileSupport.h"

#include <cmath>

namespace Core::Systems::Mc::Internal
{
InfraredMissileSupport::InfraredMissileSupport(const MissionInputs& input,
                                               const ModeData& mode,
                                               const StoresData& stores,
                                               const TargetingData& targeting,
                                               InfraredMissileData& missile)
    : input_(input),
      mode_(mode),
      stores_(stores),
      targeting_(targeting),
      missile_(missile)
{
}

bool InfraredMissileSupport::telemetry_active() const
{
	const auto& radar = input_.cockpit.radar;
	const auto& ir = input_.cockpit.ir_seeker;
	const double metric = std::abs(ir.desired_azimuth_rad) + std::abs(ir.desired_elevation_rad) +
	                      std::abs(radar.stt_azimuth_rad) + std::abs(radar.stt_elevation_rad) +
	                      std::abs(radar.stt_range_m) + std::abs(radar.stt_azimuth_stabilized_rad) +
	                      std::abs(radar.stt_elevation_stabilized_rad) + std::abs(radar.tdc_azimuth_rad) +
	                      std::abs(radar.tdc_range_normalized) + std::abs(radar.gate_range_normalized) +
	                      std::abs(radar.contact_01_azimuth_rad) + std::abs(radar.contact_01_range_normalized);
	return metric > 0.00001;
}

void InfraredMissileSupport::resolve_tracking()
{
	const auto& ir = input_.cockpit.ir_seeker;
	const auto& radar = input_.cockpit.radar;
	missile_.contact = false;
	missile_.locked = false;
	if (ir.status.available &&
	    (ir.locked || std::abs(ir.target_azimuth_rad) > 0.001 || std::abs(ir.target_elevation_rad) > 0.001))
	{
		missile_.contact = missile_.active;
		missile_.locked = missile_.active && ir.locked;
	}
	else if (ir.status.available && radar.status.available)
	{
		if (telemetry_active())
		{
			missile_.contact = missile_.active && targeting_.uncage_held &&
			                   std::abs(ir.desired_azimuth_rad) + std::abs(ir.desired_elevation_rad) > 0.00001;
			missile_.locked = missile_.active && targeting_.uncage_held && radar.stt_range_m > 1.0;
		}
		else
		{
			// Preserve the existing prototype fallback only for valid all-zero samples.
			missile_.contact = missile_.active && targeting_.uncage_held;
			missile_.locked = missile_.contact && targeting_.designation_requested;
		}
	}
}

void InfraredMissileSupport::step(std::chrono::nanoseconds, double)
{
	missile_.readiness = !mode_.air_to_air() || stores_.missile_count <= 0            ? InfraredMissileReadiness::Off
	                     : targeting_.uncage_held && targeting_.designation_requested ? InfraredMissileReadiness::Track
	                     : targeting_.uncage_held                                     ? InfraredMissileReadiness::Ready
	                                                                                  : InfraredMissileReadiness::Cool;
	missile_.active =
	    mode_.air_to_air() && stores_.missile_count > 0 && missile_.readiness != InfraredMissileReadiness::Off;
	resolve_tracking();
	missile_.seeker_state = missile_.readiness == InfraredMissileReadiness::Off ? InfraredSeekerState::Off
	                        : missile_.readiness == InfraredMissileReadiness::Track || missile_.locked
	                            ? InfraredSeekerState::Track
	                        : missile_.readiness == InfraredMissileReadiness::Ready ? InfraredSeekerState::SearchUncaged
	                                                                                : InfraredSeekerState::SearchCaged;
	missile_.requested_tone =
	    !missile_.active   ? InfraredSeekerTone::Off
	    : missile_.locked  ? InfraredSeekerTone::Lock
	    : missile_.contact ? InfraredSeekerTone::Acquire
	    : missile_.readiness == InfraredMissileReadiness::Cool || missile_.readiness == InfraredMissileReadiness::Ready
	        ? InfraredSeekerTone::Seek
	        : InfraredSeekerTone::Off;
	const auto& ir = input_.cockpit.ir_seeker;
	const auto& radar = input_.cockpit.radar;
	missile_.seeker_azimuth_rad = ir.status.available ? ir.desired_azimuth_rad : 0.0;
	missile_.seeker_elevation_rad = ir.status.available ? ir.desired_elevation_rad : 0.0;
	missile_.lock_range_m = radar.status.available ? radar.stt_range_m : 0.0;
}
}
