
#include <stdarg.h>
#include <sstream>

#include "Mp4Parse.h"
#include "Mp4ParseInternal.h"
#include "Mp4BoxTypes.h"
#include "Mp4SampleEntryTypes.h"
#include "Mp4SampleTableTypes.h"

using std::dynamic_pointer_cast;
using std::endl;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;

void defaultLogCallback(MP4_LOG_LEVEL_E logLevel, const char *logBuffer)
{
    switch (logLevel)
    {
        case MP4_LOG_LEVEL_ERR:
            fprintf(stderr, "[ERROR]%s", logBuffer);
            break;
        case MP4_LOG_LEVEL_WARN:
            fprintf(stdout, "[WARN]%s", logBuffer);
            break;
        case MP4_LOG_LEVEL_INFO:
            fprintf(stdout, "[INFO]%s", logBuffer);
            break;
        case MP4_LOG_LEVEL_DBG:
            fprintf(stdout, "[DEBUG]%s", logBuffer);
            break;
    }
}

std::string mp4GetFrameTypeStr(H26X_FRAME_TYPE_E frameType)
{
    switch (frameType)
    {
        case H26X_FRAME_I:
            return "I";
        case H26X_FRAME_P:
            return "P";
        case H26X_FRAME_B:
            return "B";
        default:
        case H26X_FRAME_Unknown:
            return "Unknown";
    }
}
std::string mp4GetNaluTypeStr(MP4_CODEC_TYPE_E codecType, int naluType)
{
    if (MP4_CODEC_H264 == codecType)
    {
        H264_NALU_TYPE_E h264NaluType = (H264_NALU_TYPE_E)naluType;
        switch (h264NaluType)
        {
            case H264_NALU_UNKNOWN:
                return "Unknown";
            case H264_NALU_SLICE:
                return "H264 Slice";
            case H264_NALU_SLICE_DPA:
                return "H264 Slice DPA";
            case H264_NALU_SLICE_DPB:
                return "H264 Slice DPB";
            case H264_NALU_SLICE_DPC:
                return "H264 Slice DPC";
            case H264_NALU_SLICE_IDR:
                return "H264 Slice IDR";
            case H264_NALU_SEI:
                return "H264 SEI";
            case H264_NALU_SPS:
                return "H264 SPS";
            case H264_NALU_PPS:
                return "H264 PPS";
            default:
                return "H264 Nalu Type " + std::to_string(h264NaluType);
        }
    }
    else if (MP4_CODEC_HEVC == codecType)
    {
        H265_NALU_TYPE_E h265NaluType = (H265_NALU_TYPE_E)naluType;
        switch (h265NaluType)
        {
            case H265_NALU_TRAIL_N:
            case H265_NALU_TRAIL_R:
                return "H265 Slice";
            case H265_NALU_TSA_N:
            case H265_NALU_TSA_R:
                return "H265 TSA Slice";
            case H265_NALU_STSA_N:
            case H265_NALU_STSA_R:
                return "H265 STSA Slice";
            case H265_NALU_RADL_N:
            case H265_NALU_RADL_R:
                return "H265 RADL Slice";
            case H265_NALU_RASL_N:
            case H265_NALU_RASL_R:
                return "H265 RASL Slice";
            case H265_NALU_BLA_W_LP:
            case H265_NALU_BLA_W_RADL:
            case H265_NALU_BLA_N_LP:
                return "H265 BLA Slice";
            case H265_NALU_IDR_W:
            case H265_NALU_IDR_N:
                return "H265 IDR Slice";
            case H265_NALU_CRA_NUT:
                return "H265 CRA Slice";
            case H265_NALU_VPS:
                return "H265 VPS";
            case H265_NALU_SPS:
                return "H265 SPS";
            case H265_NALU_PPS:
                return "H265 PPS";
            case H265_NALU_PRE_SEI:
            case H265_NALU_SUF_SEI:
                return "H265 SEI";
            default:
                return "H265 Nalu Type " + std::to_string(h265NaluType);
        }
    }

    return "Wrong Codec";
}
std::shared_ptr<Mp4Parser> createMp4Parser()
{
    return make_shared<MP4ParserImpl>();
}

