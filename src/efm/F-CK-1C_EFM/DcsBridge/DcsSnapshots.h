#pragma once

#include "DrawArgs.h"
#include "ParamExport.h"
#include "../Core/Fck1cEfm.h"
#include "../Diagnostics/DebugWatch.h"

namespace DcsBridge
{
DrawArgState make_draw_arg_state(const Core::Fck1cEfmSnapshot& snapshot);
ParamExportState make_param_export_state(const Core::Fck1cEfmSnapshot& snapshot);
Diagnostics::DebugWatchSnapshot make_debug_watch_snapshot(
	const Core::Fck1cEfmSnapshot& snapshot,
	const char* version,
	const char* version_date);
}
