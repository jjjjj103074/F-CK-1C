#pragma once

#include "../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsBridge
{
namespace Internal
{
class FrameInputCollector;
}

class DcsRuntime final
{
public:
	bool publish_suspension_feedback(
		Internal::FrameInputCollector& collector,
		int index,
		const ed_fm_suspension_info* info);
};
}