void MP4ParserImpl::clear()
{
    mAvailable = false;
    tracksInfo.clear();
    mContainBoxes.clear();

    while (!mErrors.empty())
        mErrors.pop();

    mFileReader.close();
}

string MP4ParserImpl::getErrorMessage()
{
    if (mErrors.empty())
        return "";
    std::string err = mErrors.front();
    mErrors.pop();
    return err;
};

int MP4ParserImpl::parse(string filepath)
{
    int ret = 0;

    clear();

    ret = mFileReader.open(filepath);
    if (ret < 0)
        return ret;

    while (mFileReader.getCursorPos() < mFileReader.getFileSize())
    {
        bool         parseErr = false;
        CommonBoxPtr curBox   = parseBox(mFileReader, nullptr, parseErr);
        if (curBox == nullptr)
            break;

        mContainBoxes.push_back(curBox);

        if (parseErr)
            break;
    }

    mMp4Type = MP4_TYPE_ISO;
    if (getContainBoxRecursive<CommonBox>("moof") != nullptr || getContainBoxRecursive<CommonBox>("mvex") != nullptr)
        mMp4Type = MP4_TYPE_FRAGMENT;
    if (MP4_TYPE_ISO == mMp4Type)
    {
        auto moov = getContainBox("moov");
        if (!moov)
        {
            MP4_PARSE_ERR("moov not found\n");
            return -1;
        }
        auto tackBoxes = moov->getContainBoxes("trak");
        if (tackBoxes.empty())
        {
            MP4_PARSE_ERR("there's no trak\n");
            return -1;
        }
    }
    CommonBoxPtr curBox;
    if ((curBox = getContainBoxRecursive<CommonBox>("ftyp")) != nullptr)
    {
        FileTypeBoxPtr pFtypBox = dynamic_pointer_cast<FileTypeBox>(curBox);
        if (pFtypBox == nullptr)
        {
            auto rawPtr = curBox.get();
            MP4_PARSE_ERR("cast fail %s\n", typeid(*rawPtr).name());
            return -1;
        }
    }

    if ((curBox = getContainBoxRecursive<CommonBox>("moov", 1)) != nullptr)
    {
        MovieHeaderBoxPtr    mvhd;
        CommonBoxPtr         mvex;
        vector<CommonBoxPtr> trakBoxes;

        mvhd = curBox->getContainBox<MovieHeaderBox>("mvhd");
        if (nullptr == mvhd)
        {
            MP4_PARSE_ERR("get mvhd fail\n");
        }
        if (mvhd != nullptr)
        {
            if (mCreationTime == 0 && mvhd->creationTime > 0)
            {
                mCreationTime = mvhd->creationTime;
            }
            if (mModificationTime == 0 && mvhd->modificationTime > 0)
            {
                mModificationTime = mvhd->modificationTime;
            }
        }

        mvex      = curBox->getContainBox<ContainBox>("mvex");
        trakBoxes = curBox->getContainBoxes("trak");
        if (mvex != nullptr)
        {
            vector<CommonBoxPtr> trexBoxes = mvex->getContainBoxes("trex");
            if (trexBoxes.size() != trakBoxes.size())
                MP4_PARSE_ERR("err %zu != %zu\n", trexBoxes.size(), trakBoxes.size());
        }

        if (mvhd != nullptr && trakBoxes.size() != mvhd->trackCount)
        {
            MP4_WARN("track count may not right %zu %d\n", trakBoxes.size(), mvhd->trackCount);
        }
        uint32_t trakIdx = 0;

        for (auto &curTrak : trakBoxes)
        {
            auto curTrackInfo = make_shared<Mp4TrackInfo>();

            TimeToSampleBoxPtr stts;

            TrackHeaderBoxPtr tkhd = curTrak->getContainBox<TrackHeaderBox>("tkhd");
            if (tkhd != nullptr)
            {
                curTrackInfo->trakIndex = trakIdx++;
                curTrackInfo->trackId   = tkhd->trackId;
                if (mCreationTime == 0 && tkhd->creationTime > 0)
                {
                    mCreationTime = tkhd->creationTime;
                }
                if (mModificationTime == 0 && tkhd->modificationTime > 0)
                {
                    mModificationTime = tkhd->modificationTime;
                }
            }
            else
            {
                MP4_PARSE_ERR("tkhd not found\n");
            }

            HandlerBoxPtr hdlr      = curTrak->getContainBoxRecursive<HandlerBox>("hdlr");
            curTrackInfo->trackType = TRACK_TYPE_BUTT;
            if (hdlr != nullptr)
            {
                for (int trackType = 0; trackType < TRACK_TYPE_BUTT; ++trackType)
                {
                    if (hdlr->handlerType == mp4GetHandlerName((MP4_TRACK_TYPE_E)trackType))
                    {
                        curTrackInfo->trackType = (MP4_TRACK_TYPE_E)trackType;
                        break;
                    }
                }
            }

            if (curTrackInfo->trackType == TRACK_TYPE_VIDEO)
            {
                curTrackInfo->mediaInfo = make_shared<Mp4VideoInfo>();
            }
            else if (curTrackInfo->trackType == TRACK_TYPE_AUDIO)
            {
                curTrackInfo->mediaInfo = make_shared<Mp4AudioInfo>();
            }
            else
            {
                MP4_WARN("unsupported track type %s\n", mp4GetTrackTypeName(curTrackInfo->trackType).c_str());
                curTrackInfo->mediaInfo = make_shared<Mp4MediaInfo>();
            }

            tracksInfo.push_back(curTrackInfo);
        }
    }
    else
    {
        MP4_PARSE_ERR("get moov fail\n");
    }

    for (unsigned int i = 0; i < tracksInfo.size(); ++i)
    {
        generateInfoTable(i);
    }

    mAvailable = true;

    return 0;
}

