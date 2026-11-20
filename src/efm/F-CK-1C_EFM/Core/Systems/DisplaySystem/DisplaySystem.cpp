#include "DisplaySystem.h"
#include "Internal/RadarPresentation.h"

#include "../SystemPipeline.h"
#include "../SystemUpdateRates.h"
#include "Common/CommandValue.h"

namespace Core::Systems
{
void DisplaySystem::setup(SystemSetup& setup)
{
	setup.update_rate_hz(kMissionComputerProjectUpdateRateHz);
	setup.read(AircraftDataKeys::kMissionComputerSnapshot);
	setup.read(AircraftDataKeys::kRadarSnapshot);
	setup.publish(AircraftDataKeys::kDisplaySnapshot, output_);
	for (CommandId id : { CommandId::PressDmsLeft, CommandId::PressDmsRight })
	{
		setup.register_command_handler(id,
		                               [this](const SystemActionContext&, const Command& command)
		                               {
			                               if (Common::command_value_is_pressed(command.value_normalized))
				                               output_.last_format_step = command.id == CommandId::PressDmsLeft
				                                                              ? DisplayLocation::LeftMfd
				                                                              : DisplayLocation::RightMfd;
		                               });
	}
}

void DisplaySystem::step(const SystemStepContext& context, const AircraftDataView& aircraft, SystemResult& result)
{
	const auto& mission = aircraft.read(AircraftDataKeys::kMissionComputerSnapshot);
	if (mission.status.available)
	{
		output_.operating_mode = mission.mode.operating_mode;
		// 集中保存本機固定頁面配置，後續頁面選擇沿用同一份對應資料。
		output_.left = output_.operating_mode == OperatingMode::Navigation
		                   ? DisplayAssignment{ DisplayFunction::HorizontalSituation, SensorControlSource::Navigation }
		                   : DisplayAssignment{ DisplayFunction::FireControlRadar, SensorControlSource::Radar };
		output_.right = { DisplayFunction::StoresManagement, SensorControlSource::None };
		output_.hud_designation_available = !is_air_to_air_mode(output_.operating_mode);
		output_.status = { true, output_.status.revision + 1 };
	}
	output_.radar = DisplayPresentation::radar(aircraft.read(AircraftDataKeys::kRadarSnapshot),
	                                           output_.operating_mode,
	                                           context.input_time);
	result.publish(AircraftDataKeys::kDisplaySnapshot, output_);
}
}
