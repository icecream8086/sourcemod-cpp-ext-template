# Z3 checks: what they prove, and what they do not

## What is checked

| 文件 | 证明内容 |
|---|---|
| `verify_dispatch.py` | 分发规则安全：缺特性/强制 scalar 时不会选 AVX2；选择是全域的 |
| `verify_completeness.py` | 32/64 位下 `count*3` 不溢出、`ValidDistanceBatch` 与指针条件完全一致、dispatch 全函数 |
| `verify_distance.py` | 距离多项式的两种写法等价、非负、对称（实数模型） |
| `verify_sort.py` | 4 元排序网络输出有序且是输入的一个置换（0..3 有界） |

## 不能证明什么

- **不证明 C++ 代码本身**：模型与实现是两份东西，没有 refinement 证明；
- **不建模浮点舍入/NaN**（距离证明是实数级）；
- **不覆盖内存安全/UB/数据竞争**——那是 ASan/UBSan、静态分析、代码审查的职责；
- **不覆盖引擎状态**（NextBot 指针、SourceMod 生命周期等）。

## 模型正确性比求解器更重要

一个真实的反例：最初 `verify_completeness.py` 用 `bits` 宽的 `count * 3` 与 `UINT_MAX` 比较，结果恒真——因为无符号乘法回绕后的结果永远 ≤ `UINT_MAX`，Z3 会"证明"一个空洞的命题。修法是零扩展到 `bits+2` 位再比较。

> Z3 只回答"模型里的命题是否成立"；模型错了，它会给你**假信心**。

## 运行

```bash
python3 -m pip install z3-solver
cmake --build build --target verify
```

## 建议用法

- L3/L4（分发、尺寸算术、接口完备性）适合 Z3，收益最高；
- L1/L2 只在契约简单、模型忠实时使用；
- 不要声称"整个扩展被形式化验证"；正确的说法是"这些契约在这些假设下被证明"。
