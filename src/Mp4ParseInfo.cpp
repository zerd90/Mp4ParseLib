
#include <math.h>
#include <iostream>
#include <sstream>

#include "Mp4BoxTypes.h"
#include "Mp4SampleEntryTypes.h"
#include "Mp4ParseInternal.h"
#include "Mp4SampleTableTypes.h"

using std::dynamic_pointer_cast;
using std::endl;
using std::shared_ptr;
using std::string;
using std::vector;

uint8_t getCodecFromEsds(ESDBoxPtr esds)
{
    if (esds == nullptr)
        return 0;

    shared_ptr<ESDescriptor> descriptor = esds->descriptor;
    while (1)
    {
        if (descriptor->descTag == MP4DecConfigDescrTag || descriptor->subDesc == nullptr)
            break;
        descriptor = descriptor->subDesc;
    }
    if (descriptor->descTag != MP4DecConfigDescrTag)
    {
        return 0;
    }

    auto decCfg = std::dynamic_pointer_cast<MP4DecConfigDescr>(descriptor);
    if (decCfg == nullptr)
    {
        auto rawPtr = descriptor.get();
        MP4_ERR("cast fail %s\n", typeid(*rawPtr).name());
        return 0;
    }

    return decCfg->objectType;
}

uint8_t getCodecFromStsd(SampleDescriptionBoxPtr stsd)
{
    uint8_t res;
    auto    containBoxes = stsd->getContainBoxes();
    if (stsd->entryCount == 0 || containBoxes.empty())
    {
        MP4_ERR("no codec info %u\n", stsd->entryCount);
        res = 0;
    }
    else
    {
        switch (containBoxes[0]->getBoxType())
        {
            case MP4_BOX_MAKE_TYPE("avc1"):
            case MP4_BOX_MAKE_TYPE("avc2"):
            case MP4_BOX_MAKE_TYPE("avc3"):
            case MP4_BOX_MAKE_TYPE("avc4"):
                res = 0x21;
                break;
            case MP4_BOX_MAKE_TYPE("hvc1"):
            case MP4_BOX_MAKE_TYPE("hev1"):
                res = 0x23;
                break;
            case MP4_BOX_MAKE_TYPE("mp4v"):
            case MP4_BOX_MAKE_TYPE("mp4a"):
            {
                auto subBoxes = containBoxes[0]->getContainBoxes();
                for (auto &subBox : subBoxes)
                {
                    if (subBox->getBoxType() == MP4_BOX_MAKE_TYPE("esds"))
                    {
                        ESDBoxPtr esds = dynamic_pointer_cast<ESDBox>(subBox);
                        res            = getCodecFromEsds(esds);
                        break;
                    }
                }
                break;
            }
            default:
                MP4_ERR("unknown codec type %s\n", containBoxes[0]->getBoxTypeStr().c_str());
                res = 0;
                break;
        }
    }

    return res;
}

MP4_CODEC_TYPE_E getCodecTypeFromStsd(SampleDescriptionBoxPtr stsd)
{
    uint8_t codec = getCodecFromStsd(stsd);

    return mp4GetCodecType(codec);
}

H26X_FRAME_TYPE_E MP4ParserImpl::getH264FrameType(BinaryData &data)
{
    uint32_t   frameType;
    BitsReader bits(data.ptr(), (uint32_t)data.length);
    /* i_first_mb */
    bits.readGolomb();
    /* picture type */
    frameType = bits.readGolomb();
    switch (frameType)
    {
        case 0:
        case 5: /* P */
            return H26X_FRAME_P;
            break;
        case 1:
        case 6: /* B */
            return H26X_FRAME_B;
            break;
        case 3:
        case 8: /* SP */
            return H26X_FRAME_P;
            break;
        case 2:
        case 7: /* I */
            return H26X_FRAME_I;
            break;
        case 4:
        case 9: /* SI */
            return H26X_FRAME_I;
            break;
    }
    return H26X_FRAME_Unknown;
}
#define HEVC_MAX_SUB_LAYERS 7
void hvccParsePtl(BitsReader &bits, unsigned int maxSubLayersMinus1)
{
    unsigned int i;
    uint8_t      subLayerProfilePresentFlag[HEVC_MAX_SUB_LAYERS];
    uint8_t      subLayerLevelPresentFlag[HEVC_MAX_SUB_LAYERS];

    bits.readBit(2);  // profile_space
    bits.readBit(1);  // tier_flag
    bits.readBit(5);  // profile_idc
    bits.readBit(32); // profile_compatibility_flags
    bits.readBit(48); // constraint_indicator_flags
    bits.readBit(8);  // level_idc

    for (i = 0; i < maxSubLayersMinus1; i++)
    {
        subLayerProfilePresentFlag[i] = (uint8_t)bits.readBit(1);
        subLayerLevelPresentFlag[i]   = (uint8_t)bits.readBit(1);
    }

    if (maxSubLayersMinus1 > 0)
        for (i = maxSubLayersMinus1; i < 8; i++)
            bits.readBit(2); // reserved_zero_2bits[i]

    for (i = 0; i < maxSubLayersMinus1; i++)
    {
        if (subLayerProfilePresentFlag[i])
        {
            /*
             * sub_layer_profile_space[i]                     u(2)
             * sub_layer_tier_flag[i]                         u(1)
             * sub_layer_profile_idc[i]                       u(5)
             * sub_layer_profile_compatibility_flag[i][0..31] u(32)
             * sub_layer_progressive_source_flag[i]           u(1)
             * sub_layer_interlaced_source_flag[i]            u(1)
             * sub_layer_non_packed_constraint_flag[i]        u(1)
             * sub_layer_frame_only_constraint_flag[i]        u(1)
             * sub_layer_reserved_zero_44bits[i]              u(44)
             */
            bits.readBit(32);
            bits.readBit(32);
            bits.readBit(24);
        }

        if (subLayerLevelPresentFlag[i])
            bits.readBit(8);
    }
}

