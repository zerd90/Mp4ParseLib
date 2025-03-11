
#include "Mp4ParseInternal.h"
#include "Mp4SampleEntryTypes.h"

using std::string;

std::shared_ptr<Mp4BoxData> AVCDecoderConfigurationRecord::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Config Version", configVersion)
        ->kvAddPair("Profile Compat", profileCompat)
        ->kvAddPair("AVC Profile Indication", avcProfileIndication)
        ->kvAddPair("AVC Lv Indication", avcLvIndication)
        ->kvAddPair("NALU Length Size", lengthSize)
        ->kvAddPair("SPS Count", spsCount);
    for (size_t i = 0, im = sps.size(); i < im; i++)
    {
        item->kvAddPair("SPS " + std::to_string(i) + " Length", sps[i].length)
            ->kvAddPair("SPS " + std::to_string(i) + " Data", data2hex(sps[i].data));
    }
    item->kvAddPair("PPS Count", ppsCount);
    for (size_t i = 0, im = pps.size(); i < im; i++)
    {
        item->kvAddPair("PPS " + std::to_string(i) + " Length", pps[i].length)
            ->kvAddPair("PPS " + std::to_string(i) + " Data", data2hex(pps[i].data));
    }

    if (avcProfileIndication == 100 || avcProfileIndication == 110 || avcProfileIndication == 122
        || avcProfileIndication == 144)
    {
        item->kvAddPair("Chroma Fmt", chromaFormat)
            ->kvAddPair("Bit Depth Luma", bitDepthLuma)
            ->kvAddPair("Bit Depth Chroma", bitDepthChroma)
            ->kvAddPair("SPSE Count", spseCount);
        for (size_t i = 0, im = spse.size(); i < im; i++)
        {
            item->kvAddPair("PPS " + std::to_string(i), data2hex(spse[i].data));
        }
    }
    return item;
}

int AVCConfigurationBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    uint8_t compact1;
    uint8_t reserve;
    uint8_t AVCProfileIndication;
    uint8_t numSps, numPps;

    BOX_PARSE_BEGIN();

    AVCConfig.configVersion = reader.readU8();

    AVCProfileIndication           = reader.readU8();
    AVCConfig.avcProfileIndication = AVCProfileIndication;
    MP4_DBG("AVCProfileIndication=%d\n", AVCProfileIndication);

    AVCConfig.profileCompat = reader.readU8();

    AVCConfig.avcLvIndication = reader.readU8();

    compact1 = reader.readU8();

    BitsReader bitsReader(&compact1, 1);
    reserve = (uint8_t)bitsReader.readBit(6);
    if (reserve != 0b111111)
    {
        MP4_ERR("read error 0x%06x\n", reserve);
    }

    AVCConfig.lengthSize = (uint8_t)bitsReader.readBit(2) + 1;

    compact1 = reader.readU8();

    bitsReader = BitsReader(&compact1, 1);
    reserve    = (uint8_t)bitsReader.readBit(3);
    if (reserve != 0b111)
    {
        MP4_ERR("read error 0x%03x\n", reserve);
    }

    numSps             = (uint8_t)bitsReader.readBit(5);
    AVCConfig.spsCount = numSps;
    MP4_DBG("numSPS=%d\n", numSps);

    for (int i = 0; i < numSps; ++i)
    {
        AVCDecoderConfigurationRecord::Nalu curSps;

        curSps.length = reader.readU16(true);
        MP4_DBG("SPS%d length=%d\n", i, curSps.length);

        curSps.data.create(curSps.length);
        reader.read(curSps.data.ptr(), curSps.length);
        AVCConfig.sps.push_back(curSps);
    }

    numPps             = reader.readU8();
    AVCConfig.ppsCount = numPps;
    MP4_DBG("numPps=%d\n", numPps);

    for (int i = 0; i < numPps; ++i)
    {
        AVCDecoderConfigurationRecord::Nalu curPps;

        curPps.length = reader.readU16(true);
        MP4_DBG("PPS%d length=%d\n", i, curPps.length);

        curPps.data.create(curPps.length);
        reader.read(curPps.data.ptr(), curPps.length);

        AVCConfig.pps.push_back(curPps);
    }

    if ((AVCProfileIndication == 100 || AVCProfileIndication == 110 || AVCProfileIndication == 122
         || AVCProfileIndication == 144)
        && reader.getCursorPos() < last)
    {
        uint8_t numSpse;
        compact1 = reader.readU8();

        bitsReader = BitsReader(&compact1, 1);
        reserve    = (uint8_t)bitsReader.readBit(6);
        if (reserve != 0b111111)
        {
            MP4_ERR("read error 0x%06x\n", reserve);
        }

        AVCConfig.chromaFormat = (uint8_t)bitsReader.readBit(2);

        compact1 = reader.readU8();

        bitsReader = BitsReader(&compact1, 1);
        reserve    = (uint8_t)bitsReader.readBit(5);
        if (reserve != 0b11111)
        {
            MP4_ERR("read error %#05x\n", reserve);
        }

        AVCConfig.bitDepthLuma = (uint8_t)bitsReader.readBit(3) + 8;

        compact1 = reader.readU8();

        bitsReader = BitsReader(&compact1, 1);
        reserve    = (uint8_t)bitsReader.readBit(5);
        if (reserve != 0b11111)
        {
            MP4_ERR("read error %#05x\n", reserve);
        }

        AVCConfig.bitDepthChroma = (uint8_t)bitsReader.readBit(3) + 8;

        numSpse             = reader.readU8();
        AVCConfig.spseCount = numSpse;
        MP4_DBG("num_SPSExt=%d\n", numSpse);

        for (int i = 0; i < numSpse; ++i)
        {
            AVCDecoderConfigurationRecord::Nalu curSpse;

            curSpse.length = reader.readU16(true);
            MP4_DBG("SPSe%d length=%d\n", i, curSpse.length);

            curSpse.data.create(curSpse.length);
            reader.read(curSpse.data.ptr(), curSpse.length);

            AVCConfig.spse.push_back(curSpse);
        }
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> AVCConfigurationBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    return AVCConfig.getData(item);
}

int AVCSampleEntry::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    CHECK_RET(VisualSampleEntry::parse(reader, boxPosition, boxSize, boxBodySize));

    BOX_PARSE_END();

    return 0;
}
