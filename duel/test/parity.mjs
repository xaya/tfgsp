// duel/test/parity.mjs — WASM parity gate (Task 5).
//
// Loads duel/engine.wasm (the wasi-sdk reactor blob, zero imports — see
// build.sh's wasm step) and replays every golden vector in
// duel/test/golden.json (regenerate via `duel/build.sh golden`, which runs
// the NATIVE binary's `--dump-golden` mode) through the same C ABI
// (duel_init/duel_apply via duel_alloc-backed scratch buffers), byte-
// comparing rc/outHex/logJson against what the native binary recorded.
//
// Any mismatch is a real engine bug (native != wasm is non-determinism
// across builds, which breaks game-channel state agreement) — this script
// exits 1 with the offending vector name and both hex outputs; it never
// weakens the comparison to "pass".
//
// Usage: node duel/test/parity.mjs

import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const here = path.dirname(fileURLToPath(import.meta.url));
const wasmPath = path.join(here, '..', 'engine.wasm');
const goldenPath = path.join(here, 'golden.json');

function toHex(bytes) {
  return Buffer.from(bytes).toString('hex');
}

function fromHex(hex) {
  return new Uint8Array(Buffer.from(hex, 'hex'));
}

async function main() {
  const wasmBytes = readFileSync(wasmPath);
  const mod = await WebAssembly.compile(wasmBytes);
  const imports = WebAssembly.Module.imports(mod);
  if (imports.length !== 0) {
    console.error(`FAIL: engine.wasm has ${imports.length} import(s), expected 0 (zero-import reactor contract broken)`);
    process.exit(1);
  }
  const instance = await WebAssembly.instantiate(mod, {});
  const exp = instance.exports;
  if (typeof exp._initialize === 'function') {
    exp._initialize();
  }

  function memBytes() {
    return new Uint8Array(exp.memory.buffer);
  }

  function writeInput(bytes) {
    const len = bytes.length;
    const ptr = exp.duel_alloc(len);
    memBytes().set(bytes, ptr);
    return ptr;
  }

  function readOut() {
    const ptr = exp.duel_out_ptr();
    const len = exp.duel_out_len();
    return memBytes().slice(ptr, ptr + len);
  }

  function readLog() {
    const ptr = exp.duel_log_ptr();
    const len = exp.duel_log_len();
    return Buffer.from(memBytes().slice(ptr, ptr + len)).toString('utf8');
  }

  const lines = readFileSync(goldenPath, 'utf8')
    .split('\n')
    .map((l) => l.trim())
    .filter((l) => l.length > 0);

  if (lines.length === 0) {
    console.error('FAIL: golden.json has zero vectors (run `duel/build.sh golden` first)');
    process.exit(1);
  }

  for (const line of lines) {
    const vec = JSON.parse(line);
    const isInit = Object.prototype.hasOwnProperty.call(vec, 'cfgHex');
    let rc;
    if (isInit) {
      const cfgPtr = writeInput(fromHex(vec.cfgHex));
      rc = exp.duel_init(cfgPtr, vec.cfgHex.length / 2);
    } else {
      const stPtr = writeInput(fromHex(vec.stateHex));
      const ordPtr = writeInput(fromHex(vec.ordersHex));
      rc = exp.duel_apply(stPtr, vec.stateHex.length / 2, ordPtr, vec.ordersHex.length / 2);
    }

    if (rc !== vec.rc) {
      console.error(`PARITY FAIL: ${vec.name} — rc mismatch: native=${vec.rc} wasm=${rc}`);
      process.exit(1);
    }

    const outHex = rc === 0 ? toHex(readOut()) : '';
    if (outHex !== vec.outHex) {
      console.error(`PARITY FAIL: ${vec.name} — outHex mismatch:`);
      console.error(`  native: ${vec.outHex}`);
      console.error(`  wasm:   ${outHex}`);
      process.exit(1);
    }

    if (!isInit) {
      const logJson = rc === 0 ? readLog() : '';
      if (logJson !== vec.logJson) {
        console.error(`PARITY FAIL: ${vec.name} — logJson mismatch:`);
        console.error(`  native: ${vec.logJson}`);
        console.error(`  wasm:   ${logJson}`);
        process.exit(1);
      }
    }
  }

  console.log(`PARITY OK (${lines.length} vectors)`);
}

main().catch((err) => {
  console.error('PARITY FAIL (exception):', err);
  process.exit(1);
});
