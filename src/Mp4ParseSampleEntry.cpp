
#include "Mp4ParseInternal.h"
#include "Mp4SampleEntryTypes.h"


int SampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    int ret = 0;
    BOX_PARSE_BEGIN();

    // for (int i = 0; i < 6; ++i)
    // {
    // 	g_reserve8=reader.read_u8();
    // }
    reader.skip(6);

    dataReferenceIndex = reader.readU16(true);

    BOX_PARSE_END();
    return ret;
}

std::shared_ptr<Mp4BoxData> SampleEntry::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Data Reference Index", dataReferenceIndex);
    return item;
}

int VisualSampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    char cCompressorName[32] = {0};
    BOX_PARSE_BEGIN();

    CHECK_RET(SampleEntry::parse(reader, boxPosition, boxSize, boxBodySize));

    // g_reserve8 = reader.read_u16(true);
    // g_reserve8 = reader.read_u16(true);
    // for (int i = 0; i < 3; ++i)
    // {
    // 	g_reserve8 = reader.read_u32(true);
    // }
    reader.skip(16);

    width          = reader.readU16(true);
    height         = reader.readU16(true);
    horizontalReso = reader.readU32(true);
    verticalReso   = reader.readU32(true);

    // g_reserve8 = reader.read_u32(true);
    reader.skip(4);

    frameCount = reader.readU16(true);

    uint8_t nameLen = reader.readU8();
    reader.readData(cCompressorName, 32, nameLen, 0);
    this->compressorName = cCompressorName;
    reader.skip(32 - 1 - nameLen);

    depth = reader.readU16(true);

    // g_reserve8 = reader.read_u16(true);
    reader.skip(2);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> VisualSampleEntry::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = SampleEntry::getData(src);
    item->kvAddPair("Width", width)
        ->kvAddPair("Height", height)
        ->kvAddPair("Horizon Reso", horizontalReso)
        ->kvAddPair("Vertical Reso", verticalReso)
        ->kvAddPair("Frame Count", frameCount)
        ->kvAddPair("Compressor Name", compressorName)
        ->kvAddPair("Depth", depth);

    return item;
}

int AudioSampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    int ret = 0;
    BOX_PARSE_BEGIN();

    CHECK_RET(SampleEntry::parse(reader, boxPosition, boxSize, boxBodySize));

    // for (int i = 0; i < 2; ++i)
    // {
    // 	g_reserve8 = reader.read_u32(true);
    // }
    reader.skip(8);

    channelCount = reader.readU16(true);
    sampleSize   = reader.readU16(true);

    // g_reserve8 = reader.read_u16(true);
    // g_reserve8 = reader.read_u16(true);
    reader.skip(4);

    sampleRate = reader.readU32(true) >> 16;

    BOX_PARSE_END();
    return ret;
}

std::shared_ptr<Mp4BoxData> AudioSampleEntry::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = SampleEntry::getData(src);
    item->kvAddPair("Channel Count", channelCount)
        ->kvAddPair("Sample Size", sampleSize)
        ->kvAddPair("Sample Rate", sampleRate);

    return item;
}
