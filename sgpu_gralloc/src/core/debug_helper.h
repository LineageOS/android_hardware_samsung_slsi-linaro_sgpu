///
/// @file debug_helper.h
/// @copyright 2020-2021 Samsung Electronics
///

#pragma once

#include <android-base/properties.h>
#include <cstring>
#include <sstream>
#include <stdint.h>

#define __STDC_FORMAT_MACROS
#include <cinttypes>

#include "config.h"
#include "hidl_defines.h"
#include "ip_format_manager.h"
#include "interface/metadata_gralloc.h"
#include "interface/private_handle.h"
#include "layout_manager.h"
#include "private_handle_helper.h"
#include "util/util.h"

namespace android {
namespace samsung {
namespace gralloc {

#define CHECK_AND_RETURN_IF_CONFIG_VERBOSE_ALLOCATION_IS_NOT_SET()\
        {                                                               \
                if(android::base::GetBoolProperty(CONFIG_VERBOSE_ALLOC, CONFIG_VERBOSE_ALLOC_DEFAULT) == false) \
                        return;                                         \
        }

#define CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET()                    \
        {                                                               \
                if(android::base::GetBoolProperty(CONFIG_VERBOSE, CONFIG_VERBOSE_DEFAULT) == false) \
                        return;                                         \
        }

#define PRINT_FLAG_IF_SET_ONE_LINE(str, value, flag)                     \
        {                                                       \
                if ((value & static_cast<uint64_t>(flag)) == static_cast<uint64_t>(flag))        \
                        str << #flag << " ";               \
        }

#define PRINT_FLAG_IF_SET(str, value, flag)                     \
        {                                                       \
                if ((value & static_cast<uint64_t>(flag)) == static_cast<uint64_t>(flag))        \
                        str << #flag << std::endl;               \
        }

#define PRINT_FLAG_IF_EQUALS_MASKED_VALUE(str, value, mask, flag)               \
        {                                                       \
                if ((value & static_cast<uint64_t>(mask)) == static_cast<uint64_t>(flag))        \
                        str << #flag << std::endl;               \
        }

inline std::string get_usage_string(uint64_t usage)
{
        std::stringstream str;

        str << "usage = 0x" << std::hex << usage << std::dec << std::endl;

        PRINT_FLAG_IF_EQUALS_MASKED_VALUE(str, usage, BufferUsage::CPU_READ_MASK, BufferUsage::CPU_READ_RARELY);
        PRINT_FLAG_IF_EQUALS_MASKED_VALUE(str, usage, BufferUsage::CPU_READ_MASK, BufferUsage::CPU_READ_OFTEN);
        PRINT_FLAG_IF_EQUALS_MASKED_VALUE(str, usage, BufferUsage::CPU_WRITE_MASK, BufferUsage::CPU_WRITE_RARELY);
        PRINT_FLAG_IF_EQUALS_MASKED_VALUE(str, usage, BufferUsage::CPU_WRITE_MASK, BufferUsage::CPU_WRITE_OFTEN);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::GPU_TEXTURE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::GPU_RENDER_TARGET);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::COMPOSER_OVERLAY);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::COMPOSER_CLIENT_TARGET);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PROTECTED);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::COMPOSER_CURSOR);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::VIDEO_ENCODER);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::CAMERA_OUTPUT);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::CAMERA_INPUT);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::RENDERSCRIPT);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::VIDEO_DECODER);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::SENSOR_DIRECT_DATA);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::GPU_DATA_BUFFER);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::GPU_CUBE_MAP);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::GPU_MIPMAP_COMPLETE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::HW_IMAGE_ENCODER);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_GDC_MODE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_NO_SAJC);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_CAMERA_RESERVED);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_SECURE_CAMERA_RESERVED);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_ROIINFO);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_DOWNSCALE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_FORCE_BACKBUFFER);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_FRONTBUFFER);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_SBWC_REQUEST_10BIT);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_HFR_MODE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_NOZEROED);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_PRIVATE_NONSECURE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_VIDEO_PRIVATE_DATA);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_VIDEO_EXT);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_DAYDREAM_SINGLE_BUFFER_MODE);
        PRINT_FLAG_IF_SET(str, usage, BufferUsage::PRIVATE_YUV_RANGE_FULL);

        return str.str();
}

