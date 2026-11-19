#pragma once

#include "../Contracts/AircraftData.h"

#include <array>
#include <optional>
#include <tuple>
#include <type_traits>

namespace Core
{
namespace Systems
{
namespace Detail
{
template <typename Variant> class AircraftDataStorageFor;

// Keep each fixed ID's optional value at its own type's size, in AircraftDataValue order.
template <typename... Types> class AircraftDataStorageFor<std::variant<Types...>> final
{
public:
	template <typename T> std::optional<T>& get()
	{
		return std::get<std::optional<T>>(values_);
	}

	template <typename T> const std::optional<T>& get() const
	{
		return std::get<std::optional<T>>(values_);
	}

	bool has(std::size_t index) const
	{
		static constexpr std::array<bool (*)(const AircraftDataStorageFor&), sizeof...(Types)> readers = {
			+[](const AircraftDataStorageFor& storage)
			{
			    return storage.template get<Types>().has_value();
			}...
		};
		return readers.at(index)(*this);
	}

	void initialize(std::size_t index, const std::optional<std::variant<Types...>>& value)
	{
		if (value)
		{
			assign(*value);
			return;
		}
		static constexpr std::array<void (*)(AircraftDataStorageFor&), sizeof...(Types)> resetters = {
			+[](AircraftDataStorageFor& storage)
			{
			    storage.template get<Types>().reset();
			}...
		};
		resetters.at(index)(*this);
	}

	void assign(const std::variant<Types...>& value)
	{
		std::visit(
		    [this](const auto& typed)
		    {
			    this->template get<std::decay_t<decltype(typed)>>() = typed;
		    },
		    value);
	}

	void clear()
	{
		std::apply(
		    [](auto&... values)
		    {
			    (values.reset(), ...);
		    },
		    values_);
	}

	void commit_to(AircraftDataStorageFor& target) const
	{
		(copy_if_present(target, get<Types>()), ...);
	}

private:
	template <typename T> static void copy_if_present(AircraftDataStorageFor& target, const std::optional<T>& value)
	{
		if (value)
		{
			target.template get<T>() = value;
		}
	}

	std::tuple<std::optional<Types>...> values_;
};

using AircraftDataStorage = AircraftDataStorageFor<AircraftDataValue>;
}
}
}
