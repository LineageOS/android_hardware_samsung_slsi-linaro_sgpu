#include <android/hardware/graphics/mapper/IMapper.h>
#include "core/hidl_defines.h"

using namespace android::samsung::gralloc;

static_assert(Error::NONE == static_cast<Error>(AIMapper_Error::AIMAPPER_ERROR_NONE));
static_assert(Error::BAD_DESCRIPTOR == static_cast<Error>(AIMapper_Error::AIMAPPER_ERROR_BAD_DESCRIPTOR));
static_assert(Error::BAD_BUFFER == static_cast<Error>(AIMapper_Error::AIMAPPER_ERROR_BAD_BUFFER));
static_assert(Error::BAD_VALUE == static_cast<Error>(AIMapper_Error::AIMAPPER_ERROR_BAD_VALUE));
static_assert(Error::NO_RESOURCES == static_cast<Error>(AIMapper_Error::AIMAPPER_ERROR_NO_RESOURCES));
static_assert(Error::UNSUPPORTED == static_cast<Error>(AIMapper_Error::AIMAPPER_ERROR_UNSUPPORTED));


inline Rect toRect(const ARect& inRect) {
    return Rect {
        .left = inRect.left,
        .top = inRect.top,
        .width = inRect.right - inRect.left,
        .height = inRect.bottom - inRect.top
    };
}

inline AIMapper_MetadataType toMetadataType(const MetadataType& in) {
    return AIMapper_MetadataType {
        .name = in.name.c_str(),
        .value = in.value,
    };
}