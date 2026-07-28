#include "SystemPipelineDataAccess.h"

#include <stdexcept>
#include <string>
#include <typeindex>
#include <utility>

namespace Core
{
namespace Systems
{
namespace
{
template <std::size_t... Indices>
std::array<std::type_index, sizeof...(Indices)> make_aircraft_data_types(
	std::index_sequence<Indices...>)
{
	return {
		std::type_index(typeid(
			std::variant_alternative_t<Indices, AircraftDataValue>))...
	};
}

static_assert(
	std::variant_size<AircraftDataValue>::value == kAircraftDataSlotCount,
	"AircraftData IDs and value types must have the same size.");

const auto kAircraftDataTypes = make_aircraft_data_types(
	std::make_index_sequence<kAircraftDataSlotCount>{});

std::type_index expected_type(AircraftDataId id)
{
	return kAircraftDataTypes[Detail::slot(id)];
}
}

namespace Detail
{
std::size_t slot(AircraftDataId id)
{
	const std::size_t value = static_cast<std::size_t>(id);
	if (value >= kAircraftDataSlotCount)
	{
		throw std::logic_error("AircraftData key has an invalid ID.");
	}
	return value;
}

void validate_key(const AircraftDataDescriptor& descriptor)
{
	(void)slot(descriptor.id);
	if (descriptor.name == nullptr || descriptor.name[0] == '\0')
	{
		throw std::logic_error("AircraftData key requires a name.");
	}
	if (expected_type(descriptor.id) != descriptor.type)
	{
		throw std::logic_error(
			std::string("AircraftData key type mismatch: ") + descriptor.name);
	}
}
}

AircraftDataSnapshot::AircraftDataSnapshot(const Storage& storage)
	: storage_(storage)
{
}

const AircraftDataValue& AircraftDataSnapshot::read_value(
	const AircraftDataDescriptor& descriptor) const
{
	Detail::validate_key(descriptor);
	const auto& value = storage_[Detail::slot(descriptor.id)];
	if (!value)
	{
		throw std::logic_error(
			std::string("AircraftData value is not initialized: ") +
			descriptor.name);
	}
	return *value;
}

bool AircraftDataSnapshot::has_value(
	const AircraftDataDescriptor& descriptor) const
{
	Detail::validate_key(descriptor);
	return storage_[Detail::slot(descriptor.id)].has_value();
}

void AircraftDataSnapshot::throw_type_error(const char* name)
{
	throw std::logic_error(
		std::string("AircraftData stored type mismatch: ") + name);
}

AircraftDataView::AircraftDataView(
	const AircraftDataSnapshot& snapshot,
	const ReadableMask& readable)
	: snapshot_(&snapshot),
	readable_(&readable)
{
}

void AircraftDataView::require_declared(
	const AircraftDataDescriptor& descriptor) const
{
	Detail::validate_key(descriptor);
	if (!(*readable_)[Detail::slot(descriptor.id)])
	{
		throw std::logic_error(
			std::string("System read undeclared AircraftData: ") +
			descriptor.name);
	}
}

void SystemResult::activate_publications(
	const std::array<bool, kAircraftDataSlotCount>& writable)
{
	writable_ = writable;
}

void SystemResult::clear()
{
	writable_.fill(false);
	for (auto& value : pending_)
	{
		value.reset();
	}
}

void SystemResult::publish_value(
	const AircraftDataDescriptor& descriptor,
	const AircraftDataValue& value)
{
	Detail::validate_key(descriptor);
	const std::size_t index = Detail::slot(descriptor.id);
	if (!writable_[index])
	{
		throw std::logic_error(
			std::string("System published undeclared AircraftData: ") +
			descriptor.name);
	}
	if (pending_[index])
	{
		throw std::logic_error(
			std::string("System published AircraftData twice in one step: ") +
			descriptor.name);
	}
	pending_[index] = value;
}
}
}
