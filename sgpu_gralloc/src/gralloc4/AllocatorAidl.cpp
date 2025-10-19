/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "android.hardware.graphics.allocator-aidl-service-sgr"

#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include "AllocatorAidl.h"
#include <aidlcommonsupport/NativeHandle.h>
#include "converter.h"
#include <android-base/logging.h>

using namespace android::samsung::gralloc;
using android::dupToAidl;
using aidl::android::hardware::common::NativeHandle;

namespace aidl::android::hardware::graphics::allocator {

::android::samsung::gralloc::BufferDescriptorInfo toInternalDescriptorInfo(const BufferDescriptorInfo& descriptor) {
    ::android::samsung::gralloc::BufferDescriptorInfo descriptor_info;

    const char *str = (const char*) descriptor.name.data();
    descriptor_info.name = std::string(str);

    descriptor_info.width = static_cast<uint32_t>(descriptor.width);
    descriptor_info.height = static_cast<uint32_t>(descriptor.height);
    descriptor_info.layerCount = static_cast<uint32_t>(descriptor.layerCount);
    descriptor_info.format = static_cast<::android::samsung::gralloc::PixelFormat>(static_cast<int32_t>(descriptor.format));
    descriptor_info.usage = static_cast<uint64_t>(descriptor.usage);
    descriptor_info.reservedSize = static_cast<uint64_t>(descriptor.reservedSize);

    return descriptor_info;
}

ndk::ScopedAStatus Allocator::allocate(const std::vector<uint8_t>& in_descriptor, int32_t count, AllocationResult *_aidl_return) {
    uint32_t stride;
    std::vector<native_handle_t*> handles(count);
    Error error;

    const auto buffer_descriptor =
            reinterpret_cast<const BufferDescriptor*>(in_descriptor.data());
    error = mAllocator.allocate(*buffer_descriptor, count, handles, &stride);

    switch (error) {
        case Error::NONE:
            break;

        case Error::BAD_DESCRIPTOR:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(AllocationError::BAD_DESCRIPTOR));

        case Error::NO_RESOURCES:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(AllocationError::NO_RESOURCES));

        case Error::UNSUPPORTED:
            return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(AllocationError::UNSUPPORTED));

        default:
            return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
    }

    _aidl_return->buffers.resize(handles.size());

    _aidl_return->stride = static_cast<int>(stride);
    for (int i = 0; i < handles.size() ; i++) {
        _aidl_return->buffers[i] = dupToAidl(handles[i]);
    }

    for (auto handle : handles) {
        mAllocator.free_handle(handle);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Allocator::allocate2(const BufferDescriptorInfo& descriptor, int32_t count, AllocationResult *_aidl_return){

    uint32_t stride;
    std::vector<native_handle_t*> handles(count);
    Error error;

    BufferDescriptor buffer_descriptor;

    error = m_mapper.create_descriptor(toInternalDescriptorInfo(descriptor), &buffer_descriptor);

    if (error == Error::NONE) {
        error = mAllocator.allocate(buffer_descriptor, count, handles, &stride);

        switch (error) {
            case Error::NONE:
                break;

            case Error::BAD_DESCRIPTOR:
                return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AllocationError::BAD_DESCRIPTOR));

            case Error::NO_RESOURCES:
                return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AllocationError::NO_RESOURCES));

            case Error::UNSUPPORTED:
                return ndk::ScopedAStatus::fromServiceSpecificError(
                    static_cast<int32_t>(AllocationError::UNSUPPORTED));

            default:
                return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
        }

        _aidl_return->buffers.resize(handles.size());

        _aidl_return->stride = static_cast<int>(stride);
        for (int i = 0; i < handles.size() ; i++) {
            _aidl_return->buffers[i] = dupToAidl(handles[i]);
        }

        for (auto handle : handles) {
            mAllocator.free_handle(handle);
        }
    } else {
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(AllocationError::BAD_DESCRIPTOR));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Allocator::isSupported(const BufferDescriptorInfo& descriptor, bool *_aidl_return){

    bool supported = m_mapper.is_supported(toInternalDescriptorInfo(descriptor));
    *_aidl_return = supported;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Allocator::getIMapperLibrarySuffix(std::string *_aidl_return){
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}