bool MP4ParserImpl::isTrackHasProperty(uint32_t trackIdx, MP4_TRACK_PROPERTY_E prop) const
{
    CommonBoxPtr         moov      = getContainBox("moov");
    vector<CommonBoxPtr> trakBoxes = moov->getContainBoxes("trak");
    if (trackIdx >= trakBoxes.size())
    {
        MP4_ERR("wrong idx %d\n", trackIdx);
        return false;
    }
    CommonBoxPtr tkhd = trakBoxes[trackIdx]->getContainBox("tkhd");
    if (tkhd != nullptr)
        return (tkhd->mFullboxFlags & prop) != 0;
    else
        return false;
}

void copySampleInfo(Mp4SampleItem &src, Mp4RawSample &dst)
{
    dst.sampleIdx  = src.sampleIdx;
    dst.fileOffset = src.sampleOffset;
    dst.sampleSize = dst.dataSize = src.sampleSize;
    dst.dtsMs                     = src.dtsMs;
    dst.ptsMs                     = src.ptsMs;
}

int MP4ParserImpl::getSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4RawSample &outSample)
{
    if (!mAvailable)
        return -1;

    if (trackIdx >= tracksInfo.size())
        return -1;

    if (sampleIdx > tracksInfo[trackIdx]->mediaInfo->samplesInfo.size())
        return -1;

    CommonBoxPtr         moov       = getContainBox("moov");
    vector<CommonBoxPtr> trakBoxes  = moov->getContainBoxes("trak");
    CommonBoxPtr         curTrakBox = trakBoxes[trackIdx];
    TrackHeaderBoxPtr    tkhd       = curTrakBox->getContainBox<TrackHeaderBox>("tkhd");

    Mp4SampleItem *curSample = &tracksInfo[trackIdx]->mediaInfo->samplesInfo[sampleIdx];
    outSample.trackIdx       = trackIdx;
    copySampleInfo(*curSample, outSample);

    outSample.sampleData = shared_ptr<uint8_t[]>(new uint8_t[outSample.dataSize]);

    mFileReader.readAt(outSample.fileOffset, outSample.sampleData.get(), outSample.sampleSize);
    return 0;
}

