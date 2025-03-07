#ifndef MP4_TYPES_H
#define MP4_TYPES_H

#include <stdint.h>

#include <string>
#include <vector>

#include "Mp4BoxData.h"

#define MP4_BOX_MAKE_TYPE(type_char)                                                                 \
    ((uint32_t)(type_char[0] << 24) + (uint32_t)(type_char[1] << 16) + (uint32_t)(type_char[2] << 8) \
     + (uint32_t)type_char[3])

struct Mp4Box
{
    virtual ~Mp4Box() {}
    virtual uint32_t    getBoxType() const                           = 0;
    virtual std::string getBoxTypeStr() const                        = 0;
    virtual uint64_t    getBoxPos() const                            = 0;
    virtual uint64_t    getBoxSize() const                           = 0;
    virtual bool        isFullbox(uint8_t &version, uint32_t &flags) = 0;

    virtual std::vector<std::shared_ptr<Mp4Box>> getContainBoxes() const = 0;

    virtual std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const = 0;
};
using BoxPtr = std::shared_ptr<const Mp4Box>;

enum MP4_TYPE_E
{
    MP4_TYPE_ISO      = 0,
    MP4_TYPE_FRAGMENT = 1,
    MP4_TYPE_BUTT,
};

enum MP4_MEDIA_TYPE_E
{
    MP4_MEDIA_TYPE_VIDEO = 0,
    MP4_MEDIA_TYPE_AUDIO = 1,
    MP4_MEDIA_TYPE_BUTT
};

enum MP4_TRACK_TYPE_E
{
    TRACK_TYPE_VIDEO = 0,
    TRACK_TYPE_AUDIO = 1,
    TRACK_TYPE_META  = 2,
    TRACK_TYPE_HINT  = 3,
    TRACK_TYPE_TEXT  = 4,
    TRACK_TYPE_SUBT  = 5,
    TRACK_TYPE_FDSM  = 6,
    TRACK_TYPE_BUTT
};

enum MP4_CODEC_TYPE_E
{
    MP4_CODEC_MOV_TEXT = 0,
    MP4_CODEC_MPEG4,
    MP4_CODEC_H264,
    MP4_CODEC_HEVC,
    MP4_CODEC_AAC,
    MP4_CODEC_MP4ALS,
    MP4_CODEC_MPEG2VIDEO,
    MP4_CODEC_MP3,
    MP4_CODEC_MP2,
    MP4_CODEC_MPEG1VIDEO,
    MP4_CODEC_MJPEG,
    MP4_CODEC_PNG,
    MP4_CODEC_JPEG2000,
    MP4_CODEC_VC1,
    MP4_CODEC_DIRAC,
    MP4_CODEC_AC3,
    MP4_CODEC_EAC3,
    MP4_CODEC_DTS,
    MP4_CODEC_VP9,
    MP4_CODEC_FLAC,
    MP4_CODEC_TSCC2,
    MP4_CODEC_EVRC,
    MP4_CODEC_VORBIS,
    MP4_CODEC_DVD_SUBTITLE,
    MP4_CODEC_QCELP,
    MP4_CODEC_MPEG4SYSTEMS,
    MP4_CODEC_NONE,
};

struct Mp4CodecString
{
    MP4_CODEC_TYPE_E codecType;
    std::string      codecName;
};

enum MP4_TRACK_PROPERTY_E
{
    MP4_TRACK_PROPERTY_ENABLE       = 0x1,
    MP4_TRACK_PROPERTY_IN_MOVIE     = 0x2,
    MP4_TRACK_PROPERTY_IN_PREVIEW   = 0x4,
    MP4_TRACK_PROPERTY_ASPECT_RATIO = 0x8,
    MP4_TRACK_PROPERTY_BUTT,
};
enum MP4_TKHD_FLAGS_E
{
    MP4_TKHD_FLAG_TRACK_ENABLED              = 0x000001,
    MP4_TKHD_FLAG_TRACK_IN_MOVIE             = 0x000002,
    MP4_TKHD_FLAG_TRACK_IN_PREVIEW           = 0x000004,
    MP4_TKHD_FLAG_TRACK_SIZE_IS_ASPECT_RATIO = 0x000008,
};

