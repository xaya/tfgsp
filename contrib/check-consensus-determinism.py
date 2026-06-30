#!/usr/bin/env python3
"""Determinism guard: fail the build if a raw `float`/`double` token appears in
the consensus C++ (src/ + database/), outside the fpm fixed-point library, the
display-only code, and the tests.

Why: the GSP's state is hashed and compared across independent node operators
(xaya::SQLiteGame).  A value computed in hardware floating point can differ by
1 ULP between builds (-ffast-math, x87 vs SSE, FMA contraction, -O level) -> the
stored bytes differ -> a silent permanent chain fork.  All consensus math is
therefore integer or fpm::fixed_24_8 fixed-point.  This lint keeps it that way
by rejecting any new `float`/`double` in a consensus translation unit.

Comments and string/char literals are blanked before scanning, so prose like
"double-deducted" or a log message never trips it; `isDouble()`/`asDouble()` are
not matched because the token search is case-sensitive and word-bounded.

Usage: check-consensus-determinism.py [SRCROOT]   (default: cwd)
Exit 0 = clean, 1 = a raw float/double was found (prints file:line).
"""

import os
import re
import sys

# Files that legitimately use floating point but never feed hashed consensus
# state: the fixed-point library's own float<->fixed conversions, the display /
# RPC serialisers, and the test/benchmark harness.
EXCLUDED_BASENAMES = {
    "gamestatejson.cpp", "gamestatejson.hpp",   # JSON display (not hashed)
    "pxrpcserver.cpp", "pxrpcserver.hpp",       # RPC server (display)
    "rest.cpp", "rest.hpp",                      # REST stub (display)
}
EXCLUDED_SUFFIXES = ("_tests.cpp", "_tests.hpp")
EXCLUDED_PREFIXES = ("testutils", "xgametestutils", "dbtest", "loadbench")
SCAN_DIRS = ("src", "database")
SCAN_EXTS = (".cpp", ".hpp", ".tpp")

TOKEN = re.compile(r"\b(?:float|double)\b")


def is_excluded(path):
    if "/fpm/" in path.replace(os.sep, "/"):
        return True
    base = os.path.basename(path)
    if base in EXCLUDED_BASENAMES:
        return True
    if base.endswith(EXCLUDED_SUFFIXES):
        return True
    return any(base.startswith(p) for p in EXCLUDED_PREFIXES)


def blank_comments_and_strings(text):
    """Replace comment and string/char-literal bodies with spaces, preserving
    newlines (so line numbers stay accurate) and code structure."""
    out = []
    i, n = 0, len(text)
    state = "code"  # code | line_comment | block_comment | string | char
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state == "code":
            if c == "/" and nxt == "/":
                out.append("  "); i += 2; state = "line_comment"; continue
            if c == "/" and nxt == "*":
                out.append("  "); i += 2; state = "block_comment"; continue
            if c == '"':
                out.append(" "); i += 1; state = "string"; continue
            if c == "'":
                out.append(" "); i += 1; state = "char"; continue
            out.append(c); i += 1; continue
        if state == "line_comment":
            if c == "\n":
                out.append("\n"); i += 1; state = "code"; continue
            out.append(" " if c != "\t" else "\t"); i += 1; continue
        if state == "block_comment":
            if c == "*" and nxt == "/":
                out.append("  "); i += 2; state = "code"; continue
            out.append("\n" if c == "\n" else " "); i += 1; continue
        # string / char: honour backslash escapes, end on matching quote
        if c == "\\":
            out.append("  "); i += 2; continue
        if (state == "string" and c == '"') or (state == "char" and c == "'"):
            out.append(" "); i += 1; state = "code"; continue
        out.append("\n" if c == "\n" else " "); i += 1; continue
    return "".join(out)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    hits = []
    for d in SCAN_DIRS:
        base = os.path.join(root, d)
        for dirpath, _dirs, files in os.walk(base):
            for fn in files:
                if not fn.endswith(SCAN_EXTS):
                    continue
                path = os.path.join(dirpath, fn)
                if is_excluded(path):
                    continue
                with open(path, "r", encoding="utf-8", errors="replace") as fh:
                    code = blank_comments_and_strings(fh.read())
                for lineno, line in enumerate(code.split("\n"), 1):
                    if TOKEN.search(line):
                        rel = os.path.relpath(path, root)
                        hits.append("%s:%d: %s" % (rel, lineno, line.strip()))
    if hits:
        sys.stderr.write(
            "DETERMINISM LINT FAILED: raw float/double in consensus code "
            "(fork risk). Use integer or fpm::fixed_24_8 fixed-point instead.\n"
            "If a hit is genuinely display-only, add the file to the exclude "
            "list in contrib/check-consensus-determinism.py.\n")
        for h in hits:
            sys.stderr.write("  " + h + "\n")
        return 1
    print("determinism lint: no raw float/double in consensus code (%s)"
          % ", ".join(SCAN_DIRS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