int hevcParseSps(uint8_t *buffer, uint32_t size, uint32_t &picWidthInLumaSamples, uint32_t &picHeightInLumaSamples,
                 uint32_t &log2MinLumaCodingBlockSizeMinus3, uint32_t &log2DiffMaxMinLumaCodingBlockSize)
{
    uint32_t i, spsMaxSubLayersMinus1;

    BitsReader bits(buffer, size);
    // header
    bits.readBit(16);

    bits.readBit(4); // sps_video_parameter_set_id

    spsMaxSubLayersMinus1 = bits.readBit(3);

    bits.readBit(1); // temporalIdNested

    hvccParsePtl(bits, spsMaxSubLayersMinus1);

    bits.readGolomb(); // sps_seq_parameter_set_id

    uint32_t chromaFormat = bits.readGolomb();

    if (chromaFormat == 3)
        bits.readBit(1); // separate_colour_plane_flag

    picWidthInLumaSamples  = bits.readGolomb(); // pic_width_in_luma_samples
    picHeightInLumaSamples = bits.readGolomb(); // pic_height_in_luma_samples

    if (bits.readBit(1))
    {                      // conformance_window_flag
        bits.readGolomb(); // conf_win_left_offset
        bits.readGolomb(); // conf_win_right_offset
        bits.readGolomb(); // conf_win_top_offset
        bits.readGolomb(); // conf_win_bottom_offset
    }

    uint32_t bitDepthLumaMinus8   = bits.readGolomb();
    uint32_t bitDepthChromaMinus8 = bits.readGolomb();
    MP4_UNUSED(bitDepthLumaMinus8);
    MP4_UNUSED(bitDepthChromaMinus8);

    /*uint32_t log2MaxPicOrderCntLsbMinus4 =  */ bits.readGolomb();

    /* sps_sub_layer_ordering_info_present_flag */
    i = bits.readBit(1) ? 0 : spsMaxSubLayersMinus1;
    for (; i <= spsMaxSubLayersMinus1; i++)
    {
        bits.readGolomb();
        bits.readGolomb();
        bits.readGolomb();
    }

    log2MinLumaCodingBlockSizeMinus3  = bits.readGolomb();
    log2DiffMaxMinLumaCodingBlockSize = bits.readGolomb();

    /* nothing useful for hvcC past this point */
    return 0;
}

int hevcParsePps(uint8_t *buffer, uint32_t size, uint8_t &dependentSliceSegmentsEnabledFlag,
                 uint8_t &numExtraSliceHeaderBits)
{
    BitsReader ppsBits(buffer, size);

    // header
    ppsBits.readBit(16);

    ppsBits.readGolomb();
    ppsBits.readGolomb();
    dependentSliceSegmentsEnabledFlag = ppsBits.readBit();
    ppsBits.readBit(1);
    numExtraSliceHeaderBits = (uint8_t)ppsBits.readBit(3);

    return 0;
}

H26X_FRAME_TYPE_E MP4ParserImpl::getH265FrameType(int naluType, BinaryData &data)
{
    int        frameType;
    BitsReader bits(data.ptr(), (uint32_t)data.length);

    uint8_t firstSliceSegmentInPicFlag;

    int MinCbLog2SizeY   = mSpsInfo.log2MinLumaCodingBlockSizeMinus3 + 3;
    int CtbLog2SizeY     = MinCbLog2SizeY + mSpsInfo.log2DiffMaxMinLumaCodingBlockSize;
    int CtbSizeY         = 1 << CtbLog2SizeY;
    int PicWidthInCtbsY  = (int)ceil((float)mSpsInfo.picWidthInLumaSamples / CtbSizeY);
    int PicHeightInCtbsY = (int)ceil(mSpsInfo.picHeightInLumaSamples / CtbSizeY);
    int PicSizeInCtbsY   = PicWidthInCtbsY * PicHeightInCtbsY;

    firstSliceSegmentInPicFlag = (uint8_t)bits.readBit(1);
    if (naluType >= 16 && naluType <= 23)
        bits.readBit(); // no_output_of_prior_pics_flag
    bits.readGolomb();  // slice_pic_parameter_set_id
    if (!firstSliceSegmentInPicFlag)
    {
        if (mPpsInfo.dependentSliceSegmentsEnabledFlag)
            /* dependentSliceSegmentFlag =  */ bits.readBit();
        int sliceSegmentAddressLen = (int)ceil(log2((float)PicSizeInCtbsY));
        bits.readBit(sliceSegmentAddressLen);
    }
    if (mPpsInfo.numExtraSliceHeaderBits > 0)
        bits.readBit(mPpsInfo.numExtraSliceHeaderBits);

    frameType = bits.readGolomb();

    switch (frameType)
    {
        case 0:
            return H26X_FRAME_B;
        case 1:
            return H26X_FRAME_P;
        case 2:
            return H26X_FRAME_I;
    }

    return H26X_FRAME_Unknown;
}

