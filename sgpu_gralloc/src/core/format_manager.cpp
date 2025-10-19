///
/// @file format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include <string.h>

#include "camera_format_manager.h"
#include "cpu_format_manager.h"
#include "dcc_layout_manager.h"
#include "debug_helper.h"
#include "hwc_format_manager.h"
#include "format_manager.h"
#include "linear_layout_manager.h"
#include "sbwc_layout_manager.h"
#include "sgpu_format_manager.h"
#include "util/util.h"
#include "video_format_manager.h"

#undef LOG_TAG
#define LOG_TAG "FormatManager"

namespace android {
namespace samsung {
namespace gralloc {

///
/// @brief FormatManager constructor
///
FormatManager::FormatManager()
{
        static CpuFormatManager    cpu_format_manager;
        static SgpuFormatManager gpu_format_manager;
        static HwcFormatManager    hwc_format_manager;
        static CameraFormatManager camera_format_manager;
        static VideoFormatManager  video_format_manager;

        m_ip_format_manager[static_cast<uint32_t>(Ip::CPU)]    = &cpu_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::GPU)]    = &gpu_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::HWC)]    = &hwc_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::CAMERA)] = &camera_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::VIDEO)]  = &video_format_manager;

        static LinearLayoutManager linear_layout_manager(m_ip_format_manager);
        static DccLayoutManager    dcc_layout_manager;
        static SbwcLayoutManager   sbwc_layout_manager;

        m_layout_manager[static_cast<uint32_t>(FormatLayout::LINEAR)] = &linear_layout_manager;
        m_layout_manager[static_cast<uint32_t>(FormatLayout::DCC)]    = &dcc_layout_manager;
        m_layout_manager[static_cast<uint32_t>(FormatLayout::SBWC)]   = &sbwc_layout_manager;
}

///
/// @brief FormatManager destructor
///
FormatManager::~FormatManager()
{
}

static inline FormatLayout get_layout(uint32_t layouts, const BufferDescriptor &descriptor)
{
        SGR_LOGV("Common Format_Layouts = 0x%x %s", layouts, get_format_layout_bitmask_string(layouts).c_str());

        SGR_ASSERT(layouts != 0);

        SGR_ASSERT(!is_all_bitmask_set(layouts, (static_cast<uint32_t>(FormatLayoutBitMask::DCC) |
                                                     static_cast<uint32_t>(FormatLayoutBitMask::SBWC))));

        if (is_any_bitmask_set_64(descriptor.usage, static_cast<uint64_t>(BufferUsage::PROTECTED)) &&
            (descriptor.width >= 4096 && descriptor.height >= 4096)) {
                // Don't use DCC due to secure heap allocation limit 64MB, but CTS isn't aware of it
                clear_bitmask(&layouts, static_cast<uint32_t>(FormatLayoutBitMask::DCC));
        }

        static const FormatLayout preference[] = {
                FormatLayout::DCC,
                FormatLayout::SBWC,
                FormatLayout::LINEAR,
        };

        FormatLayout ret = FormatLayout::NONE;
        for (uint32_t i = 0; i < SGR_ARRAY_SIZE(preference); i++) {
                if (is_bit_set(layouts, static_cast<uint32_t>(preference[i]))) {
                        ret = preference[i];
                        break;
                }
        }

        SGR_LOGV("Selected Format_Layout = 0x%x [%s]", ret, get_format_layout_string(ret).c_str());

        SGR_ASSERT(ret != FormatLayout::NONE);
        return ret;
}

