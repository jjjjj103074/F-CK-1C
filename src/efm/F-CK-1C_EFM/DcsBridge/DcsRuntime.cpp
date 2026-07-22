#include "DcsRuntime.h"

#include "Internal/FrameInputCollector.h"

namespace DcsBridge
{
bool DcsRuntime::publish_suspension_feedback(
	Internal::FrameInputCollector& collector,
	int index,
	const ed_fm_suspension_info* info)
{
	if (info == nullptr)
	{
		return false;
	}
	const Core::SuspensionFeedbackInput feedback = {
		index,
		Common::Vec3(
			info->acting_force[0],
			info->acting_force[1],
			info->acting_force[2]),
		Common::Vec3(
			info->acting_force_point[0],
			info->acting_force_point[1],
			info->acting_force_point[2]),
		info->integrity_factor,
		info->struct_compression,
		info->wheel_speed_X
	};
	return collector.publish_suspension(feedback);
}
}