H26X_FRAME_TYPE_E MP4ParserImpl::parseVideoNaluType(uint32_t trackIdx, uint64_t sampleIdx)
{
    if (mNaluLengthSize.find(trackIdx) == mNaluLengthSize.end())
        return H26X_FRAME_Unknown;

    if (trackIdx >= tracksInfo.size())
        return H26X_FRAME_Unknown;

    TrackInfoPtr mp4TrackInfo = tracksInfo[trackIdx];

    if (mp4TrackInfo->trackType != TRACK_TYPE_VIDEO)
        return H26X_FRAME_Unknown;

    MP4_CODEC_TYPE_E codecType = mp4GetCodecType(mp4TrackInfo->mediaInfo->codecCode);
    if (MP4_CODEC_H264 != codecType && MP4_CODEC_HEVC != codecType)
        return H26X_FRAME_Unknown;

    int naluLenSize = mNaluLengthSize[trackIdx];

    if (codecType != MP4_CODEC_H264 && codecType != MP4_CODEC_HEVC)
        return H26X_FRAME_Unknown;

    Mp4SampleItem *curSample = &mp4TrackInfo->mediaInfo->samplesInfo[sampleIdx];
    uint64_t       oldPos    = mFileReader.getCursorPos();
    mFileReader.setCursor(curSample->sampleOffset);
    uint64_t last = curSample->sampleOffset + curSample->sampleSize;

    curSample->frameType     = H26X_FRAME_Unknown;
    BinaryFileReader &reader = mFileReader;
    while (reader.getCursorPos() < last)
    {
        uint32_t naluSize = (uint32_t)reader.readUnsigned(naluLenSize, true);
        if (0 == naluSize)
        {
            MP4_ERR("nalu size = 0\n");
            break;
        }
        uint64_t naluLast = mFileReader.getCursorPos() + naluSize;
        uint8_t  naluType;
        mFileReader.read(&naluType, 1);
        BitsReader bitsReader(&naluType, 1);
        BinaryData data;
        // nalu type only tell if it is an I frame, parsing nalu data to tell if it's a P or B frame
        switch (codecType)
        {
            default:
            case MP4_CODEC_H264:
                bitsReader.readBit(3);
                naluType = (uint8_t)bitsReader.readBit(5);
                if (naluType > H264_NALU_MAX)
                {
                    MP4_ERR("H264 Nalu Type %d\n", naluType);
                    break;
                }
                curSample->naluTypes.push_back(naluType);
                if (H264_NALU_SLICE_IDR == naluType)
                {
                    curSample->frameType = H26X_FRAME_I;
                    break;
                }
                if (H26X_FRAME_Unknown != curSample->frameType)
                    break;
                if (H264_NALU_SLICE == naluType)
                {
                    data.create(8);
                    reader.read(data.ptr(), data.length);
                    H26X_FRAME_TYPE_E type;
                    type = getH264FrameType(data);
                    if (type == H26X_FRAME_Unknown)
                    {
                        MP4_ERR("H264 frame %llu type unknown\n", sampleIdx);
                    }
                    if (H26X_FRAME_I == type)
                    {
                        MP4_ERR("H264 frame %llu type I, not matching Nalu Type %d\n", sampleIdx, naluType);
                    }
                    curSample->frameType = type;
                    if (H26X_FRAME_I == curSample->frameType && 0 == curSample->isKeyFrame)
                    {
                        MP4_ERR("H264 frame %llu is I frame, but not marked as key frame by stts\n", sampleIdx);
                        curSample->isKeyFrame = 2;
                    }
                }
                break;
            case MP4_CODEC_HEVC:
                bitsReader.readBit(1);
                naluType = (uint8_t)bitsReader.readBit(6);
                if (naluType > H265_NALU_MAX)
                {
                    MP4_ERR("H265 Nalu Type %d\n", naluType);
                    break;
                }
                curSample->naluTypes.push_back(naluType);
                if (H265_NALU_IDR_W == naluType || H265_NALU_IDR_N == naluType)
                {
                    curSample->frameType = H26X_FRAME_I;
                    break;
                }

                if (H26X_FRAME_Unknown != curSample->frameType)
                    break;
                if (H265_NALU_TRAIL_N == naluType || H265_NALU_TRAIL_R == naluType || H265_NALU_TSA_N == naluType
                    || H265_NALU_TSA_R == naluType || H265_NALU_STSA_N == naluType || H265_NALU_STSA_R == naluType
                    || H265_NALU_RADL_N == naluType || H265_NALU_RADL_R == naluType || H265_NALU_RASL_N == naluType
                    || H265_NALU_RASL_R == naluType || H265_NALU_BLA_W_LP == naluType
                    || H265_NALU_BLA_W_RADL == naluType || H265_NALU_BLA_N_LP == naluType
                    || H265_NALU_CRA_NUT == naluType) // a picture slice
                {
                    mFileReader.read(nullptr, 1); // H265 Nalu Header is 2 bytes
                    data.create(16);
                    reader.read(data.ptr(), data.length);
                    H26X_FRAME_TYPE_E type;
                    type = getH265FrameType(naluType, data);
                    if (type == H26X_FRAME_Unknown)
                        MP4_ERR("H265 frame %llu type unknown\n", sampleIdx);

                    if (H26X_FRAME_I == type)
                        MP4_ERR("H265 frame %llu type I, not matching Nalu Type %d\n", sampleIdx, naluType);

                    curSample->frameType = type;
                    if (H26X_FRAME_I == curSample->frameType && 0 == curSample->isKeyFrame)
                    {
                        MP4_ERR("H265 frame %llu is I frame, but not marked as key frame by stts\n", sampleIdx);
                        curSample->isKeyFrame = 2;
                    }
                }
                break;
        }

        mFileReader.setCursor(naluLast);
    }

    mFileReader.setCursor(oldPos);

    return curSample->frameType;
}

