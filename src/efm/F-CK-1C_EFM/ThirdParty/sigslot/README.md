# sigslot

- Version: v1.2.3
- Upstream: https://github.com/palacaze/sigslot
- Commit: b588b791b9cf7eb17ff0a74d8aebd4a61166c2e1
- Header source: https://github.com/palacaze/sigslot/blob/b588b791b9cf7eb17ff0a74d8aebd4a61166c2e1/include/sigslot/signal.hpp
- License: MIT; see `LICENSE`.
- SHA256 `signal.hpp`: `14c00a2829a2e797b77d4d00215483c4246cf795593f329d04cd0369ccc52773`
- SHA256 `LICENSE`: `1315e4bdd10265324c80724a6506a8238c69c94c4cf5d19f76a6161a41b3a528`

`signal.hpp` and `LICENSE` are byte-for-byte copies of the pinned upstream files.
Update both from an identified upstream revision; do not edit the vendor header.
No download, separate library build or runtime DLL is needed.

All EFM C++ sources and native tests can use `#include <sigslot/signal.hpp>`
through the existing shared ThirdParty include configuration.
The project uses `sigslot::signal_st` for synchronous, single-threaded notifications.
Signals belong to the object or system instance that owns the communication scope;
there is no process-wide event bus. Lua continues to communicate through the bridge.

Keep event types with their owning system. Register listeners at assembly time,
hold `sigslot::scoped_connection` objects, and destroy connections before receivers.
Use distinct group IDs whenever listener order matters: order within one group is
unspecified. Do not rely on registration order. Process each command's notifications
before consuming the next command; callbacks must not recursively consume commands.

Use connection objects to disconnect, rather than comparing callback addresses.
Upstream documents MSVC/Clang-cl identical-function folding considerations for
address-based disconnection. Validate the actual usage in the pinned Windows
Release build; do not change global linker optimization settings preemptively.