inline std::string get_pixel_format_string(PixelFormat format)
{
        switch (format) {
                case PixelFormat::UNDEFINED:                           return std::string("UNDEFINED");
                case PixelFormat::RGBA_8888:                           return std::string("RGBA_8888");
                case PixelFormat::RGBX_8888:                           return std::string("RGBX_8888");
                case PixelFormat::RGB_888:                             return std::string("RGB_888");
                case PixelFormat::RGB_565:                             return std::string("RGB_565");
                case PixelFormat::BGRA_8888:                           return std::string("BGRA_8888");
                case PixelFormat::YCBCR_422_SP:                        return std::string("YCBCR_422_SP");
                case PixelFormat::YCRCB_420_SP:                        return std::string("YCRCB_420_SP");
                case PixelFormat::YCBCR_422_I:                         return std::string("YCBCR_422_I");
                case PixelFormat::RGBA_FP16:                           return std::string("RGBA_FP16");
                case PixelFormat::RAW16:                               return std::string("RAW16");
                case PixelFormat::BLOB:                                return std::string("BLOB");
                case PixelFormat::IMPLEMENTATION_DEFINED:              return std::string("IMPLEMENTATION_DEFINED");
                case PixelFormat::YCBCR_420_888:                       return std::string("YCBCR_420_888");
                case PixelFormat::RAW_OPAQUE:                          return std::string("RAW_OPAQUE");
                case PixelFormat::RAW10:                               return std::string("RAW10");
                case PixelFormat::RAW12:                               return std::string("RAW12");
                case PixelFormat::RGBA_1010102:                        return std::string("RGBA_1010102");
                case PixelFormat::DEPTH_16:                            return std::string("DEPTH_16");
                case PixelFormat::DEPTH_24:                            return std::string("DEPTH_24");
                case PixelFormat::DEPTH_24_STENCIL_8:                  return std::string("DEPTH_24_STENCIL_8");
                case PixelFormat::DEPTH_32F:                           return std::string("DEPTH_32F");
                case PixelFormat::DEPTH_32F_STENCIL_8:                 return std::string("DEPTH_32F_STENCIL_8");
                case PixelFormat::STENCIL_8:                           return std::string("STENCIL_8");
                case PixelFormat::YCBCR_P010:                          return std::string("YCBCR_P010");
                case PixelFormat::HSV_888:                             return std::string("HSV_888");
                case PixelFormat::Y8:                                  return std::string("Y8");
                case PixelFormat::Y16:                                 return std::string("Y16");
                case PixelFormat::YV12:                                return std::string("YV12");
                case PixelFormat::PRIVATE_YCBCR_420_P_M:               return std::string("PRIVATE_YCBCR_420_P_M");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M:              return std::string("PRIVATE_YCBCR_420_SP_M");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_TILED:        return std::string("PRIVATE_YCBCR_420_SP_M_TILED");
                case PixelFormat::PRIVATE_YV12_M:                      return std::string("PRIVATE_YV12_M");
                case PixelFormat::PRIVATE_YCRCB_420_SP_M:              return std::string("PRIVATE_YCRCB_420_SP_M");
                case PixelFormat::PRIVATE_YCRCB_420_SP_M_FULL:         return std::string("PRIVATE_YCRCB_420_SP_M_FULL");
                case PixelFormat::PRIVATE_YCBCR_420_P:                 return std::string("PRIVATE_YCBCR_420_P");
                case PixelFormat::PRIVATE_YCBCR_420_SP:                return std::string("PRIVATE_YCBCR_420_SP");
                case PixelFormat::PRIVATE_YCBCR_420_SPN:               return std::string("PRIVATE_YCBCR_420_SPN");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_S10B:         return std::string("PRIVATE_YCBCR_420_SP_M_S10B");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_S10B:          return std::string("PRIVATE_YCBCR_420_SPN_S10B");
                case PixelFormat::PRIVATE_YCBCR_P010_M:                return std::string("PRIVATE_YCBCR_P010_M");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC:         return std::string("PRIVATE_YCBCR_420_SP_M_SBWC");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC:          return std::string("PRIVATE_YCBCR_420_SPN_SBWC");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC:     return std::string("PRIVATE_YCBCR_420_SP_M_10B_SBWC");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC:      return std::string("PRIVATE_YCBCR_420_SPN_10B_SBWC");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC:      return std::string("PRIVATE_YCBCR_420_SPN_256_SBWC");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC:  return std::string("PRIVATE_YCBCR_420_SPN_10B_256_SBWC");
                case PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC:         return std::string("PRIVATE_YCRCB_420_SP_M_SBWC");
                case PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC:     return std::string("PRIVATE_YCRCB_420_SP_M_10B_SBWC");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50:     return std::string("PRIVATE_YCBCR_420_SP_M_SBWC_L50");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75:     return std::string("PRIVATE_YCBCR_420_SP_M_SBWC_L75");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L50:      return std::string("PRIVATE_YCBCR_420_SPN_SBWC_L50");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L75:      return std::string("PRIVATE_YCBCR_420_SPN_SBWC_L75");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40: return std::string("PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60: return std::string("PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80: return std::string("PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40:  return std::string("PRIVATE_YCBCR_420_SPN_10B_SBWC_L40");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60:  return std::string("PRIVATE_YCBCR_420_SPN_10B_SBWC_L60");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80:  return std::string("PRIVATE_YCBCR_420_SPN_10B_SBWC_L80");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L:     return std::string("PRIVATE_YCBCR_420_SP_M_32_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L:     return std::string("PRIVATE_YCBCR_420_SP_M_64_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L:      return std::string("PRIVATE_YCBCR_420_SPN_32_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L:      return std::string("PRIVATE_YCBCR_420_SPN_64_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L: return std::string("PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L: return std::string("PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L:  return std::string("PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L:  return std::string("PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR:  return std::string("PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR");
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR:  return std::string("PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR");
                default:                                                     return std::string("UNKNOWN");
        }
}

inline std::string get_format_layout_string(FormatLayout format_layout)
{
        switch (format_layout) {
                case FormatLayout::NONE:   return std::string("FormatLayout::NONE");
                case FormatLayout::LINEAR: return std::string("FormatLayout::LINEAR");
                case FormatLayout::DCC:    return std::string("FormatLayout::DCC");
                case FormatLayout::SBWC:   return std::string("FormatLayout::SBWC");
                default:                   return std::string("UNKNOWN");
        }
}

inline std::string get_format_layout_bitmask_string(uint32_t format_layout_mask)
{

        std::stringstream str;
        str << "[ ";
        if (format_layout_mask == 0){
                PRINT_FLAG_IF_SET_ONE_LINE(str, format_layout_mask, FormatLayoutBitMask::NONE);
        } else {
                PRINT_FLAG_IF_SET_ONE_LINE(str, format_layout_mask, FormatLayoutBitMask::LINEAR);
                PRINT_FLAG_IF_SET_ONE_LINE(str, format_layout_mask, FormatLayoutBitMask::DCC);
                PRINT_FLAG_IF_SET_ONE_LINE(str, format_layout_mask, FormatLayoutBitMask::SBWC);
        }
        str << "]";
        return str.str();
}

inline std::string get_ip_bitmask_string(uint32_t ip_bit_mask)
{
        std::stringstream str;
        str << "[ ";

        PRINT_FLAG_IF_SET_ONE_LINE(str, ip_bit_mask, IpBitMask::CPU);
        PRINT_FLAG_IF_SET_ONE_LINE(str, ip_bit_mask, IpBitMask::GPU);
        PRINT_FLAG_IF_SET_ONE_LINE(str, ip_bit_mask, IpBitMask::HWC);
        PRINT_FLAG_IF_SET_ONE_LINE(str, ip_bit_mask, IpBitMask::CAMERA);
        PRINT_FLAG_IF_SET_ONE_LINE(str, ip_bit_mask, IpBitMask::VIDEO);

        str << "]";
        return str.str();
}

inline std::string get_pixel_format_fourcc_string(uint32_t pixel_format_fourcc)
{
        std::stringstream str;

        if (pixel_format_fourcc == 0) {
                str << "DRM_FOURCC_INVALID";
                return str.str();
        }

        char *ptr = (char*)&pixel_format_fourcc;
        str << ptr[0];
        str << ptr[1];
        str << ptr[2];
        str << ptr[3];
        return str.str();
}

inline std::string get_component_type_string(uint64_t component_type)
{
        switch (component_type) {
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::R:  return std::string("R");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::G:  return std::string("G");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::B:  return std::string("B");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::A:  return std::string("A");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::Y:  return std::string("Y");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::CB: return std::string("CB");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::CR: return std::string("CR");
                case SGR_PLANE_LAYOUT_COMPONENT_TYPE::RAW:return std::string("RAW");
                default:                                  return std::string("UNKNOWN");
        }
}

inline void dump_sgr_alloc(sgr_alloc *info, uint32_t num_allocs, FormatLayout layout)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        std::stringstream str {};
        str << "\t\t num_allocs   = " << num_allocs << "\n";
        for (uint32_t i = 0; i < num_allocs; i++) {
                str << "\t\t allocs[" << i << "]:\n";
                str << "\t\t\t alignment   = " << (info[i].alignment)   << "\n";
                str << "\t\t\t total_size  = " << (info[i].total_size)  << "\n";
                switch (layout) {
                case FormatLayout::LINEAR:
                case FormatLayout::DCC:
                        str << "\t\t\t data.offset = " << (info[i].data.offset) << "\n";
                        str << "\t\t\t data.size   = " << (info[i].data.size)   << "\n";
                        str << "\t\t\t key.offset  = " << (info[i].key.offset)  << "\n";
                        str << "\t\t\t key.size    = " << (info[i].key.size)    << "\n";
                        break;
                case FormatLayout::SBWC:
                        str << "\t\t\t payload[0].offset  = " << (info[i].payload[0].offset)  << "\n";
                        str << "\t\t\t payload[0].size    = " << (info[i].payload[0].size)    << "\n";
                        str << "\t\t\t header[0].offset = " << (info[i].header[0].offset) << "\n";
                        str << "\t\t\t header[0].size   = " << (info[i].header[0].size)   << "\n";
                        str << "\t\t\t payload[1].offset  = " << (info[i].payload[1].offset)  << "\n";
                        str << "\t\t\t payload[1].size    = " << (info[i].payload[1].size)    << "\n";
                        str << "\t\t\t header[1].offset = " << (info[i].header[1].offset) << "\n";
                        str << "\t\t\t header[1].size   = " << (info[i].header[1].size)   << "\n";
                        break;
                default:
                        break;
                }
        }
        ALOGD("%s", str.str().c_str());
}

inline void dump_sgr_plane_layout(sgr_plane_layout *info, uint64_t num_planes)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        for (uint32_t i = 0; i < num_planes; i++) {
                std::stringstream str {};
                str << "\t\t plane_layouts[" << i << "]:\n";
                str << "\t\t\t num_components = " << (info[i].num_components) << "\n";
                for (uint32_t j = 0; j < info[i].num_components; j++) {
                        str << "\t\t\t components[" << j << "]:\n";
                        str << "\t\t\t\t component_type = " << get_component_type_string(info[i].components[j].component_type) << "\n";
                        str << "\t\t\t\t offset_in_bits = " << (info[i].components[j].offset_in_bits) << "\n";
                        str << "\t\t\t\t size_in_bits   = " << (info[i].components[j].size_in_bits) << "\n";
                }
                str << "\t\t\t offset_in_bytes = "          << (info[i].offset_in_bytes)          << "\n";
                str << "\t\t\t sample_increment_in_bits = " << (info[i].sample_increment_in_bits) << "\n";
                str << "\t\t\t stride_in_bytes = "          << (info[i].stride_in_bytes)          << "\n";
                str << "\t\t\t width_in_samples = "         << (info[i].width_in_samples)         << "\n";
                str << "\t\t\t height_in_samples = "        << (info[i].height_in_samples)        << "\n";
                str << "\t\t\t total_size_in_bytes = "      << (info[i].total_size_in_bytes)      << "\n";
                str << "\t\t\t horizontal_subsampling = "   << (info[i].horizontal_subsampling)   << "\n";
                str << "\t\t\t vertical_subsampling = "     << (info[i].vertical_subsampling)     << "\n";
                ALOGD("%s", str.str().c_str());
        }
}

inline void dump_metadata(sgr_metadata *meta)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        std::stringstream str {};
        str << "\t Metadata:\n";
        str << "\t\t buffer_id = "   << meta->buffer_id   << "\n";
        str << "\t\t length_name = " << meta->length_name << "\n";
        str << "\t\t name = "        << meta->name        << "\n";
        str << "\t\t width = "                                << meta->width                               << "\n";
        str << "\t\t height = "                               << meta->height                              << "\n";
        str << "\t\t layer_count = "                          << meta->layer_count                         << "\n";
        str << "\t\t pixel_format_requested = 0x"             << std::hex << meta->pixel_format_requested  << " [";
        str <<  get_pixel_format_string(static_cast<PixelFormat>(meta->pixel_format_requested)) << "]\n";
        str << "\t\t pixel_format_fourcc = 0x"    << std::hex << meta->pixel_format_fourcc    << std::dec  << " [";
        str <<  get_pixel_format_fourcc_string(meta->pixel_format_fourcc) << "]\n";
        str << "\t\t pixel_format_modifier = 0x"  << std::hex << meta->pixel_format_modifier  << std::dec  << "\n";
        str << "\t\t "                                        << get_usage_string(meta->usage);
        str << "\t\t allocation_size = "                      << meta->allocation_size                     << "\n";
        str << "\t\t protected_content = "                    << meta->protected_content                   << "\n";
        str << "\t\t compression = "                          << meta->compression                         << "\n";
        str << "\t\t interlaced = "                           << meta->interlaced                          << "\n";
        str << "\t\t chroma_siting = "                        << meta->chroma_siting                       << "\n";
        str << "\t\t num_plane_layouts = "                    << meta->num_plane_layouts                   << "\n";
        ALOGD("%s", str.str().c_str());

        dump_sgr_plane_layout(meta->plane_layouts, meta->num_plane_layouts);

        str = std::stringstream();
        str << "\t\t num_crops = " << meta->num_crops      << "\n";
        for (uint32_t i = 0; i < meta->num_crops; i++) {
                str << "\t\t crops[" << i << "]:\n";
                str << "\t\t\t left   = " << (meta->crops[i]).left   <<"\n";
                str << "\t\t\t top    = " << (meta->crops[i]).top    <<"\n";
                str << "\t\t\t right  = " << (meta->crops[i]).right  <<"\n";
                str << "\t\t\t bottom = " << (meta->crops[i]).bottom <<"\n";
        }

        str << "\t\t dataspace = "           << meta->dataspace           << "\n";
        str << "\t\t blend_mode = "          << meta->blend_mode          << "\n";

        str << "\t\t is_smpte2086_set = "    << meta->is_smpte2086_set    << "\n";
        if (meta->is_smpte2086_set){
                struct sgr_smpte2086 *ptr = &meta->smpte2086;
                str << "\t\t smpte2086: {x, y}\n";
                str << "\t\t\t primary_red   = {" << ptr->primary_red.x   << ", " << ptr->primary_red.y   << "}\n";
                str << "\t\t\t primary_green = {" << ptr->primary_green.x << ", " << ptr->primary_green.y << "}\n";
                str << "\t\t\t primary_blue  = {" << ptr->primary_blue.x  << ", " << ptr->primary_blue.y  << "}\n";
                str << "\t\t\t white_point   = {" << ptr->white_point.x   << ", " << ptr->white_point.y   << "}\n";
                str << "\t\t\t max_luminance = "  << ptr->max_luminance <<"\n";
                str << "\t\t\t min_luminance = "  << ptr->min_luminance <<"\n";
        }

        str << "\t\t is_cta861_3_set = "     << meta->is_cta861_3_set     << "\n";
        if (meta->is_cta861_3_set){
                struct sgr_cta861_3 *ptr = &meta->cta861_3;
                str << "\t\t cta861_3:\n";
                str << "\t\t\t max_content_light_level =" << ptr->max_content_light_level <<"\n";
                str << "\t\t\t max_frame_average_light_level =" << ptr->max_frame_average_light_level <<"\n";
        }

        str << "\t\t length_smpte2094_40 = " << meta->length_smpte2094_40 << "\n";
        str << "\t\t smpte2094_40 = "        << meta->smpte2094_40        << "\n";

        // Allocation info
        str << "\t\t alloc_width  = "   << meta->alloc_width  << "\n";
        str << "\t\t alloc_height = "   << meta->alloc_height << "\n";
        str << "\t\t alloc_format = 0x" << std::hex << meta->alloc_format << std::dec << " [";
        str << get_pixel_format_string(static_cast<PixelFormat>(meta->alloc_format)) << "]\n";
        str << "\t\t alloc_layout = 0x" << std::hex << meta->alloc_layout << std::dec << " [";
        str << get_format_layout_string(static_cast<FormatLayout>(meta->alloc_layout)) << "]\n";
        ALOGD("%s", str.str().c_str());

        dump_sgr_alloc(meta->allocs, meta->num_allocs, static_cast<FormatLayout>(meta->alloc_layout));
}

inline void dump_buffer_allocation(private_handle_t *info, sgr_metadata* meta)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_ALLOCATION_IS_NOT_SET();

        std::stringstream str {};
        str << "[sgr_alloc_info] ";

        str << "requested: (" << meta->width << "/" << meta->height;
        str << "/0x" << std::hex << meta->pixel_format_requested << std::dec;
        str << "[" << get_pixel_format_string(static_cast<PixelFormat>(meta->pixel_format_requested)) << "]";
        str << "/0x" << std::hex << meta->usage << std::dec << "), ";

		str << "allocated: 0x" << std::hex << meta->buffer_id << std::dec << " (" << meta->alloc_width << "/";
        str << meta->alloc_height << "/0x" << std::hex << meta->alloc_format << std::dec;
        str << "[" << get_pixel_format_string(static_cast<PixelFormat>(meta->alloc_format)) << "]/";
        str << meta->allocation_size << "/" << get_format_layout_string(static_cast<FormatLayout>(meta->alloc_layout)) << ")";

        ALOGD("%s", str.str().c_str());
}

inline void dump_private_handle(private_handle_t *info)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        std::stringstream str {};
        str << "private_handle_t : " << info << "\n";

        // Native Handle
        str << "\t version = " << (info->version)  << "\n";
        str << "\t numFds  = " << (info->numFds)   << "\n";
        str << "\t numInts = " << (info->numInts)  << "\n";

        // Private Handle
        for (uint i = 0; i < info->numFds; i++){
                str << "\t fds[" << i << "] = " << (info->fds[i]) << "\n";
        }
        str << "\t magic = 0x" << std::hex << (info->magic) << std::dec << "\n";

        // Process local information
        for (uint i = 0; i < info->numFds - SGR_NUM_METADATA; i++){
                str << "\t alloc_bases[" << i << "] = " << (info->alloc_bases[i]) << std::dec << "\n";
        }
        str << "\t metadata_base = "                << (info->metadata_base)              << "\n";
        str << "\t reserved_region_size = "         << (info->reserved_region_size)       << "\n";
        str << "\t pid = "                          << (info->pid)                        << "\n";
        str << "\t lock_cpu_usage = 0x" << std::hex << (info->lock_cpu_usage) << std::dec << "\n";
        str << "\t lock_count = "                   << (info->lock_count)                 << "\n";

        ALOGD("%s", str.str().c_str());
        if (info->metadata_base !=0) {
                dump_metadata(get_metadata(reinterpret_cast<void *>(info->metadata_base)));
        }
}

inline void dump_private_handle_bases(private_handle_t *info)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        std::stringstream str {};
        // Process local information
        for (uint i = 0; i < info->numFds - SGR_NUM_METADATA; i++){
                str << "\t alloc_bases[" << i << "] = " << std::hex << (info->alloc_bases[i]) << std::dec << "\n";
        }
        str << "\t metadata_base = " << std::hex << (info->metadata_base) << std::dec << "\n";
        str << "\t pid = "                       << (info->pid)                       << "\n";
        ALOGD("%s", str.str().c_str());
}

