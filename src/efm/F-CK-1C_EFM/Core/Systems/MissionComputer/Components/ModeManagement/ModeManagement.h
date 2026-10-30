#pragma once

#include "../FireControl/FireControl.h"

namespace Core::Systems::Mc
{
/// @brief 保存任務模式與 Master Arm，通知火控處理影響；CMS 資格暫留原行為。
class ModeManagement final
{
public:
	ModeManagement(ModeData& mode, FireControl& fire_control, ExternalActionQueue& actions);
	void handle(const Command& command);

private:
	ModeData& mode_;
	FireControl& fire_control_;
	ExternalActionQueue& actions_;
};
}
