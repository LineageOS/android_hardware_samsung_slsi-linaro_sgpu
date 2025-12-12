///
/// @file util.h
/// @copyright 2020-2021 Samsung Electronics
///

#ifndef ANDROID_SAMSUNG_GRALLOC_UTIL_H
#define ANDROID_SAMSUNG_GRALLOC_UTIL_H

#include <cassert>
#include <cutils/log.h>
#include <string.h>

#include "environment_helper.h"

namespace android {
namespace samsung {
namespace gralloc {

#define SGR_UNUSED(var) ((void)var)

#define SGR_ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#define SGR_ALIGN(value, power_of_two) (((value) + (power_of_two) - 1) & ~((power_of_two) - 1))

#define SGR_IS_ALIGNED(value, power_of_two) ((value & (power_of_two -1)) == 0)

#define SGR_INT_DIV_CEIL(value, div) (((value) + (div) -1) / (div))

#define SGR_ASSERT(cond)                    \
        assert(cond)

#define SGR_ASSERT_MSG(cond, ...)                   \
        {                                               \
                if (static_cast<bool>(cond) == false) { \
                        ALOGE(__VA_ARGS__);             \
                        assert(false);                  \
                }                                       \
        }

// Stripped out at compile time for release build. Uncomment the following to enable it
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#define SGR_LOGV(msg, ...) ALOGV("SGRDARIO [%s] " msg, __func__, ##__VA_ARGS__)

#define SGR_LOGI(msg, ...) ALOGI("SGRDARIO [%s] " msg, __func__, ##__VA_ARGS__)

#define SGR_LOGE(msg, ...) ALOGE("SGRDARIO [%s] " msg, __func__, ##__VA_ARGS__)

#define SGR_LOGD(msg, ...) ALOGD("SGRDARIO [%s] " msg, __func__, ##__VA_ARGS__)

#define SGR_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define INVALID_VALUE 0xFFFFFFFF

inline bool is_power_of_two(uint32_t value)
{
        // This function assumes 0 is not a power of 2
        return ((value != 0) && (value & (value - 1))) == 0;
}

// This function accepts only power of two value which is less than 2^32
inline uint32_t get_log2_ffs(uint32_t power_of_two)
{
        SGR_ASSERT(is_power_of_two(power_of_two));

        return static_cast<uint32_t>(ffs(static_cast<int32_t>(power_of_two)) - 1);
}

inline bool is_all_bitmask_set(uint32_t value, uint32_t mask)
{
        return ((value & mask) == mask);
}

static inline bool is_all_bitmask_set_64(uint64_t value, uint64_t mask)
{
        return ((value & mask) == mask);
}

static inline bool is_bit_set(uint32_t value, uint32_t pos)
{
        uint32_t mask = (0x1 << pos);
        return is_all_bitmask_set(value, mask);
}

inline bool is_any_bitmask_set(uint32_t value, uint32_t mask)
{
        return ((value & mask) != 0);
}

inline bool is_any_bitmask_set_64(uint64_t value, uint64_t mask)
{
        return ((value & mask) != 0);
}

inline void set_bitmask(uint32_t *value, uint32_t mask)
{
        *value |= mask;
}

inline void clear_bitmask(uint32_t *value, uint32_t mask)
{
        *value &= (~mask);
}

inline uint32_t convert_bytes_to_bits(uint32_t bytes)
{
        return (bytes << 3);
}

inline uint32_t convert_bits_to_bytes(uint32_t bits)
{
        SGR_ASSERT((bits % 8) == 0);
        return (bits >> 3);
}

inline bool is_system_api_level_gt_s()
{
        const EnvironmentHelper *env_helper = EnvironmentHelper::GetInstance();
        SGR_ASSERT(env_helper != nullptr);

        return env_helper->is_system_gt_s();
}

inline bool is_launch_api_level_gt_s()
{
        const EnvironmentHelper *env_helper = EnvironmentHelper::GetInstance();
        SGR_ASSERT(env_helper != nullptr);

        return env_helper->is_launch_gt_s();
}

} // gralloc
} // samsung
} // android

#endif // ANDROID_SAMSUNG_GRALLOC_UTIL_H
