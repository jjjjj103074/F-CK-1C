#include "SystemPipeline.h"

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
	switch (id)
	{
	case AircraftDataId::FlightControlDemand:
		return typeid(FlightControlDemand);
	case AircraftDataId::PrimaryControlPosition:
		return typeid(PrimaryControlPosition);
	case AircraftDataId::Count:
		break;
	}
	throw std::logic_error("AircraftData key has an invalid ID.");
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
	AircraftDataSnapshot step(const FrameInput& input);
	void run_group(SystemGroup group, Storage& next);
	void create_systems(
		const FlightSetupContext& context,
		std::vector<SystemEntry> catalog);
	void collect_declarations();
	void validate_and_commit_setup();
	void validate_publications(Storage& initial);
	void validate_reads(const Storage& initial) const;
	void validate_handlers();
	void commit_group(SystemGroup group, Storage& next);

	std::vector<RuntimeSystem> systems;
	Storage committed;
	std::array<int, kAircraftDataSlotCount> writers;
	std::map<CommandId, CommandHandler> command_handlers;
	std::map<DamageArea, DamageHandler> damage_handlers;
	std::vector<RepairHandler> repair_handlers;
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

SystemPipeline::Implementation::Implementation(
	const FlightSetupContext& context,
	std::vector<SystemEntry> catalog)
{
	writers.fill(kNoAircraftDataWriter);
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
	Storage initial;
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
	for (RuntimeSystem& runtime : systems)
	{
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
	}
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
	const FrameInput& input)
{
	// Production observation keys are introduced when the first System migrates.
	(void)input;
	Storage next = committed;
	run_group(SystemGroup::Control, next);
	run_group(SystemGroup::Equipment, next);
	committed = std::move(next);
	return make_snapshot();
}

void SystemPipeline::Implementation::run_group(
	SystemGroup group,
	Storage& next)
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
		if (runtime.group == group)
		{
			runtime.system->step(input, runtime.result);
		}
	}
	commit_group(group, next);
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

AircraftDataSnapshot SystemPipeline::step(const FrameInput& input)
{
	return implementation_->step(input);
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

std::size_t SystemPipeline::system_count() const
{
	return implementation_->systems.size();
}
}
}
