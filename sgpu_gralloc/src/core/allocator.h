///
/// @file allocator.h
/// @copyright 2020 Samsung Electronics
///

#pragma once

#include <cstdint>

#include "format_info.h"
#include "format_manager.h"
#include "hidl_defines.h"

namespace android {
namespace samsung {
namespace gralloc {

class MemoryManager;
struct native_buffer_t;
using ::sgr_alloc;

class Allocator
{
public:
        Allocator();
        ~Allocator() = default;
        Allocator(const Allocator&) = delete;
        Allocator(const Allocator&&) = delete;
        Allocator& operator=(const Allocator&) = delete;
        Allocator& operator=(const Allocator&&) = delete;

        std::string dump_debug_info() const;
        Error       allocate(const BufferDescriptor &descriptor, uint32_t count,
                             std::vector<native_handle_t*> &handles, uint32_t *stride);
        void free_handle(native_handle_t *handle);
private:
        bool allocate_fds(uint32_t num_allocs, const sgr_alloc *allocs, uint64_t usage,
                          uint64_t reserved_region_size, int *fds) const;
        void free_fds(uint32_t num_fds, int *fds) const;

private:
        FormatManager m_format_manager;
        MemoryManager *m_memory_manager;
        std::atomic<uint64_t> m_buffer_id;
};

} // gralloc
} // samsung
} // android
