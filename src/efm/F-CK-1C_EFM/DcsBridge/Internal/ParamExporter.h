#pragma once

#include "ParamExport.h"

#include <mutex>

namespace DcsBridge
{
namespace Internal
{
class EfmEventReporter;

class ParamExporter final
{
public:
	explicit ParamExporter(EfmEventReporter& event_reporter);

	ParamExporter(const ParamExporter&) = delete;
	ParamExporter& operator=(const ParamExporter&) = delete;

	void reset();
	void observe(const Core::FrameOutput& output);
	double read(unsigned index, const Core::FrameOutput& output);

private:
	EfmEventReporter& event_reporter_;
	std::mutex mutex_;
	ParamExportAvailabilityHistory availability_history_;
};
}
}
