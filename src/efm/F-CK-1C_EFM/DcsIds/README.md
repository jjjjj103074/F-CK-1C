# DCS ID ownership

`CommandIds.json` is the single source for custom command IDs and cockpit
parameter names shared by C++ and Lua.

Run `tools/generate_dcs_ids.ps1` after changing it. The normal DLL build runs
the generator automatically and updates:

- `DcsIds/CustomCommands.g.h`
- `DcsIds/CockpitParams.g.h`
- `Cockpit/Scripts/command_defs.lua`
- `Cockpit/Scripts/generated/CockpitParams.g.lua`

Do not edit generated files directly.

`Commands.h` contains DCS built-in input command IDs because those values are
owned by DCS rather than this module. `DrawArgs.h`, `ParamIds.h`, and
`DamageIds.h` give module-specific semantic names to the other DCS contracts.

`FM/config.lua` remains the DCS flight-model, mass, and suspension contract; it
is not used as a command-ID database.
