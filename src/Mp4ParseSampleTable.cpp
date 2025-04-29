#include "Mp4ParseInternal.h"
#include "Mp4SampleTableTypes.h"

#define READ_ENTRIES_BEGIN(entry_type)            \
    entryCount = reader.readU32(true);            \
    entries.reserve(entryCount);                  \
    for (unsigned int i = 0; i < entryCount; i++) \
    {                                             \
        auto entry = make_shared<entry_type>();

#define READ_ENTRIES_ITEM(field, data_type) entry->field = reader.read##data_type(true);

#define READ_ENTRIES_ITEM_CASE(field, case, data_type1, data_type2) \
    if (case)                                                       \
    {                                                               \
        entry->field = reader.read##data_type1(true);               \
    }                                                               \
    else                                                            \
    {                                                               \
        entry->field = reader.read##data_type2(true);               \
    }

#define READ_ENTRIES_ITEM_UNSIGNED(field, len) entry->field = reader.readUnsigned(len, true);

#define READ_ENTRIES_END()    \
    entries.push_back(entry); \
    }

using std::make_shared;

// static uint64_t g_reserve8;

std::shared_ptr<Mp4BoxData> SampleTableBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::string                 tableName = "Entry";
    std::shared_ptr<Mp4BoxData> item      = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair(tableName + " Count", entryCount);
    std::shared_ptr<Mp4BoxData> entryTable = item->kvAddKey(tableName + "s", MP4_BOX_DATA_TYPE_TABLE);
    if (entries.size() > 0)
    {
        entries[0]->setColumnsName(entryTable);
    }
    entryTable->tableSetCallbacks(
        [](const void *userData)
        {
            std::vector<std::shared_ptr<BasicSampleItem>> *pEntries =
                (std::vector<std::shared_ptr<BasicSampleItem>> *)userData;
            return pEntries->size();
        },
        [](const void *userData, uint64_t rowIdx)
        {
            std::vector<std::shared_ptr<BasicSampleItem>> *pEntries =
                (std::vector<std::shared_ptr<BasicSampleItem>> *)userData;
            return (*pEntries)[rowIdx]->getData();
        },
        [](const void *userData, uint64_t rowIdx, uint64_t colIdx)
        {
            std::vector<std::shared_ptr<BasicSampleItem>> *pEntries =
                (std::vector<std::shared_ptr<BasicSampleItem>> *)userData;
            return (*pEntries)[rowIdx]->getData(colIdx);
        },
        &entries);
    return item;
}

int TimeToSampleBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(sttsItem)
    READ_ENTRIES_ITEM(sampleCount, U32)
    READ_ENTRIES_ITEM(delta, U32)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

int CompositionOffsetBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(cttsItem)
    READ_ENTRIES_ITEM(sampleCount, U32)
    READ_ENTRIES_ITEM_CASE(sampleOffset, 0 == mFullboxVersion, U32, S32)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

int SampleToChunkBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(stscItem)
    READ_ENTRIES_ITEM(firstChunk, U32)
    READ_ENTRIES_ITEM(sampleCount, U32)
    READ_ENTRIES_ITEM(sampleDescIdx, U32)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

int SampleSizeBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    defaultSampleSize = reader.readU32(true);

    entryCount = reader.readU32(true);

    if (0 == defaultSampleSize)
    {
        entries.reserve(entryCount);
        for (unsigned int i = 0; i < entryCount; ++i)
        {
            auto stsz_entry        = make_shared<stszItem>();
            stsz_entry->sampleSize = reader.readU32(true);
            entries.push_back(stsz_entry);
        }
    }

    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> SampleSizeBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Default Sample Size", defaultSampleSize);
    SampleTableBox::getData(item);
    return item;
}

int CompactSampleSizeBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    // g_reserve8 = reader.read_un(3, false);
    reader.skip(12);

    fieldSize = reader.readU8();

    if (fieldSize != 4 && fieldSize != 8 && fieldSize != 16)
    {
        MP4_ERR("field size err %d\n", fieldSize);
        reader.setCursor(last);
        return -1;
    }

    entryCount = reader.readU32(true);
    entries.reserve(entryCount);
    for (unsigned int i = 0; i < entryCount; ++i)
    {
        auto     stz2Entry = make_shared<stz2Item>();
        uint16_t sampleSize;
        uint8_t  twoEntry = 0;
        if (4 == fieldSize)
        {
            reader.read(&twoEntry, 1);
            sampleSize = twoEntry >> 4;
        }
        else
        {
            sampleSize = static_cast<uint16_t>(reader.readUnsigned(fieldSize / 8, true));
        }

        stz2Entry->sampleSize = sampleSize;
        entries.push_back(stz2Entry);

        if (4 == fieldSize && i + 1 < entryCount)
        {
            sampleSize            = (twoEntry & 0x0f);
            stz2Entry             = make_shared<stz2Item>();
            stz2Entry->sampleSize = sampleSize;
            entries.push_back(stz2Entry);
            ++i;
        }
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> CompactSampleSizeBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Field Size", fieldSize);
    SampleTableBox::getData(item);
    return item;
}

int SampleDependencyTypeBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                   uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (0 == entryCount) // set after stbl is parsed, see parse_box()
    {
        MP4_INFO("wait for stsz or stz2\n");
        reader.setCursor(last);
        return 0;
    }

    entries.reserve(entryCount);
    for (unsigned int i = 0; i < entryCount; ++i)
    {
        uint8_t compact1;
        auto    sdtpEntry = make_shared<sdtpItem>();
        reader.read(&compact1, 1);

        BitsReader bitsReader(&compact1, 1);
        sdtpEntry->isLeading           = (uint8_t)bitsReader.readBit(2);
        sdtpEntry->sampleDependsOn     = (uint8_t)bitsReader.readBit(2);
        sdtpEntry->sampleDependedOn    = (uint8_t)bitsReader.readBit(2);
        sdtpEntry->sampleHasRedundancy = (uint8_t)bitsReader.readBit(2);

        entries.push_back(sdtpEntry);
    }

    BOX_PARSE_END();

    return 0;
}

int ChunkOffsetBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(stcoItem)
    READ_ENTRIES_ITEM(chunkOffset, U32)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

int ChunkLargeOffsetBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(co64Item)
    READ_ENTRIES_ITEM(chunkOffset, U64)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

int SyncSampleBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(stssItem)
    READ_ENTRIES_ITEM(sampleNumber, U32)
    READ_ENTRIES_END()

    BOX_PARSE_END();
    return 0;
}

int SampleGroupDescriptionBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                     uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (0 == mFullboxVersion)
    {
        MP4_WARN("SGPD version 0 not available, skip\n");
        reader.setCursor(last);
        return 0;
    }

    groupingType = reader.readU32(true);

    if (mFullboxVersion == 1)
    {
        defaultLength = reader.readU32(true);
    }

    if (mFullboxVersion >= 2)
    {
        defaultSampleDescIdx = reader.readU32(true);
    }

    entryCount = reader.readU32(true);
    entries.reserve(entryCount);
    for (unsigned int i = 0; i < entryCount; i++)
    {
        auto entry = make_shared<sgpdEntry>();

        uint32_t length;
        if (mFullboxVersion == 1 && defaultLength == 0)
        {
            length                   = reader.readU32(true);
            entry->descriptionLength = length;
        }
        else
        {
            length = defaultLength;
        }

        entry->description.create(length);
        reader.read(entry->description.ptr(), length);

        entries.push_back(entry);
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> SampleGroupDescriptionBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Grouping Type", groupingType);
    if (1 == mFullboxVersion)
        item->kvAddPair("Default Length", defaultLength);
    if (mFullboxVersion >= 2)
        item->kvAddPair("Default Sample Description Index", defaultSampleDescIdx);

    SampleTableBox::getData(item);

    return item;
}

int SampleToGroupBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{

    BOX_PARSE_BEGIN();

    groupingType = reader.readU32(true);

    if (mFullboxVersion == 1)
    {
        groupingTypePar = reader.readU32(true);
    }

    READ_ENTRIES_BEGIN(sbgpItem)
    READ_ENTRIES_ITEM(sampleCount, U32)
    READ_ENTRIES_ITEM(groupDescriptionIndex, U32)
    READ_ENTRIES_END()

    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> SampleToGroupBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Grouping Type", groupingType);
    if (1 == mFullboxVersion)
        item->kvAddPair("Grouping Type Parameter", groupingTypePar);

    SampleTableBox::getData(item);

    return item;
}

int EditListBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{

    BOX_PARSE_BEGIN();

    READ_ENTRIES_BEGIN(elstItem)
    READ_ENTRIES_ITEM_CASE(segmentDuration, 1 == mFullboxVersion, U64, U32)
    READ_ENTRIES_ITEM_CASE(mediaTime, 1 == mFullboxVersion, S64, S32)
    READ_ENTRIES_ITEM(mediaRateInteger, S16)
    READ_ENTRIES_ITEM(mediaRateFraction, S16)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

int TrackRunBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{

    BOX_PARSE_BEGIN();

    entryCount = reader.readU32(true);

    if (mFullboxFlags & MP4_TRUN_FLAG_DATA_OFFSET_PRESENT)
    {
        dataOffset = reader.readS32(true);
    }

    if (mFullboxFlags & MP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT)
    {
        firstSampleFlags = reader.readU32(true);
    }

    for (unsigned int i = 0; i < entryCount; ++i)
    {
        auto trun_sample = std::make_shared<trunItem>();

        if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
        {
            trun_sample->duration = reader.readU32(true);
        }
        if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
        {
            trun_sample->size = reader.readU32(true);
        }
        if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT)
        {
            trun_sample->flags = reader.readU32(true);
        }
        if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT)
        {
            if (1 == mFullboxVersion)
            {
                trun_sample->composOffset = reader.readS32(true);
            }
            else
            {
                trun_sample->composOffset = reader.readU32(true);
            }
        }
        entries.push_back(trun_sample);
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> TrackRunBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Data Offset", dataOffset)->kvAddPair("First Sample Flags", firstSampleFlags);

    SampleTableBox::getData(item);

    return item;
}

int TrackFragmentRandomAccessBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                        uint64_t boxBodySize)
{
    uint32_t compData;

    BOX_PARSE_BEGIN();

    trackId = reader.readU32(true);

    compData = reader.readU32(true);
    BitsReader bitsReader(&compData, 4);

    bitsReader.readBit(26);
    lenSzTrafNum   = (uint8_t)bitsReader.readBit(2);
    lenSzTrunNum   = (uint8_t)bitsReader.readBit(2);
    lenSzSampleNum = (uint8_t)bitsReader.readBit(2);

    READ_ENTRIES_BEGIN(tfraItem)
    READ_ENTRIES_ITEM_CASE(time, 1 == mFullboxVersion, U64, U32)
    READ_ENTRIES_ITEM_CASE(moofOffset, 1 == mFullboxVersion, U64, U32)
    READ_ENTRIES_ITEM_UNSIGNED(trafNum, lenSzTrafNum + 1)
    READ_ENTRIES_ITEM_UNSIGNED(trunNum, lenSzTrunNum + 1)
    READ_ENTRIES_ITEM_UNSIGNED(sampleNumber, lenSzSampleNum + 1)
    READ_ENTRIES_END()

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> TrackFragmentRandomAccessBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Track ID", trackId)
        ->kvAddPair("Length Size of Traf Number", lenSzTrafNum)
        ->kvAddPair("Length Size of Trun Number", lenSzTrunNum)
        ->kvAddPair("Length Size of Sample Number", lenSzSampleNum);

    SampleTableBox::getData(item);

    return item;
}
