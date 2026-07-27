#pragma once

#include <cstddef>
#include <variant>

namespace Core
{
struct FlightControlDemand
{
	double pitch = 0.0;
	double roll = 0.0;
	double yaw = 0.0;
};

struct PrimaryControlPosition
{
	double elevator = 0.0;
	double aileron = 0.0;
	double rudder = 0.0;
};

enum class AircraftDataId
{
	FlightControlDemand,
	PrimaryControlPosition,
	Count
};

template <typename T>
struct AircraftDataKey
{
	AircraftDataId id;
	const char* name;
};

namespace AircraftDataKeys
{
inline constexpr AircraftDataKey<FlightControlDemand> kFlightControlDemand = {
	AircraftDataId::FlightControlDemand,
	"flight_control_demand"
};

inline constexpr AircraftDataKey<PrimaryControlPosition> kPrimaryControlPosition = {
	AircraftDataId::PrimaryControlPosition,
	"primary_control_position"
};
}

using AircraftDataValue =
	std::variant<FlightControlDemand, PrimaryControlPosition>;

inline constexpr std::size_t kAircraftDataSlotCount =
	static_cast<std::size_t>(AircraftDataId::Count);
}
