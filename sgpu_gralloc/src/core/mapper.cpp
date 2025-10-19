///
/// @file mapper.cpp
/// @copyright 2020 Samsung Electronics
///

#include <cutils/native_handle.h>
#include <sync/sync.h>
#include <sys/mman.h>
#include <unordered_set>

#include "debug_helper.h"
#include "exynos_ion_memory_manager.h"
#include "format_info.h"
#include "interface/metadata_gralloc.h"
#include "interface/private_handle.h"
#include "mapper.h"
#include "memory_manager.h"
#include "private_handle_helper.h"
#include "util/util.h"

#define __STDC_FORMAT_MACROS
#include <cinttypes>

namespace android {
namespace samsung {
namespace gralloc {

static ExynosIonMemoryManager g_memory_manager;

std::unordered_set<const native_handle_t*> Mapper::m_imported_handles;
std::mutex Mapper::m_imported_handles_lock;
std::mutex Mapper::m_imported_handles_access_lock;

Mapper::Mapper()
        : m_manager(reinterpret_cast<MemoryManager*>(&g_memory_manager))
{
}

static inline bool is_multiple_of(uint32_t value, uint32_t power_of_two)
{
        SGR_ASSERT((power_of_two != 0) && ((power_of_two & (power_of_two - 1)) == 0));
        return (is_any_bitmask_set(value, (power_of_two - 1)) == false);
}

static inline bool is_supported_format(PixelFormat format)
{
        bool ret = true;
        switch (format) {
                case PixelFormat::RAW_OPAQUE:
                case PixelFormat::DEPTH_16:
                case PixelFormat::DEPTH_24:
                case PixelFormat::DEPTH_24_STENCIL_8:
                case PixelFormat::DEPTH_32F:
                case PixelFormat::DEPTH_32F_STENCIL_8:
                case PixelFormat::STENCIL_8:
                        SGR_LOGD("Unsupported format = %d", static_cast<uint32_t>(format));
                        ret = false;
                        break;
                default:
                        break;
        }

        return ret;
}

static inline bool is_supported_usage(PixelFormat format, uint64_t usage)
{
        bool ret = true;
        if (PixelFormat::IMPLEMENTATION_DEFINED == format) {
                constexpr uint64_t mask = (static_cast<uint64_t>(BufferUsage::CPU_READ_MASK) |
                                           static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK));

                if (is_any_bitmask_set_64(usage, mask)) {
                        SGR_LOGD("IMPLEMENTATION_DEFINED must not have CPU usage, [0x%" PRIx64 "]",
                                     usage);
                        ///@todo GFXSW-4937 - uncomment the following once camera app is fixed
                        //ret = false;
                }
        }

        if ((format == PixelFormat::RGB_888) ||
            (format == PixelFormat::R_8)) {
                constexpr uint64_t gpu_mask = static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                                              static_cast<uint64_t>(BufferUsage::GPU_RENDER_TARGET) |
                                              static_cast<uint64_t>(BufferUsage::GPU_DATA_BUFFER);
                if (is_any_bitmask_set_64(usage, gpu_mask)) {
                        SGR_LOGD("GPU do not support native format(%u)", static_cast<int32_t>(format));
                        return false;
                }
        }

