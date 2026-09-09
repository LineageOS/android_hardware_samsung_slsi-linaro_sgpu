/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <aidl/android/hardware/graphics/common/StandardMetadataType.h>
#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>

#include "MapperStableC.h"
#include "converter.h"
#include "util/util.h"

using namespace android::hardware::graphics::mapper;
using namespace android::samsung::gralloc;
using aidl::android::hardware::graphics::common::StandardMetadataType;

constexpr const char* STANDARD_METADATA_NAME =
        "android.hardware.graphics.common.StandardMetadataType";

AIMapper_Error MapperV5Impl::importBuffer(const native_handle_t* handle,
                                          buffer_handle_t* outBufferHandle) {
    if (handle == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    return (AIMapper_Error)mMapper.import_buffer(handle,
                                                 const_cast<native_handle_t**>(outBufferHandle));
}

AIMapper_Error MapperV5Impl::freeBuffer(buffer_handle_t buffer) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    return (AIMapper_Error)mMapper.free_buffer(const_cast<native_handle_t*>(buffer));
}

AIMapper_Error MapperV5Impl::getTransportSize(buffer_handle_t buffer, uint32_t* outNumFds,
                                              uint32_t* outNumInts) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    return (AIMapper_Error)mMapper.get_transport_size(const_cast<native_handle_t*>(buffer),
                                                      (int*)outNumFds, (int*)outNumInts);
}

AIMapper_Error MapperV5Impl::lock(buffer_handle_t buffer, uint64_t cpuUsage, ARect accessRegion,
                                  int acquireFence, void** outData) {
    if (buffer == nullptr) {
        close(acquireFence);
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    int32_t bytesPerPixel = -1;
    int32_t bytesPerStride = -1;
    auto error = (AIMapper_Error)mMapper.lock(const_cast<native_handle_t*>(buffer), cpuUsage,
                                              toRect(accessRegion), acquireFence, outData,
                                              &bytesPerPixel, &bytesPerStride);
    close(acquireFence);
    return error;
}

AIMapper_Error MapperV5Impl::unlock(buffer_handle_t buffer, int* releaseFence) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    return (AIMapper_Error)mMapper.unlock(const_cast<native_handle_t*>(buffer), releaseFence);
}

AIMapper_Error MapperV5Impl::flushLockedBuffer(buffer_handle_t buffer) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    int release_fence;
    return (AIMapper_Error)mMapper.flush_locked_buffer(const_cast<native_handle_t*>(buffer),
                                                       &release_fence);
}

AIMapper_Error MapperV5Impl::rereadLockedBuffer(buffer_handle_t buffer) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    return (AIMapper_Error)mMapper.reread_locked_buffer(const_cast<native_handle_t*>(buffer));
}

int32_t MapperV5Impl::getMetadata(buffer_handle_t buffer, AIMapper_MetadataType metadataType,
                                  void* destBuffer, size_t destBufferSize) {
    if (strcmp(metadataType.name, STANDARD_METADATA_NAME) != 0) {
        SGR_LOGE("Only StandardMetadataType is supported, got %s", metadataType.name);
        return AIMAPPER_ERROR_UNSUPPORTED;
    }
    return getStandardMetadata(buffer, metadataType.value, destBuffer, destBufferSize);
}

