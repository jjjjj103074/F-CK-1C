#include "Aim9SeekerProcessing.h"

#include <cmath>

namespace Core::Systems::Mc
{
Aim9SeekerProcessing::Aim9SeekerProcessing(const MissionInputs& input,
                                           const ModeData& mode,
                                           const StoresData& stores,
                                           const SensorData& sensor,
                                           Aim9Data& aim9)
    : input_(input),
      mode_(mode),
      stores_(stores),
      sensor_(sensor),
      aim9_(aim9)
{
}

bool Aim9SeekerProcessing::telemetry_active() const
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

void Aim9SeekerProcessing::resolve_tracking()
{
	const auto& ir = input_.cockpit.ir_seeker;
	const auto& radar = input_.cockpit.radar;
	aim9_.contact = false;
	aim9_.locked = false;
	if (ir.status.available &&
	    (ir.locked || std::abs(ir.target_azimuth_rad) > 0.001 || std::abs(ir.target_elevation_rad) > 0.001))
	{
		aim9_.contact = aim9_.active;
		aim9_.locked = aim9_.active && ir.locked;
	}
	else if (ir.status.available && radar.status.available)
	{
		if (telemetry_active())
		{
			aim9_.contact = aim9_.active && sensor_.uncage_held &&
			                std::abs(ir.desired_azimuth_rad) + std::abs(ir.desired_elevation_rad) > 0.00001;
			aim9_.locked = aim9_.active && sensor_.uncage_held && radar.stt_range_m > 1.0;
		}
		else
		{
			// Preserve the existing prototype fallback only for valid all-zero samples.
			aim9_.contact = aim9_.active && sensor_.uncage_held;
			aim9_.locked = aim9_.contact && sensor_.target_designated;
		}
	}
}

void Aim9SeekerProcessing::step(std::chrono::nanoseconds, double)
{
	aim9_.readiness = !mode_.air_to_air() || stores_.missile_count <= 0  ? Aim9Readiness::Off
	                  : sensor_.uncage_held && sensor_.target_designated ? Aim9Readiness::Track
	                  : sensor_.uncage_held                              ? Aim9Readiness::Ready
	                                                                     : Aim9Readiness::Cool;
	aim9_.active = mode_.air_to_air() && stores_.missile_count > 0 && aim9_.readiness != Aim9Readiness::Off;
	resolve_tracking();
	aim9_.seeker_state = aim9_.readiness == Aim9Readiness::Off                     ? Aim9SeekerState::Off
	                     : aim9_.readiness == Aim9Readiness::Track || aim9_.locked ? Aim9SeekerState::Track
	                     : aim9_.readiness == Aim9Readiness::Ready                 ? Aim9SeekerState::SearchUncaged
	                                                                               : Aim9SeekerState::SearchCaged;
	aim9_.requested_tone = !aim9_.active   ? Aim9Tone::Off
	                       : aim9_.locked  ? Aim9Tone::Lock
	                       : aim9_.contact ? Aim9Tone::Acquire
	                       : aim9_.readiness == Aim9Readiness::Cool || aim9_.readiness == Aim9Readiness::Ready
	                           ? Aim9Tone::Seek
	                           : Aim9Tone::Off;
	const auto& ir = input_.cockpit.ir_seeker;
	const auto& radar = input_.cockpit.radar;
	aim9_.seeker_azimuth_rad = ir.status.available ? ir.desired_azimuth_rad : 0.0;
	aim9_.seeker_elevation_rad = ir.status.available ? ir.desired_elevation_rad : 0.0;
	aim9_.lock_range_m = radar.status.available ? radar.stt_range_m : 0.0;
}
}
