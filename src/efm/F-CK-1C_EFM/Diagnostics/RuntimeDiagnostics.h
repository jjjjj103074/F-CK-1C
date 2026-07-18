#pragma once

#include <cstdio>

namespace Diagnostics
{
struct DiagnosticOutput
{
	char* data = nullptr;
	size_t capacity = 0;
};

struct DamageEventSnapshot
{
	int element = 0;
	double integrity = 0.0;
	bool invincible = false;
};

inline void format_damage_event(
	const DiagnosticOutput& output,
	const DamageEventSnapshot& snapshot)
{
	snprintf(
		output.data,
		output.capacity,
		"damage element=%d integrity=%.3f invincible=%d",
		snapshot.element,
		snapshot.integrity,
		snapshot.invincible ? 1 : 0);
}
}
