#include "NativeCountermeasureIntegration.h"
#include "../BridgeContext.h"
#include "../EventLog.h"
#include "../../../DcsIds/CockpitParams.g.h"
#include <cstdio>

namespace DcsBridge::Internal
{
bool handle_native_countermeasure_command(BridgeContext& context, int command, float value)
{
	return context.native_countermeasure_probe().handle_command(command, value);
}
void report_native_countermeasure_probe(EventLog& log,
                                        cockpit_param_api api,
                                        const NativeCmProbeSnapshot& s,
                                        const char* reason)
{
	char message[640] = {};
	std::snprintf(
	    message,
	    sizeof(message),
	    "native_cm_probe reason=%s source=ed_fm_simulate enabled=%u api_mask=%u status=%d requests=%llu calls=%llu last_request=%llu method=%d kind=%d input_thread=%u execution_thread=%u exception=0x%08x",
	    reason,
	    s.enabled ? 1U : 0U,
	    s.api_mask,
	    static_cast<int>(s.status),
	    static_cast<unsigned long long>(s.requests),
	    static_cast<unsigned long long>(s.calls),
	    static_cast<unsigned long long>(s.last_request),
	    static_cast<int>(s.method),
	    static_cast<int>(s.kind),
	    s.request_thread,
	    s.execute_thread,
	    s.exception);
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
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmMethod), static_cast<double>(s.method) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmKind), static_cast<double>(s.kind) },
		{ cockpit_parameter_writer(DcsIds::CockpitParams::NativeCmException), static_cast<double>(s.exception) }
	};
	for (auto& entry : values)
		(void)entry.parameter.write_number(api, entry.number);
}
}
