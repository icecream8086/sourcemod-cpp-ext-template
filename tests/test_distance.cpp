#include "template_ext/distance.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

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

void TestScalarContract()
{
    const float ax[] = {0}, ay[] = {0}, az[] = {0};
    const float bx[] = {3}, by[] = {4}, bz[] = {0};
    float out[1] = {};
    template_ext::DistanceBatchScalar(ax, ay, az, bx, by, bz, out, 1);
    Check(std::fabs(out[0] - 5.0f) < 1e-5f, "3-4-0 distance = 5");
}

void TestBatchEquivalence()
{
    const bool canRunAvx2 = template_ext::DetectCpuFeatures().avx2;
    if (!canRunAvx2)
    {
        std::printf("skip scalar/avx2 equivalence: CPU does not support AVX2\n");
        return;
    }

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
    for (std::size_t count : {0u, 1u, 7u, 8u, 9u, 100u, 1000u})
    {
        std::vector<float> ax(count), ay(count), az(count);
        std::vector<float> bx(count), by(count), bz(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            ax[i] = dist(rng); ay[i] = dist(rng); az[i] = dist(rng);
            bx[i] = dist(rng); by[i] = dist(rng); bz[i] = dist(rng);
        }

        std::vector<float> scalar(count), avx2(count);
        template_ext::DistanceBatchScalar(ax.data(), ay.data(), az.data(),
                                          bx.data(), by.data(), bz.data(),
                                          scalar.data(), count);
        template_ext::DistanceBatchAVX2(ax.data(), ay.data(), az.data(),
                                        bx.data(), by.data(), bz.data(),
                                        avx2.data(), count);
        for (std::size_t i = 0; i < count; ++i)
        {
            Check(std::isfinite(scalar[i]), "scalar finite");
            Check(std::fabs(scalar[i] - avx2[i]) < 1e-4f * (1.0f + std::fabs(scalar[i])),
                  "scalar/avx2 equivalence");
        }
    }
}

void TestDispatch()
{
    template_ext::CpuFeatures none;
    Check(template_ext::SelectDistanceBatchFn(none, false) == &template_ext::DistanceBatchScalar,
          "no features -> scalar");

    template_ext::CpuFeatures avx2;
    avx2.avx2 = true;
    avx2.osxsave = true;
    Check(template_ext::SelectDistanceBatchFn(avx2, false) == &template_ext::DistanceBatchAVX2,
          "avx2+osxsave -> avx2");

    avx2.osxsave = false;
    Check(template_ext::SelectDistanceBatchFn(avx2, false) == &template_ext::DistanceBatchScalar,
          "avx2 without osxsave -> scalar");

    Check(template_ext::SelectDistanceBatchFn(avx2, true) == &template_ext::DistanceBatchScalar,
          "force scalar wins");

    const template_ext::CpuFeatures real = template_ext::DetectCpuFeatures();
    std::printf("detected: sse4.2=%d avx2=%d osxsave=%d\n",
                real.sse42, real.avx2, real.osxsave);
}
}

int main()
{
    TestScalarContract();
    TestBatchEquivalence();
    TestDispatch();
    if (g_Failures != 0)
    {
        std::printf("%d failure(s)\n", g_Failures);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