enum MP4_TFHD_FLAGS_E
{
    MP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT         = 0x000001,
    MP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT = 0x000002,
    MP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT  = 0x000008,
    MP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT      = 0x000010,
    MP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT     = 0x000020,
    MP4_TFHD_FLAG_DURATION_IS_EMPTY                = 0x010000,
    // compatible brands need to contain 'iso5'or later for this bit set to 1
    MP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF             = 0x020000,
};

#define FRAG_SAMPLE_FLAG_DEGRADATION_PRIORITY_MASK 0x0000ffff
#define FRAG_SAMPLE_FLAG_IS_NON_SYNC               0x00010000
#define FRAG_SAMPLE_FLAG_PADDING_MASK              0x000e0000
#define FRAG_SAMPLE_FLAG_REDUNDANCY_MASK           0x00300000
#define FRAG_SAMPLE_FLAG_DEPENDED_MASK             0x00c00000
#define FRAG_SAMPLE_FLAG_DEPENDS_MASK              0x03000000

enum MP4_TRUN_FLAGS_E
{
    MP4_TRUN_FLAG_DATA_OFFSET_PRESENT                     = 0x000001,
    MP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT              = 0x000004,
    MP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT                 = 0x000100,
    MP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT                     = 0x000200,
    MP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT                    = 0x000400,
    MP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSETS_PRESENT = 0x000800,
};

struct Mp4TrackInfo;
using TrackInfoPtr = std::shared_ptr<Mp4TrackInfo>;

enum H264_NALU_TYPE_E
{
    H264_NALU_UNKNOWN   = 0,
    H264_NALU_SLICE     = 1,
    H264_NALU_SLICE_DPA = 2,
    H264_NALU_SLICE_DPB = 3,
    H264_NALU_SLICE_DPC = 4,
    H264_NALU_SLICE_IDR = 5, /* ref_idc != 0 */
    H264_NALU_SEI       = 6, /* ref_idc == 0 */
    H264_NALU_SPS       = 7,
    H264_NALU_PPS       = 8,
    H264_NALU_MAX       = H264_NALU_PPS,
};

enum H265_NALU_TYPE_E
{
    H265_NALU_TRAIL_N    = 0,
    H265_NALU_TRAIL_R    = 1,
    H265_NALU_TSA_N      = 2,
    H265_NALU_TSA_R      = 3,
    H265_NALU_STSA_N     = 4,
    H265_NALU_STSA_R     = 5,
    H265_NALU_RADL_N     = 6,
    H265_NALU_RADL_R     = 7,
    H265_NALU_RASL_N     = 8,
    H265_NALU_RASL_R     = 9,
    H265_NALU_BLA_W_LP   = 16,
    H265_NALU_BLA_W_RADL = 17,
    H265_NALU_BLA_N_LP   = 18,
    H265_NALU_IDR_W      = 19,
    H265_NALU_IDR_N      = 20,
    H265_NALU_CRA_NUT    = 21,
    H265_NALU_VPS        = 32,
    H265_NALU_SPS        = 33,
    H265_NALU_PPS        = 34,
    H265_NALU_PRE_SEI    = 39,
    H265_NALU_SUF_SEI    = 40,
    H265_NALU_MAX        = H265_NALU_SUF_SEI,
};

enum H26X_FRAME_TYPE_E
{
    H26X_FRAME_I = 41,
    H26X_FRAME_P,
    H26X_FRAME_B,
    H26X_FRAME_Unknown,
};

std::string mp4GetNaluTypeStr(MP4_CODEC_TYPE_E codecType, int naluType);
std::string mp4GetFrameTypeStr(H26X_FRAME_TYPE_E frameType);

struct Mp4SampleItem
{
    int64_t sampleIdx = -1;

    uint64_t sampleOffset = 0;
    uint64_t sampleSize   = 0;

    int isKeyFrame = -1;

    uint64_t dtsMs      = 0;
    uint64_t dtsDeltaMs = 0;

    uint64_t ptsMs = 0;

    // it need to parse nalu data to get frameType and naluTypes, so they are empty after call Mp4Parser::parse,
    // need to call Mp4Parser::parseVideoNaluType to fill them
    H26X_FRAME_TYPE_E frameType = H26X_FRAME_Unknown;
    // H264_NALU_TYPE_E / H265_NALU_TYPE_E
    std::vector<int>  naluTypes;
};

