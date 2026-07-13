# duel

Dependency-free C++17 engine for Treat Duels (1v1 3v3 team fights over a byte-buffer
C ABI — config in, state out, per-round orders in, new state + JSON log out), compiled
twice from the same source: a native assert-based test binary and a zero-import WASM
reactor for the browser. It is intentionally standalone — no libxayagame/protobuf/glog,
not wired into the repo's autotools `make check` — with its own build: `bash build.sh
{native|wasm|all|test}` (native → `test/duel_tests`; wasm → `engine.wasm` +
`engine.wasm.sha256`; `test`/`all` also run the native suite). Full wire-format contract,
resolve semantics, and the task-by-task build plan live in
`../docs/duels-prototype-spec.md` and `../docs/superpowers/plans/2026-07-13-duels-prototype.md`.
