# Vendored wheels

Copied for convenience; update manually and keep the origin URL/commit in this
file. Only header-only libraries are allowed (no new shared objects / GLIBC
symbols).

| 目录 | 上游 | 版本/日期 | License | 用途 |
|---|---|---|---|---|
| `radix-heap/` | https://github.com/iwiwi/radix-heap | master @ 2026-11-20 | MIT（LICENSE 见 README） | 单调优先队列（Dijkstra） |
| `ska_sort/` | https://github.com/skarupke/ska_sort | master @ 2026-11-20 | 上游未附 LICENSE，使用前确认 | 通用 radix 排序参考 |
| `robin_hood/` | https://github.com/martinus/robin-hood-hashing | master @ 2026-11-20 | MIT | 哈希表 |
| `nanobench/` | https://github.com/martinus/nanobench | master @ 2026-11-20 | MIT | 基准测试 |

## 引入规则

1. 只放 header（`.h`/`.hpp`）+ LICENSE/README；
2. 在 `third_party/README.md` 登记来源与日期；
3. 不在业务代码里直接 `#include` 整库入口除非确实使用；
4. 升级时单独提交，跑一遍 `ctest` 与 `verify`。
