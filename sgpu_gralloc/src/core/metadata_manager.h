///
/// @file metadata_manager.h
/// @copyright 2020 Samsung Electronics
///

#pragma once

#include <aidl/android/hardware/graphics/common/Cta861_3.h>
#include <aidl/android/hardware/graphics/common/PlaneLayout.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/common/Smpte2086.h>
#include <cutils/native_handle.h>

#include "hidl_defines.h"

namespace android {
namespace samsung {
namespace gralloc {

class MetadataManager {
public:
        MetadataManager() = default;
        ~MetadataManager() = default;
        MetadataManager(const MetadataManager&) = delete;
        MetadataManager(const MetadataManager&&) = delete;
        MetadataManager& operator=(const MetadataManager&) = delete;
        MetadataManager& operator=(const MetadataManager&&) = delete;

        const std::vector<MetadataTypeDescription>* get_metadata_list() const;
        Error get_buffer_id(const native_handle_t *handle, uint64_t *out) const;
        Error get_name(const native_handle_t *handle, std::string *out) const;
        Error get_width(const native_handle_t *handle, uint64_t *out) const;
        Error get_height(const native_handle_t *handle, uint64_t *out) const;
        Error get_layer_count(const native_handle_t *handle, uint64_t *out) const;
        Error get_pixel_format_requested(const native_handle_t *handle, int32_t *out) const;
        Error get_pixel_format_fourcc(const native_handle_t *handle, uint32_t *out) const;
        Error get_pixel_format_modifier(const native_handle_t *handle, uint64_t *out) const;
        Error get_usage(const native_handle_t *handle, uint64_t *out) const;
        Error get_allocation_size(const native_handle_t *handle, uint64_t *out) const;
        Error get_protected_content(const native_handle_t *handle, uint64_t *out) const;
        Error get_compression(const native_handle_t *handle,
                              aidl::android::hardware::graphics::common::ExtendableType *out) const;
        Error get_interlaced(const native_handle_t *handle,
                              aidl::android::hardware::graphics::common::ExtendableType *out) const;
        Error get_chroma_siting(const native_handle_t *handle,
                              aidl::android::hardware::graphics::common::ExtendableType *out) const;
        Error get_plane_layouts(const native_handle_t *handle,
                                std::vector<aidl::android::hardware::graphics::common::PlaneLayout> *plane_layouts) const;
        Error get_crops(const native_handle_t *handle,
                        std::vector<aidl::android::hardware::graphics::common::Rect> *crop) const;
        Error get_dataspace(const native_handle_t *handle, int32_t *out) const;
        Error get_blend_mode(const native_handle_t *handle, int32_t *out) const;
        Error get_smpte2086(const native_handle_t *handle,
                            std::optional<aidl::android::hardware::graphics::common::Smpte2086> *out) const;
        Error get_cta861_3(const native_handle_t *handle,
                           std::optional<aidl::android::hardware::graphics::common::Cta861_3> *out) const;
        Error get_smpte2094_40(const native_handle_t *handle, std::optional<std::vector<uint8_t>> *out) const;

        Error set_pixel_format_fourcc(native_handle_t *handle, uint32_t in) const;
        Error set_pixel_format_modifier(native_handle_t *handle, uint64_t in) const;
        Error set_allocation_size(native_handle_t *handle, uint64_t in) const;
        Error set_protected_content(native_handle_t *handle, uint64_t in) const;
        Error set_compression(native_handle_t *handle,
                              aidl::android::hardware::graphics::common::ExtendableType in) const;
        Error set_interlaced(native_handle_t *handle,
                              aidl::android::hardware::graphics::common::ExtendableType in) const;
        Error set_chroma_siting(native_handle_t *handle,
                              aidl::android::hardware::graphics::common::ExtendableType in) const;
        Error set_plane_layouts(native_handle_t *handle,
                                std::vector<aidl::android::hardware::graphics::common::PlaneLayout> in) const;
        Error set_crops(native_handle_t *handle,
                        std::vector<aidl::android::hardware::graphics::common::Rect> in) const;
        Error set_dataspace(native_handle_t *handle, int32_t in) const;
        Error set_blend_mode(native_handle_t *handle, int32_t in) const;
        Error set_smpte2086(native_handle_t *handle,
                            std::optional<aidl::android::hardware::graphics::common::Smpte2086> in) const;
        Error set_cta861_3(native_handle_t *handle,
                           std::optional<aidl::android::hardware::graphics::common::Cta861_3> in) const;
        Error set_smpte2094_40(native_handle_t *handle, std::optional<std::vector<uint8_t>> in, bool is_reset) const;
};

} // gralloc
} // samsung
} // android
