///
/// @file hwc_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "config.h"
#include "debug_helper.h"
#include "format_info.h"
#include "hwc_format_manager.h"
#include "hidl_defines.h"
#include "util/util.h"

#undef LOG_TAG
#define LOG_TAG "HwcFormatManager"

namespace android {
namespace samsung {
namespace gralloc {

constexpr uint32_t layout_linear = static_cast<uint32_t>(FormatLayoutBitMask::LINEAR);
constexpr uint32_t layout_sbwc = static_cast<uint32_t>(FormatLayoutBitMask::SBWC);
constexpr uint32_t layout_linear_and_dcc = (static_cast<uint32_t>(FormatLayoutBitMask::LINEAR) |
                                            static_cast<uint32_t>(FormatLayoutBitMask::DCC));

struct hwc_format_layout_info {
        PixelFormat format;
        uint32_t    layouts;
};

static const hwc_format_layout_info hwc_format_layout_table[] = {
        { PixelFormat::RGBA_8888,                           layout_linear_and_dcc },
        { PixelFormat::RGBX_8888,                           layout_linear_and_dcc },
        { PixelFormat::RGB_888,                             layout_linear },
        { PixelFormat::RGB_565,                             layout_linear },
        { PixelFormat::BGRA_8888,                           layout_linear_and_dcc },
        { PixelFormat::YCBCR_422_SP,                        layout_linear },
        { PixelFormat::YCRCB_420_SP,                        layout_linear },
        { PixelFormat::RGBA_FP16,                           layout_linear_and_dcc },
        { PixelFormat::RGBA_1010102,                        layout_linear_and_dcc },
        { PixelFormat::YCBCR_P010,                          layout_linear },
        { PixelFormat::R_8,                                 layout_linear },
        { PixelFormat::YV12,                                layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_P_M,               layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M,              layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_TILED,        layout_linear },
        { PixelFormat::PRIVATE_YV12_M,                      layout_linear },
        { PixelFormat::PRIVATE_YCRCB_420_SP_M,              layout_linear },
        { PixelFormat::PRIVATE_YCRCB_420_SP_M_FULL,         layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_P,                 layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_SPN,               layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_S10B,         layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_S10B,          layout_linear },
        { PixelFormat::PRIVATE_YCBCR_P010_M,                layout_linear },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC,         layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC,          layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC,     layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC,      layout_sbwc },
        { PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC,         layout_sbwc },
        { PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC,     layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC,      layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC,  layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50,     layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75,     layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L50,      layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L75,      layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40, layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60, layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80, layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40,  layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60,  layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80,  layout_sbwc },
        /* SBWC Lossy formats over v2.7 */
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L,    layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L,    layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L,     layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L,     layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L, layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L, layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L,  layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L,  layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR,  layout_sbwc },
        { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR,  layout_sbwc },
};

static inline int32_t get_hwc_format_index(PixelFormat format)
{
        constexpr uint32_t count = SGR_ARRAY_SIZE(hwc_format_layout_table);
        for (uint32_t i = 0; i < count; i++) {
                if (format == hwc_format_layout_table[i].format)
                        return i;
        }
        return -1;
}

///
/// @brief Get format layout for format.
///
/// @param[in] format           format
/// @param[in] descriptor       Buffer descriptor passed to request allocation
///
/// @return Flag of layouts
///
uint32_t HwcFormatManager::get_layout(PixelFormat format, const BufferDescriptor &descriptor) const
{
        const int32_t index = get_hwc_format_index(format);
        SGR_ASSERT(index != -1);
        if (index == -1) {
                SGR_LOGE("Unsupported Format = %d", static_cast<uint32_t>(format));
                return 0;
        }

        uint32_t layout = hwc_format_layout_table[static_cast<uint32_t>(index)].layouts;

        disable_sajc_if_needed(descriptor, &layout);

        SGR_LOGV("Format_Layout_Bitmask = 0x%x %s", layout, get_format_layout_bitmask_string(layout).c_str());
        return layout;
}

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info HwcFormatManager::get_linear_alignment(PixelFormat format) const
{
        SGR_UNUSED(format);

        align_info align = {
                .stride_in_bytes        = 1,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        if (format == PixelFormat::PRIVATE_YCBCR_420_SP_M_S10B ||
            format == PixelFormat::PRIVATE_YCBCR_420_SPN_S10B){
                align.stride_in_bytes = 64;
                align.vstride_in_pixels = 16;
        } else if (format == PixelFormat::PRIVATE_YCBCR_420_SPN) {
                align.vstride_in_pixels = 16;
                align.plane_padding_in_bytes = 256;

                if (is_launch_api_level_gt_s()) {
                        align.plane_padding_in_bytes = 0;
                }
        }

        dump_alignment(align, static_cast<uint32_t>(IpBitMask::HWC));

        return align;
}

} // gralloc
} // samsung
} // android
