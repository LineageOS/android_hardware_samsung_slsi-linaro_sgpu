///
/// @file camera_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "camera_format_manager.h"
#include "debug_helper.h"
#include "format_info.h"
#include "hidl_defines.h"
#include "util/util.h"

#undef LOG_TAG
#define LOG_TAG "CameraFormatManager"

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
uint32_t CameraFormatManager::get_layout(PixelFormat format, const BufferDescriptor &descriptor) const
{
        SGR_ASSERT(is_any_bitmask_set_64(descriptor.usage, (static_cast<uint64_t>(BufferUsage::CAMERA_OUTPUT) |
                                                     static_cast<uint64_t>(BufferUsage::CAMERA_INPUT))));

        uint32_t layout = static_cast<uint32_t>(FormatLayoutBitMask::NONE);

        switch(format) {
        case PixelFormat::YCRCB_420_SP:
        case PixelFormat::YCBCR_422_I:
        case PixelFormat::YCBCR_P010:
        case PixelFormat::RAW10:
        case PixelFormat::RAW12:
        case PixelFormat::RAW16:
        case PixelFormat::BLOB:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M_FULL:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M:
        case PixelFormat::PRIVATE_YCBCR_P010_M:
                layout = static_cast<uint32_t>(FormatLayoutBitMask::LINEAR);
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80:
        /* SBWC Lossy formats over v2.7 */
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L:
        /* SBWC Lossy formats over v2.8 */
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR:
                layout = static_cast<uint32_t>(FormatLayoutBitMask::SBWC);
                break;
        default:
                SGR_LOGD("Unsupported Format = %d", static_cast<uint32_t>(format));
        }

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
align_info CameraFormatManager::get_linear_alignment(PixelFormat format) const
{
        SGR_UNUSED(format);

        /// Camera IP requires 16B row alignment
        static const align_info align = {
                .stride_in_bytes        = 16,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        dump_alignment(align, static_cast<uint32_t>(IpBitMask::CAMERA));

        return align;
}

} // gralloc
} // samsung
} // android
