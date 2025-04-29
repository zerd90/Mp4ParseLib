
#include "Mp4SampleEntryTypes.h"
#include "Mp4ParseInternal.h"

#define MAX_ICC_PROFILE_LENGTH \
    (1024 * 1024) // Maximum allowed ICC profile length in bytes (1 MB). Adjust if larger profiles are expected.
int ColourInformationBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();
    colorType = reader.readU32(true);

    switch (colorType)
    {
        case MP4_BOX_MAKE_TYPE("nclx"):
            colorPrimaries  = reader.readU16(true);
            transferCharact = reader.readU16(true);
            matrixCoeff     = reader.readU16(true);
            fullRangeFlag   = ((reader.readU8() & 0x80) != 0 ? 1 : 0);
            break;
        case MP4_BOX_MAKE_TYPE("rICC"):
        case MP4_BOX_MAKE_TYPE("prof"):
        {
            size_t iccProfileLength = last - reader.getCursorPos();
            if (iccProfileLength == 0 || iccProfileLength > MAX_ICC_PROFILE_LENGTH)
            {
                MP4_WARN("Invalid ICC profile length: %zu\n", iccProfileLength);
                reader.setCursor(last);
                break;
            }
            iccProfile.create(iccProfileLength);
            // TODO: ICC profile
            reader.read(iccProfile.ptr(), iccProfileLength);
            break;
        }
        default:
            MP4_WARN("unknown type %s\n", boxType2Str(colorType).c_str());
            reader.setCursor(last);
            break;
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> ColourInformationBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    item->kvAddPair("Color Type", boxType2Str(colorType));
    if (MP4_BOX_MAKE_TYPE("nclx") == colorType)
    {
        item->kvAddPair("Color Primaries", colorPrimaries)
            ->kvAddPair("Transfer Characteristics", transferCharact)
            ->kvAddPair("Matrix Coefficients", matrixCoeff)
            ->kvAddPair("Full Range Flag", fullRangeFlag);
    }
    else if (MP4_BOX_MAKE_TYPE("rICC") == colorType || MP4_BOX_MAKE_TYPE("prof") == colorType)
    {
        auto binaryData = Mp4BoxData::createBinaryData();
        binaryData->binarySetCallbacks(
            [](const void *userData)
            {
                const BinaryData *iccProfile = (const BinaryData *)userData;
                return iccProfile->length;
            },
            [](uint64_t offset, const void *userData) -> uint8_t
            {
                const BinaryData *iccProfile = (const BinaryData *)userData;
                if (offset >= iccProfile->length)
                    return 0;
                return iccProfile->ptr()[offset];
            },
            &iccProfile);

        item->kvAddPair("ICC Profile", binaryData);
    }
    return item;
}

int HEVCConfigurationBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    uint8_t configurationVersion;

    uint8_t  compact1;
    uint16_t compact2;
    uint8_t  reserve;

    BOX_PARSE_BEGIN();

    configurationVersion     = reader.readU8();
    HEVCConfig.configVersion = configurationVersion;

    compact1 = reader.readU8();

    if (configurationVersion != 1)
    {
        MP4_WARN("err ver %d\n", configurationVersion);
    }

    BitsReader bitsReader(&compact1, 1);

    HEVCConfig.generalProfileSpace             = (uint8_t)bitsReader.readBit(2);
    HEVCConfig.generalTierFlag                 = (uint8_t)bitsReader.readBit();
    HEVCConfig.generalProfileIdc               = (uint8_t)bitsReader.readBit(5);
    HEVCConfig.generalProfileCompatFlags       = reader.readU32(true);
    HEVCConfig.generalConstraintIndicatorFlags = reader.readUnsigned(6, true);
    HEVCConfig.generalLevelIdc                 = reader.readU8();

    compact2 = reader.readU16(false);

    bitsReader = BitsReader(&compact2, 2);

    reserve = (uint8_t)bitsReader.readBit(4);
    if (reserve != 0b1111)
    {
        MP4_ERR("read error 0x%04x\n", reserve);
    }
    HEVCConfig.minSpatialSegmentIdc = (uint16_t)bitsReader.readBit(12);

    compact1 = reader.readU8();

    bitsReader = BitsReader(&compact1, 1);

    reserve = (uint8_t)bitsReader.readBit(6);
    if (reserve != 0b111111)
    {
        MP4_ERR("read error 0x%06x\n", reserve);
    }
    HEVCConfig.parallelismType = (uint8_t)bitsReader.readBit(2);

    compact1 = reader.readU8();

    bitsReader = BitsReader(&compact1, 1);

    reserve = (uint8_t)bitsReader.readBit(6);
    if (reserve != 0b111111)
    {
        MP4_ERR("read error 0x%06x\n", reserve);
    }
    HEVCConfig.chromaFormatIdc = (uint8_t)bitsReader.readBit(2);

    compact1 = reader.readU8();

    bitsReader = BitsReader(&compact1, 1);
    reserve    = (uint8_t)bitsReader.readBit(5);
    if (reserve != 0b11111)
    {
        MP4_ERR("read error 0x%05x\n", reserve);
    }
    HEVCConfig.bitDepthLuma = (uint8_t)bitsReader.readBit(3) + 8;

    compact1 = reader.readU8();

    bitsReader = BitsReader(&compact1, 1);
    reserve    = (uint8_t)bitsReader.readBit(5);
    if (reserve != 0b11111)
    {
        MP4_ERR("read error 0x%05x\n", reserve);
    }
    HEVCConfig.bitDepthChroma = (uint8_t)bitsReader.readBit(3) + 8;

    HEVCConfig.avgFrameRate = reader.readU16(true);

    compact1 = reader.readU8();

    bitsReader                   = BitsReader(&compact1, 1);
    HEVCConfig.constFrameRate    = (uint8_t)bitsReader.readBit(2);
    HEVCConfig.numTemporalLayers = (uint8_t)bitsReader.readBit(3);
    HEVCConfig.temporalIdNested  = (uint8_t)bitsReader.readBit();
    HEVCConfig.lengthSize        = (uint8_t)bitsReader.readBit(2) + 1;

    HEVCConfig.arrayCount = reader.readU8();

    for (int i = 0; i < HEVCConfig.arrayCount; ++i)
    {
        HEVCDecoderConfigurationRecord::NaluItem nalus;

        compact1 = reader.readU8();

        bitsReader          = BitsReader(&compact1, 1);
        nalus.arrayComplete = bitsReader.readBit();
        reserve             = bitsReader.readBit();
        if (reserve != 0)
        {
            MP4_ERR("read error 0x%06x\n", reserve);
        }
        nalus.naluType = (uint8_t)bitsReader.readBit(6);

        nalus.numNalus = reader.readU16(true);

        for (int j = 0; j < nalus.numNalus; j++)
        {
            HEVCDecoderConfigurationRecord::NaluItem::Nalu nalu;

            nalu.length = reader.readU16(true);
            nalu.data.create(nalu.length);
            reader.read(nalu.data.ptr(), nalu.length);

            nalus.nalus.push_back(nalu);
        }

        HEVCConfig.arrays.push_back(nalus);
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> HEVCConfigurationBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    HEVCConfig.getData(item);

    return item;
}

int HEVCSampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    CHECK_RET(VisualSampleEntry::parse(reader, boxPosition, boxSize, boxBodySize));

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> HEVCDecoderConfigurationRecord::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Config Version", configVersion)
        ->kvAddPair("General Profile Space", generalProfileSpace)
        ->kvAddPair("General Tier Flag", generalTierFlag)
        ->kvAddPair("General Profile Idc", generalProfileIdc)
        ->kvAddPair("General Profile Compat Flags", generalProfileCompatFlags)
        ->kvAddPair("General Constraint Indicator Flags", generalConstraintIndicatorFlags)
        ->kvAddPair("General Level Idc", generalLevelIdc)
        ->kvAddPair("Min Spatial Segment Idc", minSpatialSegmentIdc)
        ->kvAddPair("Parallelism Type", parallelismType)
        ->kvAddPair("Chroma Format Idc", chromaFormatIdc)
        ->kvAddPair("Bit Depth Luma", bitDepthLuma)
        ->kvAddPair("Bit Depth Chroma", bitDepthChroma)
        ->kvAddPair("Avg Frame Rate", avgFrameRate)
        ->kvAddPair("Const Frame Rate", constFrameRate)
        ->kvAddPair("Num Temporal Layers", numTemporalLayers)
        ->kvAddPair("Temporal ID Nested", temporalIdNested)
        ->kvAddPair("Length Size", lengthSize)
        ->kvAddPair("Array Count", arrayCount);
    for (size_t i = 0, im = arrays.size(); i < im; i++)
    {
        item->kvAddPair("Array " + std::to_string(i) + " Complete", arrays[i].arrayComplete)
            ->kvAddPair("Array " + std::to_string(i) + " NALU Type", arrays[i].naluType)
            ->kvAddPair("Array " + std::to_string(i) + " NALU Number", arrays[i].numNalus);
        for (size_t j = 0, jm = arrays[i].nalus.size(); j < jm; j++)
        {
            auto binaryData = Mp4BoxData::createBinaryData();
            binaryData->binarySetCallbacks(
                [](const void *userData)
                {
                    auto *nalu = (const HEVCDecoderConfigurationRecord::NaluItem::Nalu *)userData;
                    return nalu->length;
                },
                [](uint64_t offset, const void *userData) -> uint8_t
                {
                    auto *nalu = (const HEVCDecoderConfigurationRecord::NaluItem::Nalu *)userData;
                    if (offset >= nalu->length)
                        return 0;
                    return nalu->data.ptr()[offset];
                },
                &arrays[i].nalus[j]);

            item->kvAddPair("Array " + std::to_string(i) + " NALU " + std::to_string(j) + " Length",
                            arrays[i].nalus[j].length)
                ->kvAddPair("Array " + std::to_string(i) + " NALU " + std::to_string(j) + " Data",
                            binaryData);
        }
    }
    return item;
}
