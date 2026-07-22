#pragma once

#include "EventLog.h"
#include "OutputStore.h"
#include "../../Core/Fck1cEfm.h"

namespace DcsBridge
{
namespace Internal
{
struct CallbackContext
{
	const char* name;
	const char* parameter_name = nullptr;
	long long parameter_value = 0;
};

class EfmEventReporter final
{
public:
	EfmEventReporter(EventLog& event_log, const OutputStore& output_store);

	void log_callback_lifecycle_error(
		const CallbackContext& context,
		const char* state);
	void log_unavailable_output(const CallbackContext& context);
	void log_invalid_frame_dt(double dt_s);
	void log_start(Core::StartMode mode, double simulation_time_s);
	void log_damage(
		const Core::Fck1cEfmSnapshot& snapshot,
		int element,
		double integrity);
	void log_suspension_feedback_error(int index, bool null_info);
	void log_configure(const char* config_path);
	void log_release(const std::optional<double>& simulation_time_s);

private:
	std::optional<double> latest_simulation_time() const;
	void write(
		EventLevel level,
		const std::optional<double>& simulation_time_s,
		const char* message);

	EventLog& event_log_;
	const OutputStore& output_store_;
};
}
}