int MP4ParserImpl::getH26xFrame(uint32_t trackIdx, uint32_t sampleIdx, Mp4VideoFrame &outFrame)
{
    CommonBoxPtr         moov       = getContainBox("moov");
    vector<CommonBoxPtr> trakBoxes  = moov->getContainBoxes("trak");
    CommonBoxPtr         curTrakBox = trakBoxes[trackIdx];
    TrackHeaderBoxPtr    tkhd       = curTrakBox->getContainBox<TrackHeaderBox>("tkhd");

    Mp4SampleItem *pCurSample = &tracksInfo[trackIdx]->mediaInfo->samplesInfo[sampleIdx];
    uint64_t       samplePos  = pCurSample->sampleOffset;
    uint64_t       sampleSize = pCurSample->sampleSize;
    bool           attachNalu = false;
    if (pCurSample->isKeyFrame > 0)
    {
        attachNalu = true;
    }

    vector<BinaryData> naluAttach;
    uint64_t           attachSize = 0;
    int                lengthSize = 0;
    CommonBoxPtr       stsd       = curTrakBox->getContainBoxRecursive("stsd");

    if (stsd->mContainBoxes.size() <= 0)
    {
        MP4_PARSE_ERR("No SampleEntry Found\n");
        return -1;
    }

    switch (getCompatibleBoxType(stsd->mContainBoxes[0]->mBoxType))
    {
        case MP4_BOX_MAKE_TYPE("hvc1"):
        {
            CommonBoxPtr hvc1 = stsd->mContainBoxes[0];

            HEVCConfigurationBoxPtr hvcC = std::dynamic_pointer_cast<HEVCConfigurationBox>(hvc1->getContainBox("hvcC"));
            if (hvcC == nullptr)
            {
                MP4_PARSE_ERR("hvcC is null\n");
                return -1;
            }
            lengthSize = hvcC->HEVCConfig.lengthSize;
            if (attachNalu)
            {
                for (unsigned int i = 0; i < hvcC->HEVCConfig.arrayCount; ++i)
                {
                    for (unsigned int j = 0; j < hvcC->HEVCConfig.arrays[i].numNalus; j++)
                    {
                        naluAttach.push_back(hvcC->HEVCConfig.arrays[i].nalus[j].data);
                        attachSize += (hvcC->HEVCConfig.arrays[i].nalus[j].length + 4);
                    }
                }
            }
            break;
        }
        case MP4_BOX_MAKE_TYPE("avc1"):
        {
            CommonBoxPtr avc = stsd->mContainBoxes[0];

            AVCConfigurationBoxPtr avcC = dynamic_pointer_cast<AVCConfigurationBox>(avc->getContainBox("avcC"));
            if (avcC == nullptr)
            {
                MP4_PARSE_ERR("avcC is null\n");
                return -1;
            }
            lengthSize = avcC->AVCConfig.lengthSize;
            if (attachNalu)
            {
                for (unsigned int i = 0; i < avcC->AVCConfig.spsCount; ++i)
                {
                    naluAttach.push_back(avcC->AVCConfig.sps[i].data);
                    attachSize += (avcC->AVCConfig.sps[i].length + 4);
                }
                for (unsigned int i = 0; i < avcC->AVCConfig.ppsCount; ++i)
                {
                    naluAttach.push_back(avcC->AVCConfig.pps[i].data);
                    attachSize += (avcC->AVCConfig.pps[i].length + 4);
                }

                if (avcC->AVCConfig.spseCount != 0)
                {
                    for (unsigned int i = 0; i < avcC->AVCConfig.spseCount; ++i)
                    {
                        naluAttach.push_back(avcC->AVCConfig.spse[i].data);
                        attachSize += (avcC->AVCConfig.spse[i].length + 4);
                    }
                }
            }
            break;
        }
        default:
            MP4_PARSE_ERR("Unknown type %s\n", boxType2Str(stsd->mContainBoxes[0]->mBoxType).c_str());
            break;
    }
    outFrame.trackIdx = trackIdx;
    copySampleInfo(*pCurSample, outFrame);
    outFrame.dataSize += attachSize;
    outFrame.sampleData = shared_ptr<uint8_t[]>(new uint8_t[outFrame.dataSize]);

    uint8_t *frameData = outFrame.sampleData.get();
    uint64_t copyPos   = 0;

    for (unsigned int i = 0; i < naluAttach.size(); ++i)
    {
        frameData[copyPos]     = 0;
        frameData[copyPos + 1] = 0;
        frameData[copyPos + 2] = 0;
        frameData[copyPos + 3] = 1;

        copyPos += 4;
        memcpy(frameData + copyPos, naluAttach[i].ptr(), naluAttach[i].length);
        copyPos += naluAttach[i].length;
    }

    uint64_t pos = mFileReader.getCursorPos();
    mFileReader.setCursor(samplePos);
    while (sampleSize > 0)
    {
        uint32_t naluSize = 0;

        mFileReader.read(&naluSize + 4 - lengthSize, lengthSize);
        naluSize = bswap_32(naluSize);

        frameData[copyPos]     = 0;
        frameData[copyPos + 1] = 0;
        frameData[copyPos + 2] = 0;
        frameData[copyPos + 3] = 1;

        copyPos += 4;
        sampleSize -= 4;
        mFileReader.read(frameData + copyPos, naluSize);
        copyPos += naluSize;
        sampleSize -= naluSize;
    }
    mFileReader.setCursor(pos);

    outFrame.mediaType  = MP4_MEDIA_TYPE_VIDEO;
    outFrame.codec      = mp4GetCodecType(tracksInfo[trackIdx]->mediaInfo->codecCode);
    outFrame.width      = (unsigned int)tkhd->width;
    outFrame.height     = (unsigned int)tkhd->height;
    outFrame.isKeyFrame = attachNalu;

    return 0;
}

