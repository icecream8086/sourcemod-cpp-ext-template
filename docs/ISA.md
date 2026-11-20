# ISA 管理规则

> 编译器很聪明，但用户不一定；指令集管理必须由模板强制，而不是靠自觉。

## 支持的特性与探测

| 特性 | 探测表达式 | 备注 |
|---|---|---|
| SSE4.2 | `__builtin_cpu_supports("sse4.2")` | 基线之上 |
| AVX | `__builtin_cpu_supports("avx") && osxsave` | 需要 OS 支持 XSAVE |
| AVX2 | `__builtin_cpu_supports("avx2") && osxsave` | 32 位 i386 只有 8 个 `ymm` |
| FMA | `__builtin_cpu_supports("fma")` | 依赖 AVX |
| BMI2 | `__builtin_cpu_supports("bmi2")` | 32 位只有 32 位形式 |
| AVX-512 | `__builtin_cpu_supports("avx512f") && osxsave` | 需检查 opmask/ZMM 状态 |

`osxsave` 来自 CPUID.1:ECX[27]；`__builtin_cpu_supports` 会做 OS 检查，但模板仍显式记录，便于日志。

## 红线

1. **禁止**全局 `-mavx*` / `-march=native` / `-mtune=native`；
2. **禁止**在业务代码里直接调用 `DistanceBatchAVX2` 这类 ISA 函数——只能通过 dispatcher；
3. **禁止**没有标量回退的 ISA 实现；
4. **禁止**在热路径反复探测 CPUID；
5. **禁止**只测有 AVX2 的机器——必须覆盖 unsupported 分支与 `TEMPLATE_DISABLE_AVX2=ON` 构建。

CMake 会在配置阶段拦截**显式**全局 ISA 标志（`TEMPLATE_ALLOW_UNSAFE_ISA_FLAGS=ON` 才能绕过）。注意发行版编译器可能默认开启 x86-64-v3（不经过命令行），所以发布构建必须在老基线容器里做；头文件无法可靠区分"用户加的 flag"和"发行版默认值"。

## 新增一个 ISA 变体的步骤

1. 在 `src/isa/<isa>/` 新建实现文件，函数加 `__attribute__((target("<isa>")))`；
2. 在 `include/<ext>/...h` 增加声明（放 `detail` 命名空间），dispatcher 增加一个特性分支；
3. 在 `verify/verify_dispatch.py` 里同步选择规则（L3）；
4. 在 `verify/` 增加该原语的等价/不变量证明（L2）；
5. 在 `tests/smoke_*.cpp` 增加该变体的边界/冒烟用例；
6. 更新本文档的特性表。

## 为什么不能整库 `-mavx2`

`-mavx2` 会让编译器在**任何**函数里使用 AVX 指令（包括初始化、日志、异常路径），老 CPU 一加载就 `SIGILL`。target attribute 只影响被标注的函数，其余保持 baseline；dispatcher 保证不调用不支持的函数。

## 32 位注意事项

- `ymm` 只有 8 个，AVX2 能带来的并行度有限；
- BMI2 的 `pext/pdep` 只有 32 位形式；
- 通用寄存器少，SoA 布局比 AoS 更容易让编译器生成好代码。

## 位宽与 ABI（32 / 64）

模板不假设目标位宽：

| 选项 | 效果 |
|---|---|
| `TEMPLATE_ARCH=native`（默认） | 跟随编译器默认 |
| `TEMPLATE_ARCH=32` | 全局 `-m32`，并 `static_assert(sizeof(void*) == 4)` |
| `TEMPLATE_ARCH=64` | 并 `static_assert(sizeof(void*) == 8)` |

- L4D2 是 32 位 i386；SourceMod 也有 64 位游戏，模板要两边都能编；
- 指针宽度、`size_t` 宽度不得手写常量，统一用 `size_t` / `uintptr_t`；
- `ValidDistanceBatch` 的 `count * 3` 溢出检查已用 Z3 在 **32/64 位**下证明（L4）；
- 32/64 位构建在 CI 矩阵里都跑（`arch: native, 32`）；
- GLIBC 基线随架构不同，`check_glibc` 只对当前产物生效。
