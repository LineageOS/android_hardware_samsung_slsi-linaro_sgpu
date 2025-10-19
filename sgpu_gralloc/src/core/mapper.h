///
/// @file mapper.h
/// @copyright 2020 Samsung Electronics
///

#pragma once

#include <hidl/HidlSupport.h>
#include <mutex>
#include <unordered_set>

#include "metadata_manager.h"
#include "hidl_defines.h"

#ifndef LOG_TAG
#define LOG_TAG "Mapper3.0"
#endif

namespace android {
namespace samsung {
namespace gralloc {

class MemoryManager;

class Mapper {
public:
        Mapper();
        ~Mapper() = default;
        Mapper(const Mapper&) = delete;
        Mapper(const Mapper&&) = delete;
        Mapper& operator=(const Mapper&) = delete;
        Mapper& operator=(const Mapper&&) = delete;

        Error create_descriptor   (const BufferDescriptorInfo &descriptor_info, BufferDescriptor *descriptor);
        Error import_buffer       (const native_handle_t *raw_handle, native_handle_t **imported_handle);
        Error free_buffer         (native_handle_t *handle);
        Error validate_buffer_size(const native_handle_t *handle, const BufferDescriptorInfo &descriptor_info,
                                   uint32_t stride);
        Error get_transport_size  (const native_handle_t *handle, int *num_fds, int *num_ints);
        Error lock                (native_handle_t *handle, uint64_t cpu_usage, const Rect &access_region,
                                   int acquire_fence, void **data, int32_t *bytes_per_pixel,
                                   int32_t *bytes_per_stride);
        Error lock_ycbcr          (native_handle_t *handle, uint64_t cpu_usage, const Rect &access_region,
                                   int release_fence, YCbCrLayout *layout);
        Error unlock              (native_handle_t *handle, int *fence);
        Error flush_locked_buffer (native_handle_t *handle, int *release_fence);
        Error reread_locked_buffer (native_handle_t *handle);
        bool  is_supported        (const BufferDescriptorInfo &descriptor_info);
        Error get_reserved_region (native_handle_t *handle, void **reserved_region, uint64_t *reserved_region_size);
        bool  is_registered_handle(const native_handle_t *handle);

        const MetadataManager *get_metadata_manager() const { return &m_metadata_manager; }
        static std::unordered_set<const native_handle_t*> &get_imported_handles() { return m_imported_handles; }
        static std::mutex &get_imported_handles_lock()   { return m_imported_handles_lock; }

private:
        bool map_buffer(native_handle_t *handle);
        bool unmap_buffer(native_handle_t *handle);
        bool validate_lock_input_parameters(const native_handle_t *handle, const int left,
                                            const int top, const int width, const int height);

        MemoryManager           *m_manager;
        MetadataManager         m_metadata_manager;
        static std::mutex       m_imported_handles_lock;
        static std::mutex       m_imported_handles_access_lock;
        static std::unordered_set<const native_handle_t*> m_imported_handles;
};

} // gralloc
} // samsung
} // android
