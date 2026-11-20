#include <cstdio>

#include "template_ext/distance.h"

namespace
{
int g_Failures = 0;

void Check(bool condition, const char *what)
{
    if (!condition)
    {
        std::printf("FAIL: %s\n", what);
        ++g_Failures;
    }
}

// Destructive smoke test: enumerate every feature combination and force flag.
// We only *select* function pointers here; we never call the AVX2 function on
// a CPU that does not advertise AVX2, because that is exactly the crash this
// dispatch layer exists to prevent.
void TestDispatchMatrix()
{
    for (int bits = 0; bits < 8; ++bits)
    {
        template_ext::CpuFeatures f;
        f.sse42 = (bits & 1) != 0;
        f.avx2 = (bits & 2) != 0;
        f.osxsave = (bits & 4) != 0;
        const bool usableAvx2 = f.avx2 && f.osxsave;

        for (int force = 0; force <= 1; ++force)
        {
            const auto fn = template_ext::SelectDistanceBatchFn(f, force != 0);
            Check(fn != nullptr, "selection must never return null");
            if (force)
                Check(fn == &template_ext::DistanceBatchScalar, "force scalar");
            else if (usableAvx2)
                Check(fn == &template_ext::DistanceBatchAVX2, "usable avx2 selected");
            else
                Check(fn == &template_ext::DistanceBatchScalar, "unsupported -> scalar");
        }
    }
}

void TestScalarNeverCrashes()
{
    const float one[1] = {1.0f};
    float out[1] = {};
    template_ext::DistanceBatchScalar(one, one, one, one, one, one, out, 1);
    Check(out[0] == 0.0f, "same point distance is zero");

    template_ext::DistanceBatchScalar(nullptr, nullptr, nullptr,
                                      nullptr, nullptr, nullptr,
                                      out, 0);
    Check(true, "count=0 does not dereference inputs");
}
}

int main()
{
    TestDispatchMatrix();
    TestScalarNeverCrashes();
    if (g_Failures != 0)
    {
        std::printf("%d smoke failure(s)\n", g_Failures);
        return 1;
    }
    std::printf("smoke dispatch passed\n");
    return 0;
}
