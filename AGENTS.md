# AGENTS.md

本仓是 **SourceMod C++ 扩展模板**，供后续扩展复用。修改前先读 `docs/ISA.md` 与 `README.md`。

## 快速命令

```bash
cmake -S . -B build -DTEMPLATE_BUILD_EXTENSION=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build -L smoke --output-on-failure
python3 -m pip install z3-solver
cmake --build build --target verify
```

扩展目标（需要 SM/HL2/MM SDK）：

```bash
cmake -S . -B build32 -DSM_SDK_DIR=... -DHL2SDK_DIR=... -DMMSOURCE_DIR=...
cmake --build build32 --target template_ext
cmake --build build32 --target check_glibc
```

## 不允许做的事

1. **不要**在 `CMakeLists.txt` 或命令行加全局 `-mavx2` / `-march=native`（CMake 会拦截；`distance.h` 也有 `#error`）；
2. **不要**在业务代码直接调用 `DistanceBatchAVX2` 等 ISA 函数——只能通过 `SelectDistanceBatchFn`；
3. **不要**删除标量实现或让它与 ISA 版本不一致；
4. **不要**跳过 smoke/verify 就提交；
5. **不要**在热路径里反复 `cpuid`/`__builtin_cpu_supports`；
6. **不要**把 `TEMPLATE_DISABLE_AVX2=ON` 的构建漏掉——CI 必须覆盖。

## 新增原语 / ISA 变体

1. 在 `include/<ext>/` 写契约（SoA 优先），`spec/` 写不变量；
2. `src/base/` 写标量参考实现；
3. `src/isa/<isa>/` 写变体，函数加 `__attribute__((target("<isa>")))`；
4. `src/dispatch.cpp` 增加检测与选择（保持"force scalar 最高优先级"）；
5. `verify/verify_dispatch.py` 同步规则（L3）；
6. `verify/verify_<primitive>.py` 增加 L2 证明；
7. `tests/smoke_*.cpp` 增加边界/不支持路径用例；
8. 更新 `docs/ISA.md` 特性表与 `third_party/README.md`（如有新轮子）。

## 完成定义（DoD）

- [ ] 标量 + ISA 两个实现，结果一致（测试覆盖 count=0/奇数/批量边界/非有限输入）
- [ ] `ctest -L smoke` 通过
- [ ] `cmake --build build --target verify` 通过（L1+L2+L3）
- [ ] `TEMPLATE_DISABLE_AVX2=ON` 构建通过
- [ ] 扩展目标 `check_glibc` 通过（如构建）
- [ ] 日志能看出探测结果与实际选择
- [ ] 文档/AGENTS 同步

## 常见坑

- 用 `-march=native` 编整个扩展 → 老 CPU `SIGILL`；
- 只测自己的 AVX2 机器 → unsupported 分支没走到；
- 把 AVX2 函数放进公共头文件 → 用户会直接调用；
- 改了选择规则但没改 Z3 模型 → 证明与代码脱节；
- 引入非 header-only 轮子 → 新增 `.so`/GLIBC 符号。
