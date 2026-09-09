///
/// @file metadata_manager.cpp
/// @copyright 2020 Samsung Electronics
///

#include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>
#include <aidl/android/hardware/graphics/common/StandardMetadataType.h>
#include <cstring>
#include <hidl/HidlSupport.h>
#include <stddef.h>
#include <type_traits>

#include "interface/metadata_gralloc.h"
#include "interface/private_handle.h"
#include "format_info.h"
#include "hidl_defines.h"
#include "metadata_manager.h"
#include "util/util.h"

#define __STDC_FORMAT_MACROS
#include <cinttypes>

#define STANDARD_METADATA_TYPE "android.hardware.graphics.common.StandardMetadataType"
#define STANDARD_PLANE_LAYOUT_COMPONENT_TYPE \
    "android.hardware.graphics.common.PlaneLayoutComponentType"

#undef LOG_TAG
#define LOG_TAG "MetadataManager"

namespace android {
namespace samsung {
namespace gralloc {

constexpr bool is_gettable = true;
constexpr bool is_settable = true;
constexpr bool is_not_settable = false;

using aidl::android::hardware::graphics::common::StandardMetadataType;
const std::vector<MetadataTypeDescription> metadata_list = {
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::BUFFER_ID)},              "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::NAME)},                   "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::WIDTH)},                  "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::HEIGHT)},                 "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::LAYER_COUNT)},            "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::PIXEL_FORMAT_REQUESTED)}, "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::PIXEL_FORMAT_FOURCC)},    "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::PIXEL_FORMAT_MODIFIER)},  "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::USAGE)},                  "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::ALLOCATION_SIZE)},        "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::PROTECTED_CONTENT)},      "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::COMPRESSION)},            "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::INTERLACED)},             "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::CHROMA_SITING)},          "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::PLANE_LAYOUTS)},          "", is_gettable, is_not_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::CROP)},                   "", is_gettable, is_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::DATASPACE)},              "", is_gettable, is_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::BLEND_MODE)},             "", is_gettable, is_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::SMPTE2086)},              "", is_gettable, is_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::CTA861_3)},               "", is_gettable, is_settable },
        {{STANDARD_METADATA_TYPE, static_cast<int64_t>(StandardMetadataType::SMPTE2094_40)},           "", is_gettable, is_settable },
};

const std::vector<MetadataTypeDescription> *MetadataManager::get_metadata_list() const
{
        return &metadata_list;
}

static inline const sgr_metadata* get_and_validate_metadata_const(const native_handle_t *handle,
                                                                  void *metadata_field_placeholder)
{
        SGR_ASSERT_MSG(handle != nullptr, "Should be validated in hidl layer");
        SGR_ASSERT(metadata_field_placeholder != nullptr);

        const sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT_MSG(metadata != nullptr, "metadata address should be set at import");

        return metadata;
}

#define DEFINE_METADATA_GETTER(type, field)                                      \
        Error MetadataManager::get_##field(const native_handle_t *handle, type *out) const \
        {                                                               \
                const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(out)); \
                                                                        \
                *out = metadata->field;                                 \
                return Error::NONE;                                     \
        }

#define STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(type1, field1, base_adjust1,                               \
                                                    type2, field2, base_adjust2, ...)                           \
{                                                                                                               \
        static_assert(std::is_same<decltype(type1::field1), decltype(type2::field2)>::value, __VA_ARGS__);      \
        static_assert((offsetof(type1, field1) - base_adjust1) == (offsetof(type2, field2) - base_adjust2),     \
                       __VA_ARGS__);                                                                            \
}

#define STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(type1, field1,                                                  \
                                                type2, field2, ...)                                             \
{                                                                                                               \
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(type1, field1, 0, type2, field2, 0, ##__VA_ARGS__)         \
}


///
/// @brief Get the the random ID of the buffer
///
/// @param[in]       handle     Buffer handle for getting metadata
/// @param[out]      buffer_id  Random buffer ID
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, buffer_id);

///
/// @brief Get the name passed in by the client at allocation time in the BufferDescriptorInfo
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      name    Name
///
/// @return error
///
DEFINE_METADATA_GETTER(std::string, name);

///
/// @brief Get the number of elements per buffer row requested at allocation time in BufferDescriptorInfo
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      width   Number of elements per buffer row
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, width);

