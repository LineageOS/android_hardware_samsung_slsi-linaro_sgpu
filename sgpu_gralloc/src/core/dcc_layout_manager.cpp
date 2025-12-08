///
/// @file dcc_layout_manager.cpp
/// @copyright 2020 Samsung Electronics
///

#include "dcc_layout_manager.h"
#include "debug_helper.h"
#include "format_info.h"
#include "hidl_defines.h"
#include "util/util.h"

namespace android {
namespace samsung {
namespace gralloc {

constexpr uint32_t size_4k  = 4 * 1024;
constexpr uint32_t size_64k = 64 * 1024;

static inline void get_dcc_data_block_extent_64kb_r_x_1xaa(PixelFormat format, sgr_extent_2d *extent)
{
        static const sgr_extent_2d log2_bpe_to_block_extent[] = {
                {256, 256},     // bpe = 1
                {256, 128},     // bpe = 2
                {128, 128},     // bpe = 4
                {128, 64},      // bpe = 8
                {64,  64}       // bpe = 16
        };

        uint32_t index = get_log2_ffs(get_bps(format));
        SGR_ASSERT(index < SGR_ARRAY_SIZE(log2_bpe_to_block_extent));
        *extent = log2_bpe_to_block_extent[index];
}

static inline void get_dcc_data_block_extent_4kb_r_x_1xaa(PixelFormat format, sgr_extent_2d *extent)
{
        static const sgr_extent_2d log2_bpe_to_block_extent[] = {
                {64, 64},     // bpe = 1
                {64, 32},     // bpe = 2
                {32, 32},     // bpe = 4
                {32, 16},     // bpe = 8
                {16, 16}      // bpe = 16
        };

        uint32_t index = get_log2_ffs(get_bps(format));
        SGR_ASSERT(index < SGR_ARRAY_SIZE(log2_bpe_to_block_extent));
        *extent = log2_bpe_to_block_extent[index];
}

///
/// @brief Get data size of 64kb_r_x_1xaa and plane information
///
/// @param[in]  format        Format
/// @param[in]  alloc_extent  Image extent
/// @param[out] num_planes    Number of sgr_plane array.
/// @param[out] plane_layouts Pointer to sgr_plane array.
///
/// @return data size in byte
///
static inline uint32_t get_data_size_64kb_r_x_1xaa(PixelFormat format,
                                                   const sgr_extent_2d &alloc_extent,
                                                   uint32_t *num_planes, sgr_plane_layout *plane_layouts)
{
        SGR_ASSERT(plane_layouts != nullptr);
        SGR_ASSERT(num_planes != nullptr);

        sgr_extent_2d data_block_extent = {};
        get_dcc_data_block_extent_64kb_r_x_1xaa(format, &data_block_extent);
        SGR_ASSERT((alloc_extent.width % data_block_extent.width) == 0);
        SGR_ASSERT((alloc_extent.height % data_block_extent.height) == 0);

        sgr_extent_2d alloc_extent_in_block = {};
        alloc_extent_in_block.width = alloc_extent.width / data_block_extent.width;
        alloc_extent_in_block.height = alloc_extent.height / data_block_extent.height;

        constexpr uint32_t data_block_size = size_64k;
        sgr_plane_layout *plane = &plane_layouts[0];

        const component_info *comp_info = get_component_info(format);
        plane->num_components = get_num_components(format);
        uint32_t sample_bits = 0;
        for (uint i = 0; i < SGR_MAX_NUM_PLANE_LAYOUT_COMPONENTS; i++) {
                plane->components[i].component_type = comp_info[i].type;
                plane->components[i].offset_in_bits = sample_bits;
                plane->components[i].size_in_bits   = comp_info[i].bits;
                sample_bits += comp_info[i].bits;
        }

        *num_planes = 1;
        plane->offset_in_bytes = 0;
        plane->sample_increment_in_bits = 0;
        plane->width_in_samples = alloc_extent.width;
        plane->height_in_samples = alloc_extent.height;
        plane->total_size_in_bytes = alloc_extent_in_block.width *
                                     data_block_size *
                                     alloc_extent_in_block.height;
        // Set stride with 0 since they are not meaningful for DCC
        plane->stride_in_bytes = 0;
        plane->horizontal_subsampling = 1;
        plane->vertical_subsampling = 1;

        return plane->total_size_in_bytes;
}

///
/// @brief Get data size of 4kb_r_x_1xaa and plane information
///
/// @param[in]  format        Format
/// @param[in]  alloc_extent  Image extent
/// @param[out] num_planes    Number of sgr_plane array.
/// @param[out] plane_layouts Pointer to sgr_plane array.
///
/// @return data size in byte
///
static inline uint32_t get_data_size_4kb_r_x_1xaa(PixelFormat format,
                                                   const sgr_extent_2d &alloc_extent,
                                                   uint32_t *num_planes, sgr_plane_layout *plane_layouts)
{
        SGR_ASSERT(plane_layouts != nullptr);
        SGR_ASSERT(num_planes != nullptr);

        sgr_extent_2d data_block_extent = {};
        get_dcc_data_block_extent_4kb_r_x_1xaa(format, &data_block_extent);
        SGR_ASSERT((alloc_extent.width % data_block_extent.width) == 0);
        SGR_ASSERT((alloc_extent.height % data_block_extent.height) == 0);

        sgr_extent_2d alloc_extent_in_block = {};
        alloc_extent_in_block.width = alloc_extent.width / data_block_extent.width;
        alloc_extent_in_block.height = alloc_extent.height / data_block_extent.height;

        constexpr uint32_t data_block_size = size_4k;
        sgr_plane_layout *plane = &plane_layouts[0];

        const component_info *comp_info = get_component_info(format);
        plane->num_components = get_num_components(format);
        uint32_t sample_bits = 0;
        for (uint i = 0; i < SGR_MAX_NUM_PLANE_LAYOUT_COMPONENTS; i++) {
                plane->components[i].component_type = comp_info[i].type;
                plane->components[i].offset_in_bits = sample_bits;
                plane->components[i].size_in_bits   = comp_info[i].bits;
                sample_bits += comp_info[i].bits;
        }

        *num_planes = 1;
        plane->offset_in_bytes = 0;
        plane->sample_increment_in_bits = 0;
        plane->width_in_samples = alloc_extent.width;
        plane->height_in_samples = alloc_extent.height;
        plane->total_size_in_bytes = alloc_extent_in_block.width *
                                     data_block_size *
                                     alloc_extent_in_block.height;
        // Set stride with 0 since they are not meaningful for DCC
        plane->stride_in_bytes = 0;
        plane->horizontal_subsampling = 1;
        plane->vertical_subsampling = 1;

        return plane->total_size_in_bytes;
}

static inline void get_dcc_key_block_extent_64kb_r_x_1xaa(PixelFormat format, sgr_extent_2d *extent)
{
        static const sgr_extent_2d log2_bpe_to_block_extent[] = {
                {1024, 1024},   // bpe = 1
                {1024, 512},    // bpe = 2
                {512,  512},    // bpe = 4
                {512,  256},    // bpe = 8
                {256,  256}     // bpe = 16
        };


        uint32_t index = get_log2_ffs(get_bps(format));
        SGR_ASSERT(index < SGR_ARRAY_SIZE(log2_bpe_to_block_extent));
        *extent = log2_bpe_to_block_extent[index];
}

///
/// @brief Get key size of 64kb_r_x_1xaa
///
/// @param[in]  format       Format
/// @param[in]  alloc_extent Image extent
///
/// @return data size in byte
///
static inline uint32_t get_key_size_64kb_r_x_1xaa(PixelFormat format,
                                                  const sgr_extent_2d &alloc_extent)
{
        sgr_extent_2d key_block_extent = {};
        get_dcc_key_block_extent_64kb_r_x_1xaa(format, &key_block_extent);

        constexpr uint32_t key_block_size = size_4k;
        sgr_extent_2d alloc_extent_in_block = {};
        alloc_extent_in_block.width =
                SGR_INT_DIV_CEIL(alloc_extent.width, key_block_extent.width);
        alloc_extent_in_block.height =
                SGR_INT_DIV_CEIL(alloc_extent.height, key_block_extent.height);

        return alloc_extent_in_block.width * alloc_extent_in_block.height * key_block_size;
}

///
/// @brief Get block extent for DCC layout
///
/// @param[in]  format Format
/// @param[out] extent Block extent
///
void DccLayoutManager::get_block_extent(PixelFormat format, sgr_extent_2d *extent) const
{
        if(android::base::GetBoolProperty(CONFIG_SAJC_4K_SWIZZLE,
                                          CONFIG_SAJC_4K_SWIZZLE_DEFAULT) == true) {
                get_dcc_data_block_extent_4kb_r_x_1xaa(format, extent);
        } else {
                get_dcc_data_block_extent_64kb_r_x_1xaa(format, extent);
        }
}

///
/// @brief Get alloc extent
///
/// @param[in]  format       Target format
/// @param[out] alloc_extent Alloc extent
///
/// @return alloc_extent
///
void DccLayoutManager::get_alloc_extent(PixelFormat format, sgr_extent_2d *alloc_extent) const
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
uint32_t DccLayoutManager::get_alloc_info(PixelFormat format, uint32_t layer_count, uint32_t ip_flags,
                                           sgr_extent_2d *alloc_extent, sgr_alloc *allocs,
                                           uint32_t *num_planes, sgr_plane_layout *plane_layouts, uint64_t usage) const
{
        SGR_UNUSED(ip_flags);
        SGR_UNUSED(usage);

        get_alloc_extent(format, alloc_extent);

        SGR_LOGV("Alloc_infos = %p\n"
                 "\t\talloc_extent.width  = %u\n"
                 "\t\talloc_extent.height = %u\n"
                 "\t\tformat = %u [%s]\n",
                 allocs, alloc_extent->width, alloc_extent->height,
                 format, get_pixel_format_string(format).c_str());

        if (allocs != nullptr) {
                sgr_alloc *alloc = &allocs[0];

                if(android::base::GetBoolProperty(CONFIG_SAJC_4K_SWIZZLE,
                                                  CONFIG_SAJC_4K_SWIZZLE_DEFAULT) == true) {
                        alloc->alignment = size_4k;
                } else {
                        alloc->alignment = size_64k;
                }

                alloc->data.offset = 0;
                if(android::base::GetBoolProperty(CONFIG_SAJC_4K_SWIZZLE,
                                                  CONFIG_SAJC_4K_SWIZZLE_DEFAULT) == true) {
                        alloc->data.size = layer_count * get_data_size_4kb_r_x_1xaa(format,
                                                                                    *alloc_extent,
                                                                                    num_planes,
                                                                                    plane_layouts);
                } else {
                        alloc->data.size = layer_count * get_data_size_64kb_r_x_1xaa(format,
                                                                                    *alloc_extent,
                                                                                    num_planes,
                                                                                    plane_layouts);
                }
                alloc->key.offset  = alloc->data.size;
                alloc->key.size    = layer_count * get_key_size_64kb_r_x_1xaa(format,
                                                                                *alloc_extent);
                alloc->total_size  = alloc->data.size + alloc->key.size;

                if(android::base::GetBoolProperty(CONFIG_SAJC_DOUBLE_ALLOC,
                                                  CONFIG_SAJC_DOUBLE_ALLOC_DEFAULT) == true) {
                        alloc->total_size *= 2;
                        SGR_LOGV("Doubling DCC buffer alloc: total_size after 2X = %u",
                                  alloc->total_size);
                } else {
                        /// For details on reason, and calculation of this additional size, please refer to GFXSW-6893
                        constexpr uint32_t dcc_svk_metadata_additional_size = 512;
                        alloc->total_size += dcc_svk_metadata_additional_size;
                }
        }

        constexpr uint32_t num_allocs = 1;
        return num_allocs;
}

} // gralloc
} // samsung
} // android
