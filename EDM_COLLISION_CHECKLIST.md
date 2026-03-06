# F-CK-1C EDM Collision Checklist

## Goal
Fix airborne wall/building penetration (not EFM force issue; model collision data issue).

## Current status (this workspace)
- Ground sink-through is currently masked by EFM virtual ground force.
- Airborne penetration still happens.
- `FM/config.lua` wheel `collision_shell_name` entries are currently commented out.

## What to verify in model pipeline
1. In your 3D source scene, ensure the aircraft has dedicated collision geometry (simple closed convex parts, not render mesh copy).
2. Collision objects must be exported into `F-CK-1C.edm` (not only visible mesh).
3. Re-export EDM with the same model name used by Lua:
   - `F-CK-1C.lua` -> `shape_table_data.file = "F-CK-1C"`
4. Open the exported EDM in ModelViewer2 and verify collision is present (not just visuals, LODs, and bones).
5. Test with static world objects (hangar/building/terrain obstacle) in SP mission:
   - low-speed impact
   - high-speed impact
   - wingtip-first side impact

## Landing gear collision linkage (ground only)
If your EDM has named wheel collision shells, align with `FM/config.lua`:
- `WHEEL_F`
- `WHEEL_L`
- `WHEEL_R`

Then uncomment these lines in `FM/config.lua`:
- `collision_shell_name = "WHEEL_F"`
- `collision_shell_name = "WHEEL_L"`
- `collision_shell_name = "WHEEL_R"`

If EDM does not contain these names, keep them commented to avoid invalid linkage.

## Runtime DLL sanity check
After each build, copy DLL to runtime path:
- Source: `DCS-Basic-EFM-Template-main/x64/Release/BasicEFM_template.dll`
- Runtime: `bin/BasicEFM_template.dll`

Verify timestamps are identical before launching DCS.

## Important note
EFM can fix forces, suspension behavior, and ground reaction.
EFM cannot replace missing/incorrect aircraft body collision geometry in EDM for airborne impacts.
