///
/// @file allocator.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include <android-base/properties.h>
#include <cstdlib>
#include <hwbinder/IPCThreadState.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "allocator.h"
#include "debug_helper.h"
#include "config.h"
#include "interface/metadata_gpu.h"
#include "exynos_ion_memory_manager.h"
#include "hardware/exynos/eis_utils.h"
#include "private_handle_helper.h"
#include "util/util.h"

namespace android {
namespace samsung {
namespace gralloc {

static ExynosIonMemoryManager g_memory_manager;

static_assert(sizeof(sgr_metadata) < SGR_METADATA_SIZE_GRALLOC, "Invalid metadata size - gralloc");

Allocator::Allocator()
        : m_memory_manager(reinterpret_cast<MemoryManager*>(&g_memory_manager)), m_buffer_id(0)
{
}

static inline uint64_t get_callingPid() {
        android::hardware::IPCThreadState* ipc = android::hardware::IPCThreadState::self();
        return static_cast<uint64_t>(ipc->getCallingPid());
}

///
/// @brief get GDC width/height by querying to camera library
///
/// @param[in/out] alloc_extent Image extent for allocation
///
/// @return true if can allocate buffer with margin, otherwise false
///
static inline bool check_and_set_gdc_dimension(sgr_extent_2d *alloc_extent)
{
        bool gdc_appliable = false;

        int gdc_width = 0, gdc_height = 0;
        if (0 == get_margin_size(alloc_extent->width, alloc_extent->height, &gdc_width, &gdc_height)) {
                SGR_LOGD("[S.LSI_GRALLOC] GDC applied...(%d/%d)", gdc_width, gdc_height);
                alloc_extent->width = gdc_width;
                alloc_extent->height = gdc_height;
                gdc_appliable = true;
        } else {
                SGR_LOGE("[S.LSI_GRALLOC] request input size(%d/%d) is invalid",
                            alloc_extent->width, alloc_extent->height);
                gdc_appliable = false;
        }

        return gdc_appliable;
}

///
/// @brief Retrieves implementation-defined debug information, which will be
///        displayed during, for example, `dumpsys SurfaceFlinger`.
///
/// @return debug string
///
std::string Allocator::dump_debug_info() const
{
        return std::string("tdb");
}

static inline bool is_protected_content(uint64_t usage)
{
        constexpr uint64_t protected_usage_mask = (static_cast<uint64_t>(BufferUsage::PROTECTED) |
                                                   static_cast<uint64_t>(BufferUsage::PRIVATE_NOZEROED) |
                                                   static_cast<uint64_t>(BufferUsage::PRIVATE_SECURE_CAMERA_RESERVED));
        constexpr uint64_t non_secure_usage_mask = static_cast<uint64_t>(BufferUsage::PRIVATE_PRIVATE_NONSECURE);

        return (is_any_bitmask_set_64(usage, protected_usage_mask) &&
                !is_all_bitmask_set_64(usage, non_secure_usage_mask));
}

///
/// @brief Allocates fds
///
/// @param[in]  num_allocs            Number of alloc in allocs array
/// @param[in]  allocs                Point to allocs array
/// @param[in]  usage                 Buffer usages
/// @param[in]  reserved_region_size  Size of reserved region
/// @param[out] fds                   Pointer to fd array
///
/// @return true if succeed, otherwise false
///
bool Allocator::allocate_fds(uint32_t num_allocs, const sgr_alloc *allocs, uint64_t usage,
                             uint64_t reserved_region_size, int *fds) const
{
        SGR_ASSERT(fds != nullptr);

        Error error = Error::NONE;
        uint32_t alloc_index = 0;
        for(alloc_index = 0; alloc_index < num_allocs; alloc_index++) {
                int fd = m_memory_manager->alloc(allocs[alloc_index].total_size,
                                                 allocs[alloc_index].alignment,
                                                 usage);
                SGR_LOGV("alloc fd[%u] = %d", alloc_index, fd);

                if (fd < 0) {
                        SGR_LOGE("failed to alloc allocation %u", alloc_index);
                        error = Error::NO_RESOURCES;
                        break;
                }
                fds[alloc_index] = fd;

#ifdef CONFIG_MEMSET_BUFFER_PATTERN
                        const bool protected_content = is_protected_content(usage);

                        if (protected_content == false) {
                                SGR_LOGV("Mapping and writing pattern=%d to fd= %d "
                                         "and total buffer size= %u",
                                          fd, CONFIG_MEMSET_BUFFER_PATTERN_VALUE,
                                          allocs[alloc_index].total_size);

                                void *cpu_ptr = m_memory_manager->map(0,
                                                                      allocs[alloc_index].total_size,
                                                                      PROT_READ|PROT_WRITE,
                                                                      MAP_SHARED,
                                                                      fds[alloc_index],
                                                                      allocs[alloc_index].data.offset);
                                if (cpu_ptr == nullptr) {
                                        SGR_LOGV("Failed to map buffer fd=%d", fd);
                                        error = Error::NO_RESOURCES;
                                        break;
                                }

                                memset(cpu_ptr, CONFIG_MEMSET_BUFFER_PATTERN_VALUE,
                                       allocs[alloc_index].total_size);
                                SGR_LOGV("Mapping and writing pattern=%d to fd=%d done",
                                          CONFIG_MEMSET_BUFFER_PATTERN_VALUE, fd);

                                bool ret = m_memory_manager->unmap(cpu_ptr,
                                                                   allocs[alloc_index].total_size);
                                SGR_ASSERT_MSG(ret, "Failed to unmap buffer fd=%d 0, cpu_ptr %p",
                                               fd, cpu_ptr);

                                // Below SGR_UNUSED() is for release build
                                SGR_UNUSED(ret);
                        }
                }
#endif
        }

