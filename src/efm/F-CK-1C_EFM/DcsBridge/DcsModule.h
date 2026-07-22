#pragma once

#include "Internal/FrameInputCollector.h"
#include "Internal/OutputStore.h"
#include "../Core/Fck1cEfm.h"

#include <mutex>

namespace DcsBridge
{
class DcsRuntime;

Core::Fck1cEfm& efm();
DcsRuntime& runtime();
Internal::FrameInputCollector& input_collector();
Internal::OutputStore& output_store();
std::mutex& execution_mutex();
}
