# L4D2 Strict Profile

> 适用范围：L4D2（SourceMod + Metamod:Source，**32 位** `srcds_linux`）上的 C++ 扩展。
> 模板本身还支持 64 位与其他 SourceMod 游戏；**只有 L4D2 扩展必须走本 profile**。

## 0. 一句话

**能在编译期挡住的错误，不许留到运行期。** 32 位 srcds 崩一次就是全服掉线，UAF/悬垂/双重释放是最贵的一类。

## 1. 怎么启用

```bash
cmake -S . -B build-strict \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-strict.cmake \
  -DTEMPLATE_BUILD_EXTENSION=OFF
cmake --build build-strict -j
ctest --test-dir build-strict --output-on-failure
```

需要 `clang++-strict`：

- 构建机可预装 `clang++-strict`；
- 其他环境把 `scripts/clang-strict.sh` 装成 `clang++-strict`（会探测旧 Clang 是否支持 lifetime flag）。

## 2. 硬规矩与模板落点

| 硬规矩 | 模板落点 |
|---|---|
| 编译期拉满 | `clang++-strict`：`-std=c++20 -Xclang -fexperimental-lifetime-safety -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual -Werror` |
| lifetime 注解 | `#include <strict/lifetime.hpp>`，返回引用/指针且寿命挂在入参或 `*this` 上 → `STRICT_LIFETIMEBOUND` |
| 线程安全 | 后台线程只入队；回主线程再调 SM/引擎 API |
| 地址不硬编码 | 按名/签名+相对偏移+BuildID 校验；多候选，防误命中 |
| 32 位天花板 | edict 2048、用户态 2–3GB，不可突破；摸到就进程外方案 |
| 性能 | hook 后仅入队；事件驱动零轮询；日志绕 `-condebug` |
| 实体安全 | 用 SourceMod 实体引用（EntRef），不存裸 `CBaseEntity*`；尊重 edict 复用 ~0.5 s；跨 round/map 不存索引 |
| 恒真式/自证 | Z3 模型必须独立于实现；恒真式、自证循环、静默丢数据一律打回 |
| 交付带验证数字 | 改了什么 + 命令 + p50/p95/计数 + 没做到的照实标 |
| 产物形态 | SourceMod C++ 扩展（`.so`）；SourceHook + 数据表按名/按槽；兼容 SM/MM 版本 + 游戏 build |

## 3. 注解写法

```cpp
#include <strict/lifetime.hpp>

const AnneNavGraph& GetGraph() STRICT_LIFETIMEBOUND;          // 挂在 *this
const Area& Lookup(const Graph& g, uint32_t id STRICT_LIFETIMEBOUND);
std::string Name();                                       // 返回值，不标
```

⚠️ 不标 = 分析沉默，连悬垂都不报；不要求给“拥有返回值”的函数乱标。

## 4. 第三方/SDK 头

- SDK 头一律 `-isystem`（CMake：`target_include_directories(tgt SYSTEM ...)`）；
- 确需放宽，`STRICT_NO_WERROR=1`，并在 MR/PR 写清“哪几个接口、为什么”；
- 不允许默默改全局 flags 过检查。

## 5. 本模板已实际踩到的例子

`anne-cpp-suite` 的 `nav_graph.cpp` 在 strict 下报了 4 个：

```
error: implicit conversion changes signedness: 'std::size_t' to 'difference_type'
  candidates.begin() + begin
```

改成 `candidates.begin() + static_cast<std::ptrdiff_t>(begin)` 后通过；strict 构建的 p50 522 µs（GCC 597 µs，编译器不同，仅参考）。

## 6. 交付检查

- [ ] strict 构建 + `ctest` 通过
- [ ] 新接口该标 `STRICT_LIFETIMEBOUND` 的都标了
- [ ] 没有裸地址；签名/偏移有 BuildID 校验
- [ ] 后台线程没有直接调 SM/引擎
- [ ] 实体用 EntRef，不跨 round/map 存索引
- [ ] MR/PR 有验证数字（p50/p95、实体计数、GC µs）
- [ ] 放宽项写明接口与原因