int32_t MapperV5Impl::getStandardMetadata(buffer_handle_t buffer, int64_t standardMetadataType,
                                          void* destBuffer, size_t destBufferSize) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    switch (static_cast<StandardMetadataType>(standardMetadataType)) {
        case StandardMetadataType::CTA861_3:
        case StandardMetadataType::SMPTE2086:
        case StandardMetadataType::SMPTE2094_40:
            if (!mMapper.is_registered_handle(buffer)) {
                SGR_LOGE("handle is not imported... failed to get MetadataType(%s)",
                         toString(static_cast<StandardMetadataType>(standardMetadataType)).c_str());
                return AIMAPPER_ERROR_BAD_BUFFER;
            }
            break;
        default:
            break;
    }

    const MetadataManager* metadata_manager = mMapper.get_metadata_manager();

    auto provider = [&]<StandardMetadataType T>(auto&& provide) -> int32_t {
        int retValue = 0;

#define get_metadata(metadataType, name)                                      \
    if constexpr (T == StandardMetadataType::metadataType) {                  \
        StandardMetadata<StandardMetadataType::metadataType>::value_type tmp; \
        metadata_manager->get_##name(buffer, &tmp);                           \
        return provide(tmp);                                                  \
    }

#define get_metadata_with_type(metadataType, name, type)                                       \
    if constexpr (T == StandardMetadataType::metadataType) {                                   \
        type tmp;                                                                              \
        metadata_manager->get_##name(buffer, &tmp);                                            \
        return provide(                                                                        \
                static_cast<StandardMetadata<StandardMetadataType::metadataType>::value_type>( \
                        tmp));                                                                 \
    }

        get_metadata(BUFFER_ID, buffer_id);
        get_metadata(NAME, name);
        get_metadata(WIDTH, width);
        get_metadata(HEIGHT, height);
        get_metadata(LAYER_COUNT, layer_count);
        get_metadata_with_type(PIXEL_FORMAT_REQUESTED, pixel_format_requested, int32_t);
        get_metadata(PIXEL_FORMAT_FOURCC, pixel_format_fourcc);
        get_metadata(PIXEL_FORMAT_MODIFIER, pixel_format_modifier);
        get_metadata_with_type(USAGE, usage, uint64_t);
        get_metadata(ALLOCATION_SIZE, allocation_size);
        get_metadata(PROTECTED_CONTENT, protected_content);
        get_metadata(COMPRESSION, compression);
        get_metadata(INTERLACED, interlaced);
        get_metadata(CHROMA_SITING, chroma_siting);
        get_metadata(PLANE_LAYOUTS, plane_layouts);
        get_metadata(CROP, crops);
        get_metadata_with_type(DATASPACE, dataspace, int32_t);
        get_metadata_with_type(BLEND_MODE, blend_mode, int32_t);
        get_metadata(SMPTE2086, smpte2086);
        get_metadata(CTA861_3, cta861_3);
        get_metadata(SMPTE2094_40, smpte2094_40);
        get_metadata(STRIDE, stride);

        return -AIMAPPER_ERROR_UNSUPPORTED;
    };

    return provideStandardMetadata(static_cast<StandardMetadataType>(standardMetadataType),
                                   destBuffer, destBufferSize, provider);
}

AIMapper_Error MapperV5Impl::setMetadata(buffer_handle_t buffer, AIMapper_MetadataType metadataType,
                                         const void* metadata, size_t metadataSize) {
    if (strcmp(metadataType.name, "android.hardware.graphics.common.StandardMetadataType") != 0) {
        SGR_LOGE("Only StandardMetadataType is supported, got %s", metadataType.name);
        return AIMAPPER_ERROR_UNSUPPORTED;
    }
    return setStandardMetadata(buffer, metadataType.value, metadata, metadataSize);
}

AIMapper_Error MapperV5Impl::setStandardMetadata(buffer_handle_t buffer,
                                                 int64_t standardMetadataType, const void* metadata,
                                                 size_t metadataSize) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    if (metadataSize == 0) return AIMAPPER_ERROR_UNSUPPORTED;

    switch (static_cast<StandardMetadataType>(standardMetadataType)) {
        case StandardMetadataType::CTA861_3:
        case StandardMetadataType::SMPTE2086:
        case StandardMetadataType::SMPTE2094_40:
            if (!mMapper.is_registered_handle(buffer)) {
                SGR_LOGE("handle is not imported... failed to set MetadataType(%s)",
                         toString(static_cast<StandardMetadataType>(standardMetadataType)).c_str());
                return AIMAPPER_ERROR_BAD_BUFFER;
            }
            break;
        default:
            break;
    }

    const MetadataManager* metadata_manager = mMapper.get_metadata_manager();

    auto provider =
            [&]<StandardMetadataType T>(StandardMetadata<T>::value_type value) -> AIMapper_Error {
        int retValue = 0;

#define set_metadata(metadataType, name)                                                          \
    if constexpr (T == StandardMetadataType::metadataType) {                                      \
        return (AIMapper_Error)metadata_manager->set_##name(const_cast<native_handle_t*>(buffer), \
                                                            value);                               \
    }

#define set_metadata_with_type(metadataType, name, type)                                          \
    if constexpr (T == StandardMetadataType::metadataType) {                                      \
        return (AIMapper_Error)metadata_manager->set_##name(const_cast<native_handle_t*>(buffer), \
                                                            static_cast<type>(value));            \
    }

