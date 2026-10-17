#pragma once

#include "SystemPipeline.h"

namespace Core
{
namespace Systems
{
namespace Detail
{
std::size_t slot(AircraftDataId id);
void validate_key(const AircraftDataDescriptor& descriptor);
}
}
}