int MP4ParserImpl::getVideoSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4VideoFrame &outFrame)
{
    auto codecType = mp4GetCodecType(tracksInfo[trackIdx]->mediaInfo->codecCode);
    if (MP4_CODEC_H264 == codecType || MP4_CODEC_HEVC == codecType)
    {
        getH26xFrame(trackIdx, sampleIdx, outFrame);
    }
    else
    {
        CommonBoxPtr         moov        = getContainBox("moov");
        vector<CommonBoxPtr> trakBoxes   = moov->getContainBoxes("trak");
        CommonBoxPtr         pCurTrakBox = trakBoxes[trackIdx];
        TrackHeaderBoxPtr    tkhd        = pCurTrakBox->getContainBox<TrackHeaderBox>("tkhd");

        Mp4SampleItem *curSample = &tracksInfo[trackIdx]->mediaInfo->samplesInfo[sampleIdx];

        getSample(trackIdx, sampleIdx, outFrame);

        outFrame.mediaType             = MP4_MEDIA_TYPE_VIDEO;
        outFrame.codec                 = mp4GetCodecType(tracksInfo[trackIdx]->mediaInfo->codecCode);
        shared_ptr<Mp4VideoInfo> pinfo = dynamic_pointer_cast<Mp4VideoInfo>(tracksInfo[trackIdx]->mediaInfo);
        if (pinfo == nullptr)
        {
            auto rawPtr = tracksInfo[trackIdx]->mediaInfo.get();
            MP4_PARSE_ERR("cast fail %s\n", typeid(*rawPtr).name());
            return -1;
        }

        outFrame.width      = (unsigned int)tkhd->width;
        outFrame.height     = (unsigned int)tkhd->height;
        outFrame.isKeyFrame = curSample->isKeyFrame;
    }

    return 0;
}

#define ADTS_HEAD_SIZE 7

int sampleRateTable[][2] = {
    { 0, 96000},
    { 1, 88200},
    { 2, 64000},
    { 3, 48000},
    { 4, 44100},
    { 5, 32000},
    { 6, 24000},
    { 7, 22050},
    { 8, 16000},
    { 9, 12000},
    {10, 11025},
    {11,  8000},
    {12,  7350},
};

