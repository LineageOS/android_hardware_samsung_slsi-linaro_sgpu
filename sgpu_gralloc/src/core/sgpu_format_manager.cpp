///
/// @file sgpu_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include <android-base/properties.h>

#include "config.h"
#include "debug_helper.h"
#include "format_info.h"
#include "hidl_defines.h"
#include "layout_manager.h"
#include "sgpu_format_manager.h"
#include "util/util.h"

#undef LOG_TAG
#define LOG_TAG "SGpuFormatManager"

namespace android {
namespace samsung {
namespace gralloc {

///
/// @brief Get format layout for format.
///
/// @param[in] format           format
/// @param[in] descriptor       Buffer descriptor passed to request allocation
///
/// @return Flag of layouts
///
uint32_t SgpuFormatManager::get_layout(PixelFormat format, const BufferDescriptor &descriptor) const
{
        constexpr uint64_t usage_mask = static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                                        static_cast<uint64_t>(BufferUsage::GPU_RENDER_TARGET) |
                                        static_cast<uint64_t>(BufferUsage::GPU_DATA_BUFFER);
        constexpr uint64_t read_usage = static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                                        static_cast<uint64_t>(BufferUsage::GPU_DATA_BUFFER);
        constexpr uint64_t read_write_usage = usage_mask;

        constexpr uint32_t linear_layout = static_cast<uint32_t>(FormatLayoutBitMask::LINEAR);
        constexpr uint32_t linear_dcc_layout = (static_cast<uint32_t>(FormatLayoutBitMask::LINEAR) |
                                                static_cast<uint32_t>(FormatLayoutBitMask::DCC));
        constexpr uint32_t sbwc_layout = static_cast<uint32_t>(FormatLayoutBitMask::SBWC);

        static format_layout_info sgpu_format_layout_info[] = {
                { PixelFormat::RGBA_8888,                    read_write_usage, linear_dcc_layout },
                { PixelFormat::RGBX_8888,                    read_write_usage, linear_dcc_layout },
                ///@todo GFXSW-4627
                //{ PixelFormat::RGB_888,                      read_write_usage, linear_dcc_layout },
                { PixelFormat::RGB_565,                      read_write_usage, linear_layout },
                { PixelFormat::BGRA_8888,                    read_write_usage, linear_dcc_layout },
                { PixelFormat::RGBA_FP16,                    read_write_usage, linear_dcc_layout },
                { PixelFormat::RGBA_1010102,                 read_write_usage, linear_dcc_layout },
                { PixelFormat::YCRCB_420_SP,                 read_usage,       linear_layout     },
                { PixelFormat::BLOB,                         read_write_usage, linear_layout     },
                { PixelFormat::PRIVATE_YCBCR_420_SP,         read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M,       read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_TILED, read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCBCR_420_SPN,        read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCRCB_420_SP_M,       read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCRCB_420_SP_M_FULL,  read_usage,       linear_layout     },
                { PixelFormat::YCBCR_P010,                   read_usage,       linear_layout     },
                { PixelFormat::R_8,                          read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCBCR_P010_M,         read_usage,       linear_layout     },
                { PixelFormat::YV12,                         read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YV12_M,               read_usage,       linear_layout     },
                { PixelFormat::YCBCR_422_SP,                 read_usage,       linear_layout     },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC,         read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC,          read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC,     read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC,      read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC,      read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC,  read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC,         read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC,     read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50,     read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75,     read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L50,      read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L75,      read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40, read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60, read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80, read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40,  read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60,  read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80,  read_usage,      sbwc_layout },
                /* SBWC Lossy formats over v2.7 */
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L,    read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L,    read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L,     read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L,     read_usage,      sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L, read_usage,     sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L, read_usage,     sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L,  read_usage,     sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L,  read_usage,     sbwc_layout },
                /* SBWC Lossy formats over v2.8 */
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR,  read_usage,     sbwc_layout },
                { PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR,  read_usage, sbwc_layout },
        };

        SGR_ASSERT(is_any_bitmask_set_64(descriptor.usage, usage_mask));

        uint32_t layout = static_cast<uint32_t>(FormatLayoutBitMask::NONE);
        for (uint32_t i = 0; i < SGR_ARRAY_SIZE(sgpu_format_layout_info); i++) {
                if (format == sgpu_format_layout_info[i].format) {
                        if (is_any_bitmask_set_64(descriptor.usage, sgpu_format_layout_info[i].usage)) {
                                layout = sgpu_format_layout_info[i].layout;
                        }
                        break;
                }
        }

        disable_sajc_if_needed(descriptor, &layout);

        SGR_LOGV("Format %s, Usage %s, Format_Layout_Bitmask = 0x%x %s",
                 get_pixel_format_string(format).c_str(),
                 get_usage_string(descriptor.usage).c_str(), layout, get_format_layout_bitmask_string(layout).c_str());
        return layout;
}

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info SgpuFormatManager::get_linear_alignment(PixelFormat format) const
{
        ///@todo GFXSW-3833 - Remove the assertion once UMD supports RGB8
        SGR_ASSERT(format != PixelFormat::RGB_888);

        const align_info align = {
                .stride_in_bytes        = 256,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        dump_alignment(align, static_cast<uint32_t>(IpBitMask::GPU));

        return align;
}

} // gralloc
} // samsung
} // android