int MP4ParserImpl::generateInfoTable(uint32_t trackIdx)
{
    auto         mp4TrackInfo = tracksInfo[trackIdx];
    CommonBoxPtr pTrakBox     = nullptr;

    CommonBoxPtr            stbl;
    SampleDescriptionBoxPtr stsd;
    do
    {
        auto pMoov = getContainBox("moov");
        if (nullptr == pMoov)
        {
            MP4_ERR("moov box missing\n");
            break;
        }
        auto pTrakBoxes = pMoov->getContainBoxes("trak");
        if (trackIdx >= pTrakBoxes.size())
        {
            MP4_ERR("trak Index Too Big %u %zu\n", trackIdx, pTrakBoxes.size());
            break;
        }

        pTrakBox = pTrakBoxes[trackIdx];

    } while (0);

    if (pTrakBox == nullptr)
    {
        MP4_ERR("trak box missing\n");
        return -1;
    }

    stbl = pTrakBox->getContainBoxRecursive("stbl", 3);
    if (stbl == nullptr)
    {
        MP4_ERR("%d stbl not parsed\n", mp4TrackInfo->trackId);
        return -1;
    }

    stsd = stbl->getContainBox<SampleDescriptionBox>("stsd");
    if (stsd == nullptr)
    {
        MP4_ERR("stsd not found\n");
        return -1;
    }

    if (MP4_TYPE_ISO == mMp4Type)
    {
        CHECK_RET(generateIsoSamplesInfoTable(mp4TrackInfo->trakIndex));
    }
    else
    {
        CHECK_RET(generateFragmentSamplesInfoTable(mp4TrackInfo->trakIndex));
    }

    if (mp4TrackInfo->mediaInfo != nullptr)
        mp4TrackInfo->mediaInfo->getInfoFromTrack(pTrakBox, mp4TrackInfo);

    MP4_CODEC_TYPE_E codecType = getCodecTypeFromStsd(stsd);
    if (codecType != MP4_CODEC_H264 && codecType != MP4_CODEC_HEVC)
    {
        return 0;
    }

    BinaryData pps, sps;
    if (MP4_CODEC_HEVC == codecType)
    {
        HEVCConfigurationBoxPtr hvcC = pTrakBox->getContainBoxRecursive<HEVCConfigurationBox>("hvcC");
        for (unsigned int i = 0; i < hvcC->HEVCConfig.arrayCount; ++i)
        {
            if (H265_NALU_PPS == hvcC->HEVCConfig.arrays[i].naluType)
            {
                if (0 == hvcC->HEVCConfig.arrays[i].numNalus)
                {
                    MP4_ERR("What?");
                    continue;
                }
                pps.create(hvcC->HEVCConfig.arrays[i].nalus[0].length);
                memcpy(pps.ptr(), hvcC->HEVCConfig.arrays[i].nalus[0].data.ptr(),
                       hvcC->HEVCConfig.arrays[i].nalus[0].length);
            }
            else if (H265_NALU_SPS == hvcC->HEVCConfig.arrays[i].naluType)
            {
                if (0 == hvcC->HEVCConfig.arrays[i].numNalus)
                {
                    MP4_ERR("What?");
                    continue;
                }
                sps.create(hvcC->HEVCConfig.arrays[i].nalus[0].length);
                memcpy(sps.ptr(), hvcC->HEVCConfig.arrays[i].nalus[0].data.ptr(),
                       hvcC->HEVCConfig.arrays[i].nalus[0].length);
            }
        }
    }
    if (pps.length > 0)
    {
        hevcParsePps(pps.ptr(), (uint32_t)pps.length, mPpsInfo.dependentSliceSegmentsEnabledFlag,
                     mPpsInfo.numExtraSliceHeaderBits);
        mPpsInfo.parsed = true;
    }
    if (sps.length > 0)
    {
        hevcParseSps(sps.ptr(), (uint32_t)sps.length, mSpsInfo.picWidthInLumaSamples, mSpsInfo.picHeightInLumaSamples,
                     mSpsInfo.log2MinLumaCodingBlockSizeMinus3, mSpsInfo.log2DiffMaxMinLumaCodingBlockSize);
        mSpsInfo.parsed = true;
    }

    if (MP4_CODEC_H264 == codecType)
    {
        AVCConfigurationBoxPtr avcC = pTrakBox->getContainBoxRecursive<AVCConfigurationBox>("avcC");
        mNaluLengthSize[trackIdx]   = avcC->AVCConfig.lengthSize;
    }
    else if (MP4_CODEC_HEVC == codecType)
    {
        HEVCConfigurationBoxPtr hvcC = pTrakBox->getContainBoxRecursive<HEVCConfigurationBox>("hvcC");
        mNaluLengthSize[trackIdx]    = hvcC->HEVCConfig.lengthSize;
    }

    MP4_DBG("track%d NaluLengthSize=%d\n", mp4TrackInfo->trackId, mNaluLengthSize[trackIdx]);

    return 0;
}

