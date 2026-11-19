# DCS ID ownership

`CommandIds.json` defines custom commands, routes, known ignored DCS commands,
module parameters and raw DCS parameter contracts shared by C++ and Lua.

## Commands

| Route | Receiver |
|---|---|
| `efm` | ed_fm_set_command -> DcsCommandRouter -> Core semantic command |
| `bridge` | DcsBridge integration tool |
| `cockpit` | Specified cockpit device; EFM ignores the ID if received |

Raw numeric IDs stop at DcsBridge. Systems use `Core::CommandId`.
IDs and names are unique; rename repository callers together.

AP, A/T and Engine Thrust Cut commands are routed to efm.
AP/A/T behavior remains unimplemented; routing metadata does not guarantee a handler.
The reserved AUTOPILOT device ID has no active device and is not an input target.

`efm_ignored_dcs_commands` lists DCS-owned actions intentionally ignored by the EFM,
including actions dispatched by Lua. An ID absent from bindings and ignored
declarations produces a counted warning.

## Parameters

Module-owned parameters declare direction (`cpp_to_lua`, `lua_to_cpp` or
`lua_internal`), boundary unit and current writer.
A null writer means no active writer; `target_owner` records the metadata's
intended owner rather than an implemented System.
Optional `cpp_reader` identifies the importing adapter.

`raw_dcs_cockpit_params` separately lists DCS-owned radar/weapon observations,
including unit, axis convention, verification basis and reference.
`native_dcs_*_unmodified` preserves the DCS sign.
`community_documentation` cites a published contract;
`existing_module_contract` preserves a module integration pending a runtime probe.

## Generation

Run from the repository root:

```powershell
./tools/generate_dcs_ids.ps1
npm run test:dcs-id-generator
npm run test:cockpit-architecture
```

The generator owns:

- `DcsIds/CustomCommands.g.h`
- `DcsIds/CockpitParams.g.h`
- `Cockpit/Scripts/command_defs.lua`
- `Cockpit/Scripts/generated/CockpitParams.g.lua`

The DLL build does not regenerate tracked files. Edit CommandIds.json, then
generate and verify outputs. Lua uses `device_commands` for custom IDs and
`dcs_commands` for declared DCS actions.

`Commands.h` names built-in DCS inputs used by EFM bindings.
`DrawArgs.h`, `ParamIds.h` and `DamageIds.h` name the other DCS contracts.
`FM/config.lua` defines flight-model, mass and suspension settings.