        return ret;
}

static inline bool is_supported_extent(PixelFormat format, uint32_t width, uint32_t height, uint32_t layer_count)
{
        if ((width == 0) || (height == 0) || (layer_count == 0)) {
                return false;
        }

        if (PixelFormat::RAW16 == format) {
                if ((is_multiple_of(width, 2) == false) ||
                    (is_multiple_of(height, 2) == false)) {
                        SGR_LOGE("RAW16 must have 2x width (%u) and 2x height (%u)",
                                     width, height);
                        return false;
                }
        } else if (PixelFormat::BLOB == format) {
                if (height != 1) {
                        SGR_LOGE("BLOB must have height = 1, (%u)", height);
                        return false;
                }
        } else if ((PixelFormat::RAW10 == format) || (PixelFormat::RAW12 == format)) {
                if ((is_multiple_of(width, 4) == false) ||
                    (is_multiple_of(height, 2) == false)) {
                        SGR_LOGE("RAW10 and RAW12 (%x) must have 4x width (%u) and 2x height (%u)",
                                     static_cast<uint32_t>(format), width, height);
                        return false;
                }
        } else if ((PixelFormat::Y8 == format) || (PixelFormat::Y16 == format) || (PixelFormat::YV12 == format)) {
                if ((is_multiple_of(width, 2) == false) ||
                    (is_multiple_of(height, 2) == false)) {
                        SGR_LOGE("YUV (%x) must have 2x width (%u) and 2x height (%u)",
                                     static_cast<uint32_t>(format), width, height);
                        return false;
                }
        }

        return true;
}

static inline bool is_pixel_size_byte_aligned(PixelFormat format)
{
        bool ret = false;
        switch (format) {
        case PixelFormat::RAW10:
        case PixelFormat::RAW12:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC:
        case PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60:
        case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60:
        case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80:
                ret = false;
                break;
        default:
                ret = true;
                break;
        }

        return ret;
}

///
/// @brief Checks if the native handle is imported and valid
///
/// @param[in] handle            The buffer handle to validate
///
/// @return bool True if buffer is valid, imported by process and has a non null metadata
///
static bool is_valid_imported_handle(const native_handle_t *handle)
{
        bool ret = true;
        if (validate_native_handle(handle) == false) {
                SGR_LOGE("Invalid native_handle_t");
                ret = false;
        } else if (is_imported(reinterpret_cast<const private_handle_t *>(handle)) == false) {
                SGR_LOGE("native_handle_t = %p is not imported", handle);
                ret = false;
        }

        return ret;
}

static bool validate_buffer_descriptor_info(const BufferDescriptorInfo &info)
{
        ///@todo GFXSW-4430 - Validate user input for invalid PixelFormat value
        return (is_supported_format(info.format) &&
                is_supported_usage(info.format, info.usage) &&
                is_supported_extent(info.format, info.width, info.height, info.layerCount));
}

static inline void assign_YCbCrLayout(const struct sgr_plane_layout *plane_layout, uint64_t alloc_base, YCbCrLayout *layout)
{
        for(uint i = 0; i < plane_layout->num_components; i++) {
                const struct sgr_plane_layout_component *component = &(plane_layout->components[i]);

                if (component->component_type == SGR_PLANE_LAYOUT_COMPONENT_TYPE::Y) {
                        layout->yStride = plane_layout->stride_in_bytes;
                        layout->y = reinterpret_cast<void*>(alloc_base +
                                                       plane_layout->offset_in_bytes +
                                                       convert_bits_to_bytes(component->offset_in_bits));
                }
                else if (component->component_type == SGR_PLANE_LAYOUT_COMPONENT_TYPE::CB) {
                        layout->cStride = plane_layout->stride_in_bytes;
                        layout->chromaStep = convert_bits_to_bytes(plane_layout->sample_increment_in_bits);
                        layout->cb = reinterpret_cast<void*>(alloc_base +
                                                        plane_layout->offset_in_bytes +
                                                        convert_bits_to_bytes(component->offset_in_bits));
                }
                else if (component->component_type == SGR_PLANE_LAYOUT_COMPONENT_TYPE::CR) {
                        layout->cStride = plane_layout->stride_in_bytes;
                        layout->chromaStep = convert_bits_to_bytes(plane_layout->sample_increment_in_bits);
                        layout->cr = reinterpret_cast<void*>(alloc_base +
                                                        plane_layout->offset_in_bytes +
                                                        convert_bits_to_bytes(component->offset_in_bits));
                }
                else {
                        SGR_ASSERT_MSG(false, "Non-YCbCr Component in plane_layout");
                }
        }
}

///
/// @brief Creates a buffer descriptor using descriptor attributes
///
/// @param[in]  descriptor_info  Specifies the attributes of the descriptor
/// @param[out] descriptor Newly created opaque buffer descriptor.
///
/// @return error
///
Error Mapper::create_descriptor(const BufferDescriptorInfo &descriptor_info, BufferDescriptor *descriptor)
{
        dump_descriptor_info(descriptor_info);

        if (validate_buffer_descriptor_info(descriptor_info) == false){
                return Error::BAD_VALUE;
        }

        // Populate hidl_cb_params->descriptor with descriptor_info,
        // Modify assignments in /src/gralloc*/converter.h: convert_to_buffer_descriptor_info()
        // to control what is assigned to BufferDescriptor
        descriptor->length_name  = descriptor_info.name.size();
        if (descriptor->length_name >= SGR_MAX_LENGTH_NAME) {
            descriptor->length_name = SGR_MAX_LENGTH_NAME -1 ;
        }
        strncpy(descriptor->name, descriptor_info.name.c_str(), descriptor->length_name);
        descriptor->width        = descriptor_info.width;
        descriptor->height       = descriptor_info.height;
        descriptor->layerCount   = descriptor_info.layerCount;
        descriptor->format       = descriptor_info.format;
        descriptor->usage        = descriptor_info.usage;
        descriptor->reservedSize = descriptor_info.reservedSize;

        constexpr uint64_t video_mask = (static_cast<uint64_t>(BufferUsage::VIDEO_ENCODER) |
                                         static_cast<uint64_t>(BufferUsage::VIDEO_DECODER));
        if (is_any_bitmask_set(descriptor_info.usage, video_mask)) {
                descriptor->usage |= static_cast<uint64_t>(BufferUsage::PRIVATE_VIDEO_PRIVATE_DATA);
        }

        dump_descriptor(*descriptor);

        return Error::NONE;
}

///
/// @brief Imports a raw buffer handle to create an imported buffer handle for use with
///        the rest of the mapper or with other in-process libraries.
///
/// @param[in]  raw_handle Buffer handle to import.
/// @param[out] imported_handle Imported buffer handle
///
/// @return error
///
Error Mapper::import_buffer(const native_handle_t *raw_handle, native_handle_t **imported_handle)
{
        SGR_LOGV("(raw_handle = %p, imported_handle = %p)", raw_handle, imported_handle);

        *imported_handle = nullptr;
        SGR_ASSERT_MSG(raw_handle != nullptr, "Invalid buffer handle %p to import", raw_handle);
        if (validate_native_handle(raw_handle) == false) {
                SGR_LOGE("Invalid native handle");
                return Error::BAD_BUFFER;
        }

        native_handle_t *native_handle = native_handle_clone(raw_handle);
        if (native_handle == nullptr) {
                SGR_LOGE("Failed to clone buffer handle");
                return Error::NO_RESOURCES;
        }

        if (map_buffer(native_handle) == false) {
                native_handle_close(native_handle);
                native_handle_delete(native_handle);
                return Error::NO_RESOURCES;
        }

        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(native_handle);
        dump_private_handle(hnd);

#if defined(ION_MEMTRACK_ENABLED)
        const sgr_metadata *metadata = sgr_get_metadata(native_handle);
        SGR_ASSERT(metadata != nullptr);

        int idx = 0;
        for (idx = 0; idx < metadata->num_allocs; idx++) {
                if (hnd->fds[idx] >= 0) {
                        if (!m_manager->import_handle(hnd->fds[idx], &hnd->handles[idx])) {
                                SGR_LOGE("Failed to import ion handle for fd = %d", hnd->fds[idx]);
                                break;
                        }
                }
        }

        if (idx != metadata->num_allocs) {
                for (; idx >= 0; idx--) {
                        if (!m_manager->free_handle(hnd->fds[idx], &hnd->handles[idx])) {
                                SGR_LOGE("Failed to free ion handle = %d", hnd->handles[idx]);
                        }
                }
                return Error::NO_RESOURCES;
        }
#endif

        {
                std::lock_guard<std::mutex> lock(m_imported_handles_lock);
                if (m_imported_handles.insert(native_handle).second == false) {
                        SGR_LOGE("Failed to insert imported handle = %p in m_imported_handles",
                                        native_handle);
#if defined(ION_MEMTRACK_ENABLED)
                        for (idx = 0; idx < metadata->num_allocs; idx++) {
                                if (hnd->handles[idx] >= 0) {
                                        if (!m_manager->free_handle(hnd->fds[idx], &hnd->handles[idx])) {
                                                SGR_LOGE("Failed to free ion handle = %d", hnd->handles[idx]);
                                        }
                                }
                        }
#endif

                        native_handle_close(native_handle);
                        native_handle_delete(native_handle);

                        return Error::NO_RESOURCES;
                }
        }

        *imported_handle = native_handle;

        return Error::NONE;
}

///
/// Frees a buffer handle. Buffer handles returned by importBuffer() must be
/// freed with this function when no longer needed
///
/// @param handle [in] Imported buffer handle
///
/// @return Error::BAD_BUFFER if the buffer is invalid / failed to free the buffer
///         Error::NONE on successful free
///
Error Mapper::free_buffer(native_handle_t *handle)
{
        SGR_LOGV("(native_handle_t = %p)", handle);

        SGR_ASSERT_MSG(handle != nullptr, "Invalid buffer handle %p to freeBuffer", handle);
        Error ret = Error::NONE;

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

#if defined(ION_MEMTRACK_ENABLED)
        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        const sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT(metadata != nullptr);

        for (int idx = 0; idx < metadata->num_allocs; idx++) {
                if (hnd->handles[idx] >= 0) {
                        if (!m_manager->free_handle(hnd->fds[idx], &hnd->handles[idx])) {
                                SGR_LOGE("Failed to free ion handle = %d", hnd->handles[idx]);
                                ret = Error::NO_RESOURCES;
                        }
                }
        }
#endif

        {
                std::lock_guard<std::mutex> lock(m_imported_handles_lock);
                handle = m_imported_handles.erase(handle) == 1 ? handle : nullptr;
        }

        SGR_ASSERT_MSG(handle != nullptr, "Failed to erase imported handle %p", handle);
        if (handle == nullptr) {
                SGR_LOGE("Failed to erase imported handle %p", handle);
                return Error::BAD_BUFFER;
        }

        SGR_ASSERT(reinterpret_cast<private_handle_t *>(handle)->metadata_base != 0);
        if (!unmap_buffer(handle)) {
                ret = Error::BAD_BUFFER;
        }

        native_handle_close(handle);
        native_handle_delete(handle);

        return ret;
}

///
/// Validates the buffer against specified descriptor attributes
///
/// @param[i] handle          Buffer which needs to be validated.
/// @param[i] descriptor_info Required attributes of the buffer
/// @param[i] stride          Buffer stride returned by IAllocator::allocate
///
/// @return Error::NONE upon success. Otherwise,
///         Error::BAD_BUFFER upon bad buffer input
///         Error::BAD_VALUE when any of the specified attributes are invalid
///
Error Mapper::validate_buffer_size(const native_handle_t *handle, const BufferDescriptorInfo &descriptor_info, uint32_t stride)
{
        SGR_LOGV("(native_handle_t = %p, stride = %d)", handle, stride);
        SGR_LOGV("%s", descriptor_info.dump().c_str());

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

        const sgr_metadata *metadata = sgr_get_metadata(handle);

        if ((metadata->alloc_width < descriptor_info.width) || (metadata->alloc_height < descriptor_info.height)){
                SGR_LOGE("Failed to validate buffer size. Allocated buffer is smaller than requested");
                return Error::BAD_VALUE;
        }

        if (metadata->plane_layouts[0].sample_increment_in_bits != 0) {
                const uint32_t stride_in_bits = convert_bytes_to_bits(metadata->plane_layouts[0].stride_in_bytes);
                const uint32_t sample_size_in_bits = metadata->plane_layouts[0].sample_increment_in_bits;
                SGR_ASSERT((stride_in_bits % sample_size_in_bits) == 0);
                if (stride_in_bits < (stride * sample_size_in_bits)) {
                        SGR_LOGE("Stride of Allocated buffer is smaller than requested");
                        return Error::BAD_VALUE;
                }
        } else {
                // sample_increment_in_bits can be 0 when the buffer is SAJC or SBWC
                SGR_LOGV("Stride of Allocated buffer is 0, (possible when SAJC or SBWC)");
        }

        return Error::NONE;
}

///
/// Get the transport size of a buffer
///
/// @param[in]  handle  Buffer for computing transport size
/// @param[out] num_fds Number of file descriptors needed for transport
/// @param[out] num_int Number of integers needed for transport
///
/// @return error:   NONE upon success. Otherwise,
///                  BAD_BUFFER for an invalid buffer
///
Error Mapper::get_transport_size(const native_handle_t *handle, int *num_fds, int *num_ints)
{
        // Pulling these values from #defines in /src/interface/private_handle.h
        *num_fds = handle->numFds;
        *num_ints = handle->numInts;

        SGR_LOGV("(native_handle_t = %p, num_fds = %d, num_ints = %d)", handle, *num_fds, *num_ints);
        return Error::NONE;
}

///
/// Locks the given buffer for the specified CPU usage.
///
/// @param[in]  handle           Buffer to lock
/// @param[in]  cpu_usage        CPU usage flags to request
/// @param[in]  access_region    Portion of the buffer that the client intends to access
/// @param[in]  acquire_fence    fd from the aquire fence object
/// @param[out] data             CPU-accessible pointer to the buffer data
/// @param[out] bytes_per_pixel  Number of bytes per pixel in the buffer
/// @param[out] bytes_per_stride Number of bytes per stride of the buffer
///
/// @return error: BAD_BUFFER if the buffer is invalid or is incompatible with this function.
///                BAD_VALUE if cpuUsage is 0, contains non-CPU usage flags, or is incompatible with the buffer.
///                NO_RESOURCES if the buffer cannot be locked at this time. Locking may succeed at a later time.
///

Error Mapper::lock(native_handle_t *handle, uint64_t cpu_usage, const Rect &access_region,
                   int acquire_fence, void **data, int32_t *bytes_per_pixel, int32_t *bytes_per_stride)
{
        SGR_LOGV("(native_handle_t = %p, cpu_usage = 0x%" PRIx64 ", acquire_fence = %d, data = %p, bytes_per_pixel = %p, bytes_per_stride = %p)",
                        handle, cpu_usage, acquire_fence, data, bytes_per_pixel, bytes_per_stride);
        SGR_LOGV("%s", access_region.dump().c_str());

        SGR_ASSERT(bytes_per_pixel != nullptr);
        SGR_ASSERT(bytes_per_stride != nullptr);

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

        /* Validate input parameters */
        bool ret = validate_lock_input_parameters(
                     handle,
                     access_region.left,
                     access_region.top,
                     access_region.width,
                     access_region.height);
        if (!ret) {
                return Error::BAD_VALUE;
        }

        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        const sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT(metadata != nullptr);
        SGR_ASSERT_MSG(cpu_usage != 0, "Invalid cpu_usage [0x%" PRIx64 "] to lock", cpu_usage);

        if (metadata->protected_content == 1) {
                SGR_LOGE("Locking protected content is not allowed");
                return Error::BAD_BUFFER;
        }

        if (acquire_fence >= 0) {
                int status = sync_wait(acquire_fence, -1);
                if (status < 0 ) {
                        SGR_LOGE("Error encountered while waiting on fence fd");
                        return Error::NO_RESOURCES;
                }
        }

        if(hnd->alloc_bases[0] == 0) {
                SGR_LOGD("buffer lock called, but no base addresses. so map buffer now");
                void *cpu_ptr = nullptr;
                memset(hnd->alloc_bases, 0, sizeof(uint64_t) * SGR_MAX_NUM_ALLOCATIONS);
                for (int idx = 0; idx < metadata->num_allocs; idx++) {
                        SGR_ASSERT_MSG(hnd->fds[idx] >= 0, "Invalid File descriptor to map");
                        cpu_ptr = m_manager->map(0, metadata->allocs[idx].total_size, PROT_READ|PROT_WRITE,
                                        MAP_SHARED, hnd->fds[idx], 0);
                        if (cpu_ptr == nullptr) {
                                break;
                        }
                        hnd->alloc_bases[idx] = reinterpret_cast<uint64_t>(cpu_ptr);
                }

                if (cpu_ptr == nullptr){
                        SGR_LOGE("Error: Failed to map buffer");
                        unmap_buffer(handle);
                        return Error::BAD_BUFFER;
                }
        }

        bool is_non_cacheable =
                (!is_any_bitmask_set_64(cpu_usage, static_cast<uint64_t>(BufferUsage::CPU_READ_MASK))) &&
                (is_any_bitmask_set_64(cpu_usage, static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK)));
        if (is_non_cacheable) {
                *data = reinterpret_cast<void *>(hnd->alloc_bases[0]);
                if (hnd->lock_count == 0)
                        hnd->lock_cpu_usage = cpu_usage;
                hnd->lock_count++;
                return Error::NONE;
        }

