#pragma once

#include "../../Core/Fck1cEfm.h"

namespace DcsBridge
{
enum class DcsCommandMappingStatus
{
	Mapped,
	IgnoredRelease,
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
};

struct DcsCommandMapping
{
	DcsCommandMappingStatus status = DcsCommandMappingStatus::UnknownCommand;
	Core::EfmCommand command;
	CommandTableValidation table_validation;

	bool should_dispatch() const
	{
		return status == DcsCommandMappingStatus::Mapped;
	}
};

CommandTableValidation validate_command_bindings();
DcsCommandMapping map_command(int command, float value);
}
