#include "RadarPresentation.h"
#include "Common/Units.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr double kScopeHalfAzimuthRad = Common::rad(60.0);
constexpr double kReturnLifetimeS = 5.0;
constexpr double kSweepSpeedRadS = Common::rad(120.0);

bool plot_position(double azimuth, double range, double scale, double& x, double& y)
{
	if (!std::isfinite(azimuth) || !std::isfinite(range) || range <= 0.0 || range > scale ||
	    std::abs(azimuth) > kScopeHalfAzimuthRad)
		return false;
	x = azimuth / kScopeHalfAzimuthRad;
	y = range / scale;
	return true;
}
}

namespace Core::Systems::DisplayPresentation
{
RadarDisplaySnapshot radar(const RadarSnapshot& source, OperatingMode mode, std::chrono::nanoseconds time)
{
	RadarDisplaySnapshot result;
	const bool dogfight = mode == OperatingMode::Dogfight;
	result.visible = source.status.available && source.scan.enabled && is_air_to_air_mode(mode) &&
	                 dogfight == (source.active_search_mode == RadarSearchMode::AirCombat);
	if (!result.visible)
		return result;
	const auto& observation = source.observation;
	const auto& scan = source.scan;
	result.sensor_available = observation.status.available && observation.mode != RadarMode::Off;
	result.bore_half_width_mrad = scan.azimuth_width_rad * 500.0;
	result.bore_half_height_mrad = scan.elevation_height_rad * 500.0;
	result.range_scale_m = std::max(scan.display_range_m, 1.0);
	if (!result.sensor_available)
		return result;
	const bool tracking = observation.mode == RadarMode::SingleTargetTrack;
	const double seconds = std::chrono::duration<double>(time).count();
	if (tracking && std::isfinite(observation.stt_range_m) && observation.stt_range_m > 0.0 &&
	    std::isfinite(observation.stt_azimuth_rad) && std::isfinite(observation.stt_elevation_rad))
	{
		result.cue = RadarCueKind::SingleTargetTrack;
		result.cue_visible = std::abs(observation.stt_azimuth_rad) < Common::rad(89.0) &&
		                     std::abs(observation.stt_elevation_rad) < Common::rad(89.0);
		// 準直平面採毫弧度尺度，正切投影維持大偏角下的目標視線。
		result.target_x_mrad = std::tan(observation.stt_azimuth_rad) * 1000.0;
		result.target_y_mrad = std::tan(observation.stt_elevation_rad) / std::cos(observation.stt_azimuth_rad) * 1000.0;
		while (observation.stt_range_m > result.range_scale_m && result.range_scale_m < 160.0 * 1852.0)
			result.range_scale_m *= 2.0;
		result.target_visible = plot_position(observation.stt_azimuth_rad,
		                                      observation.stt_range_m,
		                                      result.range_scale_m,
		                                      result.target_x_normalized,
		                                      result.target_y_normalized);
	}
	else if (dogfight && !tracking)
	{
		const bool acquiring = source.automatic_acquisition_requested || observation.mode == RadarMode::LockAttempt;
		result.cue = acquiring ? RadarCueKind::BoresightAcquiring : RadarCueKind::BoresightReady;
		result.cue_visible = !acquiring || std::fmod(seconds, 0.5) < 0.25;
	}
	const double center = observation.scan_available ? observation.scan_azimuth_center_rad : scan.azimuth_center_rad;
	const double width = observation.scan_available ? observation.scan_azimuth_width_rad : scan.azimuth_width_rad;
	result.scan_left_normalized = std::clamp((center - width / 2.0) / kScopeHalfAzimuthRad, -1.0, 1.0);
	result.scan_right_normalized = std::clamp((center + width / 2.0) / kScopeHalfAzimuthRad, -1.0, 1.0);
	result.estimated_sweep_visible = !dogfight && !tracking && width > 0.0;
	if (result.estimated_sweep_visible)
	{
		// 虛線掃描以顯示時鐘估算位置，回波位置持續使用原生量測。
		const double phase = std::fmod(seconds * kSweepSpeedRadS / width, 2.0);
		const double progress = phase <= 1.0 ? phase : 2.0 - phase;
		result.estimated_sweep_x_normalized =
		    result.scan_left_normalized + (result.scan_right_normalized - result.scan_left_normalized) * progress;
	}
	const bool controlled_cursor = scan.override_tdc;
	const double cursor_azimuth = controlled_cursor ? scan.tdc_azimuth_rad : observation.tdc_azimuth_rad;
	const double cursor_range = controlled_cursor ? scan.tdc_range_m : observation.tdc_range_m;
	result.cursor_visible = !tracking && (controlled_cursor || observation.tdc_available) &&
	                        plot_position(cursor_azimuth,
	                                      cursor_range,
	                                      result.range_scale_m,
	                                      result.cursor_x_normalized,
	                                      result.cursor_y_normalized);
	if (observation.mode != RadarMode::Search && observation.mode != RadarMode::LockAttempt)
		return result;
	std::vector<RadarSearchReturn> returns = observation.search_returns;
	returns.erase(std::remove_if(returns.begin(),
	                             returns.end(),
	                             [](const auto& contact)
	                             {
		                             return !std::isfinite(contact.age_s) || contact.age_s < 0.0 ||
		                                    contact.age_s >= kReturnLifetimeS;
	                             }),
	              returns.end());
	std::stable_sort(returns.begin(),
	                 returns.end(),
	                 [](const auto& left, const auto& right)
	                 {
		                 return left.age_s < right.age_s;
	                 });
	std::size_t count = 0;
	for (const auto& contact : returns)
	{
		if (count == result.plots.size())
			break;
		if (!std::isfinite(contact.age_s) || contact.age_s < 0.0 || contact.age_s >= kReturnLifetimeS)
			continue;
		RadarDisplayPlot plot;
		if (!plot_position(contact.azimuth_rad,
		                   contact.range_m,
		                   result.range_scale_m,
		                   plot.x_normalized,
		                   plot.y_normalized))
			continue;
		// 畫面上重合的回波保留最新一點，讓其他方位的目標仍有顯示位置。
		const bool overlaps = std::any_of(result.plots.begin(),
		                                  result.plots.begin() + count,
		                                  [&](const auto& drawn)
		                                  {
			                                  return std::abs(drawn.x_normalized - plot.x_normalized) < 0.01 &&
			                                         std::abs(drawn.y_normalized - plot.y_normalized) < 0.01;
		                                  });
		if (overlaps)
			continue;
		plot.opacity_0_1 = 1.0 - contact.age_s / kReturnLifetimeS;
		plot.source_slot = contact.source_slot;
		result.plots[count++] = plot;
	}
	return result;
}
}
