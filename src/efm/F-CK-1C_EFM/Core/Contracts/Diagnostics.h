#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace Core
{
enum class ExecutionOwnerType
{
	System,
	SimulationModel
};

struct ExecutionErrorDetails
{
	ExecutionOwnerType owner_type;
	std::string owner;
	std::string operation;
	std::string reason;
};

class ExecutionError final : public std::runtime_error
{
public:
	explicit ExecutionError(ExecutionErrorDetails details)
		: std::runtime_error(details.reason),
		details_(std::move(details))
	{
	}

	const ExecutionErrorDetails& details() const noexcept
	{
		return details_;
	}

private:
	const ExecutionErrorDetails details_;
};
}
