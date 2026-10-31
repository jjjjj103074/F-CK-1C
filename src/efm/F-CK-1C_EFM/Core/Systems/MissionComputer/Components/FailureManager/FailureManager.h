#pragma once

#include <chrono>

namespace Core::Systems::Mc
{
struct HealthStatus;
struct FailureResponse;

/// @brief 綁定健康與處置分區，提供故障處置的排程入口。
class FailureManager final
{
public:
	FailureManager(const HealthStatus& health, FailureResponse& response);
	/// @brief 接收排程時間與週期。
	void step(std::chrono::nanoseconds scheduled_time, double dt_s);

private:
	const HealthStatus& health_;
	FailureResponse& response_;
};
}