static inline uint32_t get_ip_flags(uint64_t usage)
{
        // There are more usages but only consider format/IP-related usages
        constexpr uint64_t cpu_mask = (static_cast<uint64_t>(BufferUsage::CPU_READ_MASK) |
                                       static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK));
        constexpr uint64_t gpu_mask = (static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                                       static_cast<uint64_t>(BufferUsage::GPU_RENDER_TARGET) |
                                       static_cast<uint64_t>(BufferUsage::GPU_DATA_BUFFER));
        constexpr uint64_t hwc_mask = (static_cast<uint64_t>(BufferUsage::COMPOSER_OVERLAY) |
                                       static_cast<uint64_t>(BufferUsage::COMPOSER_CLIENT_TARGET) |
                                       static_cast<uint64_t>(BufferUsage::COMPOSER_CURSOR));
        constexpr uint64_t video_mask = (static_cast<uint64_t>(BufferUsage::VIDEO_ENCODER) |
                                         static_cast<uint64_t>(BufferUsage::VIDEO_DECODER));
        constexpr uint64_t camera_mask = (static_cast<uint64_t>(BufferUsage::CAMERA_OUTPUT) |
                                          static_cast<uint64_t>(BufferUsage::CAMERA_INPUT));

        uint32_t ip_flags = 0;
        if (is_any_bitmask_set_64(usage, cpu_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CPU));
        }

        if (is_any_bitmask_set_64(usage, gpu_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::GPU));
        }

        if (is_any_bitmask_set_64(usage, hwc_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::HWC));
        }

        if (is_any_bitmask_set_64(usage, camera_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CAMERA));
        }

        if (is_any_bitmask_set_64(usage, video_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::VIDEO));
        }

        if (usage == 0) {
                SGR_LOGD("Buffer Usage is 0, Defaulting to CPU");
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CPU));
        } else if (usage == static_cast<uint64_t>(BufferUsage::PROTECTED)) {
                SGR_LOGD("Buffer Usage contains only PROTECTED, Defaulting to CPU");
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CPU));
        }

        SGR_LOGV("Ip_flags = 0x%x %s", ip_flags, get_ip_bitmask_string(ip_flags).c_str());

        return ip_flags;
}


static inline PixelFormat get_internal_implementation_defined_format(uint64_t usage)
{
        constexpr uint64_t gpu_texture_and_composer_overlay_mask =
                static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                static_cast<uint64_t>(BufferUsage::COMPOSER_OVERLAY);
        constexpr uint64_t video_encoder_mask =
                static_cast<uint64_t>(BufferUsage::VIDEO_ENCODER);
        constexpr uint64_t video_private_data_mask =
                static_cast<uint64_t>(BufferUsage::PRIVATE_VIDEO_PRIVATE_DATA);
        constexpr uint64_t camera_mask = (static_cast<uint64_t>(BufferUsage::CAMERA_OUTPUT) |
                                          static_cast<uint64_t>(BufferUsage::CAMERA_INPUT));

        PixelFormat ret = PixelFormat::PRIVATE_YCRCB_420_SP_M;

        // This mapping is from exynos implementation
        if (is_any_bitmask_set_64(usage, gpu_texture_and_composer_overlay_mask)) {
                constexpr uint64_t yuv_range_full_mask =
                        static_cast<uint64_t>(BufferUsage::PRIVATE_YUV_RANGE_FULL);

                if (is_any_bitmask_set_64(usage, yuv_range_full_mask)) {
                        ret = PixelFormat::PRIVATE_YCRCB_420_SP_M_FULL;
                } else {
                        ret = PixelFormat::PRIVATE_YCRCB_420_SP_M;
                }
        } else if (is_any_bitmask_set_64(usage, video_encoder_mask)) {
                ret = PixelFormat::PRIVATE_YCRCB_420_SP_M;
        } else if (is_any_bitmask_set_64(usage, video_private_data_mask)) {
                ret = PixelFormat::PRIVATE_YCBCR_420_SP_M;
        } else if (is_all_bitmask_set_64(usage, camera_mask)) {
                ret = PixelFormat::YCBCR_422_I;
        }

        return ret;
}

