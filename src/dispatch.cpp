#include "template_ext/distance.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

namespace template_ext
{
CpuFeatures DetectCpuFeatures()
{
    CpuFeatures features;
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();
    features.sse42 = __builtin_cpu_supports("sse4.2");
    features.avx2 = __builtin_cpu_supports("avx2");
#if defined(__x86_64__) || defined(__i386__)
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        features.osxsave = (ecx & (1u << 27)) != 0;
#endif
#endif
    return features;
}

DistanceBatchFn SelectDistanceBatchFn(const CpuFeatures &features,
                                      bool forceScalar)
{
    if (forceScalar)
        return &DistanceBatchScalar;
    if (features.avx2 && features.osxsave)
        return &DistanceBatchAVX2;
    return &DistanceBatchScalar;
}
}