int MP4ParserImpl::generateIsoSamplesInfoTable(uint64_t trackIdx)
{
    uint32_t     chunkCount;
    uint32_t     sampleCount;
    CommonBoxPtr pTrakBox;
    do
    {
        auto pMoov = getContainBox("moov");
        if (nullptr == pMoov)
        {
            MP4_ERR("moov box missing\n");
            break;
        }
        auto pTrakBoxes = pMoov->getContainBoxes("trak");
        if (trackIdx >= pTrakBoxes.size())
        {
            MP4_ERR("trak Index Too Big %llu %zu\n", trackIdx, pTrakBoxes.size());
            break;
        }

        pTrakBox = pTrakBoxes[trackIdx];

    } while (0);

    if (pTrakBox == nullptr)
    {
        MP4_ERR("trak box missing\n");
        return -1;
    }

    Mp4TrackInfo *trackInfo      = tracksInfo[trackIdx].get();
    Mp4MediaInfo *trackMediaInfo = trackInfo->mediaInfo.get();

    CommonBoxPtr      stbl = pTrakBox->getContainBoxRecursive("stbl", 3);
    MediaHeaderBoxPtr mdhd = pTrakBox->getContainBoxRecursive<MediaHeaderBox>("mdhd");

    if (stbl == nullptr)
    {
        MP4_ERR("%llu stbl not parsed\n", trackIdx);
        return -1;
    }

    SampleDescriptionBoxPtr stsd = stbl->getContainBox<SampleDescriptionBox>("stsd");
    SampleTableBoxPtr       stss = stbl->getContainBox<SampleTableBox>("stss");
    SampleSizeBoxPtr        stsz = stbl->getContainBox<SampleSizeBox>("stsz");
    SampleTableBoxPtr       stz2 = stbl->getContainBox<SampleTableBox>("stz2");
    SampleTableBoxPtr       stco = stbl->getContainBox<SampleTableBox>("stco");
    SampleTableBoxPtr       co64 = stbl->getContainBox<SampleTableBox>("co64");
    SampleTableBoxPtr       stts = stbl->getContainBox<SampleTableBox>("stts");
    SampleTableBoxPtr       stsc = stbl->getContainBox<SampleTableBox>("stsc");
    SampleTableBoxPtr       ctts = stbl->getContainBox<SampleTableBox>("ctts");

    MP4_CODEC_TYPE_E codeType;

    if (stsd == nullptr)
    {
        MP4_ERR("stsd not found\n");
        return -1;
    }

    codeType = getCodecTypeFromStsd(stsd);
    if (mdhd == nullptr)
    {
        MP4_ERR("necessary box not parsed\n");
        return -1;
    }
    else if (trackInfo->trackType == TRACK_TYPE_VIDEO && (codeType == MP4_CODEC_H264 || codeType == MP4_CODEC_HEVC)
             && stss == nullptr)
    {
        MP4_ERR("necessary box not parsed\n");
        return -1;
    }
    else if (stsz == nullptr && stz2 == nullptr)
    {
        MP4_ERR("necessary box not parsed\n");
        return -1;
    }
    else if (stco == nullptr && co64 == nullptr)
    {
        MP4_ERR("necessary box not parsed\n");
        return -1;
    }
    else if (stts == nullptr || stsc == nullptr)
    {
        MP4_ERR("necessary box not parsed\n");
        return -1;
    }

    if (stco != nullptr)
    {
        chunkCount = stco->entryCount;
    }
    else
    {
        chunkCount = co64->entryCount;
    }

    uint64_t chunkStart = 0;

    for (unsigned int chunkIdx = 0; chunkIdx < chunkCount; ++chunkIdx)
    {
        Mp4ChunkItem curChunk;
        curChunk.chunkIdx = chunkIdx;
        if (stco != nullptr)
            curChunk.chunkOffset = stco->getEntry<stcoItem>(chunkIdx)->chunkOffset;
        else
            curChunk.chunkOffset = co64->getEntry<co64Item>(chunkIdx)->chunkOffset;
        for (unsigned int j = 0, jm = stsc->entryCount; j < jm; j++)
        {
            if (chunkIdx + 1 >= stsc->getEntry<stscItem>(j)->firstChunk
                && (j == jm - 1 || chunkIdx + 1 < stsc->getEntry<stscItem>(j + 1)->firstChunk))
            {
                curChunk.sampleCount    = stsc->getEntry<stscItem>(j)->sampleCount;
                curChunk.sampleStartIdx = chunkStart;
                chunkStart += curChunk.sampleCount;
                break;
            }
        }
        curChunk.chunkSize = 0;
        trackMediaInfo->chunksInfo.push_back(curChunk);
    }

    if (chunkCount == 0)
        return 0;

    sampleCount           = (stsz ? stsz : stz2)->entryCount;
    uint32_t curChunkIdx  = 0;
    uint64_t sampleOffset = trackMediaInfo->chunksInfo[curChunkIdx].chunkOffset;
    uint32_t stssIdx      = 0;
    uint32_t sttsIdx      = 0;
    uint32_t sttsCount    = stts->getEntry<sttsItem>(sttsIdx)->sampleCount;
    uint32_t cttsIdx      = 0;
    uint32_t cttsCount    = 0;
    if (ctts != nullptr)
        cttsCount = ctts->getEntry<cttsItem>(cttsIdx)->sampleCount;
    uint64_t curDts        = 0;
    uint64_t nextIframeIdx = 0;

    if (stss != nullptr)
        nextIframeIdx = stss->getEntry<stssItem>(stssIdx)->sampleNumber - 1;

    for (unsigned int i = 0; i < sampleCount; ++i)
    {
        if (i >= trackMediaInfo->chunksInfo[curChunkIdx].sampleStartIdx
                     + trackMediaInfo->chunksInfo[curChunkIdx].sampleCount)
        {
            curChunkIdx++;
            sampleOffset = trackMediaInfo->chunksInfo[curChunkIdx].chunkOffset;
        }
        Mp4SampleItem curSample;
        curSample.sampleIdx = i;
        if (stss != nullptr)
        {
            if (i == nextIframeIdx)
            {
                curSample.isKeyFrame = 1;
                trackMediaInfo->syncSampleTable.push_back(i);
                stssIdx++;
                if (stssIdx < stss->entryCount)
                {
                    nextIframeIdx = stss->getEntry<stssItem>(stssIdx)->sampleNumber - 1;
                }
            }
            else
            {
                curSample.isKeyFrame = 0;
            }
        }
        else
        {
            curSample.isKeyFrame = -1;
        }

        if (i >= sttsCount)
        {
            sttsIdx++;
            sttsCount += stts->getEntry<sttsItem>(sttsIdx)->sampleCount;
        }

        curSample.dtsDeltaMs = stts->getEntry<sttsItem>(sttsIdx)->delta;
        curSample.dtsDeltaMs = curSample.dtsDeltaMs * 1000 / mdhd->timescale;
        curSample.dtsMs      = curDts;
        curDts += curSample.dtsDeltaMs;

        uint64_t deltaTs = 0;

        if (ctts != nullptr)
        {
            if (i >= cttsCount)
            {
                cttsIdx++;
                cttsCount += ctts->getEntry<cttsItem>(cttsIdx)->sampleCount;
                ;
            }

            deltaTs = ctts->getEntry<cttsItem>(cttsIdx)->sampleOffset;
            deltaTs = deltaTs * 1000 / mdhd->timescale;
        }
        curSample.ptsMs = curSample.dtsMs + deltaTs;

        if (stsz != nullptr && stsz->defaultSampleSize != 0)
        {
            curSample.sampleSize = stsz->defaultSampleSize;
        }
        else
        {
            if (stsz)
                curSample.sampleSize = stsz->getEntry<stszItem>(i)->sampleSize;
            else
                curSample.sampleSize = stz2->getEntry<stz2Item>(i)->sampleSize;
        }

        curSample.sampleOffset = sampleOffset;
        sampleOffset += curSample.sampleSize;

        trackMediaInfo->chunksInfo[curChunkIdx].chunkSize += curSample.sampleSize;

        trackMediaInfo->samplesInfo.push_back(curSample);
    }

    for (unsigned int i = 0; i < trackMediaInfo->chunksInfo.size(); ++i)
    {
        Mp4ChunkItem  &curChunk    = trackMediaInfo->chunksInfo[i];
        Mp4SampleItem &startSample = trackMediaInfo->samplesInfo[curChunk.sampleStartIdx];
        Mp4SampleItem &endSample   = trackMediaInfo->samplesInfo[curChunk.sampleStartIdx + curChunk.sampleCount - 1];
        curChunk.startPtsMs        = startSample.dtsMs;
        curChunk.durationMs        = endSample.dtsMs + endSample.dtsDeltaMs - curChunk.startPtsMs;
        curChunk.avgBitrateBps     = (double)curChunk.chunkSize * 8 * 1000 / curChunk.durationMs;
    }

    return 0;
}

// the default_* field in tfhd will overwrite the same filed in trex;
// the field in trun, if exist, overwrite the default_*
uint32_t MP4ParserImpl::fragmentGetSampleFlags(TrackExtendsBoxPtr pTrexBox, TrackFragmentHeaderBoxPtr pTfhdBox,
                                               TrackRunBoxPtr pTrunBox, uint64_t sampleIdx)
{
    if (sampleIdx >= pTrunBox->entryCount)
        return 0;

    if (pTrunBox->mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT)
    {
        return pTrunBox->getEntry<trunItem>(sampleIdx)->flags;
    }
    else if (pTrunBox->mFullboxFlags & MP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT && 0 == sampleIdx)
    {
        return pTrunBox->firstSampleFlags;
    }
    else
    {
        if (pTfhdBox->mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT)
        {
            return pTfhdBox->defaultSampleFlags;
        }
        else
        {
            return pTrexBox->defaultSampleFlags;
        }
    }
}
#define FRAG_IS_IFRAME(sample_flags) (((sample_flags) & FRAG_SAMPLE_FLAG_IS_NON_SYNC) == 0x00000000)

