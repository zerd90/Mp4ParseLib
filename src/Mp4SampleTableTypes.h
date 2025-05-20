#ifndef MP4_SAMPLE_TABLE_H
#define MP4_SAMPLE_TABLE_H

#include <stdint.h>
#include "Mp4ParseTools.h"
#include "Mp4BoxTypes.h"

struct BasicSampleItem
{
    virtual std::shared_ptr<const Mp4BoxData> getData()                  = 0;
    virtual std::shared_ptr<const Mp4BoxData> getData(uint64_t data_idx) = 0;

    virtual void setColumnsName(std::shared_ptr<Mp4BoxData> src) = 0;
};

struct SampleTableBox : public FullBox
{
    uint32_t                                      entryCount = 0; // u32
    std::vector<std::shared_ptr<BasicSampleItem>> entries;

    explicit SampleTableBox(const char *boxTypeStr) : FullBox(boxTypeStr), entryCount(0) {}
    template <typename _T>
    std::shared_ptr<_T> getEntry(uint64_t idx)
    {
        return std::dynamic_pointer_cast<_T>(entries[idx]);
    }

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SampleTableBoxPtr = std::shared_ptr<SampleTableBox>;

struct sttsItem : public BasicSampleItem
{
    uint32_t                          sampleCount = 0; // u32
    uint32_t                          delta       = 0; // u32
    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(sampleCount)->arrayAddItem(delta);
        return res;
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(sampleCount);
            case 1:
                return Mp4BoxData::createBasicData(delta);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override { src->setColumnsName("Sample Count", "Delta"); }
};
struct TimeToSampleBox : public SampleTableBox
{
    TimeToSampleBox() : SampleTableBox("stts") {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using TimeToSampleBoxPtr = std::shared_ptr<TimeToSampleBox>;

struct cttsItem : public BasicSampleItem
{
    uint32_t sampleCount  = 0; // u32
    uint64_t sampleOffset = 0; // u32/s32

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(sampleCount)->arrayAddItem(sampleOffset);
        return res;
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(sampleCount);
            case 1:
                return Mp4BoxData::createBasicData(sampleOffset);
            default:
                return nullptr;
        }
    }
    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        src->setColumnsName("Sample Count", "Sample Offset");
    }
};
struct CompositionOffsetBox : public SampleTableBox
{
    CompositionOffsetBox() : SampleTableBox("ctts") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using CompositionOffsetBoxPtr = std::shared_ptr<CompositionOffsetBox>;

struct stscItem : public BasicSampleItem
{
    uint32_t firstChunk    = 0; // u32
    uint32_t sampleCount   = 0; // u32
    uint32_t sampleDescIdx = 0; // u32

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(firstChunk)->arrayAddItem(sampleCount)->arrayAddItem(sampleDescIdx);
        return res;
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(firstChunk);
            case 1:
                return Mp4BoxData::createBasicData(sampleCount);
            case 2:
                return Mp4BoxData::createBasicData(sampleDescIdx);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        src->setColumnsName("First Chunk", "Sample Count", "Sample Description index");
    }
};
struct SampleToChunkBox : public SampleTableBox
{
    SampleToChunkBox() : SampleTableBox("stsc") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using SampleToChunkBoxPtr = std::shared_ptr<SampleToChunkBox>;

struct stszItem : public BasicSampleItem
{
    uint32_t sampleSize = 0; // u32

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        return Mp4BoxData::createArrayData()->arrayAddItem(sampleSize);
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(sampleSize);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override { src->setColumnsName("Sample Size"); }
};
struct SampleSizeBox : public SampleTableBox
{
    uint32_t defaultSampleSize = 0; // u32

