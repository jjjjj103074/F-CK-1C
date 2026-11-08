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

void StationSelection::select_mode(OperatingMode mode, bool cycle_weapon)
{
	reset_selection_ = mode != mode_ || mode == OperatingMode::Navigation;
	cycle_requested_ = !reset_selection_ && cycle_weapon;
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

bool StationSelection::update(const WeaponStationObservation& observation)
{
	const auto previous = state_;
	acknowledge(observation);
	if (mode_ == OperatingMode::Navigation)
	{
		request(std::nullopt);
		reset_selection_ = false;
	}
	else if (inventory_.observation.available)
	{
		const auto candidates = find_missile_stations(inventory_, { WeaponRole::AirToAir, {} });
		auto selected = std::find(candidates.begin(), candidates.end(), state_.requested_station.value_or(-1));
		if (reset_selection_ || selected == candidates.end())
			selected = candidates.begin();
		if (!candidates.empty())
		{
			const auto offset = static_cast<std::uint64_t>(selected - candidates.begin());
			selected = candidates.begin() +
			           static_cast<std::ptrdiff_t>((offset + (cycle_requested_ ? 1 : 0)) % candidates.size());
		}
		const std::optional<int> station = selected == candidates.end() ? std::nullopt : std::optional<int>(*selected);
		const bool rearmed = rearm_revision_ != inventory_.rearm_revision;
		const bool replaced = station && *station == state_.requested_station &&
		                      requested_load_revision_ != inventory_.stations[*station].load_revision;
		request(station, reset_selection_ || cycle_requested_ || rearmed || replaced);
		reset_selection_ = false;
		cycle_requested_ = false;
		rearm_revision_ = inventory_.rearm_revision;
	}

	return previous.request_revision != state_.request_revision ||
	       previous.acknowledged_revision != state_.acknowledged_revision;
}
}
