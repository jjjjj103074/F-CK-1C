#pragma once

#include "DrawArgs.h"
#include "ParamExport.h"
#include "../Core/FrameContracts.h"

namespace DcsBridge
{
DrawArgState make_draw_arg_state(const Core::FrameOutput& output);
ParamExportState make_param_export_state(const Core::FrameOutput& output);
}
