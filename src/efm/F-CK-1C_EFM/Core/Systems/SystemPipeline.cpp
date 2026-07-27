#include "SystemPipeline.h"
#include "../AircraftState.h"
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace Core
{
namespace Systems
{
namespace
{
constexpr int kNoAircraftDataWriter = -1;
constexpr int kExternalAircraftDataWriter = -2;
constexpr double kDynamicPressureCoefficient = 0.5;

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

std::size_t slot(AircraftDataId id)
{
	const std::size_t value = static_cast<std::size_t>(id);
	if (value >= kAircraftDataSlotCount)
	{
		throw std::logic_error("AircraftData key has an invalid ID.");
	}
	return value;
}

std::type_index expected_type(AircraftDataId id)
{
	return kAircraftDataTypes[slot(id)];
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

std::string system_error(
	const std::string& system_id,
	const std::string& message)
{
	return "System '" + system_id + "': " + message;
}

AircraftObservation make_observation(const AircraftState& state)
{
	return {
		state.altitude_asl,
		state.altitude_agl,
		state.atmosphere_density,
		state.speed_scalar,
		ground_speed(state),
		state.mach,
		kDynamicPressureCoefficient * state.atmosphere_density *
			state.speed_scalar * state.speed_scalar,
		state.g,
		state.alpha,
		state.beta,
		state.roll,
		state.pitch,
		state.roll_rate,
		state.pitch_rate,
		state.yaw_rate
	};
}
}

struct DataReadDeclaration
{
	AircraftDataId id;
	std::type_index type;
	std::string name;
	InitialValueRequirement initial;
};

struct DataPublicationDeclaration
{
	AircraftDataId id;
	std::type_index type;
	std::string name;
	std::optional<AircraftDataValue> initial;
};

struct CommandRegistration
{
	CommandId id;
	CommandHandler handler;
};

struct DamageRegistration
{
	DamageArea area;
	DamageHandler handler;
};

struct SystemSetup::State
{
	std::string system_id;
	std::vector<DataReadDeclaration> reads;
	std::vector<DataPublicationDeclaration> publications;
	std::vector<CommandRegistration> commands;
	std::vector<DamageRegistration> damage_handlers;
	std::vector<RepairHandler> repair_handlers;
	std::optional<FuelManagementHandlers> fuel_management;
};

struct RuntimeSystem
{
	SystemGroup group;
	std::unique_ptr<System> system;
	SystemResult result;
	SystemSetup::State setup;
};

struct SystemPipeline::Implementation
{
	using Storage = AircraftDataSnapshot::Storage;

	explicit Implementation(
		const FlightSetupContext& context,
		std::vector<SystemEntry> catalog);

	AircraftDataSnapshot make_snapshot() const;
	AircraftDataSnapshot make_snapshot(const Storage& storage) const;
	AircraftDataSnapshot step(
		const FrameInput& input,
		const SystemStepOptions& options);
	void run_group(
		SystemGroup group,
		Storage& next,
		const SystemStepOptions& options);
	void create_systems(
		const FlightSetupContext& context,
		std::vector<SystemEntry> catalog);
	void collect_declarations();
	void validate_and_commit_setup();
	void validate_publications(Storage& initial);
	void validate_reads(const Storage& initial) const;
	void validate_handlers();
	void validate_fuel_management(
		const RuntimeSystem& runtime,
		std::size_t system_index);
	void commit_group(SystemGroup group, Storage& next);
	bool should_run(
		const RuntimeSystem& runtime,
		const SystemStepOptions& options) const;
	const FuelManagementHandlers& require_fuel_management() const;
	FuelManagementHandlers& require_fuel_management();
	void commit_current_fuel_data();

	std::vector<RuntimeSystem> systems;
	AircraftState observation_state;
	Storage committed;
	std::array<int, kAircraftDataSlotCount> writers;
	std::map<CommandId, CommandHandler> command_handlers;
	std::map<DamageArea, DamageHandler> damage_handlers;
	std::vector<RepairHandler> repair_handlers;
	std::optional<FuelManagementHandlers> fuel_management;
};

AircraftDataSnapshot::AircraftDataSnapshot(const Storage& storage)
	: storage_(storage)
{
}

const AircraftDataValue& AircraftDataSnapshot::read_value(
	const AircraftDataDescriptor& descriptor) const
{
	validate_key(descriptor);
	const auto& value = storage_[slot(descriptor.id)];
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
	validate_key(descriptor);
	return storage_[slot(descriptor.id)].has_value();
}

void AircraftDataSnapshot::throw_type_error(const char* name)
{
	throw std::logic_error(
		std::string("AircraftData stored type mismatch: ") + name);
}

void SystemResult::allow_publication(AircraftDataId id)
{
	writable_[slot(id)] = true;
}

void SystemResult::clear()
{
	for (auto& value : pending_)
	{
		value.reset();
	}
}

void SystemResult::publish_value(
	const AircraftDataDescriptor& descriptor,
	const AircraftDataValue& value)
{
	validate_key(descriptor);
	const std::size_t index = slot(descriptor.id);
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

SystemSetup::SystemSetup(State& state)
	: state_(&state)
{
}

void SystemSetup::declare_read(
	const AircraftDataDescriptor& descriptor,
	InitialValueRequirement initial)
{
	state_->reads.push_back({
		descriptor.id,
		descriptor.type,
		descriptor.name == nullptr ? "" : descriptor.name,
		initial
	});
}

void SystemSetup::declare_publication(
	const AircraftDataDescriptor& descriptor,
	const AircraftDataValue* initial)
{
	DataPublicationDeclaration declaration = {
		descriptor.id,
		descriptor.type,
		descriptor.name == nullptr ? "" : descriptor.name,
		std::nullopt
	};
	if (initial != nullptr)
	{
		declaration.initial = *initial;
	}
	state_->publications.push_back(std::move(declaration));
}

void SystemSetup::register_command_handler(
	CommandId id,
	CommandHandler handler)
{
	state_->commands.push_back({ id, std::move(handler) });
}

void SystemSetup::register_damage_handler(
	DamageArea area,
	DamageHandler handler)
{
	state_->damage_handlers.push_back({ area, std::move(handler) });
}

void SystemSetup::register_repair_handler(RepairHandler handler)
{
	state_->repair_handlers.push_back(std::move(handler));
}

void SystemSetup::register_fuel_management(FuelManagementHandlers handlers)
{
	if (state_->fuel_management)
	{
		throw std::logic_error(
			"System registered fuel management more than once.");
	}
	state_->fuel_management = std::move(handlers);
}

SystemPipeline::Implementation::Implementation(
	const FlightSetupContext& context,
	std::vector<SystemEntry> catalog)
{
	writers.fill(kNoAircraftDataWriter);
	const std::size_t frame_input_slot =
		slot(AircraftDataId::FrameInput);
	writers[frame_input_slot] = kExternalAircraftDataWriter;
	committed[frame_input_slot] = FrameInput{};
	const std::size_t observation_slot =
		slot(AircraftDataId::AircraftObservation);
	writers[observation_slot] = kExternalAircraftDataWriter;
	committed[observation_slot] = make_observation(observation_state);
	create_systems(context, std::move(catalog));
	collect_declarations();
	validate_and_commit_setup();
}

void SystemPipeline::Implementation::create_systems(
	const FlightSetupContext& context,
	std::vector<SystemEntry> catalog)
{
	std::set<std::string> ids;
	systems.reserve(catalog.size());
	for (SystemEntry& entry : catalog)
	{
		if (entry.id.empty() || !ids.insert(entry.id).second)
		{
			throw std::logic_error("System catalog contains an invalid or duplicate ID.");
		}
		if (!entry.factory)
		{
			throw std::logic_error(
				system_error(entry.id, "factory is not registered."));
		}
		std::unique_ptr<System> instance = entry.factory(context);
		if (!instance)
		{
			throw std::logic_error(
				system_error(entry.id, "factory returned no System."));
		}
		systems.push_back({
			entry.group,
			std::move(instance),
			SystemResult(),
			SystemSetup::State{ entry.id }
		});
	}
}

void SystemPipeline::Implementation::collect_declarations()
{
	for (RuntimeSystem& runtime : systems)
	{
		SystemSetup setup(runtime.setup);
		runtime.system->setup(setup);
	}
}

void SystemPipeline::Implementation::validate_and_commit_setup()
{
	Storage initial = committed;
	validate_publications(initial);
	validate_reads(initial);
	validate_handlers();
	committed = std::move(initial);
}

void SystemPipeline::Implementation::validate_publications(Storage& initial)
{
	for (std::size_t index = 0; index < systems.size(); ++index)
	{
		RuntimeSystem& runtime = systems[index];
		for (const DataPublicationDeclaration& declaration :
			runtime.setup.publications)
		{
			validate_key({
				declaration.id,
				declaration.type,
				declaration.name.c_str()
			});
			const std::size_t data_slot = slot(declaration.id);
			if (writers[data_slot] != kNoAircraftDataWriter)
			{
				throw std::logic_error(
					"AircraftData key has more than one publisher: " +
					declaration.name);
			}
			writers[data_slot] = static_cast<int>(index);
			runtime.result.allow_publication(declaration.id);
			initial[data_slot] = declaration.initial;
		}
	}
}

void SystemPipeline::Implementation::validate_reads(
	const Storage& initial) const
{
	for (const RuntimeSystem& runtime : systems)
	{
		for (const DataReadDeclaration& declaration : runtime.setup.reads)
		{
			validate_key({
				declaration.id,
				declaration.type,
				declaration.name.c_str()
			});
			const std::size_t data_slot = slot(declaration.id);
			if (writers[data_slot] == kNoAircraftDataWriter)
			{
				throw std::logic_error(
					system_error(runtime.setup.system_id,
						"missing AircraftData provider: " + declaration.name));
			}
			if (declaration.initial == InitialValueRequirement::Required &&
				!initial[data_slot])
			{
				throw std::logic_error(
					system_error(runtime.setup.system_id,
						"required AircraftData has no initial value: " +
						declaration.name));
			}
		}
	}
}

void SystemPipeline::Implementation::validate_handlers()
{
	for (std::size_t index = 0; index < systems.size(); ++index)
	{
		RuntimeSystem& runtime = systems[index];
		for (CommandRegistration& registration : runtime.setup.commands)
		{
			if (!registration.handler ||
				!command_handlers.emplace(
					registration.id, std::move(registration.handler)).second)
			{
				throw std::logic_error(
					"Command ID has more than one or no handler.");
			}
		}
		for (DamageRegistration& registration : runtime.setup.damage_handlers)
		{
			if (!registration.handler ||
				!damage_handlers.emplace(
					registration.area, std::move(registration.handler)).second)
			{
				throw std::logic_error(
					"Damage area has more than one or no owner.");
			}
		}
		for (RepairHandler& handler : runtime.setup.repair_handlers)
		{
			if (!handler)
			{
				throw std::logic_error("Repair handler is empty.");
			}
			repair_handlers.push_back(std::move(handler));
		}
		validate_fuel_management(runtime, index);
	}
}

void SystemPipeline::Implementation::validate_fuel_management(
	const RuntimeSystem& runtime,
	std::size_t system_index)
{
	if (!runtime.setup.fuel_management)
	{
		return;
	}
	const FuelManagementHandlers& handlers =
		*runtime.setup.fuel_management;
	const bool complete = handlers.read && handlers.current_data &&
		handlers.set_internal && handlers.set_external &&
		handlers.set_reported_flow && handlers.take_mass_delta;
	const int fuel_writer = writers[slot(AircraftDataId::FuelData)];
	if (!complete || fuel_management ||
		fuel_writer != static_cast<int>(system_index))
	{
		throw std::logic_error(
			"Fuel management requires one complete FuelData owner.");
	}
	fuel_management = handlers;
}

AircraftDataSnapshot SystemPipeline::Implementation::make_snapshot() const
{
	return AircraftDataSnapshot(committed);
}

AircraftDataSnapshot SystemPipeline::Implementation::make_snapshot(
	const Storage& storage) const
{
	return AircraftDataSnapshot(storage);
}

AircraftDataSnapshot SystemPipeline::Implementation::step(
	const FrameInput& input,
	const SystemStepOptions& options)
{
	Storage next = committed;
	next[slot(AircraftDataId::FrameInput)] = input;
	AircraftState next_observation = observation_state;
	apply_aircraft_observations(next_observation, input);
	update_airspeed(next_observation);
	next[slot(AircraftDataId::AircraftObservation)] =
		make_observation(next_observation);
	run_group(SystemGroup::Control, next, options);
	run_group(SystemGroup::Equipment, next, options);
	committed = std::move(next);
	observation_state = std::move(next_observation);
	return make_snapshot();
}

void SystemPipeline::Implementation::run_group(
	SystemGroup group,
	Storage& next,
	const SystemStepOptions& options)
{
	const AircraftDataSnapshot input = make_snapshot(next);
	for (RuntimeSystem& runtime : systems)
	{
		if (runtime.group == group)
		{
			runtime.result.clear();
		}
	}
	for (RuntimeSystem& runtime : systems)
	{
		if (runtime.group == group && should_run(runtime, options))
		{
			runtime.system->step(input, runtime.result);
		}
	}
	commit_group(group, next);
}

bool SystemPipeline::Implementation::should_run(
	const RuntimeSystem& runtime,
	const SystemStepOptions& options) const
{
	return options.advance_fuel || !runtime.setup.fuel_management;
}

void SystemPipeline::Implementation::commit_group(
	SystemGroup group,
	Storage& next)
{
	for (RuntimeSystem& runtime : systems)
	{
		if (runtime.group != group)
		{
			continue;
		}
		for (std::size_t index = 0; index < kAircraftDataSlotCount; ++index)
		{
			if (runtime.result.pending_[index])
			{
				next[index] = runtime.result.pending_[index];
			}
		}
	}
}

SystemPipeline::SystemPipeline(const FlightSetupContext& setup)
	: SystemPipeline(setup, load_generated_system_catalog())
{
}

SystemPipeline::SystemPipeline(
	const FlightSetupContext& setup,
	std::vector<SystemEntry> catalog)
	: implementation_(
		std::make_unique<Implementation>(setup, std::move(catalog)))
{
}

SystemPipeline::~SystemPipeline() = default;

AircraftDataSnapshot SystemPipeline::snapshot() const
{
	return implementation_->make_snapshot();
}

AircraftDataSnapshot SystemPipeline::step(
	const FrameInput& input,
	const SystemStepOptions& options)
{
	return implementation_->step(input, options);
}

DispatchResult SystemPipeline::send(const Command& command)
{
	const auto handler = implementation_->command_handlers.find(command.id);
	if (handler == implementation_->command_handlers.end())
	{
		return DispatchResult::Unhandled;
	}
	handler->second(command);
	return DispatchResult::Handled;
}

DispatchResult SystemPipeline::apply(const DamageEvent& event)
{
	const auto handler = implementation_->damage_handlers.find(event.area);
	if (handler == implementation_->damage_handlers.end())
	{
		return DispatchResult::Unhandled;
	}
	handler->second(event);
	return DispatchResult::Handled;
}

std::size_t SystemPipeline::apply(const RepairEvent& event)
{
	for (const RepairHandler& handler : implementation_->repair_handlers)
	{
		handler(event);
	}
	return implementation_->repair_handlers.size();
}

const FuelManagementHandlers& SystemPipeline::Implementation::
	require_fuel_management() const
{
	if (!fuel_management)
	{
		throw std::logic_error("SystemPipeline has no fuel management owner.");
	}
	return *fuel_management;
}

FuelManagementHandlers& SystemPipeline::Implementation::
	require_fuel_management()
{
	if (!fuel_management)
	{
		throw std::logic_error("SystemPipeline has no fuel management owner.");
	}
	return *fuel_management;
}

void SystemPipeline::Implementation::commit_current_fuel_data()
{
	committed[slot(AircraftDataId::FuelData)] =
		require_fuel_management().current_data();
}

FlightFuelState SystemPipeline::fuel_state() const
{
	return implementation_->require_fuel_management().read();
}

void SystemPipeline::set_internal_fuel(double fuel)
{
	implementation_->require_fuel_management().set_internal(fuel);
	implementation_->commit_current_fuel_data();
}

void SystemPipeline::set_external_fuel(const ExternalFuelInput& fuel)
{
	implementation_->require_fuel_management().set_external(fuel);
	implementation_->commit_current_fuel_data();
}

void SystemPipeline::set_reported_fuel_flow(double flow_rate)
{
	implementation_->require_fuel_management().set_reported_flow(flow_rate);
	implementation_->commit_current_fuel_data();
}

MassDeltaResult SystemPipeline::take_mass_delta()
{
	return implementation_->require_fuel_management().take_mass_delta();
}

std::size_t SystemPipeline::system_count() const
{
	return implementation_->systems.size();
}
}
}
