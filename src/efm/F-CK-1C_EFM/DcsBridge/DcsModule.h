#pragma once

#include "Internal/FrameInputCollector.h"
#include "../Core/Fck1cEfm.h"

namespace DcsBridge
{
class DcsRuntime;

Core::Fck1cEfm& efm();
DcsRuntime& runtime();
Internal::FrameInputCollector& input_collector();
}
