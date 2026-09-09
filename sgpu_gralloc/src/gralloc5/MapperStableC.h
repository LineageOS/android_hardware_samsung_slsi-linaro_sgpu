#include <android/hardware/graphics/mapper/IMapper.h>
#include <android/hardware/graphics/mapper/utils/IMapperProvider.h>

#include "core/mapper.h"
#include "interface/g_private_handle_interface.h"

class MapperV5Impl final : public vendor::mapper::IMapperV5Impl {
  public:
    AIMapper_Error importBuffer(const native_handle_t* handle,
                                buffer_handle_t* outBufferHandle) override;
    AIMapper_Error freeBuffer(buffer_handle_t buffer) override;
    AIMapper_Error getTransportSize(buffer_handle_t buffer, uint32_t* outNumFds,
                                    uint32_t* outNumInts) override;
    AIMapper_Error lock(buffer_handle_t buffer, uint64_t cpuUsage, ARect accessRegion,
                        int acquireFence, void** outData) override;
    AIMapper_Error unlock(buffer_handle_t buffer, int* releaseFence) override;
    AIMapper_Error flushLockedBuffer(buffer_handle_t buffer) override;
    AIMapper_Error rereadLockedBuffer(buffer_handle_t buffer) override;
    int32_t getMetadata(buffer_handle_t buffer, AIMapper_MetadataType metadataType,
                        void* destBuffer, size_t destBufferSize) override;
    int32_t getStandardMetadata(buffer_handle_t buffer, int64_t standardMetadataType,
                                void* destBuffer, size_t destBufferSize) override;
    AIMapper_Error setMetadata(buffer_handle_t buffer, AIMapper_MetadataType metadataType,
                               const void* metadata, size_t metadataSize) override;
    AIMapper_Error setStandardMetadata(buffer_handle_t buffer, int64_t standardMetadataType,
                                       const void* metadata, size_t metadataSize) override;
    AIMapper_Error listSupportedMetadataTypes(
            const AIMapper_MetadataTypeDescription** outDescriptionList,
            size_t* outNumberOfDescriptions) override;
    AIMapper_Error dumpBuffer(buffer_handle_t bufferHandle,
                              AIMapper_DumpBufferCallback dumpBufferCallback,
                              void* context) override;
    AIMapper_Error dumpAllBuffers(AIMapper_BeginDumpBufferCallback beginDumpBufferCallback,
                                  AIMapper_DumpBufferCallback dumpBufferCallback,
                                  void* context) override;
    AIMapper_Error getReservedRegion(buffer_handle_t buffer, void** outReservedRegion,
                                     uint64_t* outReservedSize) override;

  private:
    android::samsung::gralloc::Mapper mMapper;
};