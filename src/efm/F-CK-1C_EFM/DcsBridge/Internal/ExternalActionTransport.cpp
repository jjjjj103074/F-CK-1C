#include "ExternalActionTransport.h"
#include <algorithm>
#include <stdexcept>

namespace DcsBridge::Internal
{
void ExternalActionTransport::reset()
{
	pending_.clear();
	feedback_ = {};
	last_wire_sequence_ = 0;
	acknowledged_wire_ = 0;
	published_through_ = 0;
	collected_mission_ = 0;
	collected_radar_ = 0;
}

void ExternalActionTransport::append(Core::MissionAction action,
                                     std::uint64_t mission_source,
                                     std::uint64_t mission_complete,
                                     std::uint64_t radar_source)
{
	if (last_wire_sequence_ == Core::kMaximumExactCockpitInteger)
		throw std::overflow_error("DCS action sequence exhausted.");
	pending_.push_back({ ++last_wire_sequence_, action, mission_source, mission_complete, radar_source });
}

void ExternalActionTransport::acknowledge(std::uint64_t sequence)
{
	if (sequence <= acknowledged_wire_ || sequence > published_through_)
		return;
	while (!pending_.empty() && pending_.front().wire_sequence <= sequence)
	{
		const auto& delivery = pending_.front();
		if (delivery.mission_complete)
			feedback_.mission_acknowledged_through = delivery.mission_complete;
		if (delivery.radar_source)
			feedback_.radar_acknowledged_through = delivery.radar_source;
		pending_.pop_front();
	}
	acknowledged_wire_ = sequence;
}

Core::ExternalActionFeedback ExternalActionTransport::feedback() const noexcept
{
	return feedback_;
}

const Core::ExternalActionRequest* ExternalActionTransport::next_radar_action(const Core::RadarSnapshot& radar) const
{
	const auto& items = radar.action_requests.items;
	const auto it = std::find_if(items.begin(),
	                             items.end(),
	                             [&](const auto& entry)
	                             {
		                             return entry.sequence == collected_radar_ + 1;
	                             });
	return it == items.end() ? nullptr : &*it;
}

void ExternalActionTransport::collect_background(const Core::RadarSnapshot& radar)
{
	while (const auto* action = next_radar_action(radar))
	{
		if (action->parent_request != 0)
			break;
		append(action->action, 0, 0, action->sequence);
		collected_radar_ = action->sequence;
	}
}

bool ExternalActionTransport::collect_radar_result(std::uint64_t request, const Core::RadarSnapshot& radar)
{
	const auto found = std::find_if(radar.command_results.begin(),
	                                radar.command_results.end(),
	                                [&](const auto& result)
	                                {
		                                return result.request_sequence == request;
	                                });
	if (found == radar.command_results.end())
		return false;
	collect_background(radar);
	if (found->first_action_sequence == 0)
	{
		// 空操作以傳送中的佔位完成來源要求，保留既有順序與確認進度。
		append({}, request, request, 0);
		return true;
	}
	if (found->first_action_sequence != collected_radar_ + 1 ||
	    found->last_action_sequence < found->first_action_sequence)
		throw std::logic_error("Radar operation action sequence is discontinuous.");
	for (auto sequence = found->first_action_sequence; sequence <= found->last_action_sequence; ++sequence)
	{
		const auto* action = next_radar_action(radar);
		if (!action || action->parent_request != request)
			throw std::logic_error("Radar operation action payload is missing.");
		append(action->action, request, sequence == found->last_action_sequence ? request : 0, sequence);
		collected_radar_ = sequence;
	}
	return true;
}

void ExternalActionTransport::collect(const Core::ExternalActionRequests& mission, const Core::RadarSnapshot& radar)
{
	// 來源可將尚待執行的操作撤回；已分配的傳送序號保留為空操作。
	for (auto& delivery : pending_)
	{
		if (!delivery.mission_source || delivery.radar_source)
			continue;
		const auto it = std::find_if(mission.items.begin(),
		                             mission.items.end(),
		                             [&](const auto& entry)
		                             {
			                             return entry.sequence == delivery.mission_source;
		                             });
		if (it != mission.items.end() && it->kind == Core::ExternalRequestKind::Action)
			delivery.action = it->action;
	}
	for (const auto& request : mission.items)
	{
		if (request.sequence <= collected_mission_)
			continue;
		if (request.sequence != collected_mission_ + 1)
			throw std::logic_error("MC operation request sequence is discontinuous.");
		if (request.kind == Core::ExternalRequestKind::RadarOperation)
		{
			if (!collect_radar_result(request.sequence, radar))
				break;
		}
		else
			append(request.action, request.sequence, request.sequence, 0);
		collected_mission_ = request.sequence;
	}
	collect_background(radar);
}

Core::MissionActionBatch ExternalActionTransport::batch()
{
	Core::MissionActionBatch result;
	for (const auto& delivery : pending_)
	{
		if (result.count == result.actions.size())
			break;
		result.actions[result.count++] = delivery.action;
	}
	result.first_sequence = result.count == 0 ? 0 : pending_.front().wire_sequence;
	if (result.count)
		published_through_ = result.first_sequence + result.count - 1;
	return result;
}
}
