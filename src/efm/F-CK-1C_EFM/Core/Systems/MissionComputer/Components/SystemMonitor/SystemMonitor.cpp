#include "SystemMonitor.h"

namespace Core::Systems::Mc
{
SystemMonitor::SystemMonitor(const InputOutputData& input_output, HealthStatus& health)
    : input_output_(input_output),
      health_(health)
{
}

void SystemMonitor::step(std::chrono::nanoseconds scheduled_time, double dt_s)
{
	(void)scheduled_time;
	(void)dt_s;
}
}
