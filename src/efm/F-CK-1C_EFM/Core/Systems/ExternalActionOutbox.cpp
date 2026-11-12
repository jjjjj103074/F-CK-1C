#include "ExternalActionOutbox.h"
#include <algorithm>
#include <stdexcept>

namespace Core::Systems
{
std::uint64_t ExternalActionOutbox::append_request(ExternalActionRequest request)
{
	if (last_sequence_ == kMaximumExactCockpitInteger)
		throw std::overflow_error("External action request sequence exhausted.");
	request.sequence = last_sequence_ + 1;
	pending_.push_back(request);
	last_sequence_ = request.sequence;
	return request.sequence;
}

std::uint64_t ExternalActionOutbox::append(MissionActionKind action, std::uint64_t parent_request)
{
	return append(MissionAction{ action }, parent_request);
}

std::uint64_t ExternalActionOutbox::append(const MissionAction& action, std::uint64_t parent_request)
{
	if (action.kind == MissionActionKind::None)
		throw std::invalid_argument("External action requires an executable operation.");
	return append_request({ 0, ExternalRequestKind::Action, action, parent_request });
}

std::uint64_t ExternalActionOutbox::reserve_radar_operation()
{
	return append_request({ 0, ExternalRequestKind::RadarOperation, {}, 0 });
}

bool ExternalActionOutbox::contains(MissionActionKind action) const noexcept
{
	return std::any_of(pending_.begin(),
	                   pending_.end(),
	                   [action](const auto& request)
	                   {
		                   return request.kind == ExternalRequestKind::Action && request.action.kind == action;
	                   });
}

void ExternalActionOutbox::acknowledge(std::uint64_t sequence)
{
	if (sequence <= acknowledged_through_ || sequence > published_through_)
		return;
	while (!pending_.empty() && pending_.front().sequence <= sequence)
		pending_.pop_front();
	acknowledged_through_ = sequence;
}

bool ExternalActionOutbox::acknowledged(std::uint64_t sequence) const noexcept
{
	return sequence != 0 && sequence <= acknowledged_through_;
}

ExternalActionRequests ExternalActionOutbox::pending()
{
	published_through_ = last_sequence_;
	return { acknowledged_through_, { pending_.begin(), pending_.end() } };
}
}
