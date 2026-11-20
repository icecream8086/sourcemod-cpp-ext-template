#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <cstdio>
#include <vector>

#include "template_ext/distance.h"

int main()
{
    constexpr std::size_t count = 1u << 20;
    std::vector<float> ax(count, 1.0f), ay(count, 2.0f), az(count, 3.0f);
    std::vector<float> bx(count, 2.0f), by(count, 3.0f), bz(count, 4.0f);
    std::vector<float> out(count);

    const template_ext::CpuFeatures features = template_ext::DetectCpuFeatures();
    std::printf("avx2=%d osxsave=%d\n", features.avx2, features.osxsave);

    ankerl::nanobench::Bench bench;
    bench.title("distance batch (SoA)").unit("distance").warmup(10).relative(true);
    bench.run("scalar", [&] {
        template_ext::DistanceBatchScalar(ax.data(), ay.data(), az.data(),
                                          bx.data(), by.data(), bz.data(),
                                          out.data(), count);
    });
    bench.run("avx2", [&] {
        template_ext::DistanceBatchAVX2(ax.data(), ay.data(), az.data(),
                                        bx.data(), by.data(), bz.data(),
                                        out.data(), count);
    });
    ankerl::nanobench::doNotOptimizeAway(out.data());
    return 0;
}
