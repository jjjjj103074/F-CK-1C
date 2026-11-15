# Direct native countermeasure flight test

This isolated test profile validates C++ inventory reads and one-round flare/chaff
calls from `ed_fm_simulate`. `cases.json` defines the three human checks. The package
adds two default test keys, a passive Lua inventory reference, and a side-by-side
C++/Lua indicator. The exact Windows DLL and source revision must match the passed
CI manifest before assembly.

The native host checks the supplied CockpitBase version and five code fingerprints,
then reacquires context/human/payload for each sample. It compares the human payload
with `c_payload()` before reading both counts or dispensing one round. EFM sampling
runs at 10 Hz; the cockpit recorder runs at 50 Hz. Calls run after the Bridge lock
has been released. Input edges and per-flight epochs control queued requests.

`FM/native-countermeasure-probe.enabled` enables the probe in this test package.
The ordinary release assembly keeps the probe disabled. The test profile uses
Bridge commands 3150/3151, bound to left Alt + left Shift + Insert/Delete.

Build the module through `mc-windows-validation.yml`, then use the existing
`package_test_bundle.py` arguments to assemble it. `START-HERE.zh-TW.html` contains
installation, all default keys, three cases, expected results, and collection steps.

`COLLECT-RESULTS.bat` gathers the cockpit CSVs, DCS/EFM logs, existing input profiles,
installed DLL hash, installed input scripts and their hashes, and build manifest into `FCK1C-CM-DIRECT-RESULTS-<time>.zip`.
Its first argument accepts a Saved Games directory, including drag-and-drop.

Validation uses the existing package, Lua recorder/indicator, and PowerShell
collector fixtures. Native probe tests cover input edges, sampling, ordering,
arm rejection, unavailable sources, exceptions, reentrancy, and flight changes.
DCS execution remains the pilot's real-world check after the build passes CI.

The input-trace build records every raw `ed_fm_set_command` call after module
initialization and before command handlers. `native_cm_input stage=efm_entry`
contains the unmodified command ID, decimal float, float bits, and finiteness.
`stage=probe_result` records the test-command state after handling. The test flag
controls this trace. Existing request and native-call events complete the path.
The collector adds `EFM-COMMAND-TRACE.log` and `INPUT-SUMMARY.json`; command 3040
provides the known master-arm input alongside the two probe commands 3150/3151.
