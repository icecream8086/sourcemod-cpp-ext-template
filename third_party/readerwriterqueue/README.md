# ReaderWriterQueue (vendored)

| 项 | 值 |
|---|---|
| 上游 | https://github.com/cameron314/readerwriterqueue |
| 版本 | master snapshot（2026-11-21） |
| License | Simplified BSD（见 `LICENSE.md`） |
| 文件 | `readerwriterqueue.h`、`atomicops.h` |
| 用途 | 后台线程与主线程之间的无锁队列（#11 第一条：hook 只入队） |

只 vendor 两个头文件；不包含 benchmarks/tests。