int writeAdts(uint8_t *buf, uint8_t codec, uint64_t size, int sampleRate, int channel)
{
    int profile;
    switch (codec)
    {
        case 0x66:
            profile = 1; // Main
            break;
        case 0x68:
            profile = 3; // SSR
            break;
        default:
        case 0x67:
        case 0x40:
            profile = 2; // LC
            break;
    }
    profile--;
    int sampleRateIdx = 13;
    if (channel == 8)
        channel = 7;
    for (unsigned int i = 0; i < ARRAY_SIZE(sampleRateTable); ++i)
    {
        if (sampleRate == sampleRateTable[i][1])
        {
            sampleRateIdx = sampleRateTable[i][0];
        }
    }

    BitsWriter bitsWriter(buf, 7);
    bitsWriter.writBits(12, 0xfff);  /* syncword */
    bitsWriter.writBits(1, 0);       /* ID */
    bitsWriter.writBits(2, 0b00);    /* layer */
    bitsWriter.writBits(1, 1);       /* protection_absent */
    bitsWriter.writBits(2, profile); /* profile_objecttype */
    bitsWriter.writBits(4, sampleRateIdx);
    bitsWriter.writBits(1, 0);       /* private_bit */
    bitsWriter.writBits(3, channel); /* channel_configuration */
    bitsWriter.writBits(1, 0);       /* original_copy */
    bitsWriter.writBits(1, 0);       /* home */

    /* adts_variable_header */
    bitsWriter.writBits(1, 0);                      /* copyright_identification_bit */
    bitsWriter.writBits(1, 0);                      /* copyright_identification_start */
    bitsWriter.writBits(13, size + ADTS_HEAD_SIZE); /* aac_frame_length */
    bitsWriter.writBits(11, 0x7ff);                 /* adts_buffer_fullness */
    bitsWriter.writBits(2, 0);                      /* number_of_raw_data_blocks_in_frame */

    return 0;
}

int MP4ParserImpl::getAudioSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4AudioFrame &outFrame)
{
    Mp4SampleItem *curSample = &tracksInfo[trackIdx]->mediaInfo->samplesInfo[sampleIdx];

    copySampleInfo(*curSample, outFrame);
    outFrame.dataSize += ADTS_HEAD_SIZE;
    outFrame.sampleData = shared_ptr<uint8_t[]>(new uint8_t[outFrame.dataSize]);

    shared_ptr<Mp4AudioInfo> audioInfo = dynamic_pointer_cast<Mp4AudioInfo>(tracksInfo[trackIdx]->mediaInfo);
    if (audioInfo == nullptr)
    {
        auto rawPtr = tracksInfo[trackIdx]->mediaInfo.get();
        MP4_PARSE_ERR("cast fail %s\n", typeid(*rawPtr).name());
        return -1;
    }
    writeAdts(outFrame.sampleData.get(), audioInfo->codecCode, outFrame.sampleSize, audioInfo->audioSampleRate,
              audioInfo->channels);

    mFileReader.readAt(outFrame.fileOffset, outFrame.sampleData.get() + ADTS_HEAD_SIZE, outFrame.sampleSize);

    outFrame.mediaType       = MP4_MEDIA_TYPE_AUDIO;
    outFrame.codec           = mp4GetCodecType(audioInfo->codecCode);
    outFrame.audioSampleRate = audioInfo->audioSampleRate;
    outFrame.audioSampleSize = audioInfo->audioSampleSize;
    outFrame.channels        = audioInfo->channels;

    return 0;
}

