# DCS ID ownership

`CommandIds.json` is the single source for custom command IDs, their routing
owner, known DCS commands intentionally ignored by the EFM, and cockpit
parameter names shared by C++ and Lua.

Every custom command declares one route:

- `efm`: the input profile sends the command to `ed_fm_set_command`, where
  `DcsCommandRouter` must provide a Core binding.
- `cockpit`: the input profile targets a `cockpit_device_id`; if the same
  numeric ID reaches `ed_fm_set_command`, the EFM intentionally ignores it.

Custom command names and numeric IDs are unique. Rename all in-repository
callers together instead of retaining legacy aliases.

`efm_ignored_dcs_commands` lists DCS-owned command IDs that are known to reach
the EFM callback but require no EFM action. An incoming ID absent from both the
supported EFM bindings and the declared ignored IDs remains unknown and must
produce the counted warning defined by the DCSBridge logging policy.

Run `tools/generate_dcs_ids.ps1` explicitly after changing it. The DLL build
does not generate or modify source files. The generator updates:

- `DcsIds/CustomCommands.g.h`
- `DcsIds/CockpitParams.g.h`
- `Cockpit/Scripts/command_defs.lua`
- `Cockpit/Scripts/generated/CockpitParams.g.lua`

Do not edit generated files directly.

`Commands.h` contains DCS built-in input command IDs used by Core bindings
because those values are owned by DCS rather than this module. Known ignored
DCS IDs stay in `CommandIds.json` with the reason they require no EFM action.
`DrawArgs.h`, `ParamIds.h`, and `DamageIds.h` give module-specific semantic
names to the other DCS contracts.

`FM/config.lua` remains the DCS flight-model, mass, and suspension contract; it
is not used as a command-ID database.
