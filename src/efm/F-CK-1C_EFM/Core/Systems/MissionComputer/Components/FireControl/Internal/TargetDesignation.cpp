#include "TargetDesignation.h"

namespace Core::Systems::Mc::Internal
{
TargetDesignation::TargetDesignation(TargetDesignationData& data)
    : data_(data)
{
}

void TargetDesignation::request(TargetDesignationRequest request)
{
	data_.requested = request != TargetDesignationRequest::Cancel;
}

void TargetDesignation::observe(const RadarObservation& radar)
{
	data_.radar_observation = radar.status;
	data_.radar_tracking = radar.status.available && radar.mode == RadarMode::SingleTargetTrack;
	data_.radar_azimuth_rad = data_.radar_tracking ? radar.stt_azimuth_rad : 0.0;
	data_.radar_elevation_rad = data_.radar_tracking ? radar.stt_elevation_rad : 0.0;
	data_.radar_range_m = data_.radar_tracking ? radar.stt_range_m : 0.0;
}
}