///
/// @brief Get the number of elements per buffer column requested at allocation time in BufferDescriptorInfo
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      height  Number of elements per buffer column
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, height);

///
/// @brief Get the number of layers requested at allocation time in the BufferDescriptorInfo
///
/// @param[in]       handle       Buffer handle for getting metadata
/// @param[out]      layer_count  Number of layers
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, layer_count);

///
/// @brief Get the buffer format requested at allocation time in the BufferDescriptorInfo.
///
/// @param[in]       handle                  Buffer handle for getting metadata
/// @param[out]      pixel_format_requested  Pixel format
///
/// @return error
///
DEFINE_METADATA_GETTER(int32_t, pixel_format_requested);

///
/// @brief Get the fourcc code for the format
///
/// @param[in]       handle               Buffer handle for getting metadata
/// @param[out]      pixel_format_fourcc  Fourcc code
///
/// @return error
///
DEFINE_METADATA_GETTER(uint32_t, pixel_format_fourcc);

///
/// @brief Get the modifier for the format
///
/// Together fourcc and modifier describe the real pixel format.
///
/// @param[in]       handle                 Buffer handle for getting metadata
/// @param[out]      pixel_format_modifier  Modifier
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, pixel_format_modifier);

///
/// @brief Get the usage requested at allocation time in the BufferDescriptorInfo
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      Usage   Usage
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, usage);

///
/// @brief Get the total size in bytes of any memory used by the buffer including its metadata and extra padding
///
/// @param[in]       handle           Buffer handle for getting metadata
/// @param[out]      allocation_size  Allocation size
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, allocation_size);

///
/// @brief Get if a buffer has protected content
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      in      If a buffer has protected content
///
/// @return error
///
DEFINE_METADATA_GETTER(uint64_t, protected_content);

