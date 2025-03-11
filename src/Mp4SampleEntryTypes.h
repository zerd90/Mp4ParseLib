#ifndef PARSE_SAMPLE_ENTRY_H
#define PARSE_SAMPLE_ENTRY_H

#include <map>
#include <stdint.h>

#include "Mp4ParseTools.h"
#include "Mp4Types.h"
#include "Mp4BoxTypes.h"

struct SampleEntry : public CommonBox
{
    /* uint8_t reserved[6]; */
    uint16_t dataReferenceIndex = 0; // u16

    explicit SampleEntry(uint32_t boxType) : CommonBox(boxType) {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;
    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SampleEntryPtr = std::shared_ptr<SampleEntry>;

struct ColourInformationBox : public CommonBox
{
    uint32_t colorType = 0; // u32 "nclx", "rICC", "prof"

    // for "nclx"
    uint16_t colorPrimaries  = 0; // u16
    uint16_t transferCharact = 0; // u16
    uint16_t matrixCoeff     = 0; // u16
    uint8_t  fullRangeFlag   = 0; // 1bit
    /* bit(7) reserved = 0; */

    // for "rICC", "prof"
    BinaryData iccProfile; // see ISO/IEC 14496-12:2015(E) 12.1.5.3

    explicit ColourInformationBox() : CommonBox("colr") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using ColourInformationBoxPtr = std::shared_ptr<ColourInformationBox>;

struct VisualSampleEntry : public SampleEntry // ISO/IEC 14496-12
{
    /* uint16_t pre_defined = 0; */
    /* uint16_t reserved1 = 0; */
    /* uint32_t pre_defined2[3] = {0}; */
    uint16_t    width          = 0; // u16
    uint16_t    height         = 0; // u16
    uint32_t    horizontalReso = 0; // u32
    uint32_t    verticalReso   = 0; // u32
    /* uint32_t reserved2 = 0; */
    uint16_t    frameCount     = 0; // u16
    std::string compressorName;     // string
    uint16_t    depth = 0;          // u16
    /* int16_t pre_defined3 = -1; */

    explicit VisualSampleEntry(uint32_t entry_type) : SampleEntry(entry_type) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using VisualSampleEntryPtr = std::shared_ptr<VisualSampleEntry>;

struct AudioSampleEntry : public SampleEntry // ISO/IEC 14496-12
{
    // uint32_t reserved1[2] = {0};
    uint16_t channelCount = 0; // uint16
    uint16_t sampleSize   = 0; // uint16
    // uint16_t pre_defined = 0;
    // uint16_t reserved3 = 0;
    uint32_t sampleRate   = 0; // u32  {default sample rate of media}<<16

    explicit AudioSampleEntry(uint32_t entry_type) : SampleEntry(entry_type) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using AudioSampleEntryPtr = std::shared_ptr<AudioSampleEntry>;

struct HEVCDecoderConfigurationRecord // ISO/IEC 14496-15
{
    uint8_t  configVersion                   = 0; // uint8
    uint8_t  generalProfileSpace             = 0; // 2 bits
    uint8_t  generalTierFlag                 = 0; // 1 bits
    uint8_t  generalProfileIdc               = 0; // 5 bits
    uint32_t generalProfileCompatFlags       = 0; // u32
    uint64_t generalConstraintIndicatorFlags = 0; // 48 bits
    uint8_t  generalLevelIdc                 = 0; // u8
    // bit(4) reserved = '1111'b;
    uint16_t minSpatialSegmentIdc            = 0; // 12 bits
    // bit(6) reserved = '111111'b;
    uint8_t  parallelismType                 = 0; // 2 bits
    // bit(6) reserved = '111111'b;
    uint8_t  chromaFormatIdc                 = 0; // 2 bits
    // bit(5) reserved = '11111'b;
    uint8_t  bitDepthLuma                    = 0; // 3 bits
    // bit(5) reserved = '11111'b;
    uint8_t  bitDepthChroma                  = 0; // 3 bits
    uint16_t avgFrameRate                    = 0; // u16
    uint8_t  constFrameRate                  = 0; // 2 bits
    uint8_t  numTemporalLayers               = 0; // 3 bits
    uint8_t  temporalIdNested                = 0; // 1 bits
    uint8_t  lengthSize                      = 0; // 2 bits
    uint8_t  arrayCount                      = 0; // u8
    struct NaluItem
    {
        uint8_t arrayComplete = 0; // 1 bits
        // bit(1) reserved = 0;
        uint8_t naluType      = 0; // 6 bits

        uint16_t numNalus = 0; // u16
        struct Nalu
        {
            uint16_t   length = 0; // u16
            BinaryData data;
        };
        std::vector<Nalu> nalus;
    };
    std::vector<NaluItem> arrays;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const;
};
struct HEVCConfigurationBox : public CommonBox // ISO/IEC 14496-15
{
public:
    HEVCDecoderConfigurationRecord HEVCConfig;

public:
    HEVCConfigurationBox() : CommonBox("hvcC") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using HEVCConfigurationBoxPtr = std::shared_ptr<HEVCConfigurationBox>;

struct HEVCSampleEntry : public VisualSampleEntry // ISO/IEC 14496-15 "hvc1", "hev1"
{
    explicit HEVCSampleEntry(uint32_t entry_type) : VisualSampleEntry(entry_type) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override
    {
        return VisualSampleEntry::getData(src);
    }
};
using HEVCSampleEntryPtr = std::shared_ptr<HEVCSampleEntry>;

struct AVCDecoderConfigurationRecord
{
    uint8_t configVersion        = 0; // uint8
    uint8_t profileCompat        = 0; // uint8
    uint8_t avcProfileIndication = 0; // uint8
    uint8_t avcLvIndication      = 0; // uint8
    /* bit(6) reserved = '111111' b; */
    uint8_t lengthSize           = 0; // 2bits
    /* bit(3) reserved = '111' b; */

    struct Nalu
    {
        uint16_t   length = 0;
        BinaryData data;
    };

    uint8_t           spsCount = 0; // 5 bits
    std::vector<Nalu> sps;

    uint8_t           ppsCount = 0; // uint8
    std::vector<Nalu> pps;

    // if (AVCProfileIndication == 100 || AVCProfileIndication == 110 ||AVCProfileIndication == 122 ||
    // AVCProfileIndication == 144)
    /* bit(6) reserved = '111111' b; */
    uint8_t           chromaFormat   = 0; // 2 bits
    /*  bit(5) reserved = '11111' b; */
    uint8_t           bitDepthLuma   = 0; // 3 bits
    /*  bit(5) reserved = '11111' b; */
    uint8_t           bitDepthChroma = 0; // 3 bits
    uint8_t           spseCount      = 0; // uint8
    std::vector<Nalu> spse;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const;
};
struct AVCConfigurationBox : public CommonBox // ISO/IEC 14496-15 avc Configuration
{
    AVCDecoderConfigurationRecord AVCConfig;

    AVCConfigurationBox() : CommonBox("avcC") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using AVCConfigurationBoxPtr = std::shared_ptr<AVCConfigurationBox>;

struct AVCSampleEntry : public VisualSampleEntry //"avc1","avc2","avc3","avc4"
{
    explicit AVCSampleEntry(uint32_t entry_type) : VisualSampleEntry(entry_type) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override
    {
        return VisualSampleEntry::getData(src);
    }
};
using AVCSampleEntryPtr = std::shared_ptr<AVCSampleEntry>;

enum ES_DESCRIPTOR_TAG_E
{
    UnKnownTag             = 0x00,
    MP4ODescrTag           = 0x01,
    MP4IODescrTag          = 0x02,
    MP4ESDescrTag          = 0x03,
    MP4DecConfigDescrTag   = 0x04,
    MP4DecSpecificDescrTag = 0x05,
    MP4SLDescrTag          = 0x06,
};

struct ESDescriptor // tag+length+data, sub descriptor is in data
{
    ES_DESCRIPTOR_TAG_E descTag = UnKnownTag;
    uint32_t descLength = 0; // length of data, 4 bytes longest, the last byte has height bit of 0, the lower 7 bits
                             // combined as the length

    uint64_t descPos  = 0; // tag位置
    uint64_t descSize = 0; // 总长度

    std::shared_ptr<ESDescriptor> subDesc;
    std::weak_ptr<ESDescriptor>   upper_descr;

    ESDescriptor() = delete;
    explicit ESDescriptor(ES_DESCRIPTOR_TAG_E desc_tag) : descTag(desc_tag) {}

    virtual int parse(BinaryFileReader &reader, uint64_t last)
    {
        MP4_UNUSED(reader);
        MP4_UNUSED(last);
        return 0;
    }

    void getLength(BinaryFileReader &reader);

    virtual std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const
    {
        std::shared_ptr<Mp4BoxData> item = nullptr;
        if (nullptr == src)
            item = Mp4BoxData::createKeyValuePairsData();
        else
            item = src;

        return item;
    }
};

extern std::map<ES_DESCRIPTOR_TAG_E, std::string> gDescriptorTypeString;

struct MP4ODescr : public ESDescriptor
{
    MP4ODescr() = delete;
    explicit MP4ODescr(ES_DESCRIPTOR_TAG_E desc_tag) : ESDescriptor(desc_tag) {}

    int parse(BinaryFileReader &reader, uint64_t last) override;
};

struct MP4IODescr : public ESDescriptor
{
    MP4IODescr() = delete;
    explicit MP4IODescr(ES_DESCRIPTOR_TAG_E desc_tag) : ESDescriptor(desc_tag) {}

    int parse(BinaryFileReader &reader, uint64_t last) override;
};

struct MP4ESDescr : public ESDescriptor
{
    uint16_t esId   = 0;
    uint8_t  esFlag = 0;

    MP4ESDescr() = delete;
    explicit MP4ESDescr(ES_DESCRIPTOR_TAG_E desc_tag) : ESDescriptor(desc_tag) {}

    int parse(BinaryFileReader &reader, uint64_t last) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};

struct MP4DecConfigDescr : public ESDescriptor
{
    uint8_t  objectType   = 0;
    uint8_t  streamType   = 0;
    uint32_t bufferSizeDb = 0; // 24 bits
    uint32_t maxBitrate   = 0;
    uint32_t avgBitrate   = 0;

    MP4DecConfigDescr() = delete;
    explicit MP4DecConfigDescr(ES_DESCRIPTOR_TAG_E desc_tag) : ESDescriptor(desc_tag) {}

    int parse(BinaryFileReader &reader, uint64_t last) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};

struct MP4DecSpecificDescr : public ESDescriptor
{
    BinaryData specificCfg;

    MP4DecSpecificDescr() = delete;
    explicit MP4DecSpecificDescr(ES_DESCRIPTOR_TAG_E desc_tag) : ESDescriptor(desc_tag) {}

    int parse(BinaryFileReader &reader, uint64_t last) override;
};

struct MP4SLDescr : public ESDescriptor
{
    BinaryData SldData;

    MP4SLDescr() = delete;
    explicit MP4SLDescr(ES_DESCRIPTOR_TAG_E desc_tag) : ESDescriptor(desc_tag) {}

    int parse(BinaryFileReader &reader, uint64_t last) override;
};

struct ESDBox : public FullBox
{
    std::shared_ptr<ESDescriptor> descriptor;

    ESDBox() : FullBox("esds") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using ESDBoxPtr = std::shared_ptr<ESDBox>;

struct MP4AudioSampleEntry : public AudioSampleEntry // ISO/IEC 14496-14
{
    MP4AudioSampleEntry() : AudioSampleEntry(MP4_BOX_MAKE_TYPE("mp4a")) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override
    {
        return AudioSampleEntry::getData(src);
    }
};
using MP4AudioSampleEntryPtr = std::shared_ptr<MP4AudioSampleEntry>;

struct MP4VisualSampleEntry : public VisualSampleEntry // ISO/IEC 14496-14
{
    MP4VisualSampleEntry() : VisualSampleEntry(MP4_BOX_MAKE_TYPE("mp4v")) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using MP4VisualSampleEntryPtr = std::shared_ptr<MP4VisualSampleEntry>;

#endif