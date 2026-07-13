#pragma once

#include "../Core/Fck1cEfm.h"

namespace DcsBridge
{
class DcsRuntime;

Core::Fck1cEfm& efm();
DcsRuntime& runtime();
}