///
/// @brief Get the PlaneLayout(s) of the buffer
///
/// @param[in]       handle         Buffer handle for getting metadata
/// @param[out]      plane_layouts  PlaneLayout data
///
/// @return error
///
Error MetadataManager::get_plane_layouts(const native_handle_t *handle,
                                         std::vector<aidl::android::hardware::graphics::common::PlaneLayout> *plane_layouts) const
{
        const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(plane_layouts));

        using namespace aidl::android::hardware::graphics::common;

        constexpr uint32_t aidl_planelayoutcomponent_base_adjust  = offsetof(PlaneLayoutComponent, type.value);
        constexpr uint32_t sgr_plane_layout_component_base_adjust = offsetof(sgr_plane_layout_component, component_type);
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayoutComponent, type.value,
                                                     aidl_planelayoutcomponent_base_adjust,
                                                     sgr_plane_layout_component, component_type,
                                                     sgr_plane_layout_component_base_adjust,
                                                     "PlaneLayoutComponent::type::value (component_type) mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayoutComponent, offsetInBits,
                                                     aidl_planelayoutcomponent_base_adjust,
                                                     sgr_plane_layout_component, offset_in_bits,
                                                     sgr_plane_layout_component_base_adjust,
                                                     "PlaneLayoutComponent::offsetInBits mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayoutComponent, sizeInBits,
                                                     aidl_planelayoutcomponent_base_adjust,
                                                     sgr_plane_layout_component, size_in_bits,
                                                     sgr_plane_layout_component_base_adjust,
                                                     "PlaneLayoutComponent::sizeInBits mismatch");

        constexpr uint32_t aidl_planelayout_base_adjust = offsetof(PlaneLayout, offsetInBytes);
        constexpr uint32_t sgr_plane_layout_base_adjust = offsetof(sgr_plane_layout, offset_in_bytes);
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, offsetInBytes,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, offset_in_bytes,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::offsetInBytes mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, sampleIncrementInBits,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, sample_increment_in_bits,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::sampleIncrementInBits mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, strideInBytes,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, stride_in_bytes,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::strideInBytes mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, widthInSamples,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, width_in_samples,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::widthInSamples mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, heightInSamples,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, height_in_samples,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::heightInSamples mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, totalSizeInBytes,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, total_size_in_bytes,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::totalSizeInBytes mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, horizontalSubsampling,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, horizontal_subsampling,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::horizontalSubsampling mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_BASE_MSG(PlaneLayout, verticalSubsampling,
                                                     aidl_planelayout_base_adjust,
                                                     sgr_plane_layout, vertical_subsampling,
                                                     sgr_plane_layout_base_adjust,
                                                     "PlaneLayout::verticalSubsampling mismatch");

        // We have to calculate plane addresses for origin image planes only, if buffer has DOWNSCALE BufferUsage
        uint32_t num_planes = is_any_bitmask_set_64(metadata->usage,
                                                    static_cast<uint64_t>(BufferUsage::PRIVATE_DOWNSCALE)) ?
                              metadata->num_plane_layouts/2 : metadata->num_plane_layouts;

        plane_layouts->resize(num_planes);
        for (uint32_t i = 0; i < num_planes; i++) {
                PlaneLayout *plane_layout = &(*plane_layouts)[i];
                plane_layout->components.resize(metadata->plane_layouts[i].num_components);

                for (uint32_t j = 0; j < metadata->plane_layouts[i].num_components; j++) {
                        plane_layout->components[j].type.name = STANDARD_PLANE_LAYOUT_COMPONENT_TYPE;

                        memcpy(&plane_layout->components[j].type.value,
                               &metadata->plane_layouts[i].components[j].component_type,
                               sizeof(sgr_plane_layout_component));
                }

                constexpr size_t copy_size = sizeof(int64_t) + offsetof(PlaneLayout, verticalSubsampling)
                                              - offsetof(PlaneLayout, offsetInBytes);
                memcpy(&plane_layout->offsetInBytes,
                       &metadata->plane_layouts[i].offset_in_bytes,
                       copy_size);

                if (metadata->num_allocs > 1) {
                        const private_handle_t *hnd = reinterpret_cast<const private_handle_t *>(handle);
                        uint32_t alloc_index = get_alloc_index(static_cast<PixelFormat>(metadata->alloc_format), i);
                        plane_layout->offsetInBytes += static_cast<int64_t>(hnd->alloc_bases[alloc_index] - hnd->alloc_bases[0]);
                }
        }

        /// @todo GFXSW-5761 - Fix YUV formats that have 16 bit components
        /// Might apply to PixelFormat::Y16 as well
        if ((metadata->pixel_format_requested == static_cast<int32_t>(PixelFormat::YCBCR_P010)) ||
            (metadata->pixel_format_requested == static_cast<int32_t>(PixelFormat::PRIVATE_YCBCR_P010_M)) ||
            (metadata->pixel_format_requested == static_cast<int32_t>(PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40)) ||
            (metadata->pixel_format_requested == static_cast<int32_t>(PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60))) {
                if (is_system_api_level_gt_s()) {
                        (*plane_layouts)[0].components[0].offsetInBits = 6;
                        (*plane_layouts)[0].components[0].sizeInBits = 10;

                        (*plane_layouts)[1].components[0].offsetInBits = 6;
                        (*plane_layouts)[1].components[0].sizeInBits = 10;
                        (*plane_layouts)[1].components[1].offsetInBits = 22;
                        (*plane_layouts)[1].components[1].sizeInBits = 10;
                } else {
                        (*plane_layouts)[0].components[0].sizeInBits = 8;
                        (*plane_layouts)[0].sampleIncrementInBits    = 8;
                        (*plane_layouts)[1].components[0].sizeInBits   = 8;
                        (*plane_layouts)[1].components[1].offsetInBits = 8;
                        (*plane_layouts)[1].components[1].sizeInBits   = 8;
                        (*plane_layouts)[1].sampleIncrementInBits      = 16;
                }
        }

        return Error::NONE;
}

///
/// @brief Get crop of the buffer
///
/// Some buffer producers require extra padding to their output buffer; therefore the
/// physical size of the native buffer will be larger than its logical size. The crop rectangle(s) determine
/// the offset and logical size of the buffer that should be read by consumers.
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      crop    Crop data
///
/// @return error
///
Error MetadataManager::get_crops(const native_handle_t *handle,
                                 std::vector<aidl::android::hardware::graphics::common::Rect> *crop) const
{
        const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(crop));

        using namespace aidl::android::hardware::graphics;

        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(common::Rect, left, sgr_rect, left, "Rect::left mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(common::Rect, top, sgr_rect, top,  "Rect::top mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(common::Rect, right, sgr_rect, right, "Rect::right mismatch");
        STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(common::Rect, bottom, sgr_rect, bottom, "Rect::bottom mismatch");
        SGR_ASSERT(metadata->num_crops > 0);

        crop->resize(metadata->num_crops);
        memcpy(crop->data(), metadata->crops, sizeof(common::Rect) * metadata->num_crops);

        return Error::NONE;
}

