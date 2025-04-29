#ifndef MP4_BOX_TYPES_H
#define MP4_BOX_TYPES_H

#include <math.h>

#include <vector>

#include "Mp4ParseTools.h"
#include "Mp4Types.h"
#include "Mp4Parse.h"

struct CommonBox : public Mp4Box
{
    CommonBox() = delete;
    CommonBox(uint32_t boxType, bool isFullbox) : mBoxType(boxType), mIsFullbox(isFullbox) {}
    CommonBox(const char *boxTypeStr, bool isFullbox) : mBoxType(MP4_BOX_MAKE_TYPE(boxTypeStr)), mIsFullbox(isFullbox)
    {
    }

    explicit CommonBox(uint32_t boxType) : CommonBox(boxType, false) {}
    explicit CommonBox(const char *boxTypeStr) : CommonBox(boxTypeStr, false) {}

    virtual ~CommonBox() {}
    virtual int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize);

    virtual uint64_t                             getBoxPos() const override { return mBoxOffset; }
    virtual uint64_t                             getBoxSize() const override { return mBoxSize; }
    virtual uint32_t                             getBoxType() const override { return mBoxType; }
    virtual std::string                          getBoxTypeStr() const override;
    virtual std::vector<std::shared_ptr<Mp4Box>> getSubBoxes() const override;
    std::shared_ptr<Mp4BoxData>                  getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;

    virtual std::string getDataString() { return getData()->toString(); }
    virtual bool        isFullbox(uint8_t &version, uint32_t &flags) override
    {
        if (mIsFullbox)
        {
            version = mFullboxVersion;
            flags   = mFullboxFlags;
        }
        return mIsFullbox;
    }

    template <typename T = CommonBox>
    std::shared_ptr<T> getUpperBox() const
    {
        if constexpr (std::is_same_v<T, CommonBox>)
        {
            return mParentBox.lock();
        }
        else
        {
            return std::dynamic_pointer_cast<T>(mParentBox.lock());
        }
    }

    template <typename T = CommonBox>
    std::shared_ptr<T> getSubBox(uint32_t require_type) const
    {
        std::shared_ptr<CommonBox> res;
        for (auto &subBox : mContainBoxes)
        {
            if (!isSameBoxType(subBox->mBoxType, require_type))
            {
                continue;
            }

            res = subBox;
            break;
        }
        if constexpr (std::is_same_v<T, CommonBox>)
            return res;
        else
            return std::dynamic_pointer_cast<T>(res);
    }

    template <typename T = CommonBox>
    std::shared_ptr<T> getSubBox(const char *typeStr) const
    {
        return getSubBox<T>(MP4_BOX_MAKE_TYPE(typeStr));
    }

    template <typename T = CommonBox>
    std::shared_ptr<T> getSubBoxRecursive(uint32_t type, int layer = INT32_MAX) const
    {
        if (layer <= 0)
            return nullptr;

        std::shared_ptr<CommonBox> res;
        for (auto &subBox : mContainBoxes)
        {
            if (isSameBoxType(subBox->mBoxType, type))
            {
                res = subBox;
                break;
            }
            res = subBox->getSubBoxRecursive<T>(type, layer - 1);
            if (res != nullptr)
                break;
        }
        if constexpr (std::is_same_v<T, CommonBox>)
            return res;
        else
            return std::dynamic_pointer_cast<T>(res);
    }

    template <typename T = CommonBox>
    std::shared_ptr<T> getSubBoxRecursive(const char *type_str, int layer = INT32_MAX) const
    {
        return getSubBoxRecursive<T>(MP4_BOX_MAKE_TYPE(type_str), layer);
    }

    template <typename T = CommonBox>
    std::vector<std::shared_ptr<T>> getSubBoxes(uint32_t type) const
    {
        std::shared_ptr<T>              item;
        std::vector<std::shared_ptr<T>> res;
        for (auto &subBox : mContainBoxes)
        {
            if (!isSameBoxType(subBox->mBoxType, type))
                continue;

            if constexpr (std::is_same_v<T, CommonBox>)
                item = subBox;
            else
                item = std::dynamic_pointer_cast<T>(subBox);
            if (item)
                res.push_back(item);
        }
        return res;
    }

    template <typename T = CommonBox>
    std::vector<std::shared_ptr<T>> getSubBoxes(const char *type_str) const
    {
        return getSubBoxes<T>(MP4_BOX_MAKE_TYPE(type_str));
    }

protected:
    bool     mInvalid   = false;
    uint32_t mBoxType   = 0;
    bool     mIsFullbox = 0;
    uint64_t mBoxOffset = 0;
    uint64_t mBoxSize   = 0;
    uint64_t mBodyPos   = 0;
    uint64_t mBodySize  = 0;

    uint8_t  mFullboxVersion = 0;
    uint32_t mFullboxFlags   = 0;

    std::weak_ptr<CommonBox> mParentBox;

    std::vector<std::shared_ptr<CommonBox>> mContainBoxes;

    friend class MP4ParserImpl;
};
using CommonBoxPtr = std::shared_ptr<CommonBox>;

