# SourceMod C++ Extension Template

A reusable template for SourceMod C++ extensions with:

- **CMake** build (extension target + host-side tests/bench),
- **runtime ISA dispatch** (CPUID detection + function pointers + scalar fallback),
- **Z3 formal checks** for dispatch safety and algorithm invariants,
- **destructive smoke tests** (unsupported ISA paths, edge sizes, forced scalar, disabled-AVX2 build),
- **vendored header-only wheels** (radix heap, robin_hood, nanobench; ska_sort reference),
- **GLIBC symbol ceiling gate** for old-distro portability.

See `docs/ISA.md` for ISA rules and `AGENTS.md` for the contribution/DoD checklist.

> Compatibility policy: the template provides detection, dispatch, and a GLIBC
> check **as features**. What "compatible" means for your server is your call —
> this repo does not promise legacy runtimes. Keep the scalar path and the
> symbol gate, choose your own baseline.

## Layout

```text
include/template_ext/     public headers + contracts
src/extension.cpp         SourceMod extension glue (optional target)
src/base/                 scalar reference implementations
src/isa/avx2/             ISA variants (target attribute, not global -mavx2)
src/dispatch.cpp          CPUID detection + selection rule
spec/                     human-readable contracts / invariants
verify/                   Z3 models (dispatch, distance, sorting)
tests/                    host-side tests (no SM SDK needed)
bench/                    nanobench benchmark
third_party/              vendored header-only wheels
cmake/check_glibc.cmake   GLIBC ceiling check
```

## Quick start (host only)

```bash
cmake -S . -B build -DTEMPLATE_BUILD_EXTENSION=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/template_bench            # needs TEMPLATE_BUILD_BENCH=ON
```

Formal checks (requires `pip install z3-solver`):

```bash
cmake --build build --target verify
```

## Build the extension

```bash
cmake -S . -B build32 \
  -DSM_SDK_DIR=/path/to/sourcemod \
  -DHL2SDK_DIR=/path/to/hl2sdk-l4d2 \
  -DMMSOURCE_DIR=/path/to/metamod-source \
  -DTEMPLATE_GLIBC_MAX=2.31
cmake --build build32 --target template_ext -j
cmake --build build32 --target check_glibc
```

The output is `template.ext.2.l4d2.so` (32-bit i386), with
`-static-libgcc -static-libstdc++` and `--strip-debug`.

## ISA dispatch pattern

- The AVX2 function uses `__attribute__((target("avx2,fma")))`; the rest of the
  translation unit stays baseline. Do **not** compile the whole extension with
  `-mavx2`.
- `dispatch.cpp` reads CPUID once, then `SelectDistanceBatchFn` returns either
  `DistanceBatchScalar` or `DistanceBatchAVX2`.
- A scalar implementation must always exist. Add ConVar/env overrides in real
  extensions (see `docs/anne/52-通用C++扩展标准` for the policy).

## Formal support (minimum)

| Level | Requirement | Example here |
|---|---|---|
| L0 | contract in `spec/` | `spec/distance.md` |
| L1 | invariant proof for base implementation | `verify_sort.py` (sorting network) |
| L2 | equivalence/invariant proof for the accelerated primitive | `verify_distance.py` |
| L3 | dispatch safety proof | `verify_dispatch.py` |

Any ISA-accelerated extension must provide **L1 + L2 + L3**; a base
implementation must provide **L1**. The Z3 models are spec-level (not
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