        if (error == Error::NONE) {
                constexpr uint32_t alignment = SIZE_4K;
                constexpr uint64_t metadata_usage =
                        (static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN) |
                         static_cast<uint64_t>(BufferUsage::CPU_WRITE_RARELY));

                SGR_ASSERT(num_allocs == alloc_index);
                const uint32_t metadata_fd_index = num_allocs;
                fds[metadata_fd_index] = m_memory_manager->alloc(SGR_METADATA_SIZE_SUB_TOTAL + reserved_region_size,
                                                                 alignment, metadata_usage);
                SGR_LOGV("metadata fd = %d", fds[metadata_fd_index]);

                if (fds[metadata_fd_index] < 0) {
                        SGR_LOGE("failed to alloc metadata");
                        error = Error::NO_RESOURCES;
                }
        }

        if (error != Error::NONE) {
                SGR_LOGE("Failed to allocate fds");
                free_fds(alloc_index, fds);
                return false;
        }

        return true;
}

///
/// @brief Free fds
///
/// @param[in]     num_fds     Number of fds
/// @param[in/out] fds         Pointer to fd array
///
void Allocator::free_fds(uint32_t num_fds, int *fds) const
{
        SGR_ASSERT(fds != nullptr);

        for(uint32_t i = 0; i < num_fds; i++) {
                SGR_ASSERT(fds[i] > 0);
                SGR_LOGV("alloc fd[%u] = %d", i, fds[i]);

                m_memory_manager->free(fds[i]);
                fds[i] = -1;
        }
}

static inline bool check_extent_bt2020(uint32_t width, uint32_t height)
{
        return (width >= 3840 || height >= 3840 || (width * height >= 3840*1634));
}

static inline bool check_extent_bt601_625(uint32_t width, uint32_t height)
{
        return ((width <= 768 && height > 480 && height <= 576) || (height <= 768 && width > 480 && width <= 576));
}

static inline bool check_extent_bt601_525(uint32_t width, uint32_t height)
{
        return ((width <= 768 && height <= 480) || (height <= 768 && width <= 480));
}

static inline uint32_t get_dataspace(const PixelFormat alloc_format, const uint32_t width, const uint32_t height,
                                     uint64_t usage)
{
        SGR_UNUSED(usage);

        /* Default RGB dataspace. Expected by Mapper VTS. */
        uint32_t dataspace = static_cast<uint32_t>(Dataspace::UNKNOWN);

        if (is_yuv_format(alloc_format)) {
                /* This resolution is the cut-off point at which BT709 is used (as default)
                 * instead of BT601 for YUV formats < 10 bits.
                 */
                Dataspace color_space = Dataspace::STANDARD_BT709;
                Dataspace range = Dataspace::RANGE_LIMITED;

                if(check_extent_bt2020(width, height)) {
                        color_space = Dataspace::STANDARD_BT2020;
                } else if (check_extent_bt601_625(width, height)) {
                        color_space = Dataspace::STANDARD_BT601_625;
                } else if (check_extent_bt601_525(width, height)) {
                        color_space = Dataspace::STANDARD_BT601_525;
                }
                dataspace = (static_cast<uint32_t>(color_space) | static_cast<uint32_t>(range));
        }

        return dataspace;
}

///
/// @brief Certain pixel formats expect specific metadata plane_layouts field
///        values, even if those values weren't used to carry out the allocation.
///        Based on the format, corresponding plane_layouts fields are updated here.
///
/// @param[in]     format             PixelFormat of buffer
/// @param[in]     num_plane_layouts  Number of sgr_plane_layout structs
/// @param[in/out] plane_layouts      Array of sgr_plane_layout structs
///
static inline void update_metadata_plane_layouts(PixelFormat format,
                                                 uint64_t num_plane_layouts,
                                                 sgr_plane_layout *plane_layouts)
{
        SGR_ASSERT(plane_layouts != nullptr);

        bool plane_layout_updated = false;
        switch (format) {
        case PixelFormat::RAW10:
        case PixelFormat::RAW12:
                SGR_ASSERT(num_plane_layouts == 1);
                plane_layouts[0].sample_increment_in_bits = 0;
                plane_layouts[0].components[0].size_in_bits = -1;
                plane_layout_updated = true;
                break;
        default:
                plane_layout_updated = false;
                break;
        }

        if (plane_layout_updated) {
                SGR_LOGV("plane_layouts updated:");
                dump_sgr_plane_layout(plane_layouts, num_plane_layouts);
        } else {
                SGR_LOGV("No change in plane_layouts");
        }
}

static inline void set_metadata(const BufferDescriptor &descriptor,
                                const sgr_extent_2d &alloc_extent,
                                PixelFormat alloc_format,
                                FormatLayout alloc_layout,
                                uint32_t num_allocs,
                                const sgr_alloc *allocs,
                                uint32_t num_planes,
                                const sgr_plane_layout *plane_layouts,
                                sgr_metadata *metadata,
                                uint64_t buffer_id)
{
        uint32_t i = 0;
        metadata->buffer_id = buffer_id;
        metadata->length_name = descriptor.length_name;
        memcpy(metadata->name, descriptor.name, metadata->length_name);
        metadata->width = descriptor.width;
        metadata->height = descriptor.height;
        metadata->layer_count = descriptor.layerCount;
        metadata->pixel_format_requested = static_cast<uint64_t>(descriptor.format);
        metadata->pixel_format_fourcc = get_pixel_format_fourcc(alloc_format);
        metadata->pixel_format_modifier = 0;
        metadata->usage = descriptor.usage;

        metadata->allocation_size = 0;
        for(int i = 0; i < num_allocs; i++)
                metadata->allocation_size += allocs[i].total_size;

        metadata->protected_content = is_protected_content(descriptor.usage);

        metadata->compression = 0;                   ///@todo GFXSW-4188 - Update metadata
        metadata->interlaced = 0;                    ///@todo GFXSW-4188 - Update metadata
        metadata->chroma_siting = 0;                 ///@todo GFXSW-4188 - Update metadata
        metadata->num_plane_layouts = num_planes;
        memcpy(metadata->plane_layouts, plane_layouts, sizeof(sgr_plane_layout) * num_planes);
        update_metadata_plane_layouts(descriptor.format, metadata->num_plane_layouts, metadata->plane_layouts);

        metadata->num_crops = num_planes;
        for (i = 0; i < metadata->num_crops; i++) {
                metadata->crops[i].left = 0;
                metadata->crops[i].top = 0;
                metadata->crops[i].right =  metadata->width;
                metadata->crops[i].bottom = metadata->height;
        }

        metadata->dataspace = get_dataspace(alloc_format, descriptor.width, descriptor.height, metadata->usage);
        metadata->blend_mode = 0;
        metadata->is_smpte2086_set = false;
        metadata->is_cta861_3_set = false;
        metadata->length_smpte2094_40  = 0;
        memset(metadata->smpte2094_40, 0, SGR_MAX_LENGTH_SMPTE2094_40);

        metadata->alloc_width = alloc_extent.width;
        metadata->alloc_height = alloc_extent.height;
        metadata->alloc_format = convert_to_sgr_alloc_format(alloc_format);
        metadata->alloc_layout = convert_to_sgr_alloc_layout(alloc_layout);
        metadata->num_allocs = num_allocs;
        ///@todo GFXSW-4188 - Update metadata
        memcpy(metadata->allocs, allocs, sizeof(allocs[0]) * num_allocs);
}

static void update_handle(private_handle_t *handle, sgr_metadata *metadata)
{
        handle->major_version = PRIVATE_HANDLE_INTERFACE_VERSION_MAJOR;
        handle->minor_version = PRIVATE_HANDLE_INTERFACE_VERSION_MINOR;

        handle->width  = static_cast<uint32_t>(metadata->width);
        handle->height = static_cast<uint32_t>(metadata->height);
        handle->format = static_cast<uint32_t>(metadata->pixel_format_requested);
        handle->usage  = metadata->usage;

        handle->alloc_width  = metadata->alloc_width;
        handle->alloc_height = metadata->alloc_height;
        handle->alloc_format = metadata->alloc_format;
        handle->alloc_layout = metadata->alloc_layout;

        handle->num_planes = metadata->num_plane_layouts;
        handle->num_allocs = metadata->num_allocs;

        for(uint32_t i = 0; i< metadata->num_plane_layouts; i++) {
            plane_info *dst = &handle->plane_infos[i];
            const sgr_plane_layout *src = &metadata->plane_layouts[i];

            dst->size = src->total_size_in_bytes;
            dst->offset = src->offset_in_bytes;
            dst->stride_in_bytes = src->stride_in_bytes;
            dst->width_in_samples = src->width_in_samples;
            dst->height_in_samples = src->height_in_samples;
        }

        for(uint32_t i = 0; i<metadata->num_allocs; i++) {
            alloc_info *dst = &handle->alloc_infos[i];
            const sgr_alloc *src = &metadata->allocs[i];

            dst->size = src->total_size;
            dst->key_offset = src->key.offset;
        }
}

///
/// @brief Allocates buffers with the properties specified by the descriptor
///
/// @param[in]  descriptor Specifies the properties of the buffers to allocate.
/// @param[in]  count      Number of buffers to allocate
/// @param[out] handles    Buffer_handles allocates
/// @param[out] stride     Number of pixels between two consecutive rows
///
/// @return error
///
Error Allocator::allocate(const BufferDescriptor &descriptor, uint32_t count,
                          std::vector<native_handle_t*> &handles, uint32_t *stride)
{
        dump_descriptor(descriptor);

        SGR_ASSERT(stride != nullptr);
        SGR_LOGV("stride = %p, count = %u", stride, count);

        // Get alloc format and layout
        FormatLayout alloc_layout = FormatLayout::NONE;
        uint32_t ip_flags = 0;
        PixelFormat alloc_format = m_format_manager.get_format(descriptor, &alloc_layout, &ip_flags);

        SGR_ASSERT((alloc_format != PixelFormat::UNDEFINED) &&
                       (alloc_layout != FormatLayout::NONE));

        if (alloc_format == PixelFormat::UNDEFINED || alloc_layout == FormatLayout::NONE) {
            // both format and layout should be derived at this stage
            SGR_LOGD("Unsupported format or layout. (format: %" PRIi32 ", layout: %" PRIu32 ", ip_flags: %" PRIu32 ")",
                                                     descriptor.format, alloc_layout, ip_flags);
            return Error::UNSUPPORTED;
        }

        // Get allocation information
        sgr_alloc allocs[SGR_MAX_NUM_ALLOCATIONS] = {};
        sgr_extent_2d alloc_extent = {descriptor.width, descriptor.height};
        sgr_plane_layout planes[SGR_MAX_NUM_PLANES] = {};

        int gdc_width = 0, gdc_height = 0;
        if (is_any_bitmask_set_64(descriptor.usage, static_cast<uint64_t>(BufferUsage::PRIVATE_GDC_MODE))) {
                if(check_and_set_gdc_dimension(&alloc_extent) == false) {
                        return Error::UNSUPPORTED;
                }
                gdc_width = alloc_extent.width;
                gdc_height = alloc_extent.height;
        }

        uint32_t num_planes = 0;
        uint32_t num_allocs =
                m_format_manager.get_allocation_info(alloc_format, alloc_layout, descriptor.usage ,ip_flags,
                                                     descriptor.layerCount, &alloc_extent, allocs, &num_planes,
                                                     planes);

        // couldn't get allocation information successfully
        if (num_allocs == 0) {
                return Error::UNSUPPORTED;
        }

        // Allocate the first buffer
        private_handle_t *handle = create_private_handle(descriptor, alloc_extent,
                                                         alloc_format, alloc_layout,
                                                         num_allocs, allocs,
                                                         num_planes, planes);
        if (handle == nullptr) {
                return Error::NO_RESOURCES;
        }

        if (!allocate_fds(num_allocs, allocs, descriptor.usage, descriptor.reservedSize, handle->fds)) {
                free(handle);
                return Error::NO_RESOURCES;
        }

        const uint32_t metadata_fd_index = num_allocs;
        constexpr uint32_t metadata_map_size = SGR_METADATA_SIZE_SUB_TOTAL;
        SGR_ASSERT(handle->fds[metadata_fd_index] >= 0);
        void *metadata_base = m_memory_manager->map(0, metadata_map_size, PROT_READ|PROT_WRITE,
                                                    MAP_SHARED, handle->fds[metadata_fd_index], 0);
        sgr_metadata *metadata = get_metadata(metadata_base);
        if (metadata == nullptr) {
                free_fds(num_allocs, handle->fds);
                free(handle);
                return Error::NO_RESOURCES;
        }

        // The stock gralloc writes the swizzle mode into sgr_metadata_gpu.dcc_sw_mode at
        // allocation time, so that the GPU/Vulkan driver and the SAJC DRM modifier path
        // agree on the tile geometry
#ifdef ENABLE_SWIZZLE_4K_MODE
        struct sgr_metadata_gpu *gpu_meta = (struct sgr_metadata_gpu *) ((uintptr_t)metadata_base + SGR_METADATA_OFFSET_GPU);
        gpu_meta->dcc_sw_mode = SAJC_SWIZZLE_4KB_R_X;
#else
        // Normally we should write SAJC_SWIZZLE_64KB_R_X into dcc_sw_mode, if 4k path is
        // not chosen, however this breaks SoCs older than 9945 (9925) which are not expecting
        // this value to be set. Not setting it does not break 9945 when not using 4k swizzle
        // path but the code is left here for clarity to the reader.
        // struct sgr_metadata_gpu *gpu_meta = (struct sgr_metadata_gpu *) ((uintptr_t)metadata_base + SGR_METADATA_OFFSET_GPU);
        // gpu_meta->dcc_sw_mode = SAJC_SWIZZLE_64KB_R_X;
#endif

        if (is_any_bitmask_set_64(descriptor.usage, static_cast<uint64_t>(BufferUsage::PRIVATE_GDC_MODE))) {
                alloc_extent.width = gdc_width;
                alloc_extent.height = gdc_height;
        }

        uint64_t pid = get_callingPid();
        uint64_t buffer_id = (pid << 40 | m_buffer_id++);
        set_metadata(descriptor, alloc_extent, alloc_format, alloc_layout,
                     num_allocs, allocs, num_planes, planes, metadata, buffer_id);
        handles[0] = reinterpret_cast<native_handle_t*>(handle);
        update_handle(handle, metadata);

        if (alloc_format == PixelFormat::RAW10 || alloc_format == PixelFormat::RAW12) {
                *stride = metadata->plane_layouts[0].stride_in_bytes;
        } else if (metadata->plane_layouts[0].sample_increment_in_bits == 0) {
                *stride = 0;
        } else {
                *stride = metadata->plane_layouts[0].width_in_samples;
        }

        SGR_LOGV("handles[0] = ");
        dump_private_handle(handle);
        dump_buffer_allocation(handle, metadata);

        // Allocate the other buffers when count > 1
        Error error = Error::NONE;
        uint32_t buffer = 1;
        for(buffer = 1; buffer < count; buffer++) {
                handle = copy_create_private_handle(reinterpret_cast<private_handle_t*>(handles[0]));

                if (!allocate_fds(num_allocs, allocs, descriptor.usage, handle->reserved_region_size, handle->fds)) {
                        free(handle);
                        error = Error::NO_RESOURCES;
                        break;
                }

                SGR_ASSERT(handle->fds[metadata_fd_index] >= 0);
                void *tmp_metadata_base = m_memory_manager->map(0, metadata_map_size, PROT_READ|PROT_WRITE,
                                                                MAP_SHARED, handle->fds[metadata_fd_index], 0);
                sgr_metadata *tmp_metadata = get_metadata(tmp_metadata_base);
                if (tmp_metadata == nullptr) {
                        free_fds(num_allocs, handle->fds);
                        free(handle);
                        error = Error::NO_RESOURCES;
                        break;
                }

                ///@todo GFXSW-4188 - Update name
                memcpy(reinterpret_cast<void*>(tmp_metadata), reinterpret_cast<void*>(metadata),
                       SGR_METADATA_SIZE_GRALLOC);
                tmp_metadata->buffer_id = m_buffer_id++;
                update_handle(handle, tmp_metadata);

                bool ret = m_memory_manager->unmap(tmp_metadata_base, metadata_map_size);
                SGR_ASSERT_MSG(ret, "Failed to munmap buffer %u, metadata %p", buffer,
                                   tmp_metadata_base);

                handles[buffer] = handle;
                SGR_LOGV("handles[%u] = ", buffer);
                dump_private_handle(handle);
        }

        if (error != Error::NONE) {
                SGR_ASSERT(count > 1);
                for(uint32_t i = 0; i < buffer; i++) {
                        auto handle = reinterpret_cast<private_handle_t*>(handles[i]);
                        SGR_LOGV("Freeing handles[%u] = %p", i, handle);
                        SGR_ASSERT(handle != nullptr);
                        free_fds(num_allocs + SGR_NUM_METADATA, handle->fds);
                        free(handle);
                        handles[i] = nullptr;
                }
                *stride = 0;
        }

        bool ret = m_memory_manager->unmap(metadata_base, metadata_map_size);
        SGR_ASSERT_MSG(ret, "Failed to munmap buffer 0, metadata_base %p", metadata_base);

        return error;
}

void Allocator::free_handle(native_handle_t *handle)
{
        SGR_LOGV("(native_handle_t = %p)", handle);
        SGR_ASSERT_MSG(handle != nullptr, "Invalid buffer handle %p to free_handle", handle);

        int32_t status = native_handle_close(handle);
        SGR_ASSERT_MSG(status == 0, "Failed to close handle %p", handle);
        status = native_handle_delete(handle);
        SGR_ASSERT_MSG(status == 0, "Failed to delete handle %p", handle);
}

} // gralloc
} // samsung
} // android
