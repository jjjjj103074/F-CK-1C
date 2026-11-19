#include "StationSelection.h"

#include "../../../Common/StoresQueries.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace Core::Systems::Mc::Internal
{
StationSelection::StationSelection(const StoresInventorySnapshot& inventory, StationSelectionState& state)
    : inventory_(inventory),
      state_(state)
{
}

void StationSelection::select_mode(OperatingMode mode, MissileSelectionStep step, bool reassert_selection)
{
	reset_selection_ = mode != mode_ || !is_air_to_air_mode(mode);
	step_requested_ = reset_selection_ ? MissileSelectionStep::None : step;
	reassert_requested_ = reassert_selection;
	mode_ = mode;
}

void StationSelection::request(std::optional<int> station, bool force)
{
	if (!force && state_.requested_station == station)
		return;
	if (state_.request_revision == kMaximumExactCockpitInteger)
		throw std::overflow_error("MC station request revision exhausted its exact numeric range.");
	state_.requested_station = station;
	++state_.request_revision;
	requests_.push_back({ state_.request_revision, station });
	requested_load_revision_ = station ? inventory_.stations[*station].load_revision : 0;
}

bool StationSelection::acknowledge(const WeaponStationObservation& observation)
{
	const auto previous = state_.acknowledged_revision;
	// 核對已發出的要求，保存轉接層最近完成的選站。
	if (inventory_.observation.available && observation.status.available)
	{
		const auto found = std::find_if(requests_.begin(),
		                                requests_.end(),
		                                [&](const Request& request)
		                                {
			                                return request.revision == observation.selection_ack_revision &&
			                                       request.station == observation.selected_station;
		                                });
		if (found != requests_.end())
		{
			state_.acknowledged_revision = found->revision;
			state_.acknowledged_station = found->station;
			requests_.erase(requests_.begin(), std::next(found));
		}
	}
	return previous != state_.acknowledged_revision;
}

bool StationSelection::cancel_pending()
{
	if (state_.request_revision <= state_.acknowledged_revision ||
	    state_.request_revision <= state_.canceled_through_revision)
		return false;
	state_.canceled_through_revision = state_.request_revision;
	return true;
}

bool StationSelection::update(const WeaponStationObservation& observation)
{
	const auto previous = state_;
	acknowledge(observation);
	if (!is_air_to_air_mode(mode_))
	{
		selected_type_.clear();
		request(std::nullopt, reset_selection_ && state_.request_revision != state_.acknowledged_revision);
		reset_selection_ = false;
		step_requested_ = MissileSelectionStep::None;
		reassert_requested_ = false;
	}
	else if (inventory_.observation.available)
	{
		const auto selected = select_air_to_air_station(inventory_,
		                                                reset_selection_ ? std::nullopt : state_.requested_station,
		                                                reset_selection_ ? std::string_view{} : selected_type_,
		                                                step_requested_);
		const auto station = selected.station;
		selected_type_ = selected.weapon_type;
		const bool rearmed = rearm_revision_ != inventory_.rearm_revision;
		const bool replaced = station && *station == state_.requested_station &&
		                      requested_load_revision_ != inventory_.stations[*station].load_revision;
		request(station,
		        reset_selection_ || step_requested_ == MissileSelectionStep::Station || rearmed || replaced ||
		            reassert_requested_);
		reset_selection_ = false;
		step_requested_ = MissileSelectionStep::None;
		reassert_requested_ = false;
		rearm_revision_ = inventory_.rearm_revision;
	}

	return previous.request_revision != state_.request_revision ||
	       previous.acknowledged_revision != state_.acknowledged_revision;
}
}