        if (hnd->lock_count == 0) {
                int idx = 0;
                for (idx = 0; idx < metadata->num_allocs; idx++) {
                        if (!m_manager->sync_start(hnd->fds[idx], cpu_usage)) {
                                break;
                        }
                }

                if (idx != metadata->num_allocs) {
                        for (; idx >= 0; idx--) {
                                m_manager->sync_end(hnd->fds[idx], cpu_usage);
                        }
                        SGR_LOGE("Failed to sync_start");
                        return Error::NO_RESOURCES;
                }
        }

        *data = reinterpret_cast<void *>(hnd->alloc_bases[0]);

        if (is_pixel_size_byte_aligned(static_cast<PixelFormat>(metadata->pixel_format_requested))) {
                *bytes_per_pixel  = convert_bits_to_bytes(metadata->plane_layouts[0].sample_increment_in_bits);
        } else {
                *bytes_per_pixel  = -1;
        }
        *bytes_per_stride = metadata->plane_layouts[0].stride_in_bytes;

        if (hnd->lock_count == 0)
                hnd->lock_cpu_usage = cpu_usage;
        hnd->lock_count++;

        SGR_LOGV("EXIT (data = %p, bytes_per_pixel = %d, bytes_per_stride = %d)", *data, *bytes_per_pixel, *bytes_per_stride);