///
/// @brief Get the dataspace of the buffer
///
/// @param[in]       handle     Buffer handle for getting metadata
/// @param[out]      dataspace  Dataspace
///
/// @return error
///
DEFINE_METADATA_GETTER(int32_t, dataspace);

///
/// @brief Get the blend mode
///
/// @param[in]       handle      Buffer handle for getting metadata
/// @param[out]      blend_mode  Blend mode
///
/// @return error
///
DEFINE_METADATA_GETTER(int32_t, blend_mode);

///
/// @brief Get static HDR metadata specified by SMPTE ST 2086
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      out     Static HDR metadata specified by SMPTE ST 2086
///
/// @return error
///
Error MetadataManager::get_smpte2086(const native_handle_t *handle,
                                    std::optional<aidl::android::hardware::graphics::common::Smpte2086> *out) const
{
        const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(out));

        if (metadata->is_smpte2086_set) {
                using aidl::android::hardware::graphics::common::Smpte2086;

                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, primaryRed.x, sgr_smpte2086, primary_red.x,
                                                        "Smpte2086::PrimaryRed::x mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, primaryRed.y, sgr_smpte2086, primary_red.y,
                                                        "Smpte2086::PrimaryRed::y mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, primaryGreen.x, sgr_smpte2086, primary_green.x,
                                                        "Smpte2086::PrimaryGreen::x mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, primaryGreen.y, sgr_smpte2086, primary_green.y,
                                                        "Smpte2086::PrimaryGreen::y mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, primaryBlue.x, sgr_smpte2086, primary_blue.x,
                                                        "Smpte2086::PrimaryBlue::x mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, primaryBlue.y, sgr_smpte2086, primary_blue.y,
                                                        "Smpte2086::PrimaryBlue::y mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, whitePoint.x, sgr_smpte2086, white_point.x,
                                                        "Smpte2086::WhitePoint::x mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Smpte2086, whitePoint.y, sgr_smpte2086, white_point.y,
                                                        "Smpte2086::WhitePoint::y mismatch");

                *out = *reinterpret_cast<const Smpte2086*>(&metadata->smpte2086);
        } else {
                out->reset();
        }

        return Error::NONE;
}

///
/// @brief Get static HDR metadata specified by CTA 861.3
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      out     Static HDR metadaa specified by CTA 861.3
///
/// @return error
///
Error MetadataManager::get_cta861_3(const native_handle_t *handle,
                                    std::optional<aidl::android::hardware::graphics::common::Cta861_3> *out) const
{
        const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(out));

        if (metadata->is_cta861_3_set) {
                using aidl::android::hardware::graphics::common::Cta861_3;

                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Cta861_3, maxContentLightLevel,
                                                        sgr_cta861_3, max_content_light_level,
                                                        "Cta861_3::maxContentLightLevel mismatch");
                STATIC_ASSERT_CHECK_TYPE_AND_OFFSET_MSG(Cta861_3, maxFrameAverageLightLevel,
                                                        sgr_cta861_3, max_frame_average_light_level,
                                                        "Cta861_3::maxFrameAverageLightLevel mismatch");

                *out = *reinterpret_cast<const Cta861_3*>(&metadata->cta861_3);
        } else {
                out->reset();
        }

        return Error::NONE;
}

///
/// @brief Get dynamic HDR metadata specified by SMPTE ST 2094-40:2016
///
/// @param[in]       handle  Buffer handle for getting metadata
/// @param[out]      in      Dynamic HDR metadata specified by SMPTE ST 2094-40:2016
///
/// @return error
///
Error MetadataManager::get_smpte2094_40(const native_handle_t *handle, std::optional<std::vector<uint8_t>> *out) const
{
        const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(out));

        if (metadata->length_smpte2094_40 > 0) {
                std::vector<uint8_t> smpte2094_40(metadata->length_smpte2094_40);
                memcpy(smpte2094_40.data(), &metadata->smpte2094_40, metadata->length_smpte2094_40);
                *out = smpte2094_40;
        } else {
                out->reset();
        }

        return Error::NONE;
}

