# Vendored wheels

Copied for convenience; update manually and keep the origin URL in this file.
Only header-only libraries are allowed (no new shared objects / GLIBC symbols).

| 目录 | 上游 | License | 用途 | 当前是否使用 |
|---|---|---|---|---|
| `radix-heap/` | https://github.com/iwiwi/radix-heap | MIT（Copyright 2015 Takuya Akiba） | 单调优先队列（Dijkstra） | 未接入，待 `anne_spawn_accel` 优化 |
| `ska_sort/` | https://github.com/skarupke/ska_sort | Boost Software License 1.0 | 通用 radix 排序参考 | 仅参考，未接入 |
| `robin_hood/` | https://github.com/martinus/robin-hood-hashing | MIT | 哈希表 | 未接入 |
| `nanobench/` | https://github.com/martinus/nanobench | MIT | 基准测试 | **已使用**：`bench/bench_distance.cpp` |

## 为什么是这几个

- **radix-heap**：Dijkstra 的单调优先队列，官方称常比 `std::priority_queue` 快约 2×；对应我们测到的 reachability 热点。
- **ska_sort**：通用 radix 混合排序，用来和自写 radix 做 A/B；只作为参考实现。
- **robin_hood**：构建期 `idToIndex` 哈希，非热路径，收益小但替换成本低。
- **nanobench**：header-only 基准，模板自带 benchmark 直接用。

AVX2/SSE 相关的 `highway`、`xsimd` **没有 vendor**：它们的运行时分发和模板自己的 dispatcher 有重叠，等算法层榨干后再评估。

## 引入规则

1. 只放 header（`.h`/`.hpp`）+ LICENSE/README；
2. 在 `third_party/README.md` 登记来源、License、用途、是否接入；
3. 不在业务代码里直接 `#include` 整库入口除非确实使用；
4. 升级时单独提交，跑一遍 `ctest`、`check_isa` 与 `verify`。
