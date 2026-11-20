# Distance primitive contract (SoA)

`template_ext::DistanceBatchFn`

```cpp
void fn(const float *ax, const float *ay, const float *az,
        const float *bx, const float *by, const float *bz,
        float *out, std::size_t count);
```

- Arrays are structure-of-arrays: `count` elements each.
- For every `i` in `[0, count)`:
  `out[i] = sqrt((ax[i]-bx[i])^2 + (ay[i]-by[i])^2 + (az[i]-bz[i])^2)`
- `count == 0` is a no-op.
- Output must be finite for finite inputs.
- Symmetric in `a`/`b`; non-negative.

SoA is intentionally chosen: the ISA variant becomes plain contiguous loads
(no gathers), and the scalar fallback stays trivial.

## Implementations

| 实现 | 要求 |
|---|---|
| `DistanceBatchScalar` | 基线，任何 CPU 可执行 |
| `DistanceBatchAVX2` | 仅当 `avx2 && osxsave` 时被 `SelectDistanceBatchFn` 选中；函数带 `target("avx2,fma")`，其余 TU 保持 baseline |

## Formal support

- `verify/verify_dispatch.py`：证明选择规则不会在缺特性时选 AVX2，强制 scalar 一定走 scalar。
- `verify/verify_distance.py`：证明两种写法的多项式等价与非负性（实数模型，不含浮点舍入）。
- 如需 bit-exact 证明，需建立 IEEE-754 模型或使用 translation validation；模板只要求 spec 级证明 + 测试。
