# MC flight-test bundle

`package_test_bundle.py` wraps the module produced by `mc-windows-validation.yml`
in a test bundle with a human runbook, result collection, a passive cockpit recorder,
and a small parameter indicator. The source revision and DLL hash must match the
successful CI manifest before assembly.

```text
python tools/mc-flight-test/package_test_bundle.py \
  --module-zip dist/F-CK-1C-mc-<source>.zip \
  --manifest <validated-artifact>/manifest.json \
  --catalog src/efm/F-CK-1C_EFM/DcsIds/CommandIds.json \
  --expected-source <full-source-commit> \
  --output dist/mc-flight-test
```

`cases.json` is the shared source for the Markdown/HTML runbook and result sheet.
The committed human guide is [MC flight testing](../../docs/MC_FLIGHT_TEST.zh-TW.md).

The bundle appends device 10 and its indicator registration to `device_init.lua`,
four prebound native probe controls before the keyboard profile returns, and a
read-only countermeasure observer after the existing weapon-device update.
The module DLL is the exact CI-validated DLL. Existing controls and production MC,
radar and CMS algorithms remain intact. `BUILD.json` lists the three edited runtime
files and every added entry, including `FM/native-countermeasure-probe.enabled`.
The ordinary release package omits this flag and keeps the probe disabled.

The four controls invoke Bridge commands 3150–3153. The C++ probe dispatches the
native digital/analog commands from `ed_fm_simulate`, outside the Bridge execution
lock. `NativeCountermeasureObservation.lua` reads stock only; it sends no commands.
Its validity mask (flare=1, chaff=2) distinguishes zero stock from missing getters.
The test includes nine native cases plus the existing 69 MC cases.

The recorder runs at 50 Hz. Each mission creates a separate CSV under
`Saved Games/DCS/Logs/FCK1C-MC-TEST/`. MC and station generations have separate
coherence flags; raw DCS sensor and presentation fields remain individually sampled.
The file records the source commit and DLL SHA256 on each row. `release` closes the
file; I/O errors are reported in `dcs.log`.

`COLLECT-RESULTS.bat` runs the PowerShell collector after DCS exits. It copies the
recordings, DCS/EFM logs, existing input profiles, installed DLL hash and result sheet
into a timestamped ZIP. Its optional first argument selects the Saved Games root.

Checks:

```text
python -m unittest discover -s tools/mc-flight-test -p "test_*.py"
lua tools/mc-flight-test/test_recorder.lua tools/mc-flight-test <fixture-root>
powershell.exe -NoProfile -File tools/mc-flight-test/test_collection.ps1
```

Create `<fixture-root>/Logs/FCK1C-MC-TEST` before running the Lua fixtures.
The Windows CI executes the collector fixtures, package fixtures and actual
adapter/recorder Lua fixtures, then uploads `mc-flight-test-<source-commit>`.
In-game results remain pending until the pilot returns the evidence.