struct Mp4ChunkItem
{
    uint64_t chunkIdx = 0;

    uint64_t chunkOffset = 0;
    uint64_t chunkSize   = 0;

    uint64_t sampleStartIdx = 0;
    uint64_t sampleCount    = 0;

    uint64_t startPtsMs    = 0;
    uint64_t durationMs    = 0;
    double   avgBitrateBps = 0;
};

struct Mp4MediaInfo
{
    MP4_MEDIA_TYPE_E           mediaType   = MP4_MEDIA_TYPE_BUTT;
    uint8_t                    codecCode   = 0;
    uint64_t                   sampleCount = 0;
    uint64_t                   durationMs  = 0;
    uint64_t                   totalSize   = 0;
    double                     avgBitrate  = 0; // bps
    std::vector<Mp4ChunkItem>  chunksInfo;
    std::vector<Mp4SampleItem> samplesInfo;
    std::vector<uint64_t>      syncSampleTable;

    virtual std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const;
    virtual int                         getInfoFromTrack(BoxPtr pTrakBox, TrackInfoPtr trak);
    virtual std::string                 getInfoString();
};

struct Mp4VideoInfo : public Mp4MediaInfo
{
    int    width  = 0;
    int    height = 0;
    double avgFps = 0;

    uint32_t profile       = 0;
    uint32_t profileCompat = 0;
    uint32_t level         = 0;

    virtual std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
    int                                 getInfoFromTrack(BoxPtr pTrakBox, TrackInfoPtr trak) override;
    std::string                         getInfoString() override;
};

struct Mp4AudioInfo : public Mp4MediaInfo
{
    int channels = 0;

    int audioSampleRate = 0;
    int audioSampleSize = 0;

    virtual std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
    int                                 getInfoFromTrack(BoxPtr pTrakBox, TrackInfoPtr trak) override;
    std::string                         getInfoString() override;
};

struct Mp4TrackInfo
{
    MP4_TRACK_TYPE_E trackType = TRACK_TYPE_BUTT;

    uint32_t trakIndex = 0;

    int trackId = 0; // start from 1

    std::shared_ptr<Mp4MediaInfo> mediaInfo;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const;
    std::string                 getInfoString();
};

struct Mp4RawSample
{
    uint32_t                   trackIdx   = 0;
    uint64_t                   fileOffset = 0;
    uint64_t                   sampleSize = 0;
    uint64_t                   dataSize   = 0;
    uint64_t                   dtsMs      = 0;
    uint64_t                   ptsMs      = 0;
    uint64_t                   sampleIdx  = 0;
    std::shared_ptr<uint8_t[]> sampleData;
};

struct Mp4MediaFrame : public Mp4RawSample
{
    MP4_MEDIA_TYPE_E mediaType = MP4_MEDIA_TYPE_BUTT;
    MP4_CODEC_TYPE_E codec     = MP4_CODEC_NONE;
};

struct Mp4VideoFrame : public Mp4MediaFrame
{
    bool         isKeyFrame = false;
    unsigned int width      = 0;
    unsigned int height     = 0;
};

struct Mp4AudioFrame : public Mp4MediaFrame
{
    int audioSampleRate = 0;
    int audioSampleSize = 0;
    int channels        = 0;
};

std::string     &mp4GetHandlerName(MP4_TRACK_TYPE_E type);
std::string     &mp4GetTrackTypeName(MP4_TRACK_TYPE_E type);
std::string     &mp4GetMediaTypeName(MP4_MEDIA_TYPE_E type);
std::string     &mp4GetCodecName(uint8_t codec);
MP4_CODEC_TYPE_E mp4GetCodecType(uint8_t codec);

enum MP4_LOG_LEVEL_E
{
    MP4_LOG_LEVEL_ERR = 0,
    MP4_LOG_LEVEL_WARN,
    MP4_LOG_LEVEL_INFO,
    MP4_LOG_LEVEL_DBG,
    MP4_LOG_LEVEL_ALL = MP4_LOG_LEVEL_DBG,
};

#endif // MP4_TYPES_H