# DCS ID ownership

`CommandIds.json` is the single source for custom command IDs, their routing
owner, known DCS commands intentionally ignored by the EFM, and cockpit
parameter names shared by C++ and Lua.

Raw DCS numeric IDs stop at DCSBridge. A supported flight-model command is
translated into the semantic `Core::CommandId` declared in
`Core/Contracts/Commands.h`; concrete Systems never compare DCS numbers.

Every custom command declares one route:

- `efm`: the input profile sends the command to `ed_fm_set_command`, where
  `DcsCommandRouter` must provide a Core binding.
- `cockpit`: the input profile targets a `cockpit_device_id`; if the same
  numeric ID reaches `ed_fm_set_command`, the EFM intentionally ignores it.

Custom command names and numeric IDs are unique. Rename all in-repository
callers together instead of retaining legacy aliases.

`efm_ignored_dcs_commands` lists DCS-owned command IDs that require no EFM
action. This includes every DCS command directly used or dispatched by this
module's Lua, plus other runtime commands observed entering the EFM callback.
An incoming ID absent from both the supported EFM bindings and the declared
ignored IDs remains unknown and must produce the counted warning defined by
the DCSBridge logging policy.

Run `tools/generate_dcs_ids.ps1` explicitly from the repository root after
changing it. The DLL build does not generate or modify these tracked source
files. The generator updates:

- `DcsIds/CustomCommands.g.h`
- `DcsIds/CockpitParams.g.h`
- `Cockpit/Scripts/command_defs.lua`
- `Cockpit/Scripts/generated/CockpitParams.g.lua`

Do not edit generated files directly.

`command_defs.lua` exposes custom IDs as `device_commands` and declared DCS
IDs as `dcs_commands`. Cockpit Lua that calls `dispatch_action` must use the
generated `dcs_commands` value instead of repeating a raw numeric ID.

`Commands.h` contains DCS built-in input command IDs used by DCSBridge command
bindings to produce semantic Core commands; those numeric values are owned by
DCS rather than this module. Known ignored DCS IDs stay in `CommandIds.json`
with the reason they require no EFM action.
`DrawArgs.h`, `ParamIds.h`, and `DamageIds.h` give module-specific semantic
names to the other DCS contracts.

`FM/config.lua` remains the DCS flight-model, mass, and suspension contract; it
is not used as a command-ID database.