struct UserDefineBox : public CommonBox
{
    explicit UserDefineBox(uint32_t boxType, BoxParseFunc parseFunc, BoxDataFunc dataFunc, void *userData);

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;

private:
    BoxParseFunc mParseFunc;
    BoxDataFunc  mDataFunc;
    void        *mUserData = 0;
};
using UserDefineBoxPtr = std::shared_ptr<UserDefineBox>;

//'edts' 'stbl' 'moov' 'trak' 'minf' 'dinf' 'mdia' 'moof' 'mvex' 'traf' 'mfra'
struct ContainBox : public CommonBox
{
    explicit ContainBox(uint32_t boxType) : CommonBox(boxType) {}
    explicit ContainBox(const char *boxType) : CommonBox(boxType) {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using ContainBoxPtr = std::shared_ptr<ContainBox>;

struct FullBox : public CommonBox
{
    FullBox() = delete;
    explicit FullBox(uint32_t boxType) : CommonBox(boxType, true) {}
    explicit FullBox(const char *boxTypeStr) : CommonBox(boxTypeStr, true) {}
};
using FullBoxPtr = std::shared_ptr<FullBox>;

struct FileTypeBox : public CommonBox
{
    std::string              majorBrand;       // u32
    uint32_t                 minorVersion = 0; // u32
    std::vector<std::string> compatibles;      // u32*n

    FileTypeBox() : CommonBox("ftyp") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using FileTypeBoxPtr = std::shared_ptr<FileTypeBox>;

struct MovieHeaderBox : public FullBox
{
    uint64_t creationTime     = 0;   // u32/u64
    uint64_t modificationTime = 0;   // u32/u64
    uint32_t timescale        = 0;   // u32
    uint64_t duration         = 0;   // u32/u64
    float    rate             = 0;   // s32 fixed-point 16.16
    float    volume           = 0;   // s16 fixed-point 8.8
    int32_t  matrix[9]        = {0}; // s32*9
    uint32_t nextTrackId      = 0;   // u32

    unsigned int trackCount = 0;

    MovieHeaderBox() : FullBox("mvhd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using MovieHeaderBoxPtr = std::shared_ptr<MovieHeaderBox>;

struct TrackHeaderBox : public FullBox
{
    uint64_t creationTime     = 0; // u32/u64
    uint64_t modificationTime = 0; // u32/u64
    uint32_t trackId          = 0; // u32
    uint64_t duration         = 0; // u32/u64
    int16_t  layer            = 0; // s16
    int16_t  alternateGroup   = 0; // s16
    float    volume           = 0; // s16 fixed-point 8.8
    int32_t  matrix[9]        = {0};
    float    width            = 0; // u32 fixed-point 16.16
    float    height           = 0; // u32 fixed-point 16.16

    TrackHeaderBox() : FullBox("tkhd") {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};

using TrackHeaderBoxPtr = std::shared_ptr<TrackHeaderBox>;

struct MediaHeaderBox : public FullBox
{
    uint64_t creationTime     = 0;   // u32/u64
    uint64_t modificationTime = 0;   // u32/u64
    uint64_t timescale        = 0;   // u32
    uint64_t duration         = 0;   // u32/u64
    char     language[4]      = {0}; // u5[3]

    MediaHeaderBox() : FullBox("mdhd") {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using MediaHeaderBoxPtr = std::shared_ptr<MediaHeaderBox>;

struct HandlerBox : public FullBox
{
    std::string handlerType; // u32
    std::string handlerName; // string

    HandlerBox() : FullBox("hdlr") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using HandlerBoxPtr = std::shared_ptr<HandlerBox>;

struct VideoMediaHeaderBox : public FullBox
{
    uint16_t graphicsMode = 0; // u16
    uint16_t r            = 0; // u16
    uint16_t g            = 0; // u16
    uint16_t b            = 0; // u16

    VideoMediaHeaderBox() : FullBox("vmhd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using VideoMediaHeaderBoxPtr = std::shared_ptr<VideoMediaHeaderBox>;

struct SoundMediaHeaderBox : public FullBox
{
    float balance = 0; // uint16 fixed-point 8.8

    SoundMediaHeaderBox() : FullBox("smhd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SoundMediaHeaderBoxPtr = std::shared_ptr<SoundMediaHeaderBox>;

struct DataEntryUrlBox : public FullBox
{
    std::string location; // string

    DataEntryUrlBox() : FullBox("url ") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using DataEntryUrlBoxPtr = std::shared_ptr<DataEntryUrlBox>;

struct DataEntryUrnBox : public FullBox
{
    std::string name;     // string
    std::string location; // string

    DataEntryUrnBox() : FullBox("urn ") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using DataEntryUrnBoxPtr = std::shared_ptr<DataEntryUrnBox>;

struct DataReferenceBox : public FullBox
{
    uint32_t entryCount = 0; // u32

    DataReferenceBox() : FullBox("dref") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using DataReferenceBoxPtr = std::shared_ptr<DataReferenceBox>;

struct SampleDescriptionBox : public FullBox
{
    uint32_t entryCount = 0; // u32

    SampleDescriptionBox() : FullBox("stsd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SampleDescriptionBoxPtr = std::shared_ptr<SampleDescriptionBox>;

struct BitRateBox : public CommonBox
{
    uint32_t bufferSizeDb   = 0; // u32
    uint32_t maxBitrate     = 0; // u32
    uint32_t averageBitrate = 0; // u32

    BitRateBox() : CommonBox("btrt"), bufferSizeDb(0), maxBitrate(0), averageBitrate(0) {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using BitRateBoxPtr = std::shared_ptr<BitRateBox>;

struct TrackExtendsBox : public FullBox
{
    uint32_t trackId               = 0; // u32
    uint32_t defaultSampleDescIdx  = 0; // u32
    uint32_t defaultSampleDuration = 0; // u32
    uint32_t defaultSampleSize     = 0; // u32
    uint32_t defaultSampleFlags    = 0; // u32
    TrackExtendsBox() : FullBox("trex") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using TrackExtendsBoxPtr = std::shared_ptr<TrackExtendsBox>;

struct MovieExtendsHeaderBox : public FullBox
{
    uint64_t fragmentDuration = 0; // u64/u32
    MovieExtendsHeaderBox() : FullBox("mehd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using MovieExtendsHeaderBoxPtr = std::shared_ptr<MovieExtendsHeaderBox>;

struct MovieFragmentHeaderBox : public FullBox
{
    uint32_t seqNum = 0; // u32
    MovieFragmentHeaderBox() : FullBox("mfhd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using MovieFragmentHeaderBoxPtr = std::shared_ptr<MovieFragmentHeaderBox>;

struct TrackFragmentHeaderBox : public FullBox
{
    uint32_t trackId               = 0; // u32
    uint64_t baseDataOffset        = 0; // u64
    uint32_t sampleDescIdx         = 0; // u32
    uint32_t defaultSampleDuration = 0; // u32
    uint32_t defaultSampleSize     = 0; // u32
    uint32_t defaultSampleFlags    = 0; // u32
    TrackFragmentHeaderBox() : FullBox("tfhd") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using TrackFragmentHeaderBoxPtr = std::shared_ptr<TrackFragmentHeaderBox>;

struct TrackFragmentBaseMediaDecodeTimeBox : public FullBox
{
    uint64_t baseDecTime = 0; // u64/u32
    TrackFragmentBaseMediaDecodeTimeBox() : FullBox("tfdt") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using TrackFragmentBaseMediaDecodeTimeBoxPtr = std::shared_ptr<TrackFragmentBaseMediaDecodeTimeBox>;

struct MovieFragmentRandomAccessOffsetBox : public FullBox
{
    uint32_t size = 0; // u32
    MovieFragmentRandomAccessOffsetBox() : FullBox("mfro") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using MovieFragmentRandomAccessOffsetBoxPtr = std::shared_ptr<MovieFragmentRandomAccessOffsetBox>;

struct ChannelLayOutBox : public FullBox
{
    uint16_t channelCount = 0; // from AudioSampleEntry

#define channelStructured 1
#define objectStructured  2

    uint8_t streamStructure = 0; // u8

    //	if (streamStructure & channelStructured) {
    uint8_t definedLayout = 0; // u8
    struct ChannelInfo
    {
        uint8_t speakerPosition = 0; // u8

        //	if (speakerPosition == 126) {
        int16_t azimuth   = 0; // s16
        int8_t  elevation = 0; // s8
                               //	}
    };
    // 		if (definedLayout==0){
    std::vector<ChannelInfo> channelsInfo;
    // 		} else {
    uint64_t                 omittedChannelsMap = 0; // u64
    //		}
    //	}

    // if (streamStructure & objectStructured)
    uint8_t objCount = 0;

    ChannelLayOutBox() : FullBox("chnl") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using ChannelLayOutBoxPtr = std::shared_ptr<ChannelLayOutBox>;

struct SamplingRateBox : public FullBox
{
    uint32_t samplingRate = 0; // u32

    SamplingRateBox() : FullBox("srat") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using SamplingRateBoxPtr = std::shared_ptr<SamplingRateBox>;

// user define box, parse by callback function
struct UdtaBox : public CommonBox
{
    UdtaBox() : CommonBox("udta") {}

    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;
};
using UdtaBoxPtr = std::shared_ptr<UdtaBox>;

struct UuidBox : public CommonBox
{
    UuidBox() : CommonBox("uuid") {}
    int parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize) override;

    std::shared_ptr<Mp4BoxData> getData(std::shared_ptr<Mp4BoxData> src = nullptr) const override;

private:
    uint8_t  uuid[MP4_UUID_LEN] = {0};
    uint64_t uuidBodyPos        = 0;
};
using UuidBoxPtr = std::shared_ptr<UuidBox>;

#define MAX_HDL_TYPE_CNT 7

#endif
