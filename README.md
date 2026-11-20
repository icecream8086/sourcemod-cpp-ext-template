# SourceMod C++ Extension Template

A reusable template for SourceMod C++ extensions with:

- **32-bit and 64-bit targets** (`TEMPLATE_ARCH=32|64|native`) — L4D2 is 32-bit i386, but SourceMod also runs on 64-bit games;
- **CMake** build (extension target + host-side tests/bench),
- **runtime ISA dispatch** (CPUID detection + function pointers + scalar fallback),
- **fool-proof C++ API** (bool-returning primitives, null/overflow guards, static ABI assertions),
- **destructive smoke tests** (unsupported ISA paths, edge sizes, forced scalar, disabled-AVX2 build, sanitizers),
- **Z3 formal checks** for dispatch safety, completeness and algorithm invariants,
- **vendored header-only wheels** (radix heap, robin_hood, nanobench; ska_sort reference),
- **GLIBC symbol ceiling gate** for old-distro portability,
- **L4D2 strict profile** (`clang++-strict` + lifetime annotations + EntRef/BuildID rules) — see `docs/COVENANT-L4D2.md`.

See `docs/ISA.md` for ISA/ABI rules and `AGENTS.md` for the contribution/DoD checklist.

> Compatibility policy: the template provides detection, dispatch, and a GLIBC
> check **as features**. What "compatible" means for your server is your call —
> this repo does not promise legacy runtimes. Keep the scalar path and the
> symbol gate, choose your own baseline.

## Layout

```text
include/template_ext/     public headers + contracts + static assertions
src/extension.cpp         SourceMod extension glue (optional target)
src/base/                 scalar reference implementations
src/isa/avx2/             ISA variants (target attribute, not global -mavx2)
src/dispatch.cpp          CPUID detection + selection rule
spec/                     human-readable contracts / invariants
verify/                   Z3 models (dispatch, completeness, distance, sorting)
tests/                    host-side tests + destructive smoke tests
bench/                    nanobench benchmark
third_party/              vendored header-only wheels
docs/ISA.md               ISA/ABI rules
cmake/check_glibc.cmake   GLIBC ceiling check
```

## Quick start (host only)

```bash
# native 64-bit host
cmake -S . -B build -DTEMPLATE_BUILD_EXTENSION=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build -L smoke --output-on-failure

# force 32-bit (needs gcc-multilib)
cmake -S . -B build32 -DTEMPLATE_BUILD_EXTENSION=OFF -DTEMPLATE_ARCH=32
cmake --build build32 -j
ctest --test-dir build32 --output-on-failure

# sanitizers
cmake -S . -B build-san -DTEMPLATE_BUILD_EXTENSION=OFF -DTEMPLATE_SANITIZE=address,undefined

# L4D2 strict profile (clang++-strict + lifetime annotations)
cmake -S . -B build-strict \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-strict.cmake \
  -DTEMPLATE_BUILD_EXTENSION=OFF
cmake --build build-strict -j && ctest --test-dir build-strict --output-on-failure
```

Formal checks (requires `pip install z3-solver`):

```bash
cmake --build build --target verify
```

## API fool-proofing

The primitive returns `bool` and validates its arguments:

```cpp
using DistanceBatchFn = bool (*)(...);

bool ValidDistanceBatch(...);   // rejects nulls (count>0) and count*3 overflow
```

Implementations return `false` instead of crashing on invalid input; the
extension native maps `false` to `0`.

## Build the extension

```bash
cmake -S . -B build32 \
  -DSM_SDK_DIR=/path/to/sourcemod \
  -DHL2SDK_DIR=/path/to/hl2sdk-l4d2 \
  -DMMSOURCE_DIR=/path/to/metamod-source \
  -DTEMPLATE_ARCH=32 \
  -DTEMPLATE_GLIBC_MAX=2.31
cmake --build build32 --target template_ext -j
cmake --build build32 --target check_glibc
```

The output is `template.ext.2.l4d2.so` (32-bit i386) with
`-static-libgcc -static-libstdc++` and `--strip-debug`.

## ISA dispatch pattern

- The AVX2 function uses `__attribute__((target("avx2,fma")))`; the rest of the
  translation unit stays baseline. Do **not** compile the whole extension with
  `-mavx2` (CMake rejects explicit flags; `docs/ISA.md` explains distro baselines).
- `dispatch.cpp` reads CPUID once, then `SelectDistanceBatchFn` returns either
  `DistanceBatchScalar` or `DistanceBatchAVX2`.
- A scalar implementation must always exist. Add ConVar/env overrides in real
  extensions.

## Formal support (minimum)

| Level | Requirement | Example here |
|---|---|---|
| L0 | contract in `spec/` | `spec/distance.md` |
| L1 | invariant proof for base implementation | `verify_sort.py` (sorting network) |
| L2 | equivalence/invariant proof for the accelerated primitive | `verify_distance.py` |
| L3 | dispatch safety proof | `verify_dispatch.py` |
| L4 | **completeness**: size arithmetic + validity totality + dispatch totality (32/64-bit) | `verify_completeness.py` |

Any ISA-accelerated extension must provide **L1 + L2 + L3 + L4**; a base
implementation must provide **L1 + L4**. The Z3 models are spec-level (not
bit-exact float models) — extend them when the contract needs it.

## Vendored wheels

| 库 | 用途 | License |
|---|---|---|
| `radix_heap.h` (iwiwi/radix-heap) | 单调优先队列（Dijkstra） | MIT（见 README） |
| `robin_hood.h` | 哈希表 | MIT |
| `nanobench.h` | 基准 | MIT |
| `ska_sort.hpp` | 通用 radix 排序参考 | 上游未附 LICENSE，使用前确认 |

See `third_party/README.md` for sources and commits.

## License

GPL-3.0 (matches the SourceMod/Anne ecosystem). Vendored headers keep their
upstream licenses.

See `verify/README.md` for what Z3 proves and what it does not.