uint32_t MP4ParserImpl::fragmentGetSampleSize(TrackExtendsBoxPtr pTrexBox, TrackFragmentHeaderBoxPtr pTfhdBox,
                                              TrackRunBoxPtr pTrunBox, uint64_t sampleIdx)
{
    if (sampleIdx >= pTrunBox->entryCount)
        return 0;

    if (pTrunBox->mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT)
    {
        return pTrunBox->getEntry<trunItem>(sampleIdx)->size;
    }
    else
    {
        if (pTfhdBox->mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT)
        {
            return pTfhdBox->defaultSampleSize;
        }
        else
        {
            return pTrexBox->defaultSampleSize;
        }
    }
}

uint32_t MP4ParserImpl::fragmentGetSampleDuration(TrackExtendsBoxPtr pTrexBox, TrackFragmentHeaderBoxPtr pTfhdBox,
                                                  TrackRunBoxPtr pTrunBox, uint64_t sampleIdx)
{
    if (sampleIdx >= pTrunBox->entryCount)
        return 0;

    if (pTrunBox->mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT)
    {
        return pTrunBox->getEntry<trunItem>(sampleIdx)->duration;
    }
    else
    {
        if (pTfhdBox->mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT)
        {
            return pTfhdBox->defaultSampleDuration;
        }
        else
        {
            return pTrexBox->defaultSampleDuration;
        }
    }
}

uint32_t MP4ParserImpl::fragmentGetSampleCompositionOffset(TrackRunBoxPtr pTrunBox, uint64_t sampleIdx)
{
    if (sampleIdx >= pTrunBox->entryCount)
        return 0;

    if (pTrunBox->mFullboxFlags & MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT)
    {
        return pTrunBox->getEntry<trunItem>(sampleIdx)->composOffset;
    }
    else
    {
        return 0;
    }
}

int MP4ParserImpl::generateFragmentSamplesInfoTable(uint64_t trackIdx)
{
    CommonBoxPtr               pMvexBox;
    CommonBoxPtr               pMoovBox;
    vector<CommonBoxPtr>       pTrakBoxes;
    MediaHeaderBoxPtr          pMdhdBox;
    vector<TrackExtendsBoxPtr> pTrexBoxes;
    vector<CommonBoxPtr>       pMoofBoxes;
    vector<CommonBoxPtr>       pTrafBoxes;
    TrackFragmentHeaderBoxPtr  pTfhdBox;
    TrackRunBoxPtr             pTrunBox;

    pMoovBox = getContainBox("moov");
    if (pMoovBox == nullptr)
    {
        MP4_ERR("Get moov fail\n");
        return -1;
    }

    pTrakBoxes = pMoovBox->getContainBoxes("trak");
    if (pTrakBoxes.size() == 0)
    {
        MP4_ERR("Get trex fail\n");
        return -1;
    }
    if (trackIdx >= pTrakBoxes.size())
    {
        MP4_ERR("Num of trex err %llu %zu\n", trackIdx, pTrakBoxes.size());
        return -1;
    }

    pMdhdBox = pTrakBoxes[trackIdx]->getContainBoxRecursive<MediaHeaderBox>("mdhd", 2);
    if (pMdhdBox == nullptr)
    {
        MP4_ERR("Get mdhd fail\n");
        return -1;
    }

    pMvexBox = getContainBoxRecursive("mvex", 2);
    if (pMvexBox == nullptr)
    {
        MP4_ERR("Get mvex fail\n");
        return -1;
    }

    pTrexBoxes = pMvexBox->getContainBoxes<TrackExtendsBox>("trex");
    if (pTrexBoxes.size() == 0)
    {
        MP4_ERR("Get trex fail\n");
        return -1;
    }
    if (trackIdx >= pTrexBoxes.size())
    {
        MP4_ERR("Num of trex err %llu %zu\n", trackIdx, pTrexBoxes.size());
        return -1;
    }

    pMoofBoxes = getContainBoxes("moof");
    if (pMoofBoxes.size() == 0)
    {
        MP4_ERR("Get moof fail\n");
        return -1;
    }

    uint64_t               totalSampleCount = 0;
    uint64_t               sampleDts        = 0;
    vector<Mp4SampleItem> &sampleList       = tracksInfo[trackIdx]->mediaInfo->samplesInfo;

    for (uint64_t moofIdx = 0, moofCount = pMoofBoxes.size(); moofIdx < moofCount; ++moofIdx)
    {
        pTrafBoxes = pMoofBoxes[moofIdx]->getContainBoxes("traf");
        if (pTrafBoxes.size() == 0)
        {
            MP4_ERR("Get traf fail\n");
            return -1;
        }
        if (trackIdx >= pTrafBoxes.size())
        {
            MP4_ERR("Num of traf err %llu %zu\n", trackIdx, pTrafBoxes.size());
            return -1;
        }

        pTfhdBox = pTrafBoxes[trackIdx]->getContainBox<TrackFragmentHeaderBox>("tfhd");
        if (pTfhdBox == nullptr)
        {
            MP4_ERR("Get tfhd fail\n");
            return -1;
        }

        pTrunBox = pTrafBoxes[trackIdx]->getContainBox<TrackRunBox>("trun");
        if (pTrunBox == nullptr)
        {
            MP4_ERR("Get trun fail\n");
            return -1;
        }

        uint64_t fragOffset          = 0;
        uint64_t fragTotalSampleSize = 0;

        printf("==== tfhd fullbox flags=0x%x\n", pTfhdBox->mFullboxFlags);
        // calculate fragment data offset in file
        if (pTfhdBox->mFullboxFlags & MP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT)
            fragOffset += pTfhdBox->baseDataOffset;
        else if (pTfhdBox->mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF)
            fragOffset += pMoofBoxes[moofIdx]->mBodyPos;
        printf("==== fragOffset=%llu\n", fragOffset);
        printf("==== trun fullbox flags=0x%x\n", pTrunBox->mFullboxFlags);
        if (pTrunBox->mFullboxFlags & MP4_TRUN_FLAG_DATA_OFFSET_PRESENT)
            fragOffset += pTrunBox->dataOffset;
        printf("==== fragOffset=%llu %u\n", fragOffset, pTrunBox->dataOffset);

        for (uint64_t sampleIdx = 0, sampleCount = pTrunBox->entryCount; sampleIdx < sampleCount; ++sampleIdx)
        {
            Mp4SampleItem curSample;
            uint32_t      flags = fragmentGetSampleFlags(pTrexBoxes[trackIdx], pTfhdBox, pTrunBox, sampleIdx);

            curSample.sampleIdx    = totalSampleCount;
            curSample.sampleOffset = fragOffset + fragTotalSampleSize;
            curSample.sampleSize   = fragmentGetSampleSize(pTrexBoxes[trackIdx], pTfhdBox, pTrunBox, sampleIdx);
            fragTotalSampleSize += curSample.sampleSize;
            curSample.isKeyFrame = FRAG_IS_IFRAME(flags);

            curSample.dtsMs = sampleDts;

            curSample.dtsDeltaMs = fragmentGetSampleDuration(pTrexBoxes[trackIdx], pTfhdBox, pTrunBox, sampleIdx) * 1000
                                 / pMdhdBox->timescale;

            sampleDts += curSample.dtsDeltaMs;

            curSample.ptsMs =
                curSample.dtsMs + fragmentGetSampleCompositionOffset(pTrunBox, sampleIdx) * 1000 / pMdhdBox->timescale;
            printf("fragOffset=%llu, fragTotalSampleSize=%llu, sampleIdx=%llu, sampleSize=%llu, dts=%llu, pts=%llu\n",
                   fragOffset, fragTotalSampleSize, curSample.sampleIdx, curSample.sampleSize, curSample.dtsMs,
                   curSample.ptsMs);
            sampleList.push_back(curSample);

            totalSampleCount++;
        }
    }

    totalSampleCount = sampleList.size();

    uint64_t     gopCount = 0;
    Mp4ChunkItem curGop;

    for (uint64_t sampleIdx = 0; sampleIdx < totalSampleCount; ++sampleIdx)
    {
        if (0 == curGop.chunkOffset)
        {
            curGop.chunkOffset = sampleList[sampleIdx].sampleOffset;
        }

        if (0 == curGop.sampleStartIdx)
        {
            curGop.sampleStartIdx = sampleList[sampleIdx].sampleIdx;
            curGop.startPtsMs     = sampleList[sampleIdx].ptsMs;
        }

        curGop.chunkSize += sampleList[sampleIdx].sampleSize;
        curGop.durationMs += sampleList[sampleIdx].dtsDeltaMs;
        curGop.sampleCount++;

        if (sampleIdx == totalSampleCount - 1 || sampleList[sampleIdx + 1].isKeyFrame)
        {
            gopCount++;
            curGop.chunkIdx      = gopCount;
            curGop.avgBitrateBps = (double)curGop.chunkSize * 8 * 1000 / curGop.durationMs;
            tracksInfo[trackIdx]->mediaInfo->chunksInfo.push_back(curGop);
            curGop = Mp4ChunkItem();
        }
    }

    return 0;
}