static inline PixelFormat get_internal_ycbcr_420_888_format(uint64_t usage)
{
        constexpr uint64_t gpu_usage_mask =
                static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                static_cast<uint64_t>(BufferUsage::GPU_RENDER_TARGET);
        constexpr uint64_t video_usage_mask =
                static_cast<uint64_t>(BufferUsage::VIDEO_ENCODER) |
                static_cast<uint64_t>(BufferUsage::VIDEO_DECODER);
        // W/A for android.hardware.camera2.cts.CameraExtensionSessionTest
        constexpr uint64_t camera_texture_mask =
                static_cast<uint64_t>(BufferUsage::CAMERA_OUTPUT) |
                static_cast<uint64_t>(BufferUsage::GPU_TEXTURE);

        // This mapping is from exynos implementation
        // Flexible framework-accessible YUV format; map to NV21 for now
        PixelFormat ret = PixelFormat::YCRCB_420_SP;
        if (is_any_bitmask_set_64(usage, video_usage_mask)) {
                ret = PixelFormat::PRIVATE_YCBCR_420_SP_M;

                if (is_all_bitmask_set_64(usage, static_cast<uint64_t>(BufferUsage::PRIVATE_SBWC_REQUEST_10BIT))) {
                        ret = PixelFormat::PRIVATE_YCBCR_P010_M;
                }
                else if (is_launch_api_level_gt_s() && is_any_bitmask_set_64(usage, gpu_usage_mask)) {
                        ret = PixelFormat::PRIVATE_YCBCR_420_SPN;
                }
        } else if (is_all_bitmask_set_64(usage, camera_texture_mask)){
                ret = PixelFormat::YCRCB_420_SP;
        } else if (is_any_bitmask_set_64(usage, gpu_usage_mask)) {
                ret = PixelFormat::PRIVATE_YCBCR_420_SP_M;
        }

        return ret;
}

static inline PixelFormat get_internal_format(PixelFormat format, uint64_t usage)
{
        PixelFormat ret = format;

        if (format == PixelFormat::IMPLEMENTATION_DEFINED) {
                ret = get_internal_implementation_defined_format(usage);
        }
        else if (format == PixelFormat::YCBCR_420_888) {
                ret = get_internal_ycbcr_420_888_format(usage);
        }

        return ret;
}

static inline PixelFormat get_sub_format_layout(PixelFormat hal_format, FormatLayout *format_layout)
{
        struct support_sbwc_format {
                PixelFormat base_format;
                PixelFormat sbwc_format;
        };

        static const support_sbwc_format sub_format_info[] = {
                { PixelFormat::PRIVATE_YCBCR_420_SP_M , PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC     },
                { PixelFormat::PRIVATE_YCBCR_P010_M   , PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC },
        };

        PixelFormat sub_format = PixelFormat::UNDEFINED;
        for (uint32_t i = 0; i < SGR_ARRAY_SIZE(sub_format_info); i++) {
                if (hal_format == sub_format_info[i].base_format) {
                        sub_format = sub_format_info[i].sbwc_format;
                        break;
                }
        }

        SGR_ASSERT(sub_format != PixelFormat::UNDEFINED);
        *format_layout = FormatLayout::SBWC;

        return sub_format;
}

///
/// @brief Get internal format from android HAL format
///        Selection of internal format depends upon HAL format, usage and IP supports.
///
/// @param[in]  descriptor      Buffer descriptor passed to request allocation
/// @param[out] format_layout   Format layout
/// @param[out] ip_flags        IP List
///
/// @return internal format
///
PixelFormat FormatManager::get_format(const BufferDescriptor &descriptor, FormatLayout *format_layout,
                                      uint32_t *ip_flags) const
{
        SGR_ASSERT(format_layout != nullptr);
        SGR_ASSERT(ip_flags != nullptr);

        *ip_flags = get_ip_flags(descriptor.usage);
        uint32_t usage_ip_flags = *ip_flags;

        uint32_t common_format_layouts = static_cast<uint32_t>(FormatLayoutBitMask::ALL);
        PixelFormat internal_format = get_internal_format(descriptor.format, descriptor.usage);
        SGR_ASSERT(usage_ip_flags != 0);
        for (uint32_t pos = 0; pos < static_cast<uint32_t>(Ip::COUNT); pos++) {
                if ((usage_ip_flags & 0x1) == 0x1) {
                        uint32_t temp_format_layouts =
                                m_ip_format_manager[pos]->get_layout(internal_format, descriptor);
                        ///@todo GFXSW-3650 - Add a logic to decide a format and a layout for unsupported formats
                        SGR_ASSERT(static_cast<FormatLayout>(temp_format_layouts) != FormatLayout::NONE);
                        common_format_layouts &= temp_format_layouts;
                }
                usage_ip_flags >>= 1;
        }

        *format_layout = get_layout(common_format_layouts, descriptor);

        dump_format_info(descriptor.format, internal_format, descriptor.usage, *format_layout, *ip_flags);

        return internal_format;
}

