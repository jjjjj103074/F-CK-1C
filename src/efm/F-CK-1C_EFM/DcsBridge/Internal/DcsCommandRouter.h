#pragma once

#include "../../Core/Contracts/Commands.h"

#include <cstddef>

namespace DcsBridge
{
enum class DcsCommandMappingStatus
{
	Mapped,
	IgnoredRelease,
	IgnoredCommand,
	UnknownCommand,
	InvalidValue,
	InvalidBindingTable
};

enum class CommandBindingError
{
	None,
	DuplicateId,
	InvalidRule
};

struct CommandTableValidation
{
	CommandBindingError error = CommandBindingError::None;
	int command_id = 0;
	std::size_t binding_count = 0;
};

struct DcsCommandMapping
{
	DcsCommandMappingStatus status = DcsCommandMappingStatus::UnknownCommand;
	Core::Command command;
	CommandTableValidation table_validation;

	bool should_dispatch() const
	{
		return status == DcsCommandMappingStatus::Mapped;
	}
};

CommandTableValidation validate_command_bindings();
DcsCommandMapping map_command(int command, float value);
}