string MP4ParserImpl::getBasicInfoString() const
{
    stringstream infoString;

    if (!mAvailable)
    {
        return string();
    }

    FileTypeBoxPtr ftyp = getContainBox<FileTypeBox>("ftyp");

    infoString << "File: " << mFileReader.getFileBaseName() << mFileReader.getFileExtension() << endl;

    infoString << "Size: " << (double)mFileReader.getFileSize() / 1024 / 1024 << "MB" << endl;
    infoString << "Creation Time:" << getTimeString(mCreationTime) << endl;
    infoString << "Modification Time:" << getTimeString(mModificationTime) << endl;
    infoString << "Major brand: " << ftyp->majorBrand << endl;
    infoString << "Minor version: " << ftyp->minorVersion << endl;
    infoString << "Compatible: ";
    for (size_t i = 0, im = ftyp->compatibles.size(); i < im; ++i)
    {
        infoString << ftyp->compatibles[i];
        if (i < im - 1)
            infoString << ", ";
    }
    infoString << endl;

    for (unsigned int i = 0; i < tracksInfo.size(); ++i)
    {
        infoString << tracksInfo[i]->getInfoString();
    }

    return infoString.str();
}

std::shared_ptr<Mp4BoxData> Mp4VideoInfo::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    Mp4MediaInfo::getData(item);

    item->kvAddPair("Profile", getProfileString(profile))
        ->kvAddPair("Level", level)
        ->kvAddPair("Width", width)
        ->kvAddPair("Height", height);

    return item;
}
std::shared_ptr<Mp4BoxData> Mp4AudioInfo::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    Mp4MediaInfo::getData(item);

    item->kvAddPair("Channels", channels)
        ->kvAddPair("Sample Rate", audioSampleRate)
        ->kvAddPair("Sample Size", audioSampleSize);

    return item;
}
std::shared_ptr<Mp4BoxData> Mp4TrackInfo::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    item->kvAddPair("Track ID", trackId)->kvAddPair("Type", mp4GetTrackTypeName(trackType));

    if (mediaInfo->mediaType == MP4_MEDIA_TYPE_VIDEO)
        return mediaInfo->getData(item);
    else if (mediaInfo->mediaType == MP4_MEDIA_TYPE_AUDIO)
        return mediaInfo->getData(item);

    return item;
}

std::shared_ptr<Mp4BoxData> MP4ParserImpl::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;

    auto ftyp = getContainBox<FileTypeBox>("ftyp");
    if (ftyp != nullptr)
    {
        std::shared_ptr<Mp4BoxData> fileInfoItem = item->kvAddKey("File Info", MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS);

        fileInfoItem->kvAddPair("Major Brand", ftyp->majorBrand)->kvAddPair("Minor Version", ftyp->minorVersion);
        std::string compatiblesStr;
        if (ftyp->compatibles.size() > 0)
        {
            compatiblesStr = ftyp->compatibles[0];
            for (size_t i = 1, im = ftyp->compatibles.size(); i < im; i++)
            {
                compatiblesStr += ", " + ftyp->compatibles[i];
            }
        }
        fileInfoItem->kvAddPair("Compatibles", compatiblesStr);
    }

    auto movieItem = item->kvAddKey("Movie Info", MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS);
    auto mvhd      = getContainBoxRecursive<MovieHeaderBox>("mvhd");
    if (mvhd != nullptr)
    {
        movieItem->kvAddPair("Duration(ms)", mvhd->duration * 1000 / mvhd->timescale)
            ->kvAddPair("Track Number", mvhd->trackCount);
    }

    for (auto &Mp4TrackInfo : tracksInfo)
    {
        auto trackInfoItem =
            item->kvAddKey("Track " + std::to_string(Mp4TrackInfo->trackId), MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS);
        Mp4TrackInfo->getData(trackInfoItem);
    }

    return item;
}
float MP4ParserImpl::getParseProgress() const
{
    if (!mFileReader.isOpened())
        return 1.f;

    return (float)mFileReader.getCursorPos() / mFileReader.getFileSize();
}
const std::vector<Mp4BoxPtr> MP4ParserImpl::getBoxes() const
{
    std::vector<Mp4BoxPtr> res;
    for (auto &box : mContainBoxes)
    {
        res.push_back(box);
    }
    return res;
}
std::shared_ptr<Mp4BoxData> MP4ParserImpl::getBasicData() const
{
    return getData();
}
