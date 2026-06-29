///
/// @file config.h
/// @copyright 2021 Samsung Electronics
///

#pragma once

namespace android {
namespace samsung {
namespace gralloc {

///
/// Used to enable/disable dumping of verbose information.
///
/// The accepted values are
/// true, to enable dumping
/// false, to disable dumping
///
/// The default value is false.
///
#define CONFIG_VERBOSE "debug.sgr.verbose"

#define CONFIG_VERBOSE_DEFAULT false

///
///
///
#define CONFIG_VERBOSE_ALLOC "debug.sgr.verbose_alloc"

#define CONFIG_VERBOSE_ALLOC_DEFAULT false

///
/// Used to enable/disable SAJC.
///
/// The accepted values are
/// true, to enable SAJC
/// false, to disable SAJC
///
/// The default value is false.
#define CONFIG_SAJC    "debug.sgr.enable_sajc"

#define CONFIG_SAJC_DEFAULT true

///
/// Specify buffer name substring which when found in requested buffer descriptor name,
/// enables DCC layout allocation ONLY for buffers having that substring in name.
///
/// If empty, DCC layout is selected for all eligible buffers if debug.sgr.enable_sajc
/// is set to true.
/// Note: Won't have any effect if debug.sgr.enable_sajc is set to false
///
/// e.g. com.samsung.gputestbench
///
/// The default value is "".
#define CONFIG_SAJC_BUFFER_NAME_FILTER    "debug.sgr.enable_sajc_buffer_name_filter"

#define CONFIG_SAJC_BUFFER_NAME_FILTER_DEFAULT ""

///
/// Used to enable/disable doubling of SAJC buffer allocation size
///
/// The accepted values are
/// true, to enable doubling of SAJC buffer size
/// false, to disable doubling of SAJC buffer size
///
/// The default value is false.
#define CONFIG_SAJC_DOUBLE_ALLOC    "debug.sgr.enable_sajc_double_alloc"

#define CONFIG_SAJC_DOUBLE_ALLOC_DEFAULT false

///
/// Used to enable/disable setting the pattern that will be memset
/// to all buffers on allocation
///
/// Uncomment to enable memset'ing of pattern
/// Comment to disable memset'ing of pattern
///
/// The default is commented
///
/// Note: The max possible value for pattern is: 0xFF
// #define CONFIG_MEMSET_BUFFER_PATTERN
// #define CONFIG_MEMSET_BUFFER_PATTERN_VALUE      0x99

///
/// Used to get display panel size from HWC
///
/// The default value(w/h) is 1920x2340.
///
#define CONFIG_DISPLAY_W "vendor.hwc.display.w"
#define CONFIG_DISPLAY_H "vendor.hwc.display.h"

#define CONFIG_DISPLAY_W_DEFAULT 1080
#define CONFIG_DISPLAY_H_DEFAULT 2340

} // gralloc
} // samsung
} // android
