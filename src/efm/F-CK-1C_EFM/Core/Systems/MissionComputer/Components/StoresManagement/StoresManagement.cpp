#include "StoresManagement.h"

#include "Internal/InventoryProcessing.h"
#include "../../Common/StoresQueries.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input,
                                   StoresData& stores,
                                   ExternalActionQueue& actions,
                                   StoresEvents& events)
    : input_(input),
      stores_(stores),
      actions_(actions),
      events_(events)
{
}

void StoresManagement::request_station(std::optional<int> station, bool force)
{
	if (station && (*station < 0 || *station >= static_cast<int>(kWeaponStationCount)))
		throw std::out_of_range("MC station request is outside the aircraft inventory.");
	if (!force && stores_.requested_station == station)
		return;
	if (stores_.selection_request_revision == kMaximumExactCockpitInteger)
		throw std::overflow_error("MC station request revision exhausted its exact numeric range.");
	stores_.requested_station = station;
	++stores_.selection_request_revision;
}

InventoryChanged StoresManagement::update_inventory(std::chrono::nanoseconds time)
{
	const auto& observation = input_.cockpit.weapon_stations; // 獲得武器掛點觀測
	auto& inventory = stores_.inventory; // 獲得掛點庫存
	const bool new_batch = // 判斷是否為新批次觀測
	    observation.status.available &&
	    (!inventory.last_observed_at || observation.status.revision != inventory.last_observed_revision);
	const bool rearmed =
	    observation.status.available && observation.rearm_revision != inventory.rearm_revision; //判斷是否重新掛彈

	InventoryChanged changes; // 庫存變更
	changes.rearmed = rearmed;

	// 每次都處理觀測內容；同批資料沿用先前的有效觀測時間。
	const auto observed_at = new_batch ? time : inventory.last_observed_at.value_or(time);
	for (std::size_t i = 0; i < inventory.stations.size(); ++i)
		changes.changed_stations[i] = Internal::update_station(inventory.stations[i],
		                                                       observation.stations[i],
		                                                       observation.status,
		                                                       observed_at,
		                                                       rearmed);

	// 更新觀測狀態與時間
	inventory.observation = observation.status;
	if (observation.status.available)
	{
		inventory.last_observed_at = observed_at;
		inventory.last_observed_revision = observation.status.revision;
		inventory.rearm_revision = observation.rearm_revision;
	}

	// 更新庫存版本
	if (changes.changed_stations.any() || rearmed)
	{
		if (inventory.revision == (std::numeric_limits<std::uint64_t>::max)())
			throw std::overflow_error("MC inventory revision exhausted.");
		++inventory.revision;
	}

	// 回傳掛載變更
	changes.revision = inventory.revision;
	return changes;
}

bool StoresManagement::update_gun_quantity(std::chrono::nanoseconds time)
{
	auto& gun = stores_.gun_ammunition;
	const auto& sample = input_.cockpit.gun_ammunition;
	const auto previous_count = gun.count;
	const auto previous_source = gun.source;
	const auto previous_status = gun.observation;
	gun.observation = sample.status;
	if (sample.status.available && (sample.count < 0 || sample.count > 9999))
		gun.observation = { false, sample.status.revision, ObservationInvalidReason::InvalidNumeric };
	if (gun.observation.available && sample.status.revision == 0)
		gun.observation = { false, 0, ObservationInvalidReason::InvalidRevision };
	if (gun.observation.available && (!gun.last_observed_at || sample.status.revision != gun.last_observed_revision))
	{
		gun.count = sample.count;
		gun.source = AmmunitionSource::Native;
		gun.last_observed_at = time;
		gun.last_observed_revision = sample.status.revision;
	}
	// 來源以 10 Hz 更新；半秒沿用同批資料後，保存最近數字並標記過期。
	if (gun.last_observed_at && time - *gun.last_observed_at > std::chrono::milliseconds(500))
		gun.observation = { false, sample.status.revision, ObservationInvalidReason::Stale };
	const bool changed = previous_count != gun.count || previous_source != gun.source ||
	                     previous_status.available != gun.observation.available ||
	                     previous_status.invalid_reason != gun.observation.invalid_reason;
	if (changed)
	{
		if (gun.revision == (std::numeric_limits<std::uint64_t>::max)())
			throw std::overflow_error("MC gun ammunition revision exhausted.");
		++gun.revision;
	}
	return changed;
}

void StoresManagement::update_legacy_summary()
{
	const auto& observation = input_.cockpit.weapon_stations;
	const auto& adapter = input_.cockpit.mission_adapter;
	if (observation.status.available)
	{
		stores_.missile_count = count_missiles(stores_.inventory, { WeaponRole::AirToAir, {} }).count;
		stores_.count_source = StoresCountSource::StationInventory;
		if (observation.selection_ack_revision == stores_.selection_request_revision &&
		    observation.selected_station == stores_.requested_station)
			stores_.selected_station = observation.selected_station;
	}
	else if (adapter.missile_count_available)
	{
		stores_.missile_count = (std::max)(0, adapter.missile_count);
		stores_.count_source = StoresCountSource::BaseDataApi;
	}
	else
		stores_.count_source = StoresCountSource::Estimate;
}

void StoresManagement::step(std::chrono::nanoseconds time, double)
{
	time = (std::max)(time, input_.observed_at); // 獲得觀測時間
	const auto changes = update_inventory(time); // 更新掛點庫存
	const bool gun_changed = update_gun_quantity(time); // 更新機砲彈量
	update_legacy_summary(); // 更新選站確認
	// 如果掛載有變化發送事件
	if (changes.changed_stations.any() || changes.rearmed)
		events_.inventory_changed(changes);
	if (gun_changed)
		events_.gun_ammunition_changed(GunAmmunitionChanged{ stores_.gun_ammunition.revision });
}

void StoresManagement::request_weapon_cycle()
{
	actions_.append(MissionActionKind::CycleWeapon);
}

void StoresManagement::record_missile_release()
{
	if (stores_.count_source == StoresCountSource::Estimate)
		stores_.missile_count = (std::max)(0, stores_.missile_count - 1);
}
}
