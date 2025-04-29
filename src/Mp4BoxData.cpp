#include <iostream>
#include "Mp4BoxDataTypes.h"
#include "Mp4BoxDataBasic.h"
#include "Mp4BoxDataArray.h"
#include "Mp4BoxDataKeyValues.h"
#include "Mp4BoxDataTable.h"
#include "Mp4BoxDataBinary.h"

using namespace std;

shared_ptr<Mp4BoxData> newObject(MP4_BOX_DATA_TYPE_E type)
{
    switch (type)
    {
        default:
        case MP4_BOX_DATA_TYPE_SINT:
        case MP4_BOX_DATA_TYPE_UINT:
        case MP4_BOX_DATA_TYPE_REAL:
        case MP4_BOX_DATA_TYPE_STR:
        {
            return make_shared<Mp4BoxDataBasic>(type);
        }
        case MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS:
        {
            return make_shared<Mp4BoxDataKeyValues>();
        }
        case MP4_BOX_DATA_TYPE_ARRAY:
        {
            return make_shared<Mp4BoxDataArray>();
        }
        case MP4_BOX_DATA_TYPE_TABLE:
        {
            return make_shared<Mp4BoxDataTable>();
        }
        case MP4_BOX_DATA_TYPE_BINARY:
        {
            return make_shared<Mp4BoxDataBinary>();
        }
    }
}

ostream &operator<<(ostream &os, shared_ptr<const Mp4BoxData> pobj)
{
    if (pobj != nullptr)
        os << pobj->toString();
    return os;
}

template <typename T>
shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData(T data)
{
    return newObject(data);
}

template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<int8_t>(int8_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<int16_t>(int16_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<int32_t>(int32_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<int64_t>(int64_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<uint8_t>(uint8_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<uint16_t>(uint16_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<uint32_t>(uint32_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<uint64_t>(uint64_t);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<float>(float);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<double>(double);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<std::string>(std::string);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<const char *>(const char *);
template shared_ptr<Mp4BoxData> Mp4BoxData::createBasicData<char>(char);

shared_ptr<Mp4BoxData> Mp4BoxData::createArrayData()
{
    return newObject(MP4_BOX_DATA_TYPE_ARRAY);
}
shared_ptr<Mp4BoxData> Mp4BoxData::createKeyValuePairsData()
{
    return newObject(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS);
}
shared_ptr<Mp4BoxData> Mp4BoxData::createTableData()
{
    return newObject(MP4_BOX_DATA_TYPE_TABLE);
}

shared_ptr<Mp4BoxData> Mp4BoxData::createBinaryData()
{
    return newObject(MP4_BOX_DATA_TYPE_BINARY);
}

std::shared_ptr<Mp4BoxData> Mp4BoxData::createData(MP4_BOX_DATA_TYPE_E dataType)
{
    return newObject(dataType);
}
