#include <cmath>
#include <cstdio>
#include <limits>
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

template <typename Fn>
void RunCase(Fn fn, std::size_t count, std::size_t offset, const char *label)
{
    std::vector<float> ax(count + offset, 1.0f), ay(count + offset, 2.0f), az(count + offset, 3.0f);
    std::vector<float> bx(count + offset, 2.0f), by(count + offset, 3.0f), bz(count + offset, 4.0f);
    std::vector<float> out(count + 1, -1.0f);
    const bool ok = fn(ax.data() + offset, ay.data() + offset, az.data() + offset,
                       bx.data() + offset, by.data() + offset, bz.data() + offset,
                       out.data(), count);
    Check(ok, label);
    for (std::size_t i = 0; i < count; ++i)
        Check(std::isfinite(out[i]), "finite output");
    Check(out[count] == -1.0f, "output tail is untouched");
}

void TestSizesAndOffsets()
{
    for (std::size_t count : {0u, 1u, 7u, 8u, 9u, 31u, 32u})
    {
        for (std::size_t offset : {0u, 1u, 3u})
        {
            RunCase(template_ext::DistanceBatchScalar, count, offset, "scalar finite");
        }
    }

    if (template_ext::DetectCpuFeatures().avx2)
    {
        for (std::size_t count : {0u, 1u, 7u, 8u, 9u, 31u, 32u})
        {
            for (std::size_t offset : {0u, 1u, 3u})
            {
                RunCase(template_ext::DistanceBatchAVX2, count, offset, "avx2 finite");
            }
        }
    }
    else
    {
        std::printf("skip AVX2 size/offset smoke: CPU lacks AVX2\n");
    }
}

void TestNonFiniteInputs()
{
    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float ax[1] = {inf}, ay[1] = {0}, az[1] = {0};
    const float bx[1] = {0}, by[1] = {0}, bz[1] = {0};
    float out[1] = {};
    template_ext::DistanceBatchScalar(ax, ay, az, bx, by, bz, out, 1);
    Check(std::isinf(out[0]), "inf input propagates without crash");

    const float nx[1] = {nan};
    template_ext::DistanceBatchScalar(nx, ay, az, bx, by, bz, out, 1);
    Check(std::isnan(out[0]), "nan input propagates without crash");
}
}

int main()
{
    TestSizesAndOffsets();
    TestNonFiniteInputs();
    if (g_Failures != 0)
    {
        std::printf("%d smoke failure(s)\n", g_Failures);
        return 1;
    }
    std::printf("smoke edge cases passed\n");
    return 0;
}
