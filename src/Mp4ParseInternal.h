#ifndef MP4_PARSE_INTERNAL_H
#define MP4_PARSE_INTERNAL_H

#include "Mp4SampleTableTypes.h"
#include "Mp4Types.h"
#include "Mp4BoxTypes.h"
#include "Mp4Parse.h"
#include <map>
#include <queue>

#define set_zero_ar(ar) memset(ar, 0, sizeof(ar))
#define set_zero_st(st) memset(&st, 0, sizeof(st))
#define ARRAY_SIZE(ar)  (sizeof(ar) / sizeof(*(ar)))

#define BOX_PARSE_BEGIN()                                                                         \
    mBoxOffset    = boxPosition;                                                                  \
    mBoxSize      = boxSize;                                                                      \
    mBodyPos      = reader.getCursorPos();                                                        \
    mBodySize     = boxBodySize;                                                                  \
    uint64_t last = reader.getCursorPos() + boxBodySize;                                          \
    if (last > reader.getFileSize())                                                              \
    {                                                                                             \
        MP4_ERR("size err %llu + %llu = %llu > %llu\n", reader.getCursorPos(), boxBodySize, last, \
                reader.getFileSize());                                                            \
        return -1;                                                                                \
    }                                                                                             \
    if (mIsFullbox)                                                                               \
    {                                                                                             \
        CHECK_RET(read_fullbox_version_flags(reader, &mFullboxVersion, &mFullboxFlags));          \
    }

#define BOX_PARSE_END()                                                                                                \
    if (reader.getCursorPos() < last)                                                                                  \
    {                                                                                                                  \
        MP4_DBG("%s not end, remain %llu Bytes from %#06llx\n", getBoxTypeStr().c_str(), last - reader.getCursorPos(), \
                reader.getCursorPos());                                                                                \
        return 1;                                                                                                      \
    }                                                                                                                  \
    else if (reader.getCursorPos() > last)                                                                             \
    {                                                                                                                  \
        MP4_ERR("parse err %llu > %llu\n", reader.getCursorPos(), last);                                               \
        reader.setCursor(last);                                                                                        \
        return -1;                                                                                                     \
    }

/*
 * transformation matrix
 * |a  b  u|
 * |c  d  v|
 * |x  y  w|
 * All the values in a matrix are stored as 16.16 fixed‐point values,
 * except for u, v and w, which are stored as 2.30 fixed‐point values
 * The values in the matrix are stored in the order {a,b,u, c,d,v, x,y,w}.
 * (p q 1) * | a b u | = (m n z)
 *           | c d v |
 *           | x y w |
 * m = ap + cq + x; n = bp + dq + y; z = up + vq + w;
 * p' = m/z; q' = n/z
 */

#define READ_MATRIX()                         \
    do                                        \
    {                                         \
        for (int i = 0; i < 9; ++i)           \
        {                                     \
            matrix[i] = reader.readS32(true); \
        }                                     \
    } while (0)

std::string  boxType2Str(uint32_t type);
int          read_fullbox_version_flags(BinaryFileReader &reader, uint8_t *version, uint32_t *flags);
CommonBoxPtr parseBox(BinaryFileReader &reader, bool *parse_err);
uint32_t     getCompatibleBoxType(uint32_t type);
std::string  getProfileString(unsigned int profile_idc);

bool isSameBoxType(uint32_t type1, uint32_t type2);
bool hasSampleTable(uint32_t boxType);
bool hasSampleTable(std::string boxType);

class MP4ParserImpl : public Mp4Parser, public CommonBox, public std::enable_shared_from_this<MP4ParserImpl>
{

public:
    MP4ParserImpl() : CommonBox("file") {}
    virtual std::string getBoxTypeStr() const override { return mFileReader.getFileName(); }
    virtual int         parse(std::string file_path) override;
    virtual void        clear() override;

    virtual bool        isParseSuccess() const override { return mAvailable; }
    virtual std::string getErrorMessage() override;

    virtual MP4_TYPE_E  getMp4Type() const override { return mMp4Type; }
    virtual std::string getFilePath() const override { return mFileReader.getFileFullPath(); }
    virtual float       getParseProgress() const override;

    const std::vector<Mp4BoxPtr> getBoxes() const override;

    virtual const std::vector<TrackInfoPtr> &getTracksInfo() const override { return tracksInfo; }

    virtual bool isTrackHasProperty(uint32_t trackIdx, MP4_TRACK_PROPERTY_E prop) const override;

    virtual int getAudioSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4AudioFrame &frm) override;
    virtual int getVideoSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4VideoFrame &frm) override;
    virtual int getSample(uint32_t trackIdx, uint32_t sampleIdx, Mp4RawSample &outFrame) override;

    Mp4BoxPtr                      asBox() const override { return shared_from_this(); }
    virtual std::string         getBasicInfoString() const override;

    virtual H26X_FRAME_TYPE_E   parseVideoNaluType(uint32_t trackId, uint64_t sampleIdx) override;
    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override
    {
        return 0;
    }

private:
    CommonBoxPtr parseBox(BinaryFileReader &reader, CommonBoxPtr parentBox, bool &parseErr);
    uint32_t     fragmentGetSampleFlags(TrackExtendsBoxPtr pTrexBox, TrackFragmentHeaderBoxPtr pTfhdBox,
                                        TrackRunBoxPtr pTrunBox, uint64_t sampleIdx);
    uint32_t     fragmentGetSampleSize(TrackExtendsBoxPtr pTrexBox, TrackFragmentHeaderBoxPtr pTfhdBox,
                                       TrackRunBoxPtr pTrunBox, uint64_t sampleIdx);
    uint32_t     fragmentGetSampleDuration(TrackExtendsBoxPtr pTrexBox, TrackFragmentHeaderBoxPtr pTfhdBox,
                                           TrackRunBoxPtr pTrunBox, uint64_t sampleIdx);
    uint32_t     fragmentGetSampleCompositionOffset(TrackRunBoxPtr pTrunBox, uint64_t sampleIdx);

    int generateInfoTable(uint32_t trackIdx);
    int generateIsoSamplesInfoTable(uint64_t trackIdx);
    int generateFragmentSamplesInfoTable(uint64_t trackIdx);
    int getH26xFrame(uint32_t trackIdx, uint32_t sampleIdx, Mp4VideoFrame &frm);

    H26X_FRAME_TYPE_E getH264FrameType(BinaryData &data);
    H26X_FRAME_TYPE_E getH265FrameType(int nalu_type, BinaryData &data);

private:
    BinaryFileReader mFileReader;

    std::queue<std::string> mErrors;

    bool       mAvailable = false;
    MP4_TYPE_E mMp4Type   = MP4_TYPE_BUTT;

    uint64_t mCreationTime     = 0;
    uint64_t mModificationTime = 0;

    std::vector<TrackInfoPtr> tracksInfo;

    struct pps_info
    {
        bool    parsed                            = false;
        uint8_t dependentSliceSegmentsEnabledFlag = 0; // get from PPS
        uint8_t numExtraSliceHeaderBits           = 0;
    } mHevcPPSInfo;

    struct sps_info
    {
        bool     parsed                            = false;
        uint32_t log2MinLumaCodingBlockSizeMinus3  = 0;
        uint32_t log2DiffMaxMinLumaCodingBlockSize = 0;
        uint32_t picWidthInLumaSamples             = 0;
        uint32_t picHeightInLumaSamples            = 0;
    } mHevcSPSInfo;

    std::map<int /* track index, start from 0 */, int> mNaluLengthSize;
};

#endif
