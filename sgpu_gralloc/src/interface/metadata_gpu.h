///
/// @file metadata_gpu.h
/// @copyright 2020-2021 Samsung Electronics
///

#ifndef SGR_METADATA_GPU_H_
#define SGR_METADATA_GPU_H_

#include <stdint.h>

#define __ENABLE_SETTER_GENERATION        __attribute__((annotate("sgr_setter_on")))

///
/// The total size of the struct should be less than SGR_METADATA_SIZE_GPU (256 bytes) defined in gralloc
/// If it needs more, then it should negotiate with gralloc
///

struct sgr_metadata_gpu {
        // DCC info

        // 27 for AMDGPU_SWIZZLE_MODE_64KB_R_X or 23 for AMDGPU_SWIZZLE_MODE_4KB_R_X for DPU
        uint32_t dcc_sw_mode                     __ENABLE_SETTER_GENERATION;

        // Only 0 for DPU - pipe unaligned
        uint32_t dcc_is_pipe_aligned             __ENABLE_SETTER_GENERATION;

        // Set by GPU - 0, 1, 2 (0 = 64 Bytes, 1 = 128 Bytes, 2 = 256 Bytes)
        uint32_t dcc_max_uncompressed_block_size __ENABLE_SETTER_GENERATION;

        // Set by GPU - 0, 1, 2 (0 = 64 Bytes, 1 = 128 Bytes, 2 = 256 Bytes)
        uint32_t dcc_max_compressed_block_size   __ENABLE_SETTER_GENERATION;

        // 1 for 64 byte independent blocks, if 1 is set for dcc_independent_block_size_128, this field must be 0
        uint32_t dcc_independent_block_size_64   __ENABLE_SETTER_GENERATION;

        // 1 for 128 byte independent blocks, if 1 is set for dcc_independent_block_size_64, this field must be 0
        uint32_t dcc_independent_block_size_128  __ENABLE_SETTER_GENERATION;

        // 0 if this is a brand new AHB, set to 1 if the struct is initialized
        uint32_t is_metadata_initialized         __ENABLE_SETTER_GENERATION;
};

#endif //  SGR_METADATA_GPU_H_
