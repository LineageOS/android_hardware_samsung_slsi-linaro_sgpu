/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "android.hardware.graphics.allocator-aidl-service-sgr"

#include "AllocatorAidl.h"
#include <aidlcommonsupport/NativeHandle.h>
#include "converter.h"
#include <android-base/logging.h>

using namespace android::samsung::gralloc;
using android::dupToAidl;
using aidl::android::hardware::common::NativeHandle;

namespace aidl::android::hardware::graphics::allocator {

ndk::ScopedAStatus Allocator::allocate(const std::vector<uint8_t>& in_descriptor, int32_t count, AllocationResult *_aidl_return) {
    uint32_t stride;
    std::vector<native_handle_t*> handles(count);
    Error error;

    const auto buffer_descriptor =
            reinterpret_cast<const BufferDescriptor*>(in_descriptor.data());
    error = mAllocator.allocate(*buffer_descriptor, count, handles, &stride);

    if (error == Error::NONE) {
        _aidl_return->buffers.resize(handles.size());
        for (int i = 0; i < handles.size() ; i++) {
            _aidl_return->buffers[i] = dupToAidl(handles[i]);
        }
    } else {
        _aidl_return->buffers.resize(0);
        handles.resize(0);
    }

    _aidl_return->stride = static_cast<int>(stride);

    for (auto handle : handles) {
        mAllocator.free_handle(handle);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Allocator::allocate2(const BufferDescriptorInfo& descriptor, int32_t count, AllocationResult *_aidl_return){
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Allocator::isSupported(const BufferDescriptorInfo& descriptor, bool *_aidl_return){
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
ndk::ScopedAStatus Allocator::getIMapperLibrarySuffix(std::string *_aidl_return){
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}
