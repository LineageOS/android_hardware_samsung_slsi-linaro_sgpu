/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aidl/android/hardware/graphics/allocator/BnAllocator.h>
#include "core/allocator.h"
#include "core/mapper.h"

using aidl::android::hardware::common::NativeHandle;

namespace aidl::android::hardware::graphics::allocator {

using aidl::android::hardware::graphics::allocator::AllocationResult;
using aidl::android::hardware::graphics::allocator::BufferDescriptorInfo;

class Allocator : public BnAllocator {
public:
    Allocator() {};
    ndk::ScopedAStatus allocate(const std::vector<uint8_t>& in_descriptor, int32_t count, AllocationResult *_aidl_return);
    ndk::ScopedAStatus allocate2(const BufferDescriptorInfo& descriptor, int32_t count, AllocationResult *_aidl_return);
    ndk::ScopedAStatus isSupported(const BufferDescriptorInfo& descriptor, bool *_aidl_return);
    ndk::ScopedAStatus getIMapperLibrarySuffix(std::string *_aidl_return);
private:
    ::android::samsung::gralloc::Allocator mAllocator;
};

}
