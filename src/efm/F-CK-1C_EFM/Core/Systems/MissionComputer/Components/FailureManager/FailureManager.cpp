#include "FailureManager.h"

namespace Core::Systems::Mc
{
FailureManager::FailureManager(const HealthStatus& health, FailureResponse& response)
    : health_(health),
      response_(response)
{
}

void FailureManager::step(std::chrono::nanoseconds scheduled_time, double dt_s)
{
	(void)scheduled_time;
	(void)dt_s;
}
}
