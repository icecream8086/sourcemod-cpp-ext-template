#include <cstdio>
#include <vector>

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

void TestFoolProofing()
{
    float out[1] = {};
    const float one[1] = {1.0f};

    Check(!template_ext::DistanceBatchScalar(one, one, one, one, one, one, nullptr, 1),
          "null output rejected");
    Check(!template_ext::DistanceBatchScalar(one, nullptr, one, one, one, one, out, 1),
          "null component rejected");
    Check(!template_ext::DistanceBatchScalar(one, one, one, one, one, one, out,
                                             template_ext::MaxDistanceBatch() + 1),
          "overflow-sized count rejected");
    Check(template_ext::DistanceBatchScalar(one, one, one, one, one, one, out, 0),
          "empty batch accepted");
}
}

int main()
{
    TestDispatchMatrix();
    TestFoolProofing();
    if (g_Failures != 0)
    {
        std::printf("%d smoke failure(s)\n", g_Failures);
        return 1;
    }
    std::printf("smoke dispatch passed\n");
    return 0;
}
