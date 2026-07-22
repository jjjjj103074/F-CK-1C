#pragma once

#include <cstdio>
#include <limits>

// Formatting helpers used only by the DCSBridge event reporter and its tests.
namespace Diagnostics
{
struct DiagnosticOutput
{
	char* data = nullptr;
	size_t capacity = 0;
};

struct DamageEventSnapshot
{
	int element = 0;
	double integrity = 0.0;
	bool invincible = false;
};

struct CallbackLifecycleError
{
	const char* callback = nullptr;
	const char* state = nullptr;
	const char* parameter_name = nullptr;
	long long parameter_value = 0;
};

struct InvalidFrameDtError
{
	double dt_s = 0.0;
};

struct SuspensionFeedbackError
{
	int index = 0;
	bool null_info = false;
};

inline void format_damage_event(
	const DiagnosticOutput& output,
	const DamageEventSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"damage element=%d integrity=%.3f invincible=%d",
		snapshot.element,
		snapshot.integrity,
		snapshot.invincible ? 1 : 0);
}

inline void format_callback_lifecycle_error(
	const DiagnosticOutput& output,
	const CallbackLifecycleError& error)
{
	if (error.parameter_name != nullptr)
	{
		snprintf(
			output.data,
			output.capacity,
			"callback=%s %s=%lld invalid lifecycle state=%s",
			error.callback,
			error.parameter_name,
			error.parameter_value,
			error.state);
		return;
	}
	snprintf(
		output.data,
		output.capacity,
		"callback=%s invalid lifecycle state=%s",
		error.callback,
		error.state);
}

inline void format_invalid_frame_dt_error(
	const DiagnosticOutput& output,
	const InvalidFrameDtError& error)
{
	constexpr int kRoundTripDoubleDigits =
		std::numeric_limits<double>::max_digits10;
	snprintf(
		output.data,
		output.capacity,
		"callback=ed_fm_simulate invalid dt=%.*g",
		kRoundTripDoubleDigits,
		error.dt_s);
}

inline void format_suspension_feedback_error(
	const DiagnosticOutput& output,
	const SuspensionFeedbackError& error)
{
	snprintf(
		output.data,
		output.capacity,
		"callback=ed_fm_suspension_feedback index=%d info_null=%s",
		error.index,
		error.null_info ? "true" : "false");
}
}
