# AGENTS.md

本仓是 **SourceMod C++ 扩展模板**，供后续扩展复用（32/64 位、L4D2 与其他 SM 游戏）。修改前先读 `docs/ISA.md` 与 `README.md`。

## 快速命令

```bash
cmake -S . -B build -DTEMPLATE_BUILD_EXTENSION=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
ctest --test-dir build -L smoke --output-on-failure

cmake -S . -B build32 -DTEMPLATE_BUILD_EXTENSION=OFF -DTEMPLATE_ARCH=32
cmake --build build32 -j && ctest --test-dir build32 --output-on-failure

cmake -S . -B build-san -DTEMPLATE_BUILD_EXTENSION=OFF -DTEMPLATE_SANITIZE=address,undefined
cmake --build build-san -j && ctest --test-dir build-san --output-on-failure

python3 -m pip install z3-solver
cmake --build build --target verify
```

扩展目标（需要 SM/HL2/MM SDK）：

```bash
cmake -S . -B build32 -DSM_SDK_DIR=... -DHL2SDK_DIR=... -DMMSOURCE_DIR=... -DTEMPLATE_ARCH=32
cmake --build build32 --target template_ext
cmake --build build32 --target check_glibc
```

## 不允许做的事

1. **不要**加全局 `-mavx2` / `-march=native`（CMake 拦截显式 flag；发行版默认可能仍是 x86-64-v3，发布构建请用老基线容器）；
2. **不要**在业务代码直接调用 `DistanceBatchAVX2` 等 ISA 函数——只能通过 `SelectDistanceBatchFn`；
3. **不要**删除标量实现或让它与 ISA 版本不一致；
4. **不要**写"假定输入一定合法"的 API：新原语必须返回 `bool` 并用 `Valid*` 守卫空指针/乘法溢出；
5. **不要**跳过 smoke / sanitizer / verify 就提交；
6. **不要**在热路径里反复 `cpuid`/`__builtin_cpu_supports`；
7. **不要**假定 32 位或 64 位——用 `TEMPLATE_ARCH` + `size_t`/`uintptr_t`，CI 两边都跑。

## 新增原语 / ISA 变体

1. 在 `include/<ext>/` 写契约（SoA 优先）+ 编译期 `static_assert`（float 宽度、指针宽度）；
2. 定义 `Valid<Primitive>`（空指针、`count * stride` 溢出）并让实现返回 `bool`；
3. `src/base/` 写标量参考实现；
4. `src/isa/<isa>/` 写变体，函数加 `__attribute__((target("<isa>")))`；
5. `src/dispatch.cpp` 增加检测与选择（force scalar 最高优先级）；
6. `verify/verify_dispatch.py`（L3）与 `verify_completeness.py`（L4）同步；
7. `verify/verify_<primitive>.py` 增加 L2 证明；
8. `tests/smoke_*.cpp` 增加边界/不支持路径/null/溢出用例；
9. 更新 `docs/ISA.md` 特性表与 `third_party/README.md`。

## 完成定义（DoD）

- [ ] 标量 + ISA 两个实现，结果一致（count=0/奇数/批量边界/非对齐/NaN/Inf）
- [ ] 非法输入返回 `false`，不崩（null、超大 count）
- [ ] `ctest -L smoke` 通过
- [ ] `ctest`（sanitizer 构建）通过
- [ ] `cmake --build build --target verify` 通过（L1–L4）
- [ ] `TEMPLATE_ARCH=32` 与 native 构建都通过
- [ ] `TEMPLATE_DISABLE_AVX2=ON` 构建通过
- [ ] 扩展目标 `check_glibc` 通过（如构建）
- [ ] 日志能看出探测结果与实际选择
- [ ] 文档/AGENTS 同步

## 常见坑

- 用 `-march=native` 编整个扩展 → 老 CPU `SIGILL`；
- 只测自己的 AVX2 机器 → unsupported 分支没走到；
- 把 AVX2 函数放进公共头文件 → 用户会直接调用；
- 改了选择规则但没改 Z3 模型 → 证明与代码脱节；
- 32 位下用 `size_t` 以外的宽度算 `count * 3` → 溢出；
- 引入非 header-only 轮子 → 新增 `.so`/GLIBC 符号。
