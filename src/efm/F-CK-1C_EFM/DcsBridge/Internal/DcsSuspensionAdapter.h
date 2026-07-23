#pragma once

#include "../../Core/FrameContracts.h"
#include "../../include/FM/wHumanCustomPhysicsAPI.h"

namespace DcsBridge
{
namespace Internal
{
inline Core::SuspensionFeedbackInput make_suspension_feedback(
	int index,
	const ed_fm_suspension_info& info)
{
	return {
		index,
		{ info.acting_force[0], info.acting_force[1], info.acting_force[2] },
		{ info.acting_force_point[0], info.acting_force_point[1],
			info.acting_force_point[2] },
		info.integrity_factor,
		info.struct_compression,
		info.wheel_speed_X
	};
}
}
}
