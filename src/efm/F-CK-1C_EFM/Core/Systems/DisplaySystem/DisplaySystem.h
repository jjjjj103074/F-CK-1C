#pragma once

#include "../System.h"

namespace Core::Systems
{
/// @brief 依任務模式配置左右 MFD 功能，經 Pipeline 發布控制來源對應。
class DisplaySystem final : public System
{
public:
	void setup(SystemSetup& setup) override;
	void step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result) override;

private:
	DisplaySnapshot output_;
};
}
