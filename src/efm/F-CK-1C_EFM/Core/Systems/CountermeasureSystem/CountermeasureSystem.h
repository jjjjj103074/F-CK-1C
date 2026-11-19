#pragma once
#include "../System.h"
#include "../../Contracts/CockpitContracts.h"
namespace Core::Systems
{
/// @brief 依 Pipeline 程序要求排程反制施放，發布庫存、執行進度與下一輪要求。
class CountermeasureSystem final : public System
{
public:
	void setup(SystemSetup& setup) override;
	void step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result) override;

private:
	CountermeasureSnapshot output_;
	std::chrono::nanoseconds next_due_{};
	std::chrono::nanoseconds interval_{};
	std::uint64_t last_dispense_ = 0;
};
}
