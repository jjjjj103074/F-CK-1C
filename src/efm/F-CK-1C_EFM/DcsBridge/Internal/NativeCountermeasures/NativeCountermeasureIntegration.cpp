#include "NativeCountermeasureIntegration.h"
#include "../BridgeContext.h"
#include "../EventLog.h"
#include "../../../DcsIds/CockpitParams.g.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>

namespace DcsBridge::Internal
{
void trace_native_countermeasure_command(BridgeContext& context, int command, float value, const char* stage)
{
	const auto state = context.native_countermeasure_probe().snapshot();
	if (!state.enabled)
		return;
	std::uint32_t bits = 0;
	static_assert(sizeof(value) == sizeof(bits), "DCS command values use 32-bit floats.");
	std::memcpy(&bits, &value, sizeof(bits));
	char message[512] = {};
	std::snprintf(
	    message,
	    sizeof(message),
	    "native_cm_input stage=%s callback=ed_fm_set_command command=%d value=%.*g value_bits=0x%08x finite=%u status=%d requests=%llu calls=%llu",
	    stage,
	    command,
	    std::numeric_limits<float>::max_digits10,
	    static_cast<double>(value),
	    static_cast<unsigned>(bits),
	    std::isfinite(value) ? 1U : 0U,
	    static_cast<int>(state.status),
	    static_cast<unsigned long long>(state.requests),
	    static_cast<unsigned long long>(state.calls));
	(void)context.event_log().write({ EventLevel::Info, state.time_s, message });
}
bool handle_native_countermeasure_command(BridgeContext& context, int command, float value)
{
	const bool handled = context.native_countermeasure_probe().handle_command(command, value);
	if (handled)
		trace_native_countermeasure_command(context, command, value, "probe_result");
	return handled;
}
void step_native_countermeasure_probe(BridgeContext& context, const Core::FrameOutput& output)
{
	const auto& mc = output.cockpit.combat_avionics;
	context.native_countermeasure_probe().step(output.simulation_time_s,
	                                           mc.status.available &&
	                                               mc.mode.master_arm_mode == Core::MasterArmMode::On);
}
void report_native_countermeasure_probe(EventLog& log,
                                        cockpit_param_api api,
                                        const NativeCmProbeSnapshot& s,
                                        const char* reason)
{
	char message[1400] = {};
	std::snprintf(
	    message,
	    sizeof(message),
	    "native_cm_probe reason=%s source=ed_fm_simulate interface=human_virtual enabled=%u api_mask=%u status=%d requests=%llu calls=%llu last_request=%llu kind=%d input_thread=%u execution_thread=%u exception=0x%08x samples=%llu inventory_available=%u inventory_status=%d payload_matches=%u flare=%d chaff=%d before_flare=%d before_chaff=%d context=0x%llx human=0x%llx payload=0x%llx human_payload=0x%llx flare_function=0x%llx chaff_function=0x%llx",
	    reason,
	    s.enabled ? 1U : 0U,
	    s.api_mask,
	    static_cast<int>(s.status),
	    static_cast<unsigned long long>(s.requests),
	    static_cast<unsigned long long>(s.calls),
	    static_cast<unsigned long long>(s.last_request),
	    static_cast<int>(s.kind),
	    s.request_thread,
	    s.execute_thread,
	    s.exception,
	    static_cast<unsigned long long>(s.samples),
	    s.inventory.available ? 1U : 0U,
	    static_cast<int>(s.inventory.status),
	    s.inventory.payload_matches ? 1U : 0U,
	    s.inventory.flare,
	    s.inventory.chaff,
	    s.before_flare,
	    s.before_chaff,
	    static_cast<unsigned long long>(s.inventory.context),
	    static_cast<unsigned long long>(s.inventory.human),
	    static_cast<unsigned long long>(s.inventory.payload),
	    static_cast<unsigned long long>(s.inventory.human_payload),
	    static_cast<unsigned long long>(s.inventory.flare_function),
	    static_cast<unsigned long long>(s.inventory.chaff_function));
	(void)log.write(
	    { s.status == NativeCmStatus::CallFailed ? EventLevel::Error : EventLevel::Info, s.time_s, message });
	struct Value
	{
		CockpitParameterEndpoint parameter;
		double number;
	};
	Value values[] = {
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmEnabled), s.enabled ? 1.0 : 0.0 },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmApiMask), static_cast<double>(s.api_mask) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmStatus), static_cast<double>(s.status) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmRequests), static_cast<double>(s.requests) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmCalls), static_cast<double>(s.calls) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmLastRequest), static_cast<double>(s.last_request) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmKind), static_cast<double>(s.kind) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmException), static_cast<double>(s.exception) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmInventoryAvailable),
		  s.inventory.available ? 1.0 : 0.0 },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmPayloadMatches),
		  s.inventory.payload_matches ? 1.0 : 0.0 },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmFlareCount), static_cast<double>(s.inventory.flare) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmChaffCount), static_cast<double>(s.inventory.chaff) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmSamples), static_cast<double>(s.samples) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmSourceStatus),
		  static_cast<double>(s.inventory.status) }
	};
	for (auto& entry : values)
		(void)entry.parameter.write_number(api, entry.number);
}
}