    SampleSizeBox() : SampleTableBox("stsz") {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SampleSizeBoxPtr = std::shared_ptr<SampleSizeBox>;

struct stz2Item : public BasicSampleItem
{
    uint16_t sampleSize = 0; // field_size bits

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        return Mp4BoxData::createArrayData()->arrayAddItem(sampleSize);
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(sampleSize);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override { src->setColumnsName("Sample Size"); }
};
struct CompactSampleSizeBox : public SampleTableBox
{
    uint8_t fieldSize = 0; // u8

    CompactSampleSizeBox() : SampleTableBox("stz2") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using CompactSampleSizeBoxPtr = std::shared_ptr<CompactSampleSizeBox>;

struct stcoItem : public BasicSampleItem
{
    uint32_t chunkOffset = 0; // u32

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        return Mp4BoxData::createArrayData()->arrayAddItem(chunkOffset);
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(chunkOffset);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override { src->setColumnsName("Chunk Offset"); }
};
struct ChunkOffsetBox : public SampleTableBox
{
    explicit ChunkOffsetBox() : SampleTableBox("stco") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using ChunkOffsetBoxPtr = std::shared_ptr<ChunkOffsetBox>;

struct co64Item : public BasicSampleItem
{
    uint64_t chunkOffset = 0; // u64

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        return Mp4BoxData::createArrayData()->arrayAddItem(chunkOffset);
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(chunkOffset);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override { src->setColumnsName("Chunk Offset"); }
};
struct ChunkLargeOffsetBox : public SampleTableBox
{
    explicit ChunkLargeOffsetBox() : SampleTableBox("co64") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using ChunkLargeOffsetBoxPtr = std::shared_ptr<ChunkLargeOffsetBox>;

struct stssItem : public BasicSampleItem
{
    uint64_t sampleNumber = 0; // u32, start from 1

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        return Mp4BoxData::createArrayData()->arrayAddItem(sampleNumber);
    }

    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(sampleNumber);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override { src->setColumnsName("Sample Number"); }
};
struct SyncSampleBox : public SampleTableBox
{
    SyncSampleBox() : SampleTableBox("stss") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using SyncSampleBoxPtr = std::shared_ptr<SyncSampleBox>;

struct sdtpItem : public BasicSampleItem
{
    uint8_t isLeading           = 0; // 2 bits
    uint8_t sampleDependsOn     = 0; // 2 bits
    uint8_t sampleDependedOn    = 0; // 2 bits
    uint8_t sampleHasRedundancy = 0; // 2 bits

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(isLeading)
            ->arrayAddItem(sampleDependsOn)
            ->arrayAddItem(sampleDependedOn)
            ->arrayAddItem(sampleHasRedundancy);
        return res;
    }

    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(isLeading);
            case 1:
                return Mp4BoxData::createBasicData(sampleDependsOn);
            case 2:
                return Mp4BoxData::createBasicData(sampleDependedOn);
            case 3:
                return Mp4BoxData::createBasicData(sampleHasRedundancy);
            default:
                return nullptr;
        }
    }
    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        src->setColumnsName("Is Leading", "Sample Depends On", "Sample Depended On", "Sample has Redundancy");
    }
};
struct SampleDependencyTypeBox : public SampleTableBox
{
    SampleDependencyTypeBox() : SampleTableBox("sdtp") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using SampleDependencyTypeBoxPtr = std::shared_ptr<SampleDependencyTypeBox>;

struct sgpdEntry : public BasicSampleItem
{
    // if (version==1 && default_length==0)
    uint32_t descriptionLength = 0; // u32

    BinaryData description; // descriptionLength

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res        = Mp4BoxData::createArrayData();
        auto binaryData = Mp4BoxData::createBinaryData();
        binaryData->binarySetCallbacks(
            [](const void *userData)
            {
                auto *description = (const BinaryData *)userData;
                return description->length;
            },
            [](uint64_t offset, const void *userData) -> uint8_t
            {
                auto *description = (const BinaryData *)userData;
                if (offset >= description->length)
                    return 0;
                return description->ptr()[offset];
            },
            &description);

        if (0 == descriptionLength)
        {
            return res->arrayAddItem(binaryData);
        }
        else
        {
            return res->arrayAddItem(descriptionLength)->arrayAddItem(binaryData);
        }
    }
    std::shared_ptr<const Mp4BoxData> createDescriptionData()
    {
        auto binaryData = Mp4BoxData::createBinaryData();
        binaryData->binarySetCallbacks(
            [](const void *userData)
            {
                auto *description = (const BinaryData *)userData;
                return description->length;
            },
            [](uint64_t offset, const void *userData) -> uint8_t
            {
                auto *description = (const BinaryData *)userData;
                if (offset >= description->length)
                    return 0;
                return description->ptr()[offset];
            },
            &description);
        return binaryData;
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        if (0 == descriptionLength)
        {
            switch (dataIdx)
            {
                case 0:
                    return createDescriptionData();
                default:
                    return nullptr;
            }
        }
        else
        {
            switch (dataIdx)
            {
                case 0:
                    return Mp4BoxData::createBasicData(descriptionLength);
                case 1:
                    return createDescriptionData();
                default:
                    return nullptr;
            }
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        if (descriptionLength > 0)
            src->setColumnsName("Description Length", "Description");
        else
            src->setColumnsName("Description");
    }
};
struct SampleGroupDescriptionBox : public SampleTableBox
{
    uint32_t groupingType = 0; // u32

    // if (version == 1)
    uint32_t defaultLength = 0; // u32

    // if (version >= 2)
    uint32_t defaultSampleDescIdx = 0; //  u32

    SampleGroupDescriptionBox() : SampleTableBox("sgpd") {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SampleGroupDescriptionBoxPtr = std::shared_ptr<SampleGroupDescriptionBox>;

struct sbgpItem : public BasicSampleItem
{
    uint32_t sampleCount           = 0; // u32
    uint32_t groupDescriptionIndex = 0; // u32

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(sampleCount)->arrayAddItem(groupDescriptionIndex);
        return res;
    }

    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(sampleCount);
            case 1:
                return Mp4BoxData::createBasicData(groupDescriptionIndex);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        src->setColumnsName("Sample Count", "Group Description Index");
    }
};
struct SampleToGroupBox : public SampleTableBox
{
    uint32_t groupingType    = 0; // u32
    // if (version == 1)
    uint32_t groupingTypePar = 0; // u32