int Mp4MediaInfo::getInfoFromTrack(Mp4BoxPtr trakBox, TrackInfoPtr track)
{
    auto pTrakBox = dynamic_pointer_cast<const CommonBox>(trakBox);
    if (pTrakBox == nullptr)
    {
        MP4_ERR("trak box missing\n");
        return -1;
    }

    SampleDescriptionBoxPtr stsd = pTrakBox->getContainBoxRecursive<SampleDescriptionBox>("stsd");
    MovieHeaderBoxPtr       mvhd =
        pTrakBox->getUpperBox() == nullptr ? nullptr : pTrakBox->getUpperBox()->getContainBox<MovieHeaderBox>("mvhd");
    TrackHeaderBoxPtr tkhd = pTrakBox->getContainBoxRecursive<TrackHeaderBox>("tkhd");
    if (stsd == nullptr || tkhd == nullptr)
    {
        MP4_ERR("stsd/tkhd not found %p %p\n", stsd.get(), tkhd.get());
        return -1;
    }

    if (tkhd->duration > 0 && mvhd != nullptr)
    {
        durationMs = tkhd->duration * 1000 / mvhd->timescale;
    }
    else
    {
        durationMs = 0;
        for (auto &chunkInfo : track->mediaInfo->chunksInfo)
        {
            durationMs += chunkInfo.durationMs;
        }
        if (track->mediaInfo->samplesInfo.size() >= 1)
            durationMs -= track->mediaInfo->samplesInfo[0].dtsDeltaMs;
    }

    totalSize = 0;
    for (auto &chunkInfo : track->mediaInfo->chunksInfo)
    {
        totalSize += chunkInfo.chunkSize;
    }

    avgBitrate = (double)totalSize * 8. * 1000. / durationMs;

    sampleCount = track->mediaInfo->samplesInfo.size();

    codecCode = getCodecFromStsd(stsd);

    return 0;
}

string Mp4MediaInfo::getInfoString()
{
    std::stringstream infoStr;
    uint64_t          msec = durationMs % 1000;
    uint64_t          sec  = (durationMs / 1000) % 60;
    uint64_t          min  = (durationMs / 1000 / 60) % 60;
    uint64_t          hour = durationMs / 1000 / 60 / 60;

    if (durationMs > 60 * 60 * 1000)
    {
        infoStr << "\tDuration: " << hour << "h " << min << "m " << sec << "s " << msec << "ms" << endl;
    }
    else if (durationMs > 60 * 1000)
    {
        infoStr << "\tDuration: " << min << "m " << sec << "s " << msec << "ms" << endl;
    }
    else if (durationMs > 1000)
    {
        infoStr << "\tDuration: " << sec << "s " << msec << "ms" << endl;
    }
    else
    {
        infoStr << "\tDuration: " << msec << "ms" << endl;
    }

    infoStr << "\tSample count: " << sampleCount << endl;
    if (totalSize * 8 > 1024 * 1024)
        infoStr << "\tBitstream Size: " << totalSize * 8. / 1024 / 1024 << "MBits" << endl;
    else if (totalSize * 8 > 1024)
        infoStr << "\tBitstream Size: " << totalSize * 8. / 1024 << "KBits" << endl;
    else
        infoStr << "\tBitstream Size: " << totalSize * 8. << "Bits" << endl;

    infoStr << "\taverage bitrate: " << avgBitrate / 1024. << "Kbps\n";

    return infoStr.str();
}

