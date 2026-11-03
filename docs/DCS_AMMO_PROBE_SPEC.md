# Ammunition-source diagnostic package

User request: attempt the available DCS Lua ammunition-reading routes, supply a downloadable test module, and capture ordinary empty versus explicit CLEAN stations in the same test.

The diagnostic package observes cockpit sensor/device getters and actual method inventories, cockpit global/namespaced payload access, Export payload and weapon-device getters, and mission Unit:getAmmo through a GameGUI hook. Each source retains its own status and values. Native zero, absent members, call failures, and transport failures remain distinguishable. Native gun indices are not guessed.

Mission queries identify the local F-CK-1C by ownship ID, local player name, or single-player world.getPlayer. They capture ammunition entry counts and descriptive identifiers, initial payload gun percentage/pylon CLSIDs, and the actual pylon draw arguments from F-CK-1C.lua. They continue independently when Export data is absent. Diagnostic code observes data without supplying MC ammunition or issuing weapon-control actions.

The test module includes a cockpit opt-in flag. Installation deploys the matching module, the Export observer, and a GameGUI hook. Existing Export bytes/callbacks and unrelated hooks are preserved. The previous module is backed up outside the aircraft-loading directory. Repeated installation adds one registration. Removal removes only the owned registration/runtime and disables the cockpit probe while preserving logs and unrelated configuration. Collection groups DCS logs, all probe records, MC CSV and build provenance into one ZIP.

Packaging reuses the exact Windows-validated DLL from source 32db27850f4e503e980aacd280782fa25d2a3f4b and checks the entire src/efm tree matches. Runtime Lua overlays and scripts have their own immutable source revision and hashes. The baseline DLL validation manifest stays intact. Windows packaging uses the project release tool and verifies all necessary archive entries and hashes. Lua 5.1 regressions and replay fixtures, JSON round trips, and Windows PowerShell 5.1 installer lifecycle checks must pass. DCS observation itself is performed by the user.

Human instructions use default keys and focus on 100%/50% load, firing, depletion/rearm and empty/CLEAN comparison. No input binding changes are part of the package.
