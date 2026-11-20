#include "template_ext/distance.h"

#include <cmath>

namespace template_ext
{
bool DistanceBatchScalar(const float *ax, const float *ay, const float *az,
                         const float *bx, const float *by, const float *bz,
                         float *out, std::size_t count)
{
    if (!ValidDistanceBatch(ax, ay, az, bx, by, bz, out, count))
        return false;
    for (std::size_t i = 0; i < count; ++i)
    {
        const float dx = ax[i] - bx[i];
        const float dy = ay[i] - by[i];
        const float dz = az[i] - bz[i];
        out[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    return true;
}
}