int Mp4VideoInfo::getInfoFromTrack(Mp4BoxPtr trakBox, TrackInfoPtr track)
{
    if (Mp4MediaInfo::getInfoFromTrack(trakBox, track) < 0)
        return -1;

    auto pTrakBox = dynamic_pointer_cast<const CommonBox>(trakBox);
    if (pTrakBox == nullptr)
    {
        MP4_ERR("trak box missing\n");
        return -1;
    }

    if (track->trackType != TRACK_TYPE_VIDEO)
    {
        MP4_ERR("trackType err %s\n", mp4GetTrackTypeName(track->trackType).c_str());
    }
    mediaType = MP4_MEDIA_TYPE_VIDEO;

    TrackHeaderBoxPtr tkhd = pTrakBox->getContainBoxRecursive<TrackHeaderBox>("tkhd");
    if (tkhd == nullptr)
    {
        MP4_ERR("tkhd not found\n");
        return -1;
    }

    width  = (unsigned int)tkhd->width;
    height = (unsigned int)tkhd->height;

    if (durationMs > 0)
        avgFps = sampleCount * 1000.f / durationMs;
    else
        avgFps = NAN;
    MP4_INFO("codec_type = %d\n", mp4GetCodecType(codecCode));
    if (mp4GetCodecType(codecCode) == MP4_CODEC_H264)
    {
        AVCConfigurationBoxPtr avcc = pTrakBox->getContainBoxRecursive<AVCConfigurationBox>("avcC");
        if (avcc != nullptr)
        {
            profile       = avcc->AVCConfig.avcProfileIndication;
            profileCompat = avcc->AVCConfig.profileCompat;
            level         = avcc->AVCConfig.avcLvIndication;
        }
    }
    else if (mp4GetCodecType(codecCode) == MP4_CODEC_HEVC)
    {
        HEVCConfigurationBoxPtr hvcc = pTrakBox->getContainBoxRecursive<HEVCConfigurationBox>("hvcC");
        if (hvcc != nullptr)
        {
            profile       = hvcc->HEVCConfig.generalProfileIdc;
            profileCompat = hvcc->HEVCConfig.generalProfileCompatFlags;
            level         = hvcc->HEVCConfig.generalLevelIdc;
        }
    }

    return 0;
}
std::string getProfileString(unsigned int profileIdc)
{
    if (profileIdc < 77)
    {
        return "Baseline";
    }
    else if (profileIdc < 88)
    {
        return "Main";
    }
    else if (profileIdc < 100)
    {
        return "Extended";
    }
    else if (profileIdc < 110)
    {
        return "High";
    }
    else if (profileIdc < 122)
    {
        return "High 10";
    }
    else if (profileIdc < 144)
    {
        return "High 4:2:2";
    }
    else
    {
        return "High 4:4:4";
    }
}
string Mp4VideoInfo::getInfoString()
{
    std::stringstream infoStr;
    infoStr << "\tCodec type: " << mp4GetCodecName(codecCode) << endl;
    infoStr << "\tProfile: " << getProfileString(profile) << endl;
    infoStr << "\tLevel: " << level << endl;
    infoStr << "\tResolution: " << width << "x" << height << endl;
    infoStr << "\tAverage Framerate: " << avgFps << "fps" << endl;

    infoStr << Mp4MediaInfo::getInfoString();
    return infoStr.str();
}

int Mp4AudioInfo::getInfoFromTrack(Mp4BoxPtr trakBox, TrackInfoPtr track)
{
    if (Mp4MediaInfo::getInfoFromTrack(trakBox, track) < 0)
        return -1;

    auto pTrakBox = dynamic_pointer_cast<const CommonBox>(trakBox);
    if (pTrakBox == nullptr)
    {
        MP4_ERR("trak box missing\n");
        return -1;
    }

    if (track->trackType != TRACK_TYPE_AUDIO)
    {
        MP4_ERR("type err %s\n", mp4GetTrackTypeName(track->trackType).c_str());
    }

    mediaType = MP4_MEDIA_TYPE_AUDIO;

    CommonBoxPtr stsd = pTrakBox->getContainBoxRecursive("stsd");
    if (stsd == nullptr)
    {
        MP4_ERR("stsd/ not found\n");
        return -1;
    }

    MP4AudioSampleEntryPtr mp4a = stsd->getContainBox<MP4AudioSampleEntry>("mp4a");
    if (mp4a == nullptr)
        return -1;
    channels        = mp4a->channelCount;
    audioSampleRate = mp4a->sampleRate;
    audioSampleSize = mp4a->sampleSize;

    return 0;
}

string Mp4AudioInfo::getInfoString()
{
    std::stringstream infoStr;
    infoStr << "\tCodec type: " << mp4GetCodecName(codecCode) << endl;
    infoStr << "\tChannels: " << channels << endl;
    infoStr << "\tSample Rate: " << audioSampleRate << endl;
    infoStr << "\tSample Size: " << audioSampleSize << endl;
    infoStr << Mp4MediaInfo::getInfoString();

    return infoStr.str();
}

string Mp4TrackInfo::getInfoString()
{
    std::stringstream infoStr;
    infoStr << "Track ID: " << trackId << endl;
    infoStr << "\tTrack Type: " << mp4GetTrackTypeName(trackType) << endl;

    if (mediaInfo != nullptr)
        infoStr << mediaInfo->getInfoString();

    return infoStr.str();
}

std::shared_ptr<Mp4BoxData> Mp4MediaInfo::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Duration(ms)", durationMs)
        ->kvAddPair("Sample Count", sampleCount)
        ->kvAddPair("Codec", mp4GetCodecName(codecCode))
        ->kvAddPair("Media Type", mp4GetMediaTypeName(mediaType));

    return item;
}
