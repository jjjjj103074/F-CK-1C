#include "RadarDisplayExporter.h"
#include "../../DcsIds/CockpitParams.g.h"

namespace DcsBridge::Internal
{
RadarDisplayExporter::RadarDisplayExporter(cockpit_param_api api)
    : api_(api)
{
	const auto bind = [this](Parameter parameter, CockpitParameterEndpoint endpoint)
	{
		slots_[static_cast<std::size_t>(parameter)] = endpoint;
	};
	bind(Parameter::Available, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayAvailable));
	bind(Parameter::Visible, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayVisible));
	bind(Parameter::SensorAvailable, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplaySensorAvailable));
	bind(Parameter::CueKind, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayCueKind));
	bind(Parameter::CueVisible, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayCueVisible));
	bind(Parameter::BoreHalfWidthMrad, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayBoreHalfWidthMrad));
	bind(Parameter::BoreHalfHeightMrad,
	     cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayBoreHalfHeightMrad));
	bind(Parameter::TargetXMrad, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayTargetXMrad));
	bind(Parameter::TargetYMrad, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayTargetYMrad));
	bind(Parameter::RangeScaleM, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayRangeScaleM));
	bind(Parameter::RangeScaleNm, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayRangeScaleNm));
	bind(Parameter::ScanLeft, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayScanLeft));
	bind(Parameter::ScanRight, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayScanRight));
	bind(Parameter::SweepVisible, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplaySweepVisible));
	bind(Parameter::SweepX, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplaySweepX));
	bind(Parameter::CursorVisible, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayCursorVisible));
	bind(Parameter::CursorX, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayCursorX));
	bind(Parameter::CursorY, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayCursorY));
	bind(Parameter::TargetVisible, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayTargetVisible));
	bind(Parameter::TargetX, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayTargetX));
	bind(Parameter::TargetY, cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayTargetY));
	plots_ = { { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot01X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot01Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot01Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot01SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot02X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot02Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot02Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot02SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot03X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot03Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot03Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot03SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot04X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot04Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot04Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot04SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot05X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot05Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot05Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot05SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot06X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot06Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot06Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot06SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot07X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot07Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot07Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot07SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot08X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot08Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot08Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot08SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot09X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot09Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot09Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot09SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot10X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot10Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot10Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot10SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot11X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot11Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot11Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot11SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot12X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot12Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot12Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot12SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot13X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot13Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot13Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot13SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot14X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot14Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot14Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot14SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot15X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot15Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot15Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot15SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot16X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot16Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot16Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot16SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot17X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot17Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot17Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot17SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot18X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot18Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot18Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot18SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot19X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot19Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot19Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot19SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot20X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot20Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot20Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot20SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot21X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot21Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot21Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot21SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot22X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot22Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot22Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot22SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot23X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot23Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot23Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot23SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot24X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot24Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot24Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot24SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot25X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot25Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot25Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot25SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot26X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot26Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot26Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot26SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot27X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot27Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot27Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot27SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot28X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot28Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot28Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot28SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot29X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot29Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot29Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot29SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot30X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot30Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot30Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot30SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot31X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot31Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot31Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot31SourceSlot) },
		       { cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot32X),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot32Y),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot32Opacity),
		         cockpit_parameter_writer(DcsIds::CockpitParams::RadarDisplayPlot32SourceSlot) } };
}

void RadarDisplayExporter::reset()
{
	for (auto& slot : slots_)
		slot.reset();
	for (auto& plot : plots_)
	{
		plot.x.reset();
		plot.y.reset();
		plot.opacity.reset();
		plot.source_slot.reset();
	}
}

CockpitParameterEvents RadarDisplayExporter::export_snapshot(const Core::RadarDisplaySnapshot& s)
{
	bool complete = true;
	std::optional<CockpitParameterEvent> error, recovery;
	const auto write_endpoint = [&](CockpitParameterEndpoint& endpoint, double value)
	{
		const auto result = endpoint.write_number(api_, value);
		complete = complete && result.success;
		if (result.event)
		{
			auto& first = result.event->type == CockpitParameterEventType::Error ? error : recovery;
			if (!first)
				first = result.event;
		}
	};
	const auto write = [&](Parameter parameter, double value)
	{
		write_endpoint(slots_[static_cast<std::size_t>(parameter)], value);
	};
	// 以可用旗標包住整批圖形更新，完成後才讓指示器顯示。
	write(Parameter::Available, 0.0);
	write(Parameter::Visible, s.visible);
	write(Parameter::SensorAvailable, s.sensor_available);
	write(Parameter::CueKind, static_cast<double>(s.cue));
	write(Parameter::CueVisible, s.cue_visible);
	write(Parameter::BoreHalfWidthMrad, s.bore_half_width_mrad);
	write(Parameter::BoreHalfHeightMrad, s.bore_half_height_mrad);
	write(Parameter::TargetXMrad, s.target_x_mrad);
	write(Parameter::TargetYMrad, s.target_y_mrad);
	write(Parameter::RangeScaleM, s.range_scale_m);
	write(Parameter::RangeScaleNm, s.range_scale_m / 1852.0);
	write(Parameter::ScanLeft, s.scan_left_normalized);
	write(Parameter::ScanRight, s.scan_right_normalized);
	write(Parameter::SweepVisible, s.estimated_sweep_visible);
	write(Parameter::SweepX, s.estimated_sweep_x_normalized);
	write(Parameter::CursorVisible, s.cursor_visible);
	write(Parameter::CursorX, s.cursor_x_normalized);
	write(Parameter::CursorY, s.cursor_y_normalized);
	write(Parameter::TargetVisible, s.target_visible);
	write(Parameter::TargetX, s.target_x_normalized);
	write(Parameter::TargetY, s.target_y_normalized);
	for (std::size_t index = 0; index < Core::kRadarDisplayPlotCount; ++index)
	{
		const Core::RadarDisplayPlot empty;
		const auto& plot = index < s.plots.size() ? s.plots[index] : empty;
		auto& endpoints = plots_[index];
		write_endpoint(endpoints.x, plot.x_normalized);
		write_endpoint(endpoints.y, plot.y_normalized);
		write_endpoint(endpoints.opacity, plot.opacity_0_1);
		write_endpoint(endpoints.source_slot, static_cast<double>(plot.source_slot));
	}
	if (complete)
		write(Parameter::Available, 1.0);
	CockpitParameterEvents events;
	// 同一批的首個錯誤與恢復供診斷定位，圖形整批有效性由 Available 表達。
	if (error)
		events = events.with_event(*error);
	if (recovery)
		events = events.with_event(*recovery);
	return events;
}
}
