#pragma once

#include "Internal/EventLog.h"
#include "Internal/EfmEventReporter.h"
#include "Internal/FrameInputCollector.h"
#include "Internal/OutputStore.h"
#include "Internal/StateCsvWriter.h"
#include "../Core/Fck1cEfm.h"

#include <mutex>

namespace DcsBridge
{
class DcsRuntime;

Core::Fck1cEfm& efm();
DcsRuntime& runtime();
Internal::EventLog& event_log();
Internal::EfmEventReporter& event_reporter();
Internal::FrameInputCollector& input_collector();
Internal::OutputStore& output_store();
Internal::StateCsvWriter& state_csv_writer();
std::mutex& execution_mutex();
void configure_module(const char* config_path);
}
