///
/// @file sbwc_layout_manager.cpp
/// @copyright 2020 Samsung Electronics
///

#include "debug_helper.h"
#include "format_info.h"
#include "sbwc_layout_manager.h"
#include "util/macro.h"

namespace android {
namespace samsung {
namespace gralloc {

struct ycbcr_components {
        uint32_t luma;
        uint32_t chroma;
};

enum class ScaleFactor : uint32_t {
        S_NONE = 100,
        S_50   = 50,
        S_75   = 75,
        S_40   = 40,
        S_60   = 60,
        S_80   = 80,
};

enum class AlignFactor : uint32_t {
        A_NONE = 0,
        A_32,
        A_64,
        A_256,
};

static inline AlignFactor get_align_factor(PixelFormat format)
{
        AlignFactor ret = AlignFactor::A_NONE;
        switch (format) {
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L:
                ret = AlignFactor::A_32;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR:
                ret = AlignFactor::A_64;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC:
                ret = AlignFactor::A_256;
                break;
        default:
                ret = AlignFactor::A_NONE;
        }

        return ret;
}

static inline uint32_t get_loseless_payload_stride(sgr_extent_2d alloc_extent, uint32_t bps, AlignFactor align_factor)
{
        uint32_t stride = 0;

        switch (bps) {
        case 8:
        {
                if (align_factor == AlignFactor::A_256)
                {
                        stride = SBWC_256_8B_STRIDE(alloc_extent.width);
                }
                else
                {
                        stride = SBWC_8B_STRIDE(alloc_extent.width);
                }
                break;
        }
        case 16:
        {
                if (align_factor == AlignFactor::A_256)
                {
                        stride = SBWC_256_10B_STRIDE(alloc_extent.width);
                }
                else
                {
                        stride = SBWC_10B_STRIDE(alloc_extent.width);
                }
                break;
        }
        default: SGR_ASSERT_MSG(false, "Invalid bits-per-sample");
        }

        return stride;
}

static inline void get_loseless_header_size(sgr_extent_2d alloc_extent, uint32_t bps, AlignFactor align_factor, ycbcr_components *header)
{
        switch (bps) {
        case 8:
        {
                if (align_factor == AlignFactor::A_256)
                {
                        header->luma = SBWC_256_8B_Y_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                        header->chroma = SBWC_256_8B_CBCR_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                }
                else
                {
                        header->luma = SBWC_8B_Y_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                        header->chroma = SBWC_8B_CBCR_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                }
                break;
        }
        case 16:
        {
                if (align_factor == AlignFactor::A_256)
                {
                        header->luma = SBWC_256_10B_Y_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                        header->chroma = SBWC_256_10B_CBCR_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                }
                else
                {
                        header->luma = SBWC_10B_Y_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                        header->chroma = SBWC_10B_CBCR_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                }
                break;
        }
        default: SGR_ASSERT_MSG(false, "Invalid bits-per-sample");
        }
}

static inline void get_loseless_payload_size(sgr_extent_2d alloc_extent, uint32_t bps, AlignFactor align_factor, ycbcr_components *payload)
{
        switch (bps) {
        case 8:
        {
                if (align_factor == AlignFactor::A_256)
                {
                        payload->luma = SBWC_256_8B_Y_SIZE(alloc_extent.width, alloc_extent.height);
                        payload->chroma = SBWC_256_8B_CBCR_SIZE(alloc_extent.width, alloc_extent.height);
                }
                else
                {
                        payload->luma = SBWC_8B_Y_SIZE(alloc_extent.width, alloc_extent.height);
                        payload->chroma = SBWC_8B_CBCR_SIZE(alloc_extent.width, alloc_extent.height);
                }
                break;
        }
        case 16:
        {
                if (align_factor == AlignFactor::A_256)
                {
                        payload->luma = SBWC_256_10B_Y_SIZE(alloc_extent.width, alloc_extent.height);
                        payload->chroma = SBWC_256_10B_CBCR_SIZE(alloc_extent.width, alloc_extent.height);
                }
                else
                {
                        payload->luma = SBWC_10B_Y_SIZE(alloc_extent.width, alloc_extent.height);
                        payload->chroma = SBWC_10B_CBCR_SIZE(alloc_extent.width, alloc_extent.height);
                }
                break;
        }
        default: SGR_ASSERT_MSG(false, "Invalid bits-per-sample");
        }
}

static inline uint32_t get_lossy_payload_stride(sgr_extent_2d alloc_extent, uint32_t bps, ScaleFactor scale_factor, AlignFactor align_factor)
{
        uint32_t stride = 0;
        /* SBWC lossy formats over v2.7 are specified without scale factor */
        if (scale_factor == ScaleFactor::S_NONE) {
                switch (align_factor) {
                case AlignFactor::A_32:
                {
                        stride = SBWCL_32_STRIDE(alloc_extent.width);
                        break;
                }
                case AlignFactor::A_64:
                {
                        stride = SBWCL_64_STRIDE(alloc_extent.width);
                        break;
                }
                default: SGR_ASSERT_MSG(false, "Invalid align-factor");
                }
        }
        else {
                switch (bps) {
                case 8:
                {
                        stride = SBWCL_8B_STRIDE(alloc_extent.width, static_cast<uint32_t>(scale_factor));
                        break;
                }
                case 16:
                {
                        stride = SBWCL_10B_STRIDE(alloc_extent.width, static_cast<uint32_t>(scale_factor));
                        break;
                }
                default: SGR_ASSERT_MSG(false, "Invalid bits-per-sample");
                }
        }

        return stride;
}

static inline void get_lossy_header_size(sgr_extent_2d alloc_extent, uint32_t bps, ScaleFactor scale_factor, ycbcr_components *header)
{
        /* SBWC lossy formats over v2.7 are specified without scale factor, and have header */
        if (scale_factor == ScaleFactor::S_NONE) {
                header->luma = SBWCL_Y_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
                header->chroma = SBWCL_CBCR_HEADER_SIZE(alloc_extent.width, alloc_extent.height);
        }
}

static inline void get_lossy_payload_size(sgr_extent_2d alloc_extent, uint32_t bps, ScaleFactor scale_factor, AlignFactor align_factor, ycbcr_components *payload)
{
        /* SBWC lossy formats over v2.7 are specified without scale factor */
        if (scale_factor == ScaleFactor::S_NONE) {
                switch (align_factor) {
                case AlignFactor::A_32:
                {
                        payload->luma = SBWCL_32_Y_SIZE(alloc_extent.width, alloc_extent.height);
                        payload->chroma = SBWCL_32_CBCR_SIZE(alloc_extent.width, alloc_extent.height);
                        break;
                }
                case AlignFactor::A_64:
                {
                        payload->luma = SBWCL_64_Y_SIZE(alloc_extent.width, alloc_extent.height);
                        payload->chroma = SBWCL_64_CBCR_SIZE(alloc_extent.width, alloc_extent.height);
                        break;
                }
                default: SGR_ASSERT_MSG(false, "Invalid align-factor");
                }
        }
        else {
                switch (bps) {
                case 8:
                {
                        payload->luma = SBWCL_8B_Y_SIZE(alloc_extent.width, alloc_extent.height, static_cast<uint32_t>(scale_factor));
                        payload->chroma = SBWCL_8B_CBCR_SIZE(alloc_extent.width, alloc_extent.height, static_cast<uint32_t>(scale_factor));
                        break;
                }
                case 16:
                {
                        payload->luma = SBWCL_10B_Y_SIZE(alloc_extent.width, alloc_extent.height, static_cast<uint32_t>(scale_factor));
                        payload->chroma = SBWCL_10B_CBCR_SIZE(alloc_extent.width, alloc_extent.height, static_cast<uint32_t>(scale_factor));
                        break;
                }
                default: SGR_ASSERT_MSG(false, "Invalid bits-per-sample");
                }
        }
}

///
/// @brief Get the Scale Factor for a given pixel format
///
/// @param[in]  format          Format
///
/// @return Scale Factor for the format
///
static inline ScaleFactor get_scale_factor(PixelFormat format)
{
        SGR_ASSERT(format != PixelFormat::UNDEFINED);

        ScaleFactor ret = ScaleFactor::S_NONE;
        switch (format) {
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC:
        /* SBWC lossy formats over v2.7 have no scale factor */
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR:
                ret = ScaleFactor::S_NONE;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L50:
                ret = ScaleFactor::S_50;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L75:
                ret = ScaleFactor::S_75;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40:
                ret = ScaleFactor::S_40;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60:
                ret = ScaleFactor::S_60;
                break;
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80:
                ret = ScaleFactor::S_80;
                break;
        default:
                SGR_LOGE("Unsupported format for SBWC layout = %d",
                         static_cast<uint32_t>(format));
        }

        return ret;
}

static inline uint32_t get_sbwc_stride(PixelFormat format, sgr_extent_2d alloc_extent)
{
        const component_info *comp_info = get_component_info(format);
        const bool lossless_format = is_lossless_format(format);

        ScaleFactor scale_factor = get_scale_factor(format);
        AlignFactor align_factor = get_align_factor(format);

        if (lossless_format) {
                return get_loseless_payload_stride(alloc_extent, comp_info[0].bits, align_factor);
        } else {
                return get_lossy_payload_stride(alloc_extent, comp_info[0].bits, scale_factor, align_factor);
        }
}

static inline void get_sbwc_info(PixelFormat format,
                                 sgr_extent_2d alloc_extent,
                                 ycbcr_components *header,
                                 ycbcr_components *payload)
{
        const component_info *comp_info = get_component_info(format);
        const bool lossless_format = is_lossless_format(format);

        ScaleFactor scale_factor = get_scale_factor(format);
        AlignFactor align_factor = get_align_factor(format);

        if (lossless_format) {
                get_loseless_header_size(alloc_extent, comp_info[0].bits, align_factor, header);
                get_loseless_payload_size(alloc_extent, comp_info[0].bits, align_factor, payload);
        } else {
                get_lossy_header_size(alloc_extent, comp_info[0].bits, scale_factor, header);
                get_lossy_payload_size(alloc_extent, comp_info[0].bits, scale_factor, align_factor, payload);
        }
}

///
/// @brief Get block extent for SBWC layout
///
/// @param[in]  format Format
/// @param[out] extent Block extent
///
void SbwcLayoutManager::get_block_extent(PixelFormat format, sgr_extent_2d *extent) const
{
        extent->width = sbwc_blk_dimension_width;
        if (is_lossless_format(format) == true) {
                extent->height = sbwc_vstride_lossless_alignment;
        } else {
                extent->height = sbwc_vstride_lossy_alignment;
        }
}

///
/// @brief Compute plane layout and return size of plane.
///
/// @param[in]  format                 Format
/// @param[in]  layout                 Layout of the format
/// @param[in]  alloc_extent           Image extent
/// @param[in]  num_planes_per_alloc   Number of planes for currnet alloc
/// @param[out] cur_plane_layout_index Number of sgr_plane_layout array
/// @param[out] plane_layouts          Pointer to sgr_plane_layout array
///
/// @return alloc size in byte
///
uint32_t SbwcLayoutManager::compute_plane_layout_for_alloc(PixelFormat format,
                                                           const sgr_extent_2d &alloc_extent,
                                                           uint32_t num_planes_per_alloc,
                                                           uint32_t *cur_plane_layout_index,
                                                           sgr_plane_layout *plane_layouts,
                                                           uint32_t luma_size, uint32_t chroma_size) const
{
        SGR_ASSERT(plane_layouts != nullptr);
        SGR_ASSERT(cur_plane_layout_index != nullptr);
        SGR_ASSERT(*cur_plane_layout_index < SGR_MAX_NUM_PLANES);

        uint32_t num_components = get_num_components(format);
        const component_info *comp_info = get_component_info(format);

        uint32_t alloc_size = 0;

        for (uint32_t j = 0; j < num_planes_per_alloc; j++) {
                sgr_plane_layout *plane_layout = &plane_layouts[*cur_plane_layout_index];
                uint32_t idx = 0;
                uint32_t sample_bits = 0;
                for (uint i = 0; i < num_components; i++) {
                        if (comp_info[i].plane_index == *cur_plane_layout_index) {
                                plane_layout->components[idx].component_type = comp_info[i].type;
                                plane_layout->components[idx].offset_in_bits = sample_bits;
                                plane_layout->components[idx].size_in_bits   = comp_info[i].bits;
                                sample_bits += comp_info[i].bits;
                                idx++;
                        }
                }

                SGR_ASSERT(idx > 0);
                plane_layout->num_components           = idx;
                plane_layout->offset_in_bytes          = alloc_size;
                plane_layout->stride_in_bytes          = get_sbwc_stride(format, alloc_extent);
                plane_layout->total_size_in_bytes      = *cur_plane_layout_index == 0 ? luma_size : chroma_size;

                const sub_sampling_info *sub_info      = get_sampling_info(format, *cur_plane_layout_index);
                plane_layout->horizontal_subsampling   = sub_info->hsub;
                plane_layout->vertical_subsampling     = sub_info->vsub;

                sgr_extent_2d block_extent{};
                get_block_extent(format, &block_extent);

                const uint32_t sub_sampled_width       = alloc_extent.width / sub_info->hsub;
                const uint32_t sub_sampled_height      = alloc_extent.height / sub_info->vsub;
                plane_layout->sample_increment_in_bits = sample_bits;
                plane_layout->width_in_samples         = SGR_ALIGN(sub_sampled_width, block_extent.width);
                plane_layout->height_in_samples        = SGR_ALIGN(sub_sampled_height, block_extent.height);

                alloc_size += plane_layout->total_size_in_bytes;
                (*cur_plane_layout_index)++;
        }

        return alloc_size;
}

///
/// @brief Get alloc extent
///
/// @param[in]  format       Target format
/// @param[out] alloc_extent Alloc extent
///
/// @return alloc_extent
///
void SbwcLayoutManager::get_alloc_extent(PixelFormat format, sgr_extent_2d *alloc_extent) const
{
        sgr_extent_2d block_extent = {};
        get_block_extent(format, &block_extent);

        alloc_extent->width = SGR_ALIGN(alloc_extent->width,   block_extent.width);
        alloc_extent->height = SGR_ALIGN(alloc_extent->height, block_extent.height);
}

///
/// @brief Get allocation information
///
/// @param[in]     format        Format
/// @param[in]     layer_count   Number of layers
/// @param[in]     ip_flags      IP list
/// @param[in/out] alloc_extent  Image extent
/// @param[out]    allocs        Pointer to sgr_alloc array. If nullptr, then ignored
/// @param[out]    num_planes    Number of sgr_plane_layout array
/// @param[out]    plane_layouts Pointer to sgr_plane_layout array
///
/// @return the number of alloc_info which are updated
///
uint32_t SbwcLayoutManager::get_alloc_info(PixelFormat format, uint32_t layer_count, uint32_t ip_flags,
                                           sgr_extent_2d *alloc_extent, sgr_alloc *allocs,
                                           uint32_t *num_planes, sgr_plane_layout *plane_layouts, uint64_t usage) const
{
        SGR_UNUSED(ip_flags);
        SGR_UNUSED(usage);

        get_alloc_extent(format, alloc_extent);

        SGR_ASSERT(allocs != nullptr);
        SGR_LOGV("Alloc_infos = %p\n"
                 "\t\talloc_extent.width  = %u\n"
                 "\t\talloc_extent.height = %u\n"
                 "\t\tformat = %u [%s]\n",
                 allocs, alloc_extent->width, alloc_extent->height,
                 format, get_pixel_format_string(format).c_str());

        if (allocs == nullptr) {
                return 0;
        }

        struct ycbcr_components header;
        struct ycbcr_components payload;
        get_sbwc_info(format, *alloc_extent, &header, &payload);
        uint32_t luma_size = header.luma + payload.luma;
        uint32_t chroma_size = header.chroma + payload.chroma;

        const uint32_t num_allocs = get_num_allocs(format);
        const uint32_t *num_planes_per_alloc = get_num_planes_per_alloc(format);

        for(uint32_t i = 0; i < num_allocs; i++) {
                uint32_t data_size = compute_plane_layout_for_alloc(format,
                                                                    *alloc_extent,
                                                                    num_planes_per_alloc[i],
                                                                    num_planes, plane_layouts,
                                                                    luma_size, chroma_size);
                sgr_alloc *alloc = &allocs[i];
                alloc->alignment   = 4 * 1024; // 4 KB

                alloc->payload[0].offset  = 0;
                alloc->payload[0].size    = (i == 0) ? payload.luma : payload.chroma;

                alloc->header[0].offset = alloc->payload[0].offset  + alloc->payload[0].size;
                alloc->header[0].size   = (i == 0) ? header.luma : header.chroma;

                if (num_allocs == 1) { // SPN_SBWC formats
                        alloc->payload[1].offset  = alloc->header[0].offset + alloc->header[0].size;;
                        alloc->payload[1].size    = payload.chroma;

                        alloc->header[1].offset = alloc->payload[1].offset  + alloc->payload[1].size;
                        alloc->header[1].size   = header.chroma;
                }
                alloc->total_size  = data_size;
        }

        return num_allocs;
}

} // gralloc
} // samsung
} // android
