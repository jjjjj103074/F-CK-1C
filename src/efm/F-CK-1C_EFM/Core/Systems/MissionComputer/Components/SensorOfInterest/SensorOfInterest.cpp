#include "SensorOfInterest.h"

#include "Common/CommandValue.h"

namespace Core::Systems::Mc
{
SensorOfInterest::SensorOfInterest(const ModeData& mode,
                                   const DisplaySnapshot& display,
                                   SensorOfInterestSnapshot& selection)
    : mode_(mode),
      display_(display),
      selection_(selection)
{
}

SensorControlSource SensorOfInterest::source_at(DisplayLocation location) const
{
	switch (location)
	{
	case DisplayLocation::LeftMfd:
		return display_.left.control_source;
	case DisplayLocation::RightMfd:
		return display_.right.control_source;
	case DisplayLocation::Hud:
		return display_.hud_designation_available ? SensorControlSource::Hud : SensorControlSource::None;
	default:
		return SensorControlSource::None;
	}
}

bool SensorOfInterest::allowed(SensorControlSource source) const
{
	return source != SensorControlSource::None && (!mode_.air_to_air() || source == SensorControlSource::Radar);
}

bool SensorOfInterest::select(DisplayLocation location)
{
	const auto source = source_at(location);
	if (!allowed(source))
		return false;
	selection_ = { source, location, true };
	return true;
}

void SensorOfInterest::select_mfd()
{
	// 優先選取顯示中的雷達，再選取其餘可用任務來源。
	for (auto source : { SensorControlSource::Radar, SensorControlSource::Navigation })
	{
		for (auto location : { DisplayLocation::LeftMfd, DisplayLocation::RightMfd })
			if (source_at(location) == source && select(location))
				return;
	}
}

void SensorOfInterest::refresh()
{
	if (!display_.status.available || display_.operating_mode != mode_.operating_mode())
	{
		selection_ = {};
		return;
	}
	selection_.display_context_ready = true;
	if (allowed(selection_.source))
	{
		// 來源隨頁面交換移動，維持既有的任務操作對象。
		for (auto location :
		     { selection_.location, DisplayLocation::LeftMfd, DisplayLocation::RightMfd, DisplayLocation::Hud })
			if (source_at(location) == selection_.source && select(location))
				return;
	}
	selection_.source = SensorControlSource::None;
	selection_.location = DisplayLocation::None;
	for (auto location : { DisplayLocation::LeftMfd, DisplayLocation::RightMfd })
		if (source_at(location) == SensorControlSource::Radar && select(location))
			return;
	if (!select(DisplayLocation::Hud))
		select_mfd();
}

void SensorOfInterest::handle(const Command& command)
{
	if (!selection_.display_context_ready || !Common::command_value_is_pressed(command.value_normalized))
		return;
	if (command.id == CommandId::PressDmsUp)
		select(DisplayLocation::Hud);
	else if (command.id == CommandId::PressDmsDown)
	{
		if (selection_.location == DisplayLocation::LeftMfd)
			select(DisplayLocation::RightMfd);
		else if (selection_.location == DisplayLocation::RightMfd)
			select(DisplayLocation::LeftMfd);
		else
			select_mfd();
	}
}
}