///
/// @brief Get allocation information.
///
/// @param[in]     internal_format Internal format
/// @param[in]     format_layout   Format layout
/// @param[in]     usage           Android graphics BufferUsage
/// @param[in]     ip_flags        IP List
/// @param[in]     layer_count     Number of Layers
/// @param[in/out] alloc_extent    allocation width and height
/// @param[out]    alloc_infos     allocation information to be used by allocator
/// @param[out]    num_planes      Number of sgr_plane array.
/// @param[out]    planes          Pointer to sgr_plane array.
///
/// @return the number of alloc_info which are updated
///
uint32_t FormatManager::get_allocation_info(PixelFormat internal_format, FormatLayout format_layout, uint64_t usage,
                                            uint32_t ip_flags, uint32_t layer_count, sgr_extent_2d *alloc_extent,
                                            sgr_alloc *alloc_infos, uint32_t *num_planes,
                                            sgr_plane_layout *plane_layouts) const
{
        SGR_ASSERT(alloc_infos != nullptr);
        SGR_ASSERT(alloc_extent != nullptr);
        SGR_ASSERT(format_layout != FormatLayout::NONE);

        SGR_LOGV("Requested info:\n"
                 "\t\textent.width    = %u\n"
                 "\t\textent.height   = %u\n"
                 "\t\tinternal_format = %u [%s]\n"
                 "\t\tformat_layout   = %u [%s]\n"
                 "\t\tip_flags        = %u %s\n",
                 alloc_extent->width, alloc_extent->height,
                 internal_format, get_pixel_format_string(internal_format).c_str(),
                 format_layout, get_format_layout_string(format_layout).c_str(),
                 ip_flags, get_ip_bitmask_string(ip_flags).c_str());

        SGR_ASSERT(m_layout_manager[static_cast<uint32_t>(format_layout)] != nullptr);
        uint32_t num_alloc_infos = m_layout_manager[static_cast<uint32_t>(format_layout)]->
                get_alloc_info(internal_format, layer_count, ip_flags, alloc_extent,
                               alloc_infos, num_planes, plane_layouts, usage);

        // Get sub-allocation information
        if (is_any_bitmask_set_64(usage, static_cast<uint64_t>(BufferUsage::PRIVATE_DOWNSCALE))) {
                FormatLayout sub_layout = FormatLayout::NONE;
                PixelFormat sub_format = get_sub_format_layout(internal_format, &sub_layout);

                SGR_ASSERT((sub_format != PixelFormat::UNDEFINED) && (sub_layout != FormatLayout::NONE));

                if (sub_format == PixelFormat::UNDEFINED || sub_layout == FormatLayout::NONE) {
                        // both format and layout should be derived at this stage
                        SGR_LOGD("cannot derive sub format from input (format: %" PRIi32 ")", internal_format);
                        return 0;
                }

                sgr_extent_2d sub_alloc_extent = {alloc_extent->width/2, alloc_extent->height/2};

                uint32_t sub_num_planes = 0;
                uint32_t sub_num_alloc = m_layout_manager[static_cast<uint32_t>(sub_layout)]->
                        get_alloc_info(sub_format, layer_count, ip_flags, &sub_alloc_extent,
                                       alloc_infos + num_alloc_infos, &sub_num_planes, plane_layouts + (*num_planes),
                                       usage);

                *num_planes += sub_num_planes;
                num_alloc_infos += sub_num_alloc;
        }

        dump_sgr_alloc(alloc_infos, num_alloc_infos, format_layout);

        return num_alloc_infos;
}

} // gralloc
} // samsung
} // android