#define move_and_set_metadata(metadataType, name)                                                 \
    if constexpr (T == StandardMetadataType::metadataType) {                                      \
        return (AIMapper_Error)metadata_manager->set_##name(const_cast<native_handle_t*>(buffer), \
                                                            std::move(value));                    \
    }

        set_metadata(PIXEL_FORMAT_FOURCC, pixel_format_fourcc);
        set_metadata(PIXEL_FORMAT_MODIFIER, pixel_format_modifier);
        set_metadata(PIXEL_FORMAT_MODIFIER, pixel_format_modifier);
        set_metadata(ALLOCATION_SIZE, allocation_size);
        set_metadata(PROTECTED_CONTENT, protected_content);
        move_and_set_metadata(COMPRESSION, compression);
        move_and_set_metadata(INTERLACED, interlaced);
        move_and_set_metadata(CHROMA_SITING, chroma_siting);
        move_and_set_metadata(PLANE_LAYOUTS, plane_layouts);
        move_and_set_metadata(CROP, crops);
        set_metadata_with_type(DATASPACE, dataspace, int32_t);
        set_metadata_with_type(BLEND_MODE, blend_mode, int32_t);
        set_metadata(SMPTE2086, smpte2086);
        set_metadata(CTA861_3, cta861_3);
        if constexpr (T == StandardMetadataType::SMPTE2094_40) {
            return (AIMapper_Error)metadata_manager->set_smpte2094_40(
                    const_cast<native_handle_t*>(buffer), value, value == std::nullopt);
        }

        return AIMAPPER_ERROR_UNSUPPORTED;
    };

    return applyStandardMetadata(static_cast<StandardMetadataType>(standardMetadataType), metadata,
                                 metadataSize, provider);
}

AIMapper_Error MapperV5Impl::listSupportedMetadataTypes(
        const AIMapper_MetadataTypeDescription** outDescriptionList,
        size_t* outNumberOfDescriptions) {
    std::vector<AIMapper_MetadataTypeDescription> descriptionList;
    for (const auto& desc : *mMapper.get_metadata_manager()->get_metadata_list()) {
        AIMapper_MetadataTypeDescription description{
                .metadataType = toMetadataType(desc.metadata_type),
                .description = desc.description.c_str(),
                .isGettable = desc.is_gettable,
                .isSettable = desc.is_gettable,
                .reserved = {0}};
        descriptionList.push_back(description);
    }
    *outDescriptionList = descriptionList.data();
    *outNumberOfDescriptions = descriptionList.size();
    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error MapperV5Impl::dumpBuffer(buffer_handle_t bufferHandle,
                                        AIMapper_DumpBufferCallback dumpBufferCallback,
                                        void* context) {
    if (bufferHandle == nullptr) {
        SGR_LOGE("Invalid buffer handle");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    // 10 KiB buffer
    std::vector<char> buffer(10280);
    for (const auto& list_entry : *mMapper.get_metadata_manager()->get_metadata_list()) {
        if (!list_entry.is_gettable) {
            continue;
        }

        if (int32_t size = getStandardMetadata(bufferHandle, list_entry.metadata_type.value,
                                               buffer.data(), buffer.size())) {
            dumpBufferCallback(context, toMetadataType(list_entry.metadata_type), buffer.data(),
                               size);
        }
    }
    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error MapperV5Impl::dumpAllBuffers(
        AIMapper_BeginDumpBufferCallback beginDumpBufferCallback,
        AIMapper_DumpBufferCallback dumpBufferCallback, void* context) {
    AIMapper_Error ret = AIMAPPER_ERROR_NONE;
    {
        std::lock_guard<std::mutex> lock(Mapper::get_imported_handles_lock());
        const std::unordered_set<const native_handle_t*>& mapper_imported_handles =
                Mapper::get_imported_handles();
        for (const auto& buffer : mapper_imported_handles) {
            beginDumpBufferCallback(context);
            AIMapper_Error error = dumpBuffer(buffer, dumpBufferCallback, context);
            SGR_ASSERT_MSG(error != AIMAPPER_ERROR_BAD_BUFFER,
                           "dumpBuffer returned AIMAPPER_ERROR_BAD_BUFFER for native_handle_t = %p",
                           buffer);
            if (error == AIMAPPER_ERROR_NO_RESOURCES) ret = error;
        }
    }
    return ret;
}

AIMapper_Error MapperV5Impl::getReservedRegion(buffer_handle_t buffer, void** outReservedRegion,
                                               uint64_t* outReservedSize) {
    if (buffer == nullptr) return AIMAPPER_ERROR_BAD_BUFFER;

    return (AIMapper_Error)mMapper.get_reserved_region(const_cast<native_handle_t*>(buffer),
                                                       outReservedRegion, outReservedSize);
}

extern "C" uint32_t ANDROID_HAL_STABLEC_VERSION = AIMAPPER_VERSION_5;

AIMapper_Error AIMapper_loadIMapper(AIMapper* _Nullable* _Nonnull outImplementation) {
    static vendor::mapper::IMapperProvider<MapperV5Impl> provider;
    return provider.load(outImplementation);
}
