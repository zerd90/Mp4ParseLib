
#include "Mp4SampleEntryTypes.h"
#include "Mp4ParseInternal.h"

using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;

map<ES_DESCRIPTOR_TAG_E, string> gDescriptorTypeString = {
    {          MP4ODescrTag,           "MP4ODescr"}, // Tag 0x01
    {         MP4IODescrTag,          "MP4IODescr"}, // Tag 0x02
    {         MP4ESDescrTag,          "MP4ESDescr"}, // Tag 0x03
    {  MP4DecConfigDescrTag,   "MP4DecConfigDescr"}, // Tag 0x04
    {MP4DecSpecificDescrTag, "MP4DecSpecificDescr"}, // Tag 0x05
    {         MP4SLDescrTag,          "MP4SLDescr"}, // Tag 0x06
};

string getDescriptorString(ES_DESCRIPTOR_TAG_E tag)
{
    if (gDescriptorTypeString.find(tag) != gDescriptorTypeString.end())
        return gDescriptorTypeString[tag];
    else
        return "Unknown";
}

uint32_t getDescriptorLength(BinaryFileReader &reader)
{
    uint8_t  clen;
    uint32_t length = 0;
    for (int i = 0; i < 4; ++i)
    {
        reader.read(&clen, 1);
        length = (length << 7) | (clen & 0x7f);
        if ((clen & 0x80) == 0)
        {
            break;
        }
    }
    return length;
}

shared_ptr<ESDescriptor> getDescriptor(BinaryFileReader &reader, uint64_t last)
{
    uint64_t                 ret;
    uint8_t                  tag = UnKnownTag;
    shared_ptr<ESDescriptor> curDesc;

    if (reader.getCursorPos() + 2 > last)
        return curDesc;

    ret = reader.read(&tag, 1);
    if (0 == ret)
    {
        MP4_ERR("read err %" PRIu64 "\n", ret);
        return curDesc;
    }

    switch (tag)
    {
        case MP4ODescrTag:
            curDesc = make_shared<MP4ODescr>((ES_DESCRIPTOR_TAG_E)tag);
            break;
        case MP4IODescrTag:
            curDesc = make_shared<MP4IODescr>((ES_DESCRIPTOR_TAG_E)tag);
            break;
        case MP4ESDescrTag:
            curDesc = make_shared<MP4ESDescr>((ES_DESCRIPTOR_TAG_E)tag);
            break;
        case MP4DecConfigDescrTag:
            curDesc = make_shared<MP4DecConfigDescr>((ES_DESCRIPTOR_TAG_E)tag);
            break;
        case MP4DecSpecificDescrTag:
            curDesc = make_shared<MP4DecSpecificDescr>((ES_DESCRIPTOR_TAG_E)tag);
            break;
        case MP4SLDescrTag:
            curDesc = make_shared<MP4SLDescr>((ES_DESCRIPTOR_TAG_E)tag);
            break;
        default:
            curDesc = make_shared<ESDescriptor>((ES_DESCRIPTOR_TAG_E)tag);
            MP4_WARN("unknown descriptor %#x\n", tag);
            return curDesc;
            break;
    }

    curDesc->getLength(reader);

    MP4_INFO("get descriptor %s(code %#x) pos %#" PRIx64 "(%" PRIu64 "), size %#" PRIx64 "(%" PRIu64 ")\n",
             getDescriptorString((ES_DESCRIPTOR_TAG_E)tag).c_str(), tag, curDesc->descPos, curDesc->descPos,
             curDesc->descSize, curDesc->descSize);

    return curDesc;
}

shared_ptr<ESDescriptor> parseDescriptor(BinaryFileReader &reader, uint64_t last)
{
    shared_ptr<ESDescriptor> desc = getDescriptor(reader, last);
    if (desc == nullptr)
        return desc;

    desc->parse(reader, last);

    desc->subDesc = parseDescriptor(reader, last);
    if (desc->subDesc != nullptr)
    {
        desc->subDesc->upper_descr = desc;
    }

    return desc;
}

int ESDBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    descriptor = parseDescriptor(reader, last);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> ESDBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    std::shared_ptr<ESDescriptor> pdesc = descriptor;

    while (pdesc != nullptr)
    {
        std::shared_ptr<Mp4BoxData> descItem = item->kvAddKey(
            getDescriptorString((ES_DESCRIPTOR_TAG_E)pdesc->descTag).c_str(), MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS);
        pdesc->getData(descItem);
        pdesc = pdesc->subDesc;
    }

    return item;
}