        return Error::NONE;
}

///
/// Locks a YCbCr buffer for the specified CPU usage and exports YCbCr structure with
/// cpu accessible data.
///
/// @param[in] handle        Buffer to lock
/// @param[in] cpu_usage     CPU usage flags to request
/// @param[in] access_region Portion of the buffer that the client intends to access
/// @param[in] acquire_fence fd from aquire fence object
/// @param[out] layout       YCbCrLayout struct with cpu accessible data
///
/// @return error: NONE upon success. Otherwise,
///                BAD_BUFFER for an invalid buffer
///                BAD_VALUE for an invalid input parameters
///
Error Mapper::lock_ycbcr(native_handle *handle, uint64_t cpu_usage, const Rect &access_region,
                         int acquire_fence, YCbCrLayout *layout)
{
        SGR_LOGV("(native_handle_t = %p, cpu_usage = 0x%" PRIx64 ", acquire_fence = %d)",
                     handle, cpu_usage, acquire_fence);
        SGR_ASSERT(layout != nullptr);

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

        void *data = nullptr;
        int32_t bytes_per_pixel = 0;
        int32_t bytes_per_stride = 0;

        Error error = Mapper::lock(handle, cpu_usage, access_region, acquire_fence,
                             &data, &bytes_per_pixel, &bytes_per_stride);

        if (error != Error::NONE) {
                return error;
        }

        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        const sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT(metadata != nullptr);

        for(uint i = 0; i < metadata->num_plane_layouts; i++) {
                const struct sgr_plane_layout *plane_layout = &(metadata->plane_layouts[i]);
                uint32_t alloc_index = get_alloc_index(static_cast<PixelFormat>(metadata->alloc_format), i);
                SGR_ASSERT_MSG(hnd->alloc_bases[alloc_index] != 0, "Buffer not locked");

                assign_YCbCrLayout(plane_layout, hnd->alloc_bases[alloc_index], layout);
        }

        SGR_LOGV("EXIT\n%s", layout->dump().c_str());
        return Error::NONE;
}

///
/// Unlocks a buffer to indicate all CPU accesses to the buffer have completed
///
/// @param[in ] handle         Buffer to unlock
/// @param[out] release_fence  Referrs to a sync fence object
///
/// @return error: NONE upon success. Otherwise,
///                BAD_BUFFER for an invalid buffer
///
Error Mapper::unlock(native_handle_t *handle, int *release_fence)
{
        SGR_LOGV("(native_handle_t = %p, release_fence = %d)", handle, *release_fence);

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

        SGR_UNUSED(release_fence);

        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        if (hnd->lock_count == 0) {
                SGR_LOGE("Trying to unlock a buffer that has not been locked");
                return Error::BAD_BUFFER;
        }

        const sgr_metadata *metadata = sgr_get_metadata(handle);
        if(hnd->lock_count > 1) {
                hnd->lock_count--;
                return Error::NONE;
        }

        SGR_ASSERT(metadata != nullptr);
        SGR_ASSERT_MSG(metadata->protected_content == 0, "Locking buffer should not be a protected buffer");

        for (int idx = 0; idx < metadata->num_allocs; idx++) {
                if (hnd->fds[idx] >= 0) {
                        if (!m_manager->sync_end(hnd->fds[idx], hnd->lock_cpu_usage)) {
                                return Error::BAD_BUFFER;
                        }
                }
        }

        hnd->lock_count--;
        if(hnd->lock_count == 0)
                hnd->lock_cpu_usage = 0;

        return Error::NONE;
}

///
/// Flushes the contents of a locked buffer.
///
/// @param[in ] handle         Buffer to flushed
/// @param[out] release_fence  Refers to a sync fence object
///
/// @return error: NONE upon success. Otherwise,
///                BAD_BUFFER for an invalid buffer
///
Error Mapper::flush_locked_buffer(native_handle_t *handle, int *release_fence)
{
        SGR_LOGV("(native_handle_t = %p, release_fence = %d)", handle, *release_fence);

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

        SGR_UNUSED(release_fence);

        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        if (hnd->lock_count == 0) {
                SGR_LOGE("Trying to flush a buffer that has not been locked");
                return Error::BAD_BUFFER;
        }

        const sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT(metadata != nullptr);
        SGR_ASSERT_MSG(metadata->protected_content == 0, "Locking buffer should not be a protected buffer");

        for (int idx = 0; idx < metadata->num_allocs; idx++) {
                if (hnd->fds[idx] >= 0) {
                        if (!m_manager->sync_end(hnd->fds[idx], hnd->lock_cpu_usage)) {
                                return Error::BAD_BUFFER;
                        }
                }
        }

        return Error::NONE;
}

///
/// Reread content of locked buffer
///
/// @param[in] handle Buffer to reread
///
/// @return error: NONE upon success. Otherwise,
///                BAD_BUFFER for an invalid buffer or locked buffer
///                NO_RESOURCES if the buffer cannot be reread at this time
///
Error Mapper::reread_locked_buffer(native_handle_t *handle)
{
        SGR_LOGV("(native_handle_t = %p)", handle);

        if (is_valid_imported_handle(handle) == false) {
                return Error::BAD_BUFFER;
        }

        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        if (hnd->lock_count == 0) {
                SGR_LOGE("Trying to reread a buffer that has not been locked");
                return Error::BAD_BUFFER;
        }

        const sgr_metadata *metadata = sgr_get_metadata(handle);
        SGR_ASSERT(metadata != nullptr);
        SGR_ASSERT_MSG(metadata->protected_content == 0, "Locking buffer should not be a protected buffer");

        uint32_t idx = 0;
        for (idx = 0; idx < metadata->num_allocs ; idx++) {
                if (!m_manager->sync_start(hnd->fds[idx], hnd->lock_cpu_usage)) {
                        break;
                }
        }

        if (idx != metadata->num_allocs) {
                for (; idx >= 0; idx--) {
                        if (!m_manager->sync_end(hnd->fds[idx], hnd->lock_cpu_usage)) {
                                return Error::BAD_BUFFER;
                        }
                }
                SGR_LOGE("Failed to sync_start");
                return Error::NO_RESOURCES;
        }

        return Error::NONE;
}

///
/// Test whether the given BufferDescriptorInfo is allocatable.
///
/// If this function returns true, it means that a buffer with the given
/// description can be allocated on this implementation, unless resource
/// exhaustion occurs. If this function returns false, it means that the
/// allocation of the given description will never succeed.
///
/// @param[in] descriptor_info The description of the buffer
///
/// @return bool Whether the description is supported or not
///
bool Mapper::is_supported(const BufferDescriptorInfo &descriptor_info)
{
        SGR_LOGV("%s", descriptor_info.dump().c_str());
        return validate_buffer_descriptor_info(descriptor_info);
}

///
/// Check whether the given handle is properly imported
///
/// If this function return true, it means the given buffer is known to gralloc mapper.
/// It makes sure that the process has its own user virtual address of graphicbuffer,
/// so it can access the underlying graphicbuffer data
///
/// @param[in]  handle                 Buffer of shared memory
///
/// @return bool Whether the handle is known to gralloc mapper
///
bool Mapper::is_registered_handle(const native_handle_t *handle)
{
        bool imported = false;
        {
                std::lock_guard<std::mutex> lock(m_imported_handles_access_lock);
                imported = (m_imported_handles.find(handle) != m_imported_handles.end());
        }
        return imported;
}

///
/// Returns shared memory reserved for client use
///
/// @param[in]  handle                 Buffer of shared memory
/// @param[out] reserved_region        Base address of reserved region
/// @param[out] reserved_region_size   Size of reserved region
///
/// @return error: BAD_BUFFER if the buffer is invalid or is incompatible with this function.
///
Error Mapper::get_reserved_region(native_handle_t *handle, void **reserved_region, uint64_t *reserved_region_size)
{
        private_handle_t *hnd = reinterpret_cast<private_handle_t *>(handle);
        if (hnd->reserved_region_size == 0) {
                SGR_LOGE("native_handle_t %p has no reserved region", handle);
                return Error::BAD_BUFFER;
        }

        *reserved_region = sgr_get_metadata_user_reserved(handle);
        *reserved_region_size =  hnd->reserved_region_size;
        SGR_LOGV("(native_handle_t = %p, reserved_region = %p, reserved_region_size = %" PRIu64 ")",
                 handle, *reserved_region, *reserved_region_size);

        return Error::NONE;
}

///
/// @param handle [in] The buffer handle to map
///
bool Mapper::map_buffer(native_handle_t *handle)
{
        SGR_LOGV("(native_handle_t = %p)", handle);
        private_handle_t* hnd = reinterpret_cast<private_handle_t *>(handle);

        const uint32_t metadata_fd_index = get_metdata_fd_index(hnd);
        SGR_ASSERT(hnd->fds[metadata_fd_index] >= 0);
        void *metadata_base = m_manager->map(0, SGR_METADATA_SIZE_SUB_TOTAL + hnd->reserved_region_size,
                                             PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fds[metadata_fd_index], 0);
        SGR_LOGV("map metadata %p", metadata_base);
        if (metadata_base == nullptr) {
                SGR_LOGE("Error: Failed to map metadata");
                return false;
        }
        hnd->metadata_base = reinterpret_cast<uint64_t>(metadata_base);

        const sgr_metadata *metadata = sgr_get_metadata(handle);

        constexpr uint64_t cpu_mask = (static_cast<uint64_t>(BufferUsage::CPU_READ_MASK) |
                                       static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK));
        if(is_any_bitmask_set_64(metadata->usage, cpu_mask) == false) {
                return true;
        }

