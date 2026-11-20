#pragma once

#include <cstddef>
#include <cstdint>

namespace template_ext
{
// Compile-time sanity checks: catch type/ABI drift early.
static_assert(sizeof(float) == 4, "float must be IEEE-754 single precision");
static_assert(sizeof(std::size_t) >= 4, "size_t too small");
#if defined(TEMPLATE_EXPECT_POINTER_BITS)
static_assert(sizeof(void *) * 8 == TEMPLATE_EXPECT_POINTER_BITS,
              "pointer width does not match TEMPLATE_ARCH");
#endif

// Contract (SoA layout: one array per component, `count` elements each):
//   for i in [0, count):
//     out[i] = sqrt((ax[i]-bx[i])^2 + (ay[i]-by[i])^2 + (az[i]-bz[i])^2)
//   valid inputs only; count == 0 is a no-op.
using DistanceBatchFn = bool (*)(const float *ax, const float *ay, const float *az,
                                 const float *bx, const float *by, const float *bz,
                                 float *out, std::size_t count);

constexpr std::size_t MaxDistanceBatch()
{
    return static_cast<std::size_t>(-1) / 3;
}

// Fool-proof boundary: rejects null pointers for non-empty batches and
// `count * 3` overflow before any dereference.
inline bool ValidDistanceBatch(const float *ax, const float *ay, const float *az,
                               const float *bx, const float *by, const float *bz,
                               float *out, std::size_t count)
{
    if (count > MaxDistanceBatch())
        return false;
    if (count == 0)
        return true;
    return ax != nullptr && ay != nullptr && az != nullptr &&
           bx != nullptr && by != nullptr && bz != nullptr && out != nullptr;
}

bool DistanceBatchScalar(const float *ax, const float *ay, const float *az,
                         const float *bx, const float *by, const float *bz,
                         float *out, std::size_t count);

bool DistanceBatchAVX2(const float *ax, const float *ay, const float *az,
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