int MP4AudioSampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{

    BOX_PARSE_BEGIN();

    CHECK_RET(AudioSampleEntry::parse(reader, boxPosition, boxSize, boxBodySize));

    BOX_PARSE_END();
    return 0;
}

void ESDescriptor::getLength(BinaryFileReader &reader)
{
    descPos    = reader.getCursorPos() - 1;
    descLength = getDescriptorLength(reader);
    descSize   = descLength + reader.getCursorPos() - descPos;
}

int MP4ODescr::parse(BinaryFileReader &reader, uint64_t last)
{
    MP4_UNUSED(last);
    int idFlags;
    if (descLength < 2)
        return 0;
    reader.read(&idFlags, 2);
    if (!(idFlags & 0x0020)) // URL_Flag
    {
        // ES_Descriptor[]
    }

    return 0;
}

int MP4IODescr::parse(BinaryFileReader &reader, uint64_t last)
{
    MP4_UNUSED(last);
    reader.read(nullptr, 2); // ID
    reader.read(nullptr, 1);
    reader.read(nullptr, 1);
    reader.read(nullptr, 1);
    reader.read(nullptr, 1);
    reader.read(nullptr, 1);

    return 0;
}

#define ES_DESCRIPTOR_READ(ptr, varLen, parLen, reverse, last)         \
    do                                                                 \
    {                                                                  \
        if (reader.getCursorPos() + varLen > last)                     \
            return -1;                                                 \
        uint64_t _ret = reader.readData(ptr, varLen, parLen, reverse); \
        if (0 == _ret)                                                 \
            return -1;                                                 \
    } while (0)

int MP4ESDescr::parse(BinaryFileReader &reader, uint64_t last)
{
    MP4_UNUSED(last);
    uint64_t descLast = reader.getCursorPos() + descLength;

    ES_DESCRIPTOR_READ(&esId, 2, 2, true, descLast);

    ES_DESCRIPTOR_READ(&esFlag, 1, 1, true, descLast);

    if (esFlag & 0x80) // streamDependenceFlag
        reader.read(nullptr, 2);
    if (esFlag & 0x40) // URL_Flag
    {
        uint8_t len;
        reader.read(&len, 1);
        reader.read(nullptr, len);
    }
    if (esFlag & 0x20) // OCRstreamFlag
        reader.read(nullptr, 2);

    return 0;
}

std::shared_ptr<Mp4BoxData> MP4ESDescr::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    item->kvAddPair("ES ID", esId)->kvAddPair("ES flag", esFlag);

    return item;
}

int MP4DecConfigDescr::parse(BinaryFileReader &reader, uint64_t last)
{
    MP4_UNUSED(last);
    uint64_t descLast = reader.getCursorPos() + descLength;

    ES_DESCRIPTOR_READ(&objectType, 1, 1, false, descLast);

    ES_DESCRIPTOR_READ(&streamType, 1, 1, false, descLast);

    ES_DESCRIPTOR_READ(&bufferSizeDb, 4, 3, true, descLast);

    ES_DESCRIPTOR_READ(&maxBitrate, 4, 4, true, descLast);

    ES_DESCRIPTOR_READ(&avgBitrate, 4, 4, true, descLast);

    return 0;
}

std::shared_ptr<Mp4BoxData> MP4DecConfigDescr::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    item->kvAddPair("Object Type", objectType)
        ->kvAddPair("Stream Type", streamType)
        ->kvAddPair("Buffer Size", bufferSizeDb)
        ->kvAddPair("Max Bitrate", maxBitrate)
        ->kvAddPair("Average Bitrate", avgBitrate);

    return item;
}

int MP4DecSpecificDescr::parse(BinaryFileReader &reader, uint64_t last)
{
    MP4_UNUSED(last);

    specificCfg.create(descLength);
    reader.read(specificCfg.ptr(), descLength);

    return 0;
}

int MP4SLDescr::parse(BinaryFileReader &reader, uint64_t last)
{
    MP4_UNUSED(last);

    SldData.create(descLength);

    reader.read(SldData.ptr(), descLength);

    return 0;
}

int MP4VisualSampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    CHECK_RET(VisualSampleEntry::parse(reader, boxPosition, boxSize, boxBodySize));

    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> MP4VisualSampleEntry::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    return VisualSampleEntry::getData(item);
}
