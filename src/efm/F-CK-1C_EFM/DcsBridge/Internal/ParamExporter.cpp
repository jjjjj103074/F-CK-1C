#include "ParamExporter.h"

#include "EfmEventReporter.h"

namespace DcsBridge
{
namespace Internal
{
ParamExporter::ParamExporter(EfmEventReporter& event_reporter)
	: event_reporter_(event_reporter)
{
}

void ParamExporter::reset()
{
	const std::lock_guard<std::mutex> lock(mutex_);
	availability_history_ = {};
}

void ParamExporter::observe(const Core::FrameOutput& output)
{
	const std::lock_guard<std::mutex> lock(mutex_);
	observe_param_export_availability(
		availability_history_, output.availability);
}

double ParamExporter::read(unsigned index, const Core::FrameOutput& output)
{
	const std::lock_guard<std::mutex> lock(mutex_);
	const ParamExportResult result =
		resolve_param(index, output, availability_history_);
	observe_param_export_availability(
		availability_history_, output.availability);
	switch (result.status)
	{
	case ParamExportStatus::Value:
	case ParamExportStatus::StartCompatibility:
		return result.value;
	case ParamExportStatus::MissingRuntimeData:
		event_reporter_.log_missing_param_data(
			index, param_data_category_name(*result.missing_data));
		return result.value;
	case ParamExportStatus::Unknown:
		event_reporter_.log_missing_param(index);
		return result.value;
	}
	event_reporter_.log_missing_param_data(
		index, "invalid_param_export_status");
	return 0.0;
}
}
}
