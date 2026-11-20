#!/usr/bin/env bash
# Audit: baseline functions must not contain VEX/AVX instructions.
# Usage: scripts/audit_isa.sh <binary-or-shared-object>
set -euo pipefail

BIN="${1:?usage: $0 <binary>}"
[ -f "$BIN" ] || { echo "not found: $BIN" >&2; exit 2; }

DIS="$(mktemp)"
trap 'rm -f "$DIS"' EXIT
objdump -d -C --no-show-raw-insn "$BIN" > "$DIS"

# Functions that must stay baseline (no v* mnemonics, no ymm/zmm).
BASELINE='DistanceBatchScalar|SelectDistanceBatchFn|DetectCpuFeatures|main'
ALLOWED='DistanceBatchAVX2'

python3 - "$DIS" "$BASELINE" "$ALLOWED" <<'PY'
import re, sys

path, baseline_pat, allowed_pat = sys.argv[1], sys.argv[2], sys.argv[3]
baseline_re = re.compile(baseline_pat)
allowed_re = re.compile(allowed_pat)

func = None
violations = []
with open(path, encoding="utf-8", errors="replace") as f:
    for line in f:
        m = re.match(r"^[0-9a-f]+ <(.+)>:$", line.strip())
        if m:
            func = m.group(1)
            continue
        if func is None or allowed_re.search(func):
            continue
        if baseline_re.search(func):
            insn = line.strip().split("\t")[-1].split()[0] if "\t" in line else ""
            if not insn:
                continue
            if insn.startswith("v") or "ymm" in line or "zmm" in line:
                violations.append((func, insn, line.strip()))

if violations:
    print("FAIL: baseline functions contain ISA instructions:")
    for func, insn, line in violations[:20]:
        print(f"  {func}: {insn}  ({line[:100]})")
    sys.exit(1)
print("PASS: baseline functions are ISA-clean")
PY
