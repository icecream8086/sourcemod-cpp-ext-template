// Third-party wheels smoke test
//
// Compile-only + basic runtime checks for the vendored header-only libraries.
// No engine / SourceMod SDK is required, so this can run in the host test job.

#include <cstdio>

#include <readerwriterqueue.h>
#include <SPSCQueue.h>
#include <tinyfsm.hpp>
#include <spdlog/spdlog.h>

int main()
{
    moodycamel::ReaderWriterQueue<int> queue(64);
    if (!queue.enqueue(7))
        return 1;
    int value = 0;
    if (!queue.try_dequeue(value) || value != 7)
        return 2;

    rigtorp::SPSCQueue<int> spsc(64);
    spsc.push(9);
    int *front = spsc.front();
    if (!front || *front != 9)
        return 3;
    spsc.pop();

    // Keep TinyFSM included so the header is checked by the strict build.
    // A real state machine belongs in an extension, not in this smoke test.

    spdlog::set_level(spdlog::level::off);
    spdlog::info("third_party smoke");

    std::printf("third_party wheels ok\n");
    return 0;
}