inline void dump_descriptor_info(const BufferDescriptorInfo &descriptor_info)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        SGR_LOGD("%s", descriptor_info.dump().c_str());
        SGR_LOGD("%s", get_usage_string(descriptor_info.usage).c_str());
}

inline void dump_descriptor(const BufferDescriptor &descriptor)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        SGR_LOGD("%s", descriptor.dump().c_str());
        SGR_LOGD("%s", get_usage_string(descriptor.usage).c_str());
}

inline void dump_format_info(PixelFormat requested_format, PixelFormat alloc_format, uint64_t usage,
                             FormatLayout format_layout, uint32_t ip_flags)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        SGR_LOGD("Format Info:\n"
                 "\t\trequested_format = 0x%x [%s]\n"
                 "\t\tusage            = 0x%" PRIx64 " [%s]\n"
                 "\t\talloc_format     = 0x%x [%s]\n"
                 "\t\tlayout           = 0x%x [%s]\n"
                 "\t\tip_flags         = 0x%x [%s]\n",
                 requested_format, get_pixel_format_string(requested_format).c_str(),
                 usage, get_usage_string(usage).c_str(),
                 alloc_format, get_pixel_format_string(alloc_format).c_str(),
                 static_cast<uint32_t>(format_layout), get_format_layout_string(format_layout).c_str(),
                 ip_flags, get_ip_bitmask_string(ip_flags).c_str());
}

inline void dump_alignment(align_info align, uint32_t ip_flags)
{
        CHECK_AND_RETURN_IF_CONFIG_VERBOSE_IS_NOT_SET();

        SGR_LOGD("%s\n"
                 "align.stride_in_bytes  = %u\n"
                 "align.vstride_in_pixels = %u\n"
                 "align.plane_padding_in_bytes = %u\n",
                  get_ip_bitmask_string(ip_flags).c_str(),
                  align.stride_in_bytes,
                  align.vstride_in_pixels,
                  align.plane_padding_in_bytes);
}

} // gralloc
} // samsung
} // android
