#include "StoresManagement.h"

#include "Internal/InventoryProcessing.h"
#include "../../Common/StoresQueries.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace
{
constexpr double kFallbackGunRoundsPerSecond = 100.0;
}

namespace Core::Systems::Mc
{
StoresManagement::StoresManagement(const MissionInputs& input,
                                   const WeaponOperationData& weapon,
                                   StoresData& stores,
                                   ExternalActionQueue& actions,
                                   StoresEvents& events)
    : input_(input),
      weapon_(weapon),
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

void StoresManagement::update_gun_quantity(std::chrono::nanoseconds time)
{
	const auto& adapter = input_.cockpit.mission_adapter;
	if (adapter.gun_count_available)
		stores_.gun_count = adapter.gun_count;
	else if (weapon_.gun_requested && gun_quantity_sequence_ != 0 &&
	         gun_quantity_sequence_ == weapon_.gun_start_sequence)
	{
		// 相同射擊序號代表同一次持續射擊，依兩次更新間隔扣除彈量。
		const double elapsed_s = std::chrono::duration<double>(time - gun_quantity_time_).count();
		stores_.gun_count = (std::max)(0.0, stores_.gun_count - kFallbackGunRoundsPerSecond * elapsed_s);
	}
	gun_quantity_time_ = time;
	gun_quantity_sequence_ = weapon_.gun_requested ? weapon_.gun_start_sequence : 0;
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
	update_gun_quantity(time); // 更新機砲彈量
	update_legacy_summary(); // 更新選站確認
	// 如果掛載有變化發送事件
	if (changes.changed_stations.any() || changes.rearmed)
		events_.inventory_changed(changes);
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
