
#include <inttypes.h>
#include "Mp4BoxDataBasic.h"

using namespace std;

int64_t Mp4BoxDataBasic::basicGetValueS64() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_SINT);

    return objectValue.s64;
}

int32_t Mp4BoxDataBasic::basicGetValueS32() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_SINT);

    return static_cast<int32_t>(objectValue.s64);
}

int16_t Mp4BoxDataBasic::basicGetValueS16() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_SINT);

    return static_cast<int16_t>(objectValue.s64);
}

int8_t Mp4BoxDataBasic::basicGetValueS8() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_SINT);

    return static_cast<int8_t>(objectValue.s64);
}

uint64_t Mp4BoxDataBasic::basicGetValueU64() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_UINT);

    return objectValue.u64;
}

uint32_t Mp4BoxDataBasic::basicGetValueU32() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_UINT);

    return static_cast<uint32_t>(objectValue.u64);
}

uint16_t Mp4BoxDataBasic::basicGetValueU16() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_UINT);

    return static_cast<uint16_t>(objectValue.u64);
}

uint8_t Mp4BoxDataBasic::basicGetValueU8() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_UINT);

    return static_cast<uint8_t>(objectValue.u64);
}

double Mp4BoxDataBasic::basicGetValueReal() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_REAL);

    return objectValue.f64;
}

string Mp4BoxDataBasic::basicGetValueStr() const
{
    assert(mObjectType == MP4_BOX_DATA_TYPE_STR);

    return objectValue.str;
}

#define RETURN_STR(fmt, ...)                                    \
    do                                                          \
    {                                                           \
        char _strBuf[128];                                      \
        snprintf(_strBuf, sizeof(_strBuf), fmt, ##__VA_ARGS__); \
        return string(_strBuf);                                 \
    } while (0)

string Mp4BoxDataBasic::toString() const
{
    switch (mObjectType)
    {
        case MP4_BOX_DATA_TYPE_SINT:
            if (bhex)
                RETURN_STR("0x%" PRIx64, objectValue.s64);
            else
                return std::to_string(objectValue.s64);
        case MP4_BOX_DATA_TYPE_UINT:
            if (bhex)
                RETURN_STR("0x%" PRIx64, objectValue.u64);
            else
                return std::to_string(objectValue.u64);
        case MP4_BOX_DATA_TYPE_REAL:
            return std::to_string(objectValue.f64);
        case MP4_BOX_DATA_TYPE_STR:
            return objectValue.str;
        default:
            return string();
    }
}

std::string Mp4BoxDataBasic::toHexString() const
{
    switch (mObjectType)
    {
        case MP4_BOX_DATA_TYPE_SINT:
            RETURN_STR("0x%" PRIx64, objectValue.s64);
        case MP4_BOX_DATA_TYPE_UINT:
            RETURN_STR("0x%" PRIx64, objectValue.u64);
        case MP4_BOX_DATA_TYPE_REAL:
            return std::to_string(objectValue.f64);
        case MP4_BOX_DATA_TYPE_STR:
            return objectValue.str;
        default:
            return string();
    }
}
