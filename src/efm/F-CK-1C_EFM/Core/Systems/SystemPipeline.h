#pragma once

#include "System.h"
#include "../Contracts/AircraftData.h"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <typeindex>
#include <vector>

namespace Core
{
namespace Systems
{
enum class InitialValueRequirement
{
	Required,
	Optional
};

enum class DispatchResult
{
	Handled,
	Unhandled
};

using CommandHandler = std::function<void(const Command&)>;
using DamageHandler = std::function<void(const DamageEvent&)>;
using RepairHandler = std::function<void(const RepairEvent&)>;

struct FuelManagementHandlers
{
	std::function<FlightFuelState()> read;
	std::function<FuelData()> current_data;
	std::function<void(double)> set_internal;
	std::function<void(const ExternalFuelInput&)> set_external;
	std::function<void(double)> set_reported_flow;
	std::function<MassDeltaResult()> take_mass_delta;
};

struct SystemStepOptions
{
	bool advance_fuel = true;
};

struct RuntimeSystem;

struct AircraftDataDescriptor
{
	AircraftDataId id;
	std::type_index type;
	const char* name;
};

class AircraftDataSnapshot final
{
public:
	template <typename T>
	const T& read(const AircraftDataKey<T>& key) const
	{
		const AircraftDataValue& value =
			read_value({ key.id, typeid(T), key.name });
		const T* typed = std::get_if<T>(&value);
		if (typed == nullptr)
		{
			throw_type_error(key.name);
		}
		return *typed;
	}

	template <typename T>
	bool has(const AircraftDataKey<T>& key) const
	{
		return has_value({ key.id, typeid(T), key.name });
	}

private:
	using Storage = std::array<
		std::optional<AircraftDataValue>,
		kAircraftDataSlotCount>;

	explicit AircraftDataSnapshot(const Storage& storage);

	const AircraftDataValue& read_value(
		const AircraftDataDescriptor& descriptor) const;
	bool has_value(const AircraftDataDescriptor& descriptor) const;
	[[noreturn]] static void throw_type_error(const char* name);

	Storage storage_;

	friend class SystemPipeline;
};

class SystemResult final
{
public:
	template <typename T>
	void publish(const AircraftDataKey<T>& key, const T& value)
	{
		publish_value(
			{ key.id, typeid(T), key.name },
			AircraftDataValue(value));
	}

private:
	using PendingStorage = std::array<
		std::optional<AircraftDataValue>,
		kAircraftDataSlotCount>;

	void allow_publication(AircraftDataId id);
	void clear();
	void publish_value(
		const AircraftDataDescriptor& descriptor,
		const AircraftDataValue& value);

	std::array<bool, kAircraftDataSlotCount> writable_ = {};
	PendingStorage pending_;

	friend class SystemPipeline;
};

class SystemSetup final
{
public:
	template <typename T>
	void read(
		const AircraftDataKey<T>& key,
		InitialValueRequirement initial =
			InitialValueRequirement::Required)
	{
		declare_read({ key.id, typeid(T), key.name }, initial);
	}

	template <typename T>
	void publish(const AircraftDataKey<T>& key)
	{
		declare_publication(
			{ key.id, typeid(T), key.name },
			nullptr);
	}

	template <typename T>
	void publish(const AircraftDataKey<T>& key, const T& initial)
	{
		const AircraftDataValue value(initial);
		declare_publication(
			{ key.id, typeid(T), key.name },
			&value);
	}

	void register_command_handler(CommandId id, CommandHandler handler);
	void register_damage_handler(DamageArea area, DamageHandler handler);
	void register_repair_handler(RepairHandler handler);
	void register_fuel_management(FuelManagementHandlers handlers);

private:
	struct State;

	explicit SystemSetup(State& state);
	void declare_read(
		const AircraftDataDescriptor& descriptor,
		InitialValueRequirement initial);
	void declare_publication(
		const AircraftDataDescriptor& descriptor,
		const AircraftDataValue* initial);

	State* state_;

	friend class SystemPipeline;
	friend struct RuntimeSystem;
};

class SystemPipeline final
{
public:
	explicit SystemPipeline(const FlightSetupContext& setup);
	SystemPipeline(
		const FlightSetupContext& setup,
		std::vector<SystemEntry> catalog);
	~SystemPipeline();

	SystemPipeline(const SystemPipeline&) = delete;
	SystemPipeline& operator=(const SystemPipeline&) = delete;

	AircraftDataSnapshot snapshot() const;
	AircraftDataSnapshot step(const FrameInput& input)
	{
		return step(input, {});
	}
	AircraftDataSnapshot step(
		const FrameInput& input,
		const SystemStepOptions& options);
	DispatchResult send(const Command& command);
	DispatchResult apply(const DamageEvent& event);
	std::size_t apply(const RepairEvent& event);
	FlightFuelState fuel_state() const;
	void set_internal_fuel(double fuel);
	void set_external_fuel(const ExternalFuelInput& fuel);
	void set_reported_fuel_flow(double flow_rate);
	MassDeltaResult take_mass_delta();
	std::size_t system_count() const;

private:
	struct Implementation;

	std::unique_ptr<Implementation> implementation_;
};

std::vector<SystemEntry> load_generated_system_catalog();
}
}
