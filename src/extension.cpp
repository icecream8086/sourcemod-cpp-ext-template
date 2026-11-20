#include <smsdk_ext.h>

#include "template_ext/distance.h"

namespace
{
class TemplateExtension final : public ISourceMod
{
public:
    bool SDK_OnLoad(char *error, size_t maxlen, bool late) override
    {
        (void)error;
        (void)maxlen;
        (void)late;

        const template_ext::CpuFeatures features = template_ext::DetectCpuFeatures();
        g_pSM->LogMessage(myself,
                          "[template] isa: sse4.2=%d avx2=%d osxsave=%d",
                          features.sse42, features.avx2, features.osxsave);

        CreateNative("Template_Distance", &TemplateExtension::Native_Distance);
        return true;
    }

    void SDK_OnUnload() override {}

private:
    static cell_t Native_Distance(IPluginContext *, const cell_t *params)
    {
        const float ax[1] = {sp_ctof(params[1])};
        const float ay[1] = {sp_ctof(params[2])};
        const float az[1] = {sp_ctof(params[3])};
        const float bx[1] = {sp_ctof(params[4])};
        const float by[1] = {sp_ctof(params[5])};
        const float bz[1] = {sp_ctof(params[6])};

        const template_ext::DistanceBatchFn fn =
            template_ext::SelectDistanceBatchFn(template_ext::DetectCpuFeatures());
        float out = 0.0f;
        fn(ax, ay, az, bx, by, bz, &out, 1);
        return sp_ftoc(out);
    }
};

TemplateExtension g_TemplateExtension;
}

SMEXT_LINK(&g_TemplateExtension);
