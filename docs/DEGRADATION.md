# 降级保证：任意代数处理器至少可用

目标：**扩展在任意 x86 处理器/虚拟机上都"能跑"，最差只是慢，不能 `SIGILL`。**

## 基线

| 目标 | 基线 | 说明 |
|---|---|---|
| 32 位（L4D2） | i686 / SSE2 | 标量路径不生成 VEX/AVX 指令 |
| 64 位（其他 SM 游戏） | x86-64 / SSE2 | 同上 |

ISA 变体（AVX2/FMA）只允许出现在 `__attribute__((target("...")))` 标注的函数里，
并且只能由 `SelectDistanceBatchFn` 在 CPUID 检查后选择。

## 三层保证

| 层 | 机制 |
|---|---|
| **编译期** | target attribute；禁止全局 `-mavx2/-march=native`（CMake 拦截显式 flag）；`static_assert` 检查 ABI |
| **检测期** | `__builtin_cpu_supports` + 显式 `osxsave`（CPUID.1:ECX[27]）；只探测一次并缓存；force scalar 最高优先级 |
| **验证期** | `scripts/audit_isa.sh` 反汇编基线函数，确认无 `v*`/`ymm`/`zmm`；`scripts/qemu_matrix.sh` 在 `qemu64`/`Nehalem`（无 AVX2）与 `Haswell`（有 AVX2）下跑全部测试 |

## 超售/阉割虚拟机

- 如果 hypervisor **屏蔽** AVX2（CPUID 不报）→ 自动走 scalar，正确；
- 如果 **报 AVX2 但实际很慢**（超售、模拟）→ 仍然正确，只是性能差，这是"可用性"而非"性能"保证；
- 如果 CPUID **谎报**（报有但执行非法）→ 任何用户态程序都无法预防，属于 hypervisor 缺陷。

## 不能保证什么

- 不能保证 ISA 变体一定被选中（性能问题，不是正确性问题）；
- 不能在运行中捕获 `SIGILL` 后热切换（不可靠）；
- 不覆盖非 x86 架构（ARM 等需要单独的 `src/isa/` 分支和探测）。

## 运行验证

```bash
cmake --build build --target check_isa     # 反汇编审计
sudo apt-get install qemu-user
bash scripts/qemu_matrix.sh build          # 旧/新 CPU 模型矩阵
```

CI 同时跑：`arch: native/32` × `avx2: ON/OFF`、sanitizer、`check_isa`、QEMU 矩阵。