        if (metadata->protected_content == 0) {
                void *cpu_ptr = nullptr;
                memset(hnd->alloc_bases, 0, sizeof(uint64_t) * SGR_MAX_NUM_ALLOCATIONS);
                for (int idx = 0; idx < metadata->num_allocs; idx++) {
                        SGR_ASSERT_MSG(hnd->fds[idx] >= 0, "Invalid File descriptor to map");
                        cpu_ptr = m_manager->map(0, metadata->allocs[idx].total_size, PROT_READ|PROT_WRITE,
                                                 MAP_SHARED, hnd->fds[idx], 0);
                        if (cpu_ptr == nullptr) {
                                break;
                        }
                        hnd->alloc_bases[idx] = reinterpret_cast<uint64_t>(cpu_ptr);
                }

                if (cpu_ptr == nullptr){
                        SGR_LOGE("Error: Failed to map buffer");
                        unmap_buffer(handle);
                        return false;
                }
        }

        SGR_LOGV("Dumping Base Address");
        dump_private_handle_bases(hnd);

        return true;
}

///
/// @param handle [in] The buffer handle to unmap
///
bool Mapper::unmap_buffer(native_handle_t *handle)
{
        SGR_LOGV("(native_handle_t = %p)", handle);
        private_handle_t* hnd = (private_handle_t*)handle;
        const sgr_metadata *metadata = sgr_get_metadata(handle);
        bool ret = true;
        if (metadata->protected_content == 0) {
                for (int idx = 0; idx < metadata->num_allocs; idx++) {
                        if (hnd->alloc_bases[idx] != 0) {
                                ret = m_manager->unmap(reinterpret_cast<void *>(hnd->alloc_bases[idx]),
                                                       metadata->allocs[idx].total_size);
                                SGR_ASSERT_MSG(ret, "Failed to munmap handle %p fd%d", hnd, idx);
                                hnd->alloc_bases[idx] = 0;
                        }
                }
        }

        ret = m_manager->unmap(reinterpret_cast<void *>(hnd->metadata_base),
                               SGR_METADATA_SIZE_SUB_TOTAL + hnd->reserved_region_size);
        SGR_ASSERT_MSG(ret, "Failed to munmap handle %p metadata %p",
                           hnd, reinterpret_cast<void *>(hnd->metadata_base));
        hnd->metadata_base = 0;
        return ret;
}

