# EFM Core Phase 0 Baseline

## Fixed point

- Source fixed point: `71ec0144d12f633df9f311ffd4fe1784d5f2401f`
- Production Core behavior was not changed while capturing this baseline.
- Native tests before Phase 0 additions: 1481 checks, 0 failures.
- Native tests after Phase 0 additions: 1505 checks, 0 failures.

## Stored compatibility data

- DLL ABI baseline:
  `src/efm/F-CK-1C_EFM/DcsBridge/EfmExports.baseline.txt`
- Export verification:
  `tools/check_efm_exports.ps1`
- Captured named exports: 37
- Multi-frame numerical golden and timing characterization:
  `src/efm/F-CK-1C_EFM_Tests/Fck1cEfmCharacterizationTests.cpp`

## Locked current behavior

- Repeating the same command/input/event sequence produces identical complete
  FrameOutput signatures for every captured frame.
- LandingGear position changes are visible to flap scheduling in the same frame.
- Engine throttle output is used by Fuel consumption in the same frame.
- Missing frame observations retain the last accepted values.
- Release clears transient state while fuel and preparation options set around
  release survive the next start according to existing tests.
- DCS suspension feedback does not currently suppress fallback ground forces.
- Damage affects the corresponding engine physical effect.
- Repair clears damage integrity, but engine history that changed while damaged
  does not return to an undamaged control trajectory.
- Damage received while invincible remains latent in segment storage and can
  become effective after invincibility is disabled and another damage refresh
  occurs.
- An unread mass delta is overwritten by the next simulation frame rather than
  accumulated.

The last four items describe current behavior, not approved target behavior.
Their intentional corrections remain isolated in the later implementation
steps listed by `EFM_CORE_REFACTOR_IMPLEMENTATION_PLAN.md`.

## Baseline build observations

The Release x64 baseline succeeds with existing compiler warnings in
`Common/PathUtils.h` (`fopen`/`getenv`) and `Common/Interpolation.h`
(`Common::rescale` does not return on every path). Phase 0 does not modify or
silence these warnings.
