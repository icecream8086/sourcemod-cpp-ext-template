#!/usr/bin/env bash
# Run host tests under QEMU with masked CPU features to prove graceful degradation.
# Usage: scripts/qemu_matrix.sh [build-dir]
set -euo pipefail

BUILD="${1:-build}"
QEMU64="$(command -v qemu-x86_64 || true)"
QEMU32="$(command -v qemu-i386 || true)"

if [ -z "$QEMU64" ] && [ -z "$QEMU32" ]; then
    echo "qemu-user not found; install qemu-user (Debian/Ubuntu: apt-get install qemu-user)" >&2
    exit 2
fi

# Old models must NOT advertise AVX2; Haswell must.
MODELS_OLD="qemu64 Nehalem"
MODEL_NEW="Haswell"

run_one() {
    local qemu="$1" cpu="$2" bin="$3"
    echo "== $cpu: $(basename "$bin") =="
    local log
    log="$(mktemp)"
    if ! "$qemu" -cpu "$cpu" "$bin" >"$log" 2>&1; then
        echo "FAIL: $bin crashed under -cpu $cpu" >&2
        cat "$log" >&2
        rm -f "$log"
        return 1
    fi
    tail -n 3 "$log"
    rm -f "$log"
    return 0
}

BINARIES=("$BUILD/template_tests" "$BUILD/smoke_dispatch" "$BUILD/smoke_edge")
for bin in "${BINARIES[@]}"; do
    [ -x "$bin" ] || { echo "missing $bin (build first)" >&2; exit 2; }
done

ARCH="$(file -b "$BUILD/template_tests" | grep -oE '32-bit|64-bit' || true)"
case "$ARCH" in
    64-bit) QEMU="$QEMU64" ;;
    32-bit) QEMU="$QEMU32" ;;
    *) echo "cannot detect binary arch"; exit 2 ;;
esac
[ -n "$QEMU" ] || { echo "no qemu for $ARCH" >&2; exit 2; }

for cpu in $MODELS_OLD; do
    for bin in "${BINARIES[@]}"; do run_one "$QEMU" "$cpu" "$bin"; done
done
for bin in "${BINARIES[@]}"; do run_one "$QEMU" "$MODEL_NEW" "$bin"; done

echo "PASS: all binaries survived old/new CPU models"
