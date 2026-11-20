#include "template_ext/distance.h"

#include <cmath>

#if (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>
#define TEMPLATE_X86 1
#endif

namespace template_ext
{
#if defined(TEMPLATE_X86) && !defined(TEMPLATE_DISABLE_AVX2)
__attribute__((target("avx2,fma")))
#endif
bool DistanceBatchAVX2(const float *ax, const float *ay, const float *az,
                       const float *bx, const float *by, const float *bz,
                       float *out, std::size_t count)
{
    if (!ValidDistanceBatch(ax, ay, az, bx, by, bz, out, count))
        return false;
#if defined(TEMPLATE_X86) && !defined(TEMPLATE_DISABLE_AVX2)
    std::size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        const __m256 dx = _mm256_sub_ps(_mm256_loadu_ps(ax + i), _mm256_loadu_ps(bx + i));
        const __m256 dy = _mm256_sub_ps(_mm256_loadu_ps(ay + i), _mm256_loadu_ps(by + i));
        const __m256 dz = _mm256_sub_ps(_mm256_loadu_ps(az + i), _mm256_loadu_ps(bz + i));
        __m256 sq = _mm256_mul_ps(dz, dz);
        sq = _mm256_fmadd_ps(dy, dy, sq);
        sq = _mm256_fmadd_ps(dx, dx, sq);
        _mm256_storeu_ps(out + i, _mm256_sqrt_ps(sq));
    }
    for (; i < count; ++i)
    {
        const float dx = ax[i] - bx[i];
        const float dy = ay[i] - by[i];
        const float dz = az[i] - bz[i];
        out[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
#else
    DistanceBatchScalar(ax, ay, az, bx, by, bz, out, count);
#endif
    return true;
}
}