Error MetadataManager::get_stride(const native_handle_t *handle, uint32_t* out) const
{
    const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(out));

    PixelFormat format = static_cast<PixelFormat>(metadata->alloc_format);
    if (format == PixelFormat::RAW10 || format == PixelFormat::RAW12) {
        *out = metadata->plane_layouts[0].stride_in_bytes;
    } else if (metadata->plane_layouts[0].sample_increment_in_bits == 0) {
        *out = 0;
    } else {
        *out = metadata->plane_layouts[0].width_in_samples;
    }

    return Error::NONE;
};

static inline sgr_metadata* get_and_validate_metadata(native_handle_t *handle)
{
        SGR_ASSERT_MSG(handle != nullptr, "Should be validated in hidl layer");

        sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT_MSG(metadata != nullptr, "metadata address should be set at import");

        return metadata;
}

#define DEFINE_METADATA_SETTER(type, field)                                        \
        Error MetadataManager::set_##field(native_handle_t *handle, type in) const \
        {                                                                          \
                sgr_metadata *metadata = get_and_validate_metadata(handle);        \
                metadata->field = in;                                              \
                return Error::NONE;                                                \
        }

///
/// @brief Set the fourcc code for the format
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Fourcc code
///
/// @return error
///
DEFINE_METADATA_SETTER(uint32_t, pixel_format_fourcc);

///
/// @brief Set the modifier for the format
///
/// Together fourcc and modifier describe the real pixel format.
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Modifier
///
/// @return error
///
DEFINE_METADATA_SETTER(uint64_t, pixel_format_modifier);

///
/// @brief Set the total size in bytes of any memory used by the buffer including its metadata and extra padding
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Allocation size
///
/// @return error
///
DEFINE_METADATA_SETTER(uint64_t, allocation_size);

///
/// @brief Set if a buffer has protected content
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      If a buffer has protected content
///
/// @return error
///
DEFINE_METADATA_SETTER(uint64_t, protected_content);

///
/// @brief Set the PlaneLayout(s) of the buffer
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      PlaneLayout data
///
/// @return error
///
Error MetadataManager::set_plane_layouts(native_handle_t *handle,
                                         std::vector<aidl::android::hardware::graphics::common::PlaneLayout> in) const
{
        sgr_metadata *metadata = get_and_validate_metadata(handle);

        using namespace aidl::android::hardware::graphics::common;

        metadata->num_plane_layouts = in.size();
        for (uint32_t i = 0; i < metadata->num_plane_layouts; i++) {
                PlaneLayout *plane_layout = &in[i];
                metadata->plane_layouts[i].num_components = plane_layout->components.size();

                for (uint32_t j = 0; j < metadata->plane_layouts[i].num_components; j++) {
                        memcpy(&metadata->plane_layouts[i].components[j],
                               &plane_layout->components[j].type.value,
                               sizeof(sgr_plane_layout_component));
                }

                constexpr size_t copy_size = sizeof(int64_t) + offsetof(PlaneLayout, verticalSubsampling)
                                              - offsetof(PlaneLayout, offsetInBytes);
                memcpy(&metadata->plane_layouts[i].offset_in_bytes, &plane_layout->offsetInBytes, copy_size);
        }

        return Error::NONE;
}

///
/// @brief Set crop of the buffer
///
/// Some buffer producers require extra padding to their output buffer; therefore the
/// physical size of the native buffer will be larger than its logical size. The crop rectangle(s) determine
/// the offset and logical size of the buffer that should be read by consumers.
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Crop data
///
/// @return error
///
Error MetadataManager::set_crops(native_handle_t *handle,
                                 std::vector<aidl::android::hardware::graphics::common::Rect> in) const
{
        sgr_metadata *metadata = get_and_validate_metadata(handle);

        using namespace aidl::android::hardware::graphics;

        metadata->num_crops = in.size();
        memcpy(metadata->crops, in.data(), sizeof(common::Rect) * metadata->num_crops);

        return Error::NONE;
}

///
/// @brief Set the dataspace of the buffer
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Dataspace
///
/// @return error
///
DEFINE_METADATA_SETTER(int32_t, dataspace);

///
/// @brief Set the blend mode
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Blend mode
///
/// @return error
///
DEFINE_METADATA_SETTER(int32_t, blend_mode);

