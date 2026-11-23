#include "InfraredDisplayExporter.h"
#include "../../DcsIds/CockpitParams.g.h"
namespace DcsBridge::Internal
{
InfraredDisplayExporter::InfraredDisplayExporter(cockpit_param_api api)
    : api_(api),
      parameters_{ { cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayAvailable),
	                 cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayDesiredVisible),
	                 cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayDesiredX),
	                 cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayDesiredY),
	                 cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayTargetVisible),
	                 cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayTargetX),
	                 cockpit_parameter_writer(DcsIds::CockpitParams::InfraredDisplayTargetY) } }
{
}
void InfraredDisplayExporter::reset()
{
	for (auto& parameter : parameters_)
		parameter.reset();
}
CockpitParameterEvents InfraredDisplayExporter::export_snapshot(const Core::InfraredDisplaySnapshot& snapshot)
{
	const std::array<double, 7> values{ 0.0,
		                                static_cast<double>(snapshot.desired_visible),
		                                snapshot.desired_x_mrad,
		                                snapshot.desired_y_mrad,
		                                static_cast<double>(snapshot.target_visible),
		                                snapshot.target_x_mrad,
		                                snapshot.target_y_mrad };
	CockpitParameterEvents events;
	bool complete = true;
	for (std::size_t i = 0; i < values.size(); ++i)
	{
		const auto result = parameters_[i].write_number(api_, values[i]);
		complete = complete && result.success;
		if (result.event)
			events = events.with_event(*result.event);
	}
	if (complete)
	{
		const auto result = parameters_[0].write_number(api_, 1.0);
		if (result.event)
			events = events.with_event(*result.event);
	}
	return events;
}
}