    SampleToGroupBox() : SampleTableBox("sbgp") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SampleToGroupBoxPtr = std::shared_ptr<SampleToGroupBox>;

struct elstItem : public BasicSampleItem
{
    uint64_t segmentDuration   = 0; // u64/u32
    int64_t  mediaTime         = 0; // s64/s32
    int16_t  mediaRateInteger  = 0; // s16
    int16_t  mediaRateFraction = 0; // s16

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(segmentDuration)
            ->arrayAddItem(mediaTime)
            ->arrayAddItem(mediaRateInteger)
            ->arrayAddItem(mediaRateFraction);
        return res;
    }

    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(segmentDuration);
            case 1:
                return Mp4BoxData::createBasicData(mediaTime);
            case 2:
                return Mp4BoxData::createBasicData(mediaRateInteger);
            case 3:
                return Mp4BoxData::createBasicData(mediaRateFraction);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        src->setColumnsName("Segment Duration", "Media Time", "Media Rate Integer", "Media Rate Fraction");
    }
};
struct EditListBox : public SampleTableBox
{
    EditListBox() : SampleTableBox("elst") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    // SampleTableBox::getData()
};
using EditListBoxPtr = std::shared_ptr<EditListBox>;

struct trunItem : public BasicSampleItem
{
    uint32_t boxFlags     = 0;
    // if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
    uint32_t duration     = 0;
    // if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
    uint32_t size         = 0;
    // if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT)
    uint32_t flags        = 0;
    // if (mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT)
    uint32_t composOffset = 0;

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
            res->arrayAddItem(duration);
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
            res->arrayAddItem(size);
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT)
            res->arrayAddItem(flags);
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT)
            res->arrayAddItem(composOffset);
        return res;
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        int idx = -1;
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
        {
            idx++;
            if (idx == dataIdx)
                return Mp4BoxData::createBasicData(duration);
        }

        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
        {
            idx++;
            if (idx == dataIdx)
                return Mp4BoxData::createBasicData(size);
        }

        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT)
        {
            idx++;
            if (idx == dataIdx)
                return Mp4BoxData::createBasicData(flags);
        }
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT)
        {
            idx++;
            if (idx == dataIdx)
                return Mp4BoxData::createBasicData(composOffset);
        }

        return nullptr;
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
            src->tableAddColumn("Duration");
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
            src->tableAddColumn("Size");
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT)
            src->tableAddColumn("Flags");
        if (boxFlags & MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT)
            src->tableAddColumn("Sample Composition Time Offset");
    }
};
struct TrackRunBox : public SampleTableBox
{
    // if (mFullboxFlags & MP4_TRUN_FLAG_DATA_OFFSET_PRESENT)
    int32_t  dataOffset       = 0;
    // if (mFullboxFlags & MP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT)
    uint32_t firstSampleFlags = 0;

    TrackRunBox() : SampleTableBox("trun") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using TrackRunBoxPtr = std::shared_ptr<TrackRunBox>;

struct tfraItem : public BasicSampleItem
{
    uint64_t time         = 0; // u64/u32
    uint64_t moofOffset   = 0; // u64/u32
    uint64_t trafNum      = 0; // lenSzTrafNum
    uint64_t trunNum      = 0; // lenSzTrunNum
    uint64_t sampleNumber = 0; // lenSzSampleNum

    std::shared_ptr<const Mp4BoxData> getData() override
    {
        auto res = Mp4BoxData::createArrayData();
        res->arrayAddItem(time)
            ->arrayAddItem(moofOffset)
            ->arrayAddItem(trafNum)
            ->arrayAddItem(trunNum)
            ->arrayAddItem(sampleNumber);
        return res;
    }
    std::shared_ptr<const Mp4BoxData> getData(uint64_t dataIdx) override
    {
        switch (dataIdx)
        {
            case 0:
                return Mp4BoxData::createBasicData(time);
            case 1:
                return Mp4BoxData::createBasicData(moofOffset);
            case 2:
                return Mp4BoxData::createBasicData(trafNum);
            case 3:
                return Mp4BoxData::createBasicData(trunNum);
            case 4:
                return Mp4BoxData::createBasicData(sampleNumber);
            default:
                return nullptr;
        }
    }

    void setColumnsName(std::shared_ptr<Mp4BoxData> src) override
    {
        src->setColumnsName("Time", "Moof Offset", "Traf Number", "Trun Number", "Sample Number");
    }
};
struct TrackFragmentRandomAccessBox : public SampleTableBox
{
    uint32_t trackId        = 0; // u32
    uint8_t  lenSzTrafNum   = 0; // 2 bits
    uint8_t  lenSzTrunNum   = 0; // 2 bits
    uint8_t  lenSzSampleNum = 0; // 2 bits

    TrackFragmentRandomAccessBox() : SampleTableBox("tfra") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using TrackFragmentRandomAccessBoxPtr = std::shared_ptr<TrackFragmentRandomAccessBox>;

#endif // MP4_SAMPLE_TABLE_H