///
/// @brief Set static HDR metadata specified by SMPTE ST 2086
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Static HDR metadata specified by SMPTE ST 2086
///
/// @return error
///
Error MetadataManager::set_smpte2086(native_handle_t *handle,
                                    std::optional<aidl::android::hardware::graphics::common::Smpte2086> in) const
{
        sgr_metadata *metadata = get_and_validate_metadata(handle);

        metadata->is_smpte2086_set = in.has_value();
        if (metadata->is_smpte2086_set) {
                metadata->smpte2086 = *reinterpret_cast<const sgr_smpte2086*>(&in.value());

        }

        return Error::NONE;
}

///
/// @brief Set static HDR metadata specified by CTA 861.3
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Static HDR metadata specified by CTA 861.3
///
/// @return error
///
Error MetadataManager::set_cta861_3(native_handle_t *handle,
                                    std::optional<aidl::android::hardware::graphics::common::Cta861_3> in) const
{
        sgr_metadata *metadata = get_and_validate_metadata(handle);

        metadata->is_cta861_3_set = in.has_value();
        if (metadata->is_cta861_3_set) {
                metadata->cta861_3 = *reinterpret_cast<const sgr_cta861_3*>(&in.value());
        }

        return Error::NONE;
}

///
/// @brief Set dynamic HDR metadata specified by SMPTE ST 2094-40:2016
///
/// @param[in/out]  handle  Buffer handle for setting metadata
/// @param[in]      in      Dynamic HDR metadata specified by SMPTE ST 2094-40:2016
///
/// @return error
///
Error MetadataManager::set_smpte2094_40(native_handle_t *handle, std::optional<std::vector<uint8_t>> in, bool is_reset) const
{
        sgr_metadata *metadata = get_and_validate_metadata(handle);
        if (is_reset) {
                metadata->length_smpte2094_40 = 0;
                memset(metadata->smpte2094_40, 0, sizeof(uint8_t) * SGR_MAX_LENGTH_SMPTE2094_40);
                return Error::NONE;
        }

        if (in.has_value() == false || in->size() == 0) {
                // SGR_LOGE("empty SMPTE2094_40 data");
                return Error::BAD_VALUE;
        }

        const size_t size = in.has_value() ? in->size() : 0;
        if (size >= SGR_MAX_LENGTH_SMPTE2094_40) {
                // SGR_LOGE("SMPTE2094_40 metadata size is to large to fit in gralloc metadata region(size: %zu)", size);
                return Error::BAD_VALUE;
        }

        metadata->length_smpte2094_40 = size;
        if (metadata->length_smpte2094_40 > 0) {
                memcpy(metadata->smpte2094_40, in->data(), metadata->length_smpte2094_40);
        }

        return Error::NONE;
}


#define DEFINE_EXTENDABLETYPE_METADATA_GETTER(field)                                                             \
        Error MetadataManager::get_##field(const native_handle_t *handle,                                        \
                                           aidl::android::hardware::graphics::common::ExtendableType *out) const \
        {                                                                                                        \
                const sgr_metadata *metadata = get_and_validate_metadata_const(handle, reinterpret_cast<void *>(out)); \
                out->value = metadata->field;                                                                    \
                return Error::NONE;                                                                              \
        }

#define DEFINE_EXTENDABLETYPE_METADATA_SETTER(field)                                                           \
        Error MetadataManager::set_##field(native_handle_t *handle,                                            \
                                           aidl::android::hardware::graphics::common::ExtendableType in) const \
        {                                                                                                      \
                sgr_metadata *metadata = get_and_validate_metadata(handle);                                    \
                metadata->field = in.value;                                                                    \
                return Error::NONE;                                                                            \
        }

#define DEFINE_EXTENDABLETYPE_METADATA_GETTER_AND_SETTER(field)  \
        DEFINE_EXTENDABLETYPE_METADATA_GETTER(field);            \
        DEFINE_EXTENDABLETYPE_METADATA_SETTER(field);            \

DEFINE_EXTENDABLETYPE_METADATA_GETTER_AND_SETTER(compression);
DEFINE_EXTENDABLETYPE_METADATA_GETTER_AND_SETTER(interlaced);
DEFINE_EXTENDABLETYPE_METADATA_GETTER_AND_SETTER(chroma_siting);

} // gralloc
} // samsung
} // android
