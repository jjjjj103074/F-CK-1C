#pragma once

#include "DrawArgs.h"
#include "ParamExport.h"
#include "../Core/Fck1cEfm.h"

namespace DcsBridge
{
DrawArgState make_draw_arg_state(const Core::Fck1cEfmSnapshot& snapshot);
ParamExportState make_param_export_state(const Core::Fck1cEfmSnapshot& snapshot);
}