///
/// Validates a specified access region against buffer dimensions from the handle
///
/// @param [in] handle buffer handle to validate against
/// @param [in] left   left of access region
/// @param [in] top    top of access region
/// @param [in] width  width of access region
/// @param [in] height height of access region
///
/// @return bool
///
bool Mapper::validate_lock_input_parameters(const native_handle_t *handle, const int left, const int top, const int width, const int height)
{
        SGR_LOGV("(native_handle_t = %p, left = %d, top = %d, width = %d, height = %d)",
                        handle, left, top, width, height);

        const private_handle_t *hnd = reinterpret_cast<const private_handle_t *>(handle);
        const sgr_metadata *metadata = sgr_get_metadata(hnd);

        /// @todo GFXSW-5798 - Skip check for BLOB formats
        // Codec2 attempts to lock with wrong access region
        if(metadata->alloc_format != static_cast<uint32_t>(PixelFormat::BLOB)) {
                if ((left < 0) || (top < 0) || (width < 0) || (height < 0)){
                        SGR_LOGE("Negative values for access region (Left = %d Top = %d Width = %d Height = %d)",
                                        left, top, width, height);
                        return false;
                }

                const sgr_metadata *metadata = sgr_get_metadata(handle);
                if (((top + height) > metadata->height) || ((left + width) > metadata->width)){
                        SGR_LOGE("Access region exceeds bounds (Left = %d Top = %d Width = %d Height = %d)",
                                        left, top, width, height);
                        return false;
                }
        }

        return true;
}

} // gralloc
} // samsung
} // android
