#pragma once
#include "NativeCountermeasureProbe.h"
#include "../CockpitParameterEndpoint.h"

namespace Core
{
struct FrameOutput;
}

namespace DcsBridge::Internal
{
class BridgeContext;
class EventLog;
/// @brief 將測試鍵交給原生反制驗證入口。
bool handle_native_countermeasure_command(BridgeContext& context, int command, float value);
/// @brief 在 Core 執行鎖解除後，以完成快照的主保險處理原生派送。
void step_native_countermeasure_probe(BridgeContext& context, const Core::FrameOutput& output);
/// @brief 發布驗證狀態與原生呼叫紀錄。
void report_native_countermeasure_probe(EventLog& log,
                                        cockpit_param_api api,
                                        const NativeCmProbeSnapshot& snapshot,
                                        const char* reason);
}
