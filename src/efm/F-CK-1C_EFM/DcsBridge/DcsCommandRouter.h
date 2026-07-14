#pragma once

#include "../Core/Fck1cEfm.h"

namespace DcsBridge
{
struct DcsCommandMapping
{
	bool mapped = false;
	Core::EfmCommand command;
};

DcsCommandMapping map_command(int command, float value);
}
