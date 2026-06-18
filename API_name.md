# F-CK-1C Collision Shell Contract

This file is the single source of truth for the active collision shell and
gear-contact naming used by the current mod.

## Active Runtime Assets

- Outer model: `Shapes/F-CK-1C.edm`
- Active collision shell: `Shapes/F-CK-1C-box.edm`
- LOD config owner: `Shapes/F-CK-1C.lods`

`idf_hitbox.edm` is kept in the repo only as a legacy reference. It is not the
active collision shell used by the current aircraft configuration.

## Active Gear Contact Nodes

The live gear-contact shell segments in `F-CK-1C-box.edm` are:

- `F-CK-1C-F_W` for nose gear
- `F-CK-1C-LBW` for left main gear
- `F-CK-1C-RBW` for right main gear

These exact names are the valid `FM/config.lua -> collision_shell_name` values
for the current runtime configuration. The shorter names `F_W`, `LBW`, and
`RBW` still appear in historical notes, but they are not the exact strings
embedded in the active shell.

## Required Ground-Contact Line Registrations

The following names are not used by `FM/config.lua` as suspension node names,
but they must remain registered in `F-CK-1C.lua -> Damage`:

- `lineFG`
- `lineLG`
- `lineRG`

DCS uses these line segments to expose multi-point ground contact. If they are
removed from the aircraft damage registration, the aircraft can fall back to a
single contact point and pivot around that point on the ground.

## Historical Names

The following names are historical and must not be used as active runtime
references:

- `WHEEL_F`
- `WHEEL_L`
- `WHEEL_R`

## Runtime Configuration Rules

- `Shapes/F-CK-1C.lods` must point `collision_shell` at `F-CK-1C-box.edm`.
- `FM/config.lua` must use `collision_shell_name` for suspension alignment.
- `FM/config.lua` must not mix explicit `pos = {...}` gear contact points with
  the active `collision_shell_name` contract.
- `F-CK-1C.lua` damage node names must match the active shell segment names that
  actually exist in `F-CK-1C-box.edm`.
- `F-CK-1C.lua` must keep `lineFG`, `lineLG`, and `lineRG` in `Damage` so DCS
  preserves multi-point ground-contact lines.

## Geometry Expectations

For gear-down ground contact:

- The first legal ground-contact nodes must be `F-CK-1C-F_W`,
  `F-CK-1C-LBW`, and `F-CK-1C-RBW`.
- Belly/body shell segments such as `body`, `Blap`, and `Brap` must not extend
  below the wheel-contact geometry.

For gear-up ground contact:

- Belly contact must be smooth and must not contain low spots that can create
  repeated pogo-style rebounds.

## Debug Expectations

The EFM debug log should always make it obvious which ground-contact contract is
active. The log banner should include:

- active collision shell name
- active gear node names
- suspension alignment mode
- fallback-ground-force enable state
