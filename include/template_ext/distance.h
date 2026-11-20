#pragma once

#include <cstddef>

// Note: some distro compilers default to an x86-64-v3 baseline (__AVX2__ is
// defined without any explicit flag). The template therefore cannot reliably
// detect "user added -mavx2" from a header; CMake checks explicit flags and
// docs/ISA.md explains why you must build in a baseline container.

namespace template_ext
{
// Contract (SoA layout: one array per component, `count` elements each):
//   for i in [0, count):
//     out[i] = sqrt((ax[i]-bx[i])^2 + (ay[i]-by[i])^2 + (az[i]-bz[i])^2)
//   out must be finite for finite inputs; count may be 0.
// SoA is chosen deliberately: it keeps ISA variants simple and branch-free.
using DistanceBatchFn = void (*)(const float *ax, const float *ay, const float *az,
                                 const float *bx, const float *by, const float *bz,
                                 float *out, std::size_t count);

void DistanceBatchScalar(const float *ax, const float *ay, const float *az,
                         const float *bx, const float *by, const float *bz,
                         float *out, std::size_t count);

void DistanceBatchAVX2(const float *ax, const float *ay, const float *az,
                       const float *bx, const float *by, const float *bz,
                       float *out, std::size_t count);

struct CpuFeatures
{
    bool sse42 = false;
    bool avx2 = false;
    bool osxsave = false;
};

CpuFeatures DetectCpuFeatures();

// Selection rule (mirrored in verify/verify_dispatch.py):
//   forceScalar              -> scalar
//   features.avx2 && osxsave -> avx2
//   otherwise                -> scalar
DistanceBatchFn SelectDistanceBatchFn(const CpuFeatures &features,
                                      bool forceScalar = false);
}
