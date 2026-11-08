#include "StationSelection.h"

#include "../../../Common/StoresQueries.h"

#include <algorithm>
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
	if (mode != mode_ || mode == OperatingMode::Navigation)
	{
		reset_selection_ = true;
		pending_cycles_ = 0;
	}
	else if (cycle_weapon)
	{
		if (pending_cycles_ == kMaximumExactCockpitInteger)
			throw std::overflow_error("MC pending weapon cycles exhausted.");
		++pending_cycles_;
	}
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
	requested_load_revision_ = station ? inventory_.stations[*station].load_revision : 0;
}

bool StationSelection::update(const WeaponStationObservation& observation)
{
	const auto previous = state_;
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
			           static_cast<std::ptrdiff_t>((offset + pending_cycles_ % candidates.size()) % candidates.size());
		}
		const std::optional<int> station = selected == candidates.end() ? std::nullopt : std::optional<int>(*selected);
		const bool rearmed = rearm_revision_ != inventory_.rearm_revision;
		const bool replaced = station && *station == state_.requested_station &&
		                      requested_load_revision_ != inventory_.stations[*station].load_revision;
		request(station, reset_selection_ || pending_cycles_ != 0 || rearmed || replaced);
		reset_selection_ = false;
		pending_cycles_ = 0;
		rearm_revision_ = inventory_.rearm_revision;
	}

	// 執行確認與本次要求編號、掛點相符時，保存最近完成的要求。
	if (inventory_.observation.available && observation.status.available && state_.request_revision != 0 &&
	    observation.selection_ack_revision == state_.request_revision &&
	    observation.selected_station == state_.requested_station)
	{
		state_.acknowledged_revision = observation.selection_ack_revision;
		state_.acknowledged_station = observation.selected_station;
	}
	return previous.request_revision != state_.request_revision ||
	       previous.acknowledged_revision != state_.acknowledged_revision;
}
}
