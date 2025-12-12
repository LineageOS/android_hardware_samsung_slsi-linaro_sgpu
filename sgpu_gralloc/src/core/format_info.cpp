///
/// @file format_info.cpp
/// @copyright 2020 Samsung Electronics
///

#include <drm/drm_fourcc.h>

#include "format_info.h"
#include "hidl_defines.h"
#include "util/util.h"

namespace android {
namespace samsung {
namespace gralloc {

constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_NA   = SGR_PLANE_LAYOUT_COMPONENT_TYPE(0);
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_r    = SGR_PLANE_LAYOUT_COMPONENT_TYPE::R;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_g    = SGR_PLANE_LAYOUT_COMPONENT_TYPE::G;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_b    = SGR_PLANE_LAYOUT_COMPONENT_TYPE::B;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_a    = SGR_PLANE_LAYOUT_COMPONENT_TYPE::A;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_y    = SGR_PLANE_LAYOUT_COMPONENT_TYPE::Y;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_cb   = SGR_PLANE_LAYOUT_COMPONENT_TYPE::CB;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_cr   = SGR_PLANE_LAYOUT_COMPONENT_TYPE::CR;
constexpr SGR_PLANE_LAYOUT_COMPONENT_TYPE component_raw  = SGR_PLANE_LAYOUT_COMPONENT_TYPE::RAW;

// Entries must be in ascending order of ID as table will use binary search to return requested format index
static const format_info format_info_table[] = {
        {
                .id = PixelFormat::RGBA_8888,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 4,
                .info = {
                        {.plane_index = 0, .type = component_r, .bits = 8},
                        {.plane_index = 0, .type = component_g, .bits = 8},
                        {.plane_index = 0, .type = component_b, .bits = 8},
                        {.plane_index = 0, .type = component_a, .bits = 8}
                        },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_ABGR8888
        },
        {
                .id = PixelFormat::RGBX_8888,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 4,
                .info = {
                        {.plane_index = 0, .type = component_r, .bits = 8},
                        {.plane_index = 0, .type = component_g, .bits = 8},
                        {.plane_index = 0, .type = component_b, .bits = 8},
                        {.plane_index = 0, .type = component_a, .bits = 8}
                        },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_XBGR8888
        },
        {
                .id = PixelFormat::RGB_888,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index = 0, .type = component_r, .bits = 8},
                        {.plane_index = 0, .type = component_g, .bits = 8},
                        {.plane_index = 0, .type = component_b, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                        },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_BGR888
        },
        {
                .id = PixelFormat::RGB_565,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_b, .bits = 5},
                        {.plane_index =  0, .type = component_g, .bits = 6},
                        {.plane_index =  0, .type = component_r, .bits = 5},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_RGB565
        },
        {
                .id = PixelFormat::BGRA_8888,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 4,
                .info = {
                        {.plane_index = 0, .type = component_b, .bits = 8},
                        {.plane_index = 0, .type = component_g, .bits = 8},
                        {.plane_index = 0, .type = component_r, .bits = 8},
                        {.plane_index = 0, .type = component_a, .bits = 8}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_ARGB8888
        },
        {
                // Legacy format deprecated in favor of YCBCR_420_888
                // Fourcc: NV16 0x3631564E
                .id = PixelFormat::YCBCR_422_SP,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_422,
                .format_fourcc = DRM_FORMAT_NV16
        },
        {
                // Legacy format deprecated in favor of YCBCR_420_888
                // Fourcc: NV21 0x3132564E
                .id = PixelFormat::YCRCB_420_SP,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV21
        },
        {
                // Legacy format deprecated in favor of YCBCR_420_888
                // Fourcc: YUYV 0x56595559
                .id = PixelFormat::YCBCR_422_I,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index = 0, .type = component_y,  .bits = 8},
                        {.plane_index = 0, .type = component_cb, .bits = 8},
                        {.plane_index = 0, .type = component_cr, .bits = 8},
                },
                .format_sampling = SamplingType::YUV_422,
                .format_fourcc = DRM_FORMAT_YUYV
        },
        {
                .id = PixelFormat::RGBA_FP16,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 4,
                .info = {
                        {.plane_index = 0, .type = component_r, .bits = 16},
                        {.plane_index = 0, .type = component_g, .bits = 16},
                        {.plane_index = 0, .type = component_b, .bits = 16},
                        {.plane_index = 0, .type = component_a, .bits = 16}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_ABGR16161616F
        },
        {
                .id = PixelFormat::RAW16,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_raw, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::BLOB,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                // Fourcc: NV12 0x3231564E
                .id = PixelFormat::YCBCR_420_888,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        // PixelFormat::RAW_OPAQUE
        {
                .id = PixelFormat::RAW10,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_raw, .bits = 10},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::RAW12,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_raw, .bits = 12},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA,  .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::RGBA_1010102,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 4,
                .info = {
                        {.plane_index = 0, .type = component_a, .bits =  2},
                        {.plane_index = 0, .type = component_b, .bits = 10},
                        {.plane_index = 0, .type = component_g, .bits = 10},
                        {.plane_index = 0, .type = component_r, .bits = 10}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_ABGR2101010
        },
        {
                .id = PixelFormat::DEPTH_16,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::DEPTH_24,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 32},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::DEPTH_24_STENCIL_8,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 2,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 24},
                        {.plane_index = 0,             .type = component_g,  .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::DEPTH_32F,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 32},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::DEPTH_32F_STENCIL_8,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 2,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 32},
                        {.plane_index = 0,             .type = component_g,  .bits = 32},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::STENCIL_8,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_g,  .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                // Fourcc: P010 0x30313050
                .id = PixelFormat::YCBCR_P010,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_P010
        },
        {
                .id = PixelFormat::R_8,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index = 0,             .type = component_r,  .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::RGB,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_P_M,
                .num_allocs = 3,
                .num_planes_per_alloc = {1, 1, 1},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  2, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_YUV420
        },
        // PRIVATE_CBYCRY_422_I
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_TILED,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        // PRIVATE_YCRCB_422_I
        // PRIVATE_CRYCBY_422_I
        {
                .id = PixelFormat::PRIVATE_YV12_M,
                .num_allocs = 3,
                .num_planes_per_alloc = {1, 1, 1},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index =  2, .type = component_cb, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_YVU420
        },
        {
                .id = PixelFormat::PRIVATE_YCRCB_420_SP_M,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV21
        },
       {
                .id = PixelFormat::PRIVATE_YCRCB_420_SP_M_FULL,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV21
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_P,
                .num_allocs = 1,
                .num_planes_per_alloc = {3, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  2, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_YUV420
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_S10B,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_S10B,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_P010_M,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_P010
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV21
        },
        {
                .id = PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L50,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L75,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_NV12
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cb, .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L,
                .num_allocs = 2,
                .num_planes_per_alloc = {1, 1, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                .id = PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR,
                .num_allocs = 1,
                .num_planes_per_alloc = {2, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index =  1, .type = component_cb, .bits = 16},
                        {.plane_index =  1, .type = component_cr, .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                // Fourcc: Y8 0x20203859
                .id = PixelFormat::Y8,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_444,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                // Fourcc: Y16 0x20363159
                .id = PixelFormat::Y16,
                .num_allocs = 1,
                .num_planes_per_alloc = {1, 0, 0},
                .num_components= 1,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 16},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_444,
                .format_fourcc = DRM_FORMAT_INVALID /// @todo GFXSW-4819
        },
        {
                // Fourcc: YV12 0x32315659
                .id = PixelFormat::YV12,
                .num_allocs = 1,
                .num_planes_per_alloc = {3, 0, 0},
                .num_components= 3,
                .info = {
                        {.plane_index =  0, .type = component_y,  .bits = 8},
                        {.plane_index =  1, .type = component_cr, .bits = 8},
                        {.plane_index =  2, .type = component_cb, .bits = 8},
                        {.plane_index = INVALID_VALUE, .type = component_NA, .bits = 0}
                },
                .format_sampling = SamplingType::YUV_420,
                .format_fourcc = DRM_FORMAT_YVU420
        }
};

static const sub_sampling_info sub_sampling_info_array [] = {
        {.hsub = 1, .vsub = 1}, // RGB
        {.hsub = 1, .vsub = 1}, // YUV_444
        {.hsub = 2, .vsub = 1}, // YUV_422
        {.hsub = 2, .vsub = 2}, // YUV_420
};

///
/// @brief Search for @format in format_info_table[] using binary search
///
/// @param uint32_t format
///
/// @return index of @format
///
static int32_t get_format_info_index(PixelFormat format){
        uint32_t format_id = static_cast<uint32_t>(format);
        SGR_ASSERT(format_id > 0);

        constexpr uint32_t num_formats = SGR_ARRAY_SIZE(format_info_table);
        uint32_t first = 0;
        uint32_t last  = num_formats - 1;
        uint32_t cur   = 0;
        while(first <= last){
                cur = (first + last) >> 1;
                if (format_id == static_cast<uint32_t>(format_info_table[cur].id)) {
                        return cur;
                }

                if (format_id > static_cast<uint32_t>(format_info_table[cur].id)) {
                        first = cur + 1;
                } else {
                        last = cur - 1;
                }
        }

        SGR_LOGV("Format %d is not found in format_info table", format_id);
        return -1;
}

///
/// @brief Get number of components in format
///
/// @param PixelFormat format
///
/// @return number of components
///
uint32_t get_num_components(PixelFormat format)
{
        int index = get_format_info_index(format);
        return (index == -1) ? 0 : format_info_table[index].num_components;
}

///
/// @brief Get number of allocs in format
///
/// @param PixelFormat format
///
/// @return number of allocs
///
uint32_t get_num_allocs(PixelFormat format)
{
        int index = get_format_info_index(format);
        return (index == -1) ? 0 : format_info_table[index].num_allocs;
}

///
/// @brief Get number of planes per alloc in format
///
/// @param PixelFormat format
///
/// @return number of planes per alloc
///
const uint32_t *get_num_planes_per_alloc(PixelFormat format)
{
        int index = get_format_info_index(format);
        return (index == -1) ? nullptr : format_info_table[index].num_planes_per_alloc;
}

///
/// @brief Get component info for format
///
/// @param PixelFormat format
///
/// @return component info
///
const component_info *get_component_info(PixelFormat format)
{
        int index = get_format_info_index(format);
        return (index == -1) ? nullptr : format_info_table[index].info;
}

///
/// @brief Get Sub sampling info for format
///
/// @param PixelFormat format
///
/// @return sub sampling info
///
const sub_sampling_info *get_sampling_info(PixelFormat format, uint32_t plane_index)
{
        int index = get_format_info_index(format);
        if (index == -1) {
                return nullptr;
        }

        uint32_t array_index = static_cast<uint32_t>(format_info_table[index].format_sampling);

        // Return {1, 1} (No subsampling) if plane index is 0
        // Assuming that plane 0 for all formats is not subsampled.
        // Only applying subsampling for chroma planes
        return (plane_index == 0) ? &sub_sampling_info_array[0] : &sub_sampling_info_array[array_index];
}

///
/// @brief Check if format is YUV format
///
/// @param PixelFormat format
///
/// @return bool  true if yuv format else false
///
bool is_yuv_format(PixelFormat format)
{
        int index = get_format_info_index(format);
        return (index == -1) ? false : format_info_table[index].format_sampling != SamplingType::RGB;
}

///
/// @brief Get Bytes per sample for plane 0
/// @param PixelFormat format
///
/// @return Bytes per sample
///
uint32_t get_bps(PixelFormat format)
{
        int index = get_format_info_index(format);
        if (index == -1) {
                return 0;
        }

        const component_info *info = format_info_table[index].info;
        uint32_t bps = 0;
        for (uint i = 0; i < format_info_table[index].num_components; i++) {
                if (info[i].plane_index == 0) {
                        bps += info[i].bits;
                }
        }

        return convert_bits_to_bytes(bps);
}

///
/// @brief Get alloc index of a plane for the given format
/// @param PixelFormat format
/// @param uin32_t plane_index
///
/// @return alloc index of the plane
///
uint32_t get_alloc_index(PixelFormat format, uint32_t plane_index)
{
        const uint32_t num_allocs = get_num_allocs(format);
        if (num_allocs == 1) {
                return 0;
        }

        const uint32_t *planes_per_alloc = get_num_planes_per_alloc(format);
        uint32_t total_planes = 0;
        uint32_t i = 0;
        for (i = 0; i < num_allocs; i++) {
                total_planes += planes_per_alloc[i];
                if (plane_index < total_planes) {
                        break;
                }
        }

        SGR_ASSERT(i < num_allocs);
        return i;
}

///
/// @brief Get pixel format fourcc
/// @param PixelFormat format
///
/// @return pixel fourcc format
///
uint32_t get_pixel_format_fourcc(PixelFormat format)
{
        int index = get_format_info_index(format);
        return (index == -1) ? DRM_FORMAT_INVALID : format_info_table[index].format_fourcc;
}

///
/// @brief Check if format is SBWC format
/// @param PixelFormat format
///
/// @return result true if a valid SBWC format
///
bool is_sbwc_format(PixelFormat format)
{
        bool result = false;
        switch (format) {
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_256_SBWC:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_256_SBWC:
                case PixelFormat::PRIVATE_YCRCB_420_SP_M_SBWC:
                case PixelFormat::PRIVATE_YCRCB_420_SP_M_10B_SBWC:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L50:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_SBWC_L75:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L50:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_SBWC_L75:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L40:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L60:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_SBWC_L80:
                /* SBWC Lossy formats over v2.7 */
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_32_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_32_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_64_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_32_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_32_SBWC_L:
                case PixelFormat::PRIVATE_YCBCR_420_SPN_10B_64_SBWC_L:
                /* SBWC Lossy formats over v2.8 */
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_64_SBWC_L_FR:
                case PixelFormat::PRIVATE_YCBCR_420_SP_M_10B_64_SBWC_L_FR:
                        result = true;
                        break;
                default:
                        break;
        }
        return result;
}

} // gralloc
} // samsung
} // android
