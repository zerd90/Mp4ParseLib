
#include <iterator>
#include <math.h>

#include "Mp4Parse.h"
#include "Mp4BoxTypes.h"
#include "Mp4SampleEntryTypes.h"
#include "Mp4ParseInternal.h"
#include "Mp4Types.h"
#include "Mp4SampleTableTypes.h"

using std::dynamic_pointer_cast;
using std::make_shared;
using std::map;
using std::shared_ptr;
using std::string;

#define ADD_MATRIX_DATA(item, mat, len)                                                             \
    do                                                                                              \
    {                                                                                               \
        std::shared_ptr<Mp4BoxData> matrixData = item->kvAddKey("Matrix", MP4_BOX_DATA_TYPE_ARRAY); \
        for (unsigned int i = 0; i < len; i++)                                                      \
        {                                                                                           \
            matrixData->arrayAddItem(mat[i]);                                                       \
        }                                                                                           \
    } while (0)

typedef int (*box_parse_func)(uint64_t &bodySize, void *ex_data);

map<MP4_MEDIA_TYPE_E, std::string> gMediaTypeName = {
    {MP4_MEDIA_TYPE_VIDEO, "Video"},
    {MP4_MEDIA_TYPE_AUDIO, "Audio"},
};

map<MP4_TRACK_TYPE_E, std::string> gHdlrTypeName = {
    {TRACK_TYPE_VIDEO, "vide"},
    {TRACK_TYPE_AUDIO, "soun"},
    { TRACK_TYPE_META, "meta"},
    { TRACK_TYPE_HINT, "hint"},
    { TRACK_TYPE_TEXT, "text"},
    { TRACK_TYPE_SUBT, "subt"},
    { TRACK_TYPE_FDSM, "fdsm"}
};

map<MP4_TRACK_TYPE_E, string> gTrackTypeName = {
    {TRACK_TYPE_VIDEO,    "video track"},
    {TRACK_TYPE_AUDIO,    "sound track"},
    { TRACK_TYPE_META,     "meta track"},
    { TRACK_TYPE_HINT,     "hint track"},
    { TRACK_TYPE_TEXT,     "text track"},
    { TRACK_TYPE_SUBT, "subtitle track"},
    { TRACK_TYPE_FDSM,     "font track"}
};

map<uint8_t, Mp4CodecString> gCodecTypeMap = {
    {0x08,             {MP4_CODEC_MOV_TEXT, "MOV_TEXT"}},
    {0x20,                   {MP4_CODEC_MPEG4, "MPEG4"}},
    {0x21,                     {MP4_CODEC_H264, "H264"}},
    {0x23,                     {MP4_CODEC_HEVC, "HEVC"}},
    {0x40,        {MP4_CODEC_AAC, "AAC Low Complexity"}},
    {0x40,                 {MP4_CODEC_MP4ALS, "MP4ALS"}}, /* 14496-3 ALS */
    {0x61,    {MP4_CODEC_MPEG2VIDEO, "MPEG2VIDEO Main"}}, /* MPEG-2 Main */
    {0x60,  {MP4_CODEC_MPEG2VIDEO, "MPEG2VIDEO Simple"}}, /* MPEG-2 Simple */
    {0x62,     {MP4_CODEC_MPEG2VIDEO, "MPEG2VIDEO SNR"}}, /* MPEG-2 SNR */
    {0x63, {MP4_CODEC_MPEG2VIDEO, "MPEG2VIDEO Spatial"}}, /* MPEG-2 Spatial */
    {0x64,    {MP4_CODEC_MPEG2VIDEO, "MPEG2VIDEO High"}}, /* MPEG-2 High */
    {0x65,     {MP4_CODEC_MPEG2VIDEO, "MPEG2VIDEO 422"}}, /* MPEG-2 422 */
    {0x66,                  {MP4_CODEC_AAC, "AAC Main"}}, /* MPEG-2 AAC Main */
    {0x67,                   {MP4_CODEC_AAC, "AAC Low"}}, /* MPEG-2 AAC Low */
    {0x68,                   {MP4_CODEC_AAC, "AAC SSR"}}, /* MPEG-2 AAC SSR */
    {0x69,                       {MP4_CODEC_MP3, "MP3"}}, /* 13818-3 */
    {0x69,                       {MP4_CODEC_MP2, "MP2"}}, /* 11172-3 */
    {0x6A,         {MP4_CODEC_MPEG1VIDEO, "MPEG1VIDEO"}}, /* 11172-2 */
    {0x6B,                       {MP4_CODEC_MP3, "MP3"}}, /* 11172-3 */
    {0x6C,                   {MP4_CODEC_MJPEG, "MJPEG"}}, /* 10918-1 */
    {0x6D,                       {MP4_CODEC_PNG, "PNG"}},
    {0x6E,             {MP4_CODEC_JPEG2000, "JPEG2000"}}, /* 15444-1 */
    {0xA3,                       {MP4_CODEC_VC1, "VC1"}},
    {0xA4,                   {MP4_CODEC_DIRAC, "DIRAC"}},
    {0xA5,                       {MP4_CODEC_AC3, "AC3"}},
    {0xA6,                     {MP4_CODEC_EAC3, "EAC3"}},
    {0xA9,                       {MP4_CODEC_DTS, "DTS"}}, /* mp4ra.org */
    {0xC0,                       {MP4_CODEC_VP9, "VP9"}}, /* nonstandard, update when there is a standard value */
    {0xC1,                     {MP4_CODEC_FLAC, "FLAC"}}, /* nonstandard, update when there is a standard value */
    {0xD0,                   {MP4_CODEC_TSCC2, "TSCC2"}}, /* nonstandard, camtasia uses it */
    {0xD1,                     {MP4_CODEC_EVRC, "EVRC"}}, /* nonstandard, pvAuthor uses it */
    {0xDD,                 {MP4_CODEC_VORBIS, "VORBIS"}}, /* nonstandard, gpac uses it */
    /* nonstandard, see unsupported-embedded-subs-2.mp4 */
    {0xE0,     {MP4_CODEC_DVD_SUBTITLE, "DVD_SUBTITLE"}},
    {0xE1,                   {MP4_CODEC_QCELP, "QCELP"}},
    {0x01,     {MP4_CODEC_MPEG4SYSTEMS, "MPEG4SYSTEMS"}},
    {0x02,     {MP4_CODEC_MPEG4SYSTEMS, "MPEG4SYSTEMS"}},
    {   0,                     {MP4_CODEC_NONE, "NONE"}},
};

string &mp4GetHandlerName(MP4_TRACK_TYPE_E type)
{
    return gHdlrTypeName[type];
}
string &mp4GetMediaTypeName(MP4_MEDIA_TYPE_E type)
{
    return gMediaTypeName[type];
}
string &mp4GetTrackTypeName(MP4_TRACK_TYPE_E type)
{
    return gTrackTypeName[type];
}
string &mp4GetCodecName(uint8_t codec)
{
    return gCodecTypeMap[codec].codecName;
}
MP4_CODEC_TYPE_E mp4GetCodecType(uint8_t codec)
{
    return gCodecTypeMap[codec].codecType;
}

struct userBoxCallback
{
    BoxParseFunc parseDataCallback;
    BoxDataFunc  getDataCallback;
};

struct MP4_UUID
{
    explicit MP4_UUID(const uint8_t uuid[MP4_UUID_LEN])
    {
        assert(uuid != nullptr);
        memcpy(_uuid, uuid, MP4_UUID_LEN);
    }

    bool operator<(const MP4_UUID &other) const { return memcmp(_uuid, other._uuid, MP4_UUID_LEN) < 0; }

    uint8_t _uuid[MP4_UUID_LEN] = {0};
};
std::map<MP4_UUID, userBoxCallback> gUdtaRegisterCallbacks;

void registerUdtaCallback(uint8_t uuid[MP4_UUID_LEN], BoxParseFunc parseDataCallback, BoxDataFunc getDataCallback)
{
    gUdtaRegisterCallbacks[MP4_UUID(uuid)] = {parseDataCallback, getDataCallback};
}

std::map<Mp4BoxType, userBoxCallback> gUserDefineBoxCallbacks;

void registerBoxCallback(Mp4BoxType boxType, BoxParseFunc parseDataCallback, BoxDataFunc getDataCallback)
{
    gUserDefineBoxCallbacks[boxType] = {parseDataCallback, getDataCallback};
}

map<uint32_t, uint32_t> compatible_box_types = {
    {MP4_BOX_MAKE_TYPE("free"), MP4_BOX_MAKE_TYPE("skip")},
    {MP4_BOX_MAKE_TYPE("avc2"), MP4_BOX_MAKE_TYPE("avc1")},
    {MP4_BOX_MAKE_TYPE("avc3"), MP4_BOX_MAKE_TYPE("avc1")},
    {MP4_BOX_MAKE_TYPE("avc4"), MP4_BOX_MAKE_TYPE("avc1")},
    {MP4_BOX_MAKE_TYPE("hvc2"), MP4_BOX_MAKE_TYPE("hvc1")},
    {MP4_BOX_MAKE_TYPE("hvc3"), MP4_BOX_MAKE_TYPE("hvc1")},
    {MP4_BOX_MAKE_TYPE("hev1"), MP4_BOX_MAKE_TYPE("hvc1")},
    {MP4_BOX_MAKE_TYPE("hev2"), MP4_BOX_MAKE_TYPE("hvc1")},
    {MP4_BOX_MAKE_TYPE("hev3"), MP4_BOX_MAKE_TYPE("hvc1")},
};

std::string boxType2Str(uint32_t type)
{
    char str[16];
    char cstr[5] = {0};
    cstr[0]      = (type >> 24);
    cstr[1]      = (type >> 16) & 0xff;
    cstr[2]      = (type >> 8) & 0xff;
    cstr[3]      = type & 0xff;
    if (isprint(cstr[0]) && isprint(cstr[1]) && isprint(cstr[2]) && isprint(cstr[3]))
        snprintf(str, sizeof(str), "%s", cstr);
    else
        snprintf(str, sizeof(str), "0x%08x", type);
    return std::string(str);
}

uint32_t getCompatibleBoxType(uint32_t type)
{
    if (compatible_box_types.find(type) == compatible_box_types.end())
        return type;
    else
        return compatible_box_types[type];
}

bool isSameBoxType(uint32_t type1, uint32_t type2)
{
    if (getCompatibleBoxType(type1) == getCompatibleBoxType(type2))
        return true;
    else
        return false;
}

bool hasSampleTable(uint32_t boxType)
{
    switch (boxType)
    {
        default:
            return false;
        case MP4_BOX_MAKE_TYPE("stts"):
        case MP4_BOX_MAKE_TYPE("ctts"):
        case MP4_BOX_MAKE_TYPE("stsc"):
        case MP4_BOX_MAKE_TYPE("stsz"):
        case MP4_BOX_MAKE_TYPE("stz2"):
        case MP4_BOX_MAKE_TYPE("sdtp"):
        case MP4_BOX_MAKE_TYPE("stco"):
        case MP4_BOX_MAKE_TYPE("co64"):
        case MP4_BOX_MAKE_TYPE("stss"):
        case MP4_BOX_MAKE_TYPE("sgpd"):
        case MP4_BOX_MAKE_TYPE("sbgp"):
            return true;
    }
}

bool hasSampleTable(std::string boxType)
{
    return hasSampleTable(MP4_BOX_MAKE_TYPE(boxType.c_str()));
}

int get_type_size(BinaryFileReader &reader, uint32_t &type, uint64_t &boxPos, uint64_t &boxSize, uint64_t &bodySize)
{
    uint32_t ret        = 0;
    char     boxType[4] = {0};
    if (reader.getCursorPos() + 8 > reader.getFileSize())
    {
        MP4_ERR("size err %llu + 8 > %llu\n", reader.getCursorPos(), reader.getFileSize());
        return -1;
    }

    bool     size_err = false;
    uint32_t box_sz   = 0;

    boxPos = reader.getCursorPos();

    ret = reader.read(&box_sz, 4);
    if (ret < 4)
    {
        MP4_ERR("read box size err %u\n", ret);
        return -1;
    }
    box_sz = bswap_32(box_sz);

    reader.read(boxType, 4);
    type = MP4_BOX_MAKE_TYPE(boxType);

    if (box_sz > 1)
    {
        boxSize  = box_sz;
        bodySize = boxSize - 8;
        size_err = reader.getCursorPos() + bodySize > reader.getFileSize();
    }
    else if (box_sz == 0)
    {
        boxSize  = reader.getFileSize() - reader.getCursorPos() + 8;
        bodySize = boxSize - 8;
    }
    else
    {
        reader.read(&boxSize, 8);
        boxSize  = bswap_64(boxSize);
        bodySize = boxSize - 16;
        size_err = reader.getCursorPos() + bodySize > reader.getFileSize();
        MP4_INFO("box %s using large size %llu\n", boxType2Str(type).c_str(), boxSize);
    }

    if (size_err)
    {
        MP4_ERR("type %s, size err %llu + %llu > %llu\n", boxType2Str(type).c_str(), reader.getCursorPos(), boxSize,
                reader.getFileSize());
    }
    return 0;
}

int read_fullbox_version_flags(BinaryFileReader &reader, uint8_t *version, uint32_t *flags)
{
    *version     = 0;
    *flags       = 0;
    uint64_t ret = 0;

    ret = reader.read(version, 1);
    if (0 == ret)
        return -1;
    ret = reader.read((uint8_t *)flags + 1, 3);
    if (0 == ret)
        return -1;
    *flags = bswap_32(*flags);
    return 0;
}

int CommonBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();
    reader.setCursor(last);
    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> CommonBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Box Offset", mBoxOffset)->kvAddPair("Box Size", mBoxSize);
    return item;
}

int ContainBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> ContainBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Box Offset", mBoxOffset)->kvAddPair("Box Size", mBoxSize);
    return item;
}

int FileTypeBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    uint64_t compatibleNum;

    BOX_PARSE_BEGIN();

    if (mBodySize < 8 || mBodySize % 4 != 0)
    {
        MP4_ERR("ftyp size err %llu\n", mBodySize);
        reader.setCursor(last);
        return -1;
    }

    majorBrand = boxType2Str(reader.readU32(true));

    minorVersion = reader.readU32(true);

    compatibleNum = (mBodySize - 8) / 4;

    for (unsigned int i = 0; i < compatibleNum; ++i)
    {
        compatibles.push_back(boxType2Str(reader.readU32(true)));
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> FileTypeBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    std::string compatiblesStr;
    if (compatibles.size() > 0)
    {
        compatiblesStr = compatibles[0];
        for (size_t i = 1, im = compatibles.size(); i < im; i++)
        {
            compatiblesStr += ", " + compatibles[i];
        }
    }
    item->kvAddPair("Major Brand", majorBrand)
        ->kvAddPair("Minor version", minorVersion)
        ->kvAddPair("Compatibles", compatiblesStr);

    return item;
}

int MovieHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (mFullboxVersion == 1)
    {
        creationTime     = reader.readU64(true);
        modificationTime = reader.readU64(true);
        timescale        = reader.readU32(true);
        duration         = reader.readU64(true);
    }
    else
    {
        creationTime     = reader.readU32(true);
        modificationTime = reader.readU32(true);
        timescale        = reader.readU32(true);
        duration         = reader.readU32(true);
    }

    rate   = reader.readS32(true) / powf(2, 16); // fixed-point 16.16
    volume = reader.readS16(true) / powf(2, 8);  // fixed-point 8.8

    // g_reserve8 = reader.read_u16(true);
    reader.skip(2);

    // for (int i = 0; i < 2; ++i)
    // {
    // 	g_reserve8 = reader.read_u32(true);
    // }
    reader.skip(8);

    READ_MATRIX();

    // for (int i = 0; i < 6; ++i)
    // {
    //	g_reserve8 = reader.read_u32(true);
    // }
    reader.skip(24);

    nextTrackId = reader.readU32(true);

    trackCount = nextTrackId - 1;

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> MovieHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Creation Time", getTimeString(creationTime))
        ->kvAddPair("Modification Time", getTimeString(modificationTime))
        ->kvAddPair("TimeScale", timescale)
        ->kvAddPair("Duration", duration)
        ->kvAddPair("Rate", rate)
        ->kvAddPair("Volume", volume);
    ADD_MATRIX_DATA(item, matrix, ARRAY_SIZE(matrix));
    item->kvAddPair("Next Track ID", nextTrackId);
    return item;
}

int TrackHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();
    MP4_DBG("tkhd flags=%u\n", mFullboxFlags);

    if (mFullboxVersion == 1)
    {
        creationTime     = reader.readU64(true);
        modificationTime = reader.readU64(true);
        trackId          = reader.readU32(true);
        // g_reserve8 = reader.read_u32(true);
        reader.skip(4);
        duration = reader.readU64(true);
    }
    else
    {
        creationTime     = reader.readU32(true);
        modificationTime = reader.readU32(true);
        trackId          = reader.readU32(true);
        // g_reserve8 = reader.read_u32(true);
        reader.skip(4);
        duration = reader.readU32(true);
    }

    // g_reserve8 = reader.read_u64(true);
    reader.skip(8);

    layer = reader.readS16(true);

    alternateGroup = reader.readS16(true);

    volume = reader.readS16(true) / powf(2, 8); // fixed-point 8.8

    // g_reserve8 = reader.read_u16(true);
    reader.skip(2);

    READ_MATRIX();

    width  = reader.readU32(true) / powf(2, 16); // fixed-point 16.16
    height = reader.readU32(true) / powf(2, 16); // fixed-point 16.16

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> TrackHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Creation Time", getTimeString(creationTime))
        ->kvAddPair("Modification Time", getTimeString(modificationTime))
        ->kvAddPair("Track ID", trackId)
        ->kvAddPair("Duration", duration)
        ->kvAddPair("Layer", layer)
        ->kvAddPair("Alternate Group", alternateGroup)
        ->kvAddPair("Volume", volume);
    ADD_MATRIX_DATA(item, matrix, 9);
    item->kvAddPair("Width", width)->kvAddPair("Height", height);

    return item;
}

int MediaHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (mFullboxVersion == 1)
    {
        creationTime     = reader.readU64(true);
        modificationTime = reader.readU64(true);
        timescale        = reader.readU32(true);
        duration         = reader.readU64(true);
    }
    else
    {
        creationTime     = reader.readU32(true);
        modificationTime = reader.readU32(true);
        timescale        = reader.readU32(true);
        duration         = reader.readU32(true);
    }

    uint16_t padAndLanguage;
    padAndLanguage = reader.readU16(false);
    // g_reserve8 = reader.read_u16(true);
    reader.skip(2);

    BitsReader bitsReader(&padAndLanguage, 2);
    bitsReader.readBit(1);
    language[0] = (uint8_t)bitsReader.readBit(5) - 1 + 'a';
    language[1] = (uint8_t)bitsReader.readBit(5) - 1 + 'a';
    language[2] = (uint8_t)bitsReader.readBit(5) - 1 + 'a';

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> MediaHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Creation Time", getTimeString(creationTime))
        ->kvAddPair("Modification Time", getTimeString(modificationTime))
        ->kvAddPair("TimeScale", timescale)
        ->kvAddPair("Duration", duration)
        ->kvAddPair("Language", language);

    return item;
}

int HandlerBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    // g_reserve8 = reader.read_u32(true);
    reader.skip(4);

    handlerType = boxType2Str(reader.readU32(true));

    // for (int i = 0; i < 3; ++i)
    // {
    //     g_reserve8 = reader.read_u32(true);
    // }
    reader.skip(12);

    handlerName = reader.readStr(last - reader.getCursorPos());

    MP4_DBG("handler name %s\n", handlerName.c_str());

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> HandlerBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Handler Type", handlerType);
    item->kvAddPair("Handler Name", handlerName);
    return item;
}

int VideoMediaHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    graphicsMode = reader.readU16(true);

    r = reader.readU16(true);
    g = reader.readU16(true);
    b = reader.readU16(true);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> VideoMediaHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Graphics Mode", graphicsMode)->kvAddPair("R", r)->kvAddPair("G", g)->kvAddPair("B", b);

    return item;
}

int SoundMediaHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    balance = reader.readU16(true) / powf(2, 8);

    // g_reserve8 = reader.read_u16(true);
    reader.skip(2);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> SoundMediaHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Balance", balance);
    return item;
}

int DataReferenceBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    entryCount = reader.readU32(true);

    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> DataReferenceBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Entry Count", entryCount);
    return item;
}

int DataEntryUrlBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (reader.getCursorPos() < last)
    {
        location = reader.readStr(last - reader.getCursorPos());
    }
    else if (mFullboxFlags & 0x1)
    {
        location = "Same File";
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> DataEntryUrlBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Location", location);

    return item;
}

int DataEntryUrnBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (reader.getCursorPos() < last)
    {
        name = reader.readStr(last - reader.getCursorPos());
    }

    if (reader.getCursorPos() < last)
    {
        location = reader.readStr(last - reader.getCursorPos());
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> DataEntryUrnBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Name", name)->kvAddPair("Location", location);
    return item;
}

int SampleDescriptionBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    entryCount = reader.readU32(true);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> SampleDescriptionBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Entry Count", entryCount);
    return item;
}

int MovieExtendsHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (1 == mFullboxVersion)
    {
        fragmentDuration = reader.readU64(true);
    }
    else
    {
        fragmentDuration = reader.readU32(true);
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> MovieExtendsHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Fragment Duration", fragmentDuration);
    return item;
}

int TrackExtendsBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    trackId               = reader.readU32(true);
    defaultSampleDescIdx  = reader.readU32(true);
    defaultSampleDuration = reader.readU32(true);
    defaultSampleSize     = reader.readU32(true);
    defaultSampleFlags    = reader.readU32(true);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> TrackExtendsBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Track ID", trackId)
        ->kvAddPair("Default Sample Description Index", defaultSampleDescIdx)
        ->kvAddPair("Default Sample Duration", defaultSampleDuration)
        ->kvAddPair("Default Sample Size", defaultSampleSize)
        ->kvAddPair("Default Sample Flags", defaultSampleFlags);
    return item;
}

int MovieFragmentHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                  uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    seqNum = reader.readU32(true);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> MovieFragmentHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Sequence Num", seqNum);
    return item;
}

int TrackFragmentHeaderBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                  uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    trackId = reader.readU32(true);

    if (mFullboxFlags & MP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT)
    {
        baseDataOffset = reader.readU64(true);
    }

    if (mFullboxFlags & MP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT)
    {
        sampleDescIdx = reader.readU32(true);
    }

    if (mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT)
    {
        defaultSampleDuration = reader.readU32(true);
    }

    if (mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT)
    {
        defaultSampleSize = reader.readU32(true);
    }

    if (mFullboxFlags & MP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT)
    {
        defaultSampleFlags = reader.readU32(true);
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> TrackFragmentHeaderBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Track Id", trackId)
        ->kvAddPair("Base Data Offset", baseDataOffset)
        ->kvAddPair("Sample Description Index", sampleDescIdx)
        ->kvAddPair("Default Sample Duration", defaultSampleDuration)
        ->kvAddPair("Default Sample Size", defaultSampleSize)
        ->kvAddPair("Default Sample Flags", defaultSampleFlags);
    return item;
}

int TrackFragmentBaseMediaDecodeTimeBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                               uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (1 == mFullboxVersion)
    {
        baseDecTime = reader.readU64(true);
    }
    else
    {
        baseDecTime = reader.readU32(true);
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> TrackFragmentBaseMediaDecodeTimeBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Base Media Decode Time", baseDecTime);
    return item;
}

int MovieFragmentRandomAccessOffsetBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize,
                                              uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    size = reader.readU32(true);

    return 0;
}

std::shared_ptr<Mp4BoxData> MovieFragmentRandomAccessOffsetBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Size", size);
    return item;
}

int BitRateBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    bufferSizeDb   = reader.readU32(true);
    maxBitrate     = reader.readU32(true);
    averageBitrate = reader.readU32(true);

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> BitRateBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Buffer Size DB", bufferSizeDb)
        ->kvAddPair("Max Bitrate", maxBitrate)
        ->kvAddPair("Average Bitrate", averageBitrate);
    return item;
}

int ChannelLayOutBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (0 == channelCount)
    {
        MP4_INFO("wait for channelCount set\n");
        reader.setCursor(last);
        return 0;
    }

    streamStructure = reader.readU8();

    if (streamStructure & channelStructured)
    {
        definedLayout = reader.readU8();
        if (0 == definedLayout)
        {
            for (uint16_t chn = 0; chn < channelCount; chn++)
            {
                ChannelInfo chnInfo;
                chnInfo.speakerPosition = reader.readU8();
                if (126 == chnInfo.speakerPosition)
                {
                    chnInfo.azimuth   = reader.readS16(true);
                    chnInfo.elevation = reader.readS8();
                }
                channelsInfo.push_back(chnInfo);
            }
        }
        else
        {
            omittedChannelsMap = reader.readU64(true);
        }
    }

    if (streamStructure & objectStructured)
    {
        objCount = reader.readU8();
    }

    BOX_PARSE_END();

    return 0;
}

std::shared_ptr<Mp4BoxData> ChannelLayOutBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Stream Structure", streamStructure);
    if (streamStructure & channelStructured)
    {
        item->kvAddPair("Defined Layout", definedLayout);
        if (0 == definedLayout)
        {
            for (size_t i = 0, im = channelsInfo.size(); i < im; i++)
            {
                std::shared_ptr<Mp4BoxData> chnItem =
                    item->kvAddKey("Channel " + std::to_string(i), MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS);

                chnItem->kvAddPair("Speaker Position", channelsInfo[i].speakerPosition);
                if (126 == channelsInfo[i].speakerPosition)
                {
                    chnItem->kvAddPair("Azimuth", channelsInfo[i].azimuth)
                        ->kvAddPair("Elevation", channelsInfo[i].elevation);
                }
            }
        }
        else
        {
            item->kvAddPair("Omitted Channels Map", omittedChannelsMap);
        }

        if (streamStructure & objectStructured)
            item->kvAddPair("Object Count", objCount);
    }
    return item;
}

int SamplingRateBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();
    samplingRate = reader.readU32(true);
    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> SamplingRateBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = src;
    if (nullptr == item)
        item = Mp4BoxData::createKeyValuePairsData();

    item->kvAddPair("Sampling Rate", samplingRate);
    return item;
}

int UuidBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();

    if (boxBodySize < MP4_UUID_LEN)
        return 0;

    reader.read(uuid, MP4_UUID_LEN);

    uuidBodyPos = reader.getCursorPos();

    auto callbacks = gUdtaRegisterCallbacks.find(MP4_UUID(uuid));
    if (gUdtaRegisterCallbacks.end() == callbacks)
    {
        reader.setCursor(last);
        return 0;
    }

    uint64_t udtaDataSize = boxBodySize - MP4_UUID_LEN;

    auto udtaData = std::shared_ptr<uint8_t[]>(new uint8_t[udtaDataSize]);

    reader.read(udtaData.get(), udtaDataSize);

    int parseRes = callbacks->second.parseDataCallback(udtaData.get(), udtaDataSize, &userData);

    BOX_PARSE_END();
    return parseRes;
}

CommonBoxPtr MP4ParserImpl::parseBox(BinaryFileReader &reader, CommonBoxPtr parentBox, bool &parseErr)
{
    int ret;

    Mp4BoxType type;
    uint64_t   boxPos;
    uint64_t   boxSize;
    uint64_t   bodySize;

    ret = get_type_size(reader, type, boxPos, boxSize, bodySize);
    if (-1 == ret)
    {
        return nullptr;
    }

    MP4_INFO("get box %s offset %#llx(%llu), size %#llx(%llu)\n", boxType2Str(type).c_str(), boxPos, boxPos, boxSize,
             boxSize);

    CommonBoxPtr curBox;

    uint32_t compType = type;

    auto userDefineCallback = gUserDefineBoxCallbacks.find(type);
    if (userDefineCallback != gUserDefineBoxCallbacks.end())
    {
        curBox = make_shared<UserDefineBox>(type, userDefineCallback->second.parseDataCallback,
                                            userDefineCallback->second.getDataCallback);
    }
    else
    {
        compType = getCompatibleBoxType(type);

        switch (compType)
        {
            default:
                curBox = make_shared<CommonBox>(type);
                break;
            case MP4_BOX_MAKE_TYPE("edts"):
            case MP4_BOX_MAKE_TYPE("stbl"):
            case MP4_BOX_MAKE_TYPE("moov"):
            case MP4_BOX_MAKE_TYPE("trak"):
            case MP4_BOX_MAKE_TYPE("minf"):
            case MP4_BOX_MAKE_TYPE("dinf"):
            case MP4_BOX_MAKE_TYPE("mdia"):
            case MP4_BOX_MAKE_TYPE("moof"):
            case MP4_BOX_MAKE_TYPE("mvex"):
            case MP4_BOX_MAKE_TYPE("traf"):
            case MP4_BOX_MAKE_TYPE("mfra"):
                curBox = make_shared<ContainBox>(type);
                break;
            case MP4_BOX_MAKE_TYPE("ftyp"):
                curBox = make_shared<FileTypeBox>();
                break;
            case MP4_BOX_MAKE_TYPE("mvhd"):
                curBox = make_shared<MovieHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("tkhd"):
                curBox = make_shared<TrackHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("elst"):
                curBox = make_shared<EditListBox>();
                break;
            case MP4_BOX_MAKE_TYPE("mdhd"):
                curBox = make_shared<MediaHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("hdlr"):
                curBox = make_shared<HandlerBox>();
                break;
            case MP4_BOX_MAKE_TYPE("vmhd"):
                curBox = make_shared<VideoMediaHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("smhd"):
                curBox = make_shared<SoundMediaHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("dref"):
                curBox = make_shared<DataReferenceBox>();
                break;
            case MP4_BOX_MAKE_TYPE("url "):
                curBox = make_shared<DataEntryUrlBox>();
                break;
            case MP4_BOX_MAKE_TYPE("urn "):
                curBox = make_shared<DataEntryUrnBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stts"):
                curBox = make_shared<TimeToSampleBox>();
                break;
            case MP4_BOX_MAKE_TYPE("ctts"):
                curBox = make_shared<CompositionOffsetBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stsc"):
                curBox = make_shared<SampleToChunkBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stsz"):
                curBox = make_shared<SampleSizeBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stz2"):
                curBox = make_shared<CompactSampleSizeBox>();
                break;
            case MP4_BOX_MAKE_TYPE("sdtp"):
                curBox = make_shared<SampleDependencyTypeBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stco"):
                curBox = make_shared<ChunkOffsetBox>();
                break;
            case MP4_BOX_MAKE_TYPE("co64"):
                curBox = make_shared<ChunkLargeOffsetBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stss"):
                curBox = make_shared<SyncSampleBox>();
                break;
            case MP4_BOX_MAKE_TYPE("sgpd"):
                curBox = make_shared<SampleGroupDescriptionBox>();
                break;
            case MP4_BOX_MAKE_TYPE("sbgp"):
                curBox = make_shared<SampleToGroupBox>();
                break;
            case MP4_BOX_MAKE_TYPE("stsd"):
                curBox = make_shared<SampleDescriptionBox>();
                break;
            case MP4_BOX_MAKE_TYPE("skip"):
            case MP4_BOX_MAKE_TYPE("mdat"):
                curBox = make_shared<CommonBox>((type));
                break;
            case MP4_BOX_MAKE_TYPE("colr"):
                curBox = make_shared<ColourInformationBox>();
                break;
            case MP4_BOX_MAKE_TYPE("mehd"):
                curBox = make_shared<MovieExtendsHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("trex"):
                curBox = make_shared<TrackExtendsBox>();
                break;
            case MP4_BOX_MAKE_TYPE("mfhd"):
                curBox = make_shared<MovieFragmentHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("tfhd"):
                curBox = make_shared<TrackFragmentHeaderBox>();
                break;
            case MP4_BOX_MAKE_TYPE("tfdt"):
                curBox = make_shared<TrackFragmentBaseMediaDecodeTimeBox>();
                break;
            case MP4_BOX_MAKE_TYPE("trun"):
                curBox = make_shared<TrackRunBox>();
                break;
            case MP4_BOX_MAKE_TYPE("tfra"):
                curBox = make_shared<TrackFragmentRandomAccessBox>();
                break;
            case MP4_BOX_MAKE_TYPE("mfro"):
                curBox = make_shared<MovieFragmentRandomAccessOffsetBox>();
                break;
            case MP4_BOX_MAKE_TYPE("hvc1"):
                curBox = make_shared<HEVCSampleEntry>((type));
                break;
            case MP4_BOX_MAKE_TYPE("avc1"):
                curBox = make_shared<AVCSampleEntry>((type));
                break;
            case MP4_BOX_MAKE_TYPE("mp4v"):
                curBox = make_shared<MP4VisualSampleEntry>();
                break;
            case MP4_BOX_MAKE_TYPE("mp4a"):
                curBox = make_shared<MP4AudioSampleEntry>();
                break;
            case MP4_BOX_MAKE_TYPE("avcC"):
                curBox = make_shared<AVCConfigurationBox>();
                break;
            case MP4_BOX_MAKE_TYPE("hvcC"):
                curBox = make_shared<HEVCConfigurationBox>();
                break;
            case MP4_BOX_MAKE_TYPE("esds"):
                curBox = make_shared<ESDBox>();
                break;
            case MP4_BOX_MAKE_TYPE("btrt"):
                curBox = make_shared<BitRateBox>();
                break;
            case MP4_BOX_MAKE_TYPE("udta"):
                curBox = make_shared<UdtaBox>();
                break;
            case MP4_BOX_MAKE_TYPE("uuid"):
                curBox = make_shared<UuidBox>();
                break;
        }
    }

    curBox->mParentBox = parentBox;

    ret = curBox->parse(reader, boxPos, boxSize, bodySize);

    if (ret < 0)
    {
        MP4_PARSE_ERR("%s parse fail ret = %d\n", boxType2Str(type).c_str(), ret);
        parseErr         = true;
        curBox->mInvalid = true;
        return curBox;
    }

    while (reader.getCursorPos() < curBox->mBodyPos + curBox->mBodySize)
    {
        MP4_INFO("parse sub boxes for %s from %#llx\n", boxType2Str(curBox->mBoxType).c_str(), reader.getCursorPos());
        CommonBoxPtr subBox = parseBox(reader, curBox, parseErr);
        if (subBox == nullptr)
        {
            parseErr = true;
            return curBox;
        }

        MP4_INFO("get sub box %s for %s\n", boxType2Str(subBox->mBoxType).c_str(),
                 boxType2Str(curBox->mBoxType).c_str());

        subBox->mParentBox = curBox;
        curBox->mContainBoxes.push_back(subBox);

        if (parseErr)
        {
            subBox->mInvalid = true;
            return curBox;
        }

        if (dynamic_pointer_cast<AudioSampleEntry>(curBox) != nullptr && MP4_BOX_MAKE_TYPE("chnl") == subBox->mBoxType)
        {
            AudioSampleEntryPtr audioSampleEntry = dynamic_pointer_cast<AudioSampleEntry>(curBox);
            ChannelLayOutBoxPtr chnl             = dynamic_pointer_cast<ChannelLayOutBox>(subBox);
            chnl->channelCount                   = audioSampleEntry->channelCount;
            uint64_t oldPos                      = reader.getCursorPos();
            reader.setCursor(chnl->mBodyPos);
            chnl->parse(reader, chnl->mBoxOffset, chnl->mBoxSize, chnl->mBodySize);
            reader.setCursor(oldPos);
        }
    }

    // check if there's sdtp to parse
    if (MP4_BOX_MAKE_TYPE("stbl") == compType)
    {
        SampleDependencyTypeBoxPtr sdtp = dynamic_pointer_cast<SampleDependencyTypeBox>(curBox->getContainBox("sdtp"));
        SampleSizeBoxPtr           stsz = dynamic_pointer_cast<SampleSizeBox>(curBox->getContainBox("stsz"));
        CompactSampleSizeBoxPtr    stz2 = dynamic_pointer_cast<CompactSampleSizeBox>(curBox->getContainBox("stz2"));

        if (stsz != nullptr && stz2 != nullptr)
        {
            MP4_WARN("!!!stsz stz2 both exist!!!\n");
        }

        if (sdtp != nullptr)
        {
            if (stsz == nullptr && stz2 == nullptr)
            {
                MP4_PARSE_ERR("!!!stsz/stz2 neither found!!!\n");
            }
            else
            {
                MP4_INFO("back to parse sdtp\n");
                uint32_t entryCount;
                uint64_t oldPos = reader.getCursorPos();
                reader.setCursor(sdtp->mBodyPos);
                if (stsz != nullptr)
                    entryCount = stsz->entryCount;
                else
                    entryCount = stz2->entryCount;

                sdtp->entryCount = entryCount;

                sdtp->parse(reader, sdtp->mBoxOffset, sdtp->mBoxSize, sdtp->mBodySize);

                reader.setCursor(oldPos);
            }
        }
    }

    return curBox;
}

std::shared_ptr<Mp4BoxData> UuidBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    auto callbacks = gUdtaRegisterCallbacks.find(MP4_UUID(uuid));
    if (gUdtaRegisterCallbacks.end() != callbacks)
    {
        return callbacks->second.getDataCallback(userData);
    }

    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Box Offset", mBoxOffset)->kvAddPair("Box Size", mBoxSize);
    string uuidString;
    bool   allChar = true;
    for (int i = 0; i < MP4_UUID_LEN; i++)
    {
        if (!isprint(uuid[i]))
        {
            allChar = false;
            break;
        }
    }
    if (allChar)
    {
        for (int i = 0; i < MP4_UUID_LEN; i++)
            uuidString += (char)uuid[i];
    }
    else
    {
        for (int i = 0; i < MP4_UUID_LEN; i++)
        {
            char buf[5] = {0};
            sprintf(buf, "0x%02x", uuid[i]);
            uuidString += buf;
            if (i < MP4_UUID_LEN - 1)
                uuidString += " ";
        }
    }

    item->kvAddPair("UUID", uuidString);
    return item;
}

int UdtaBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    BOX_PARSE_BEGIN();
    BOX_PARSE_END();
    return 0;
}

std::shared_ptr<Mp4BoxData> UdtaBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    std::shared_ptr<Mp4BoxData> item = nullptr;
    if (nullptr == src)
        item = Mp4BoxData::createKeyValuePairsData();
    else
        item = src;
    item->kvAddPair("Box Offset", mBoxOffset)->kvAddPair("Box Size", mBoxSize);
    return item;
}

UserDefineBox::UserDefineBox(uint32_t boxType, BoxParseFunc parseFunc, BoxDataFunc dataFunc) : CommonBox(boxType)
{
    assert(parseFunc != nullptr && dataFunc != nullptr);
}
int UserDefineBox::parse(BinaryFileReader &reader, uint64_t boxPosition, uint64_t boxSize, uint64_t boxBodySize)
{
    if (nullptr == mParseFunc)
        return -1;

    BinaryData data(boxBodySize);
    reader.read(data.ptr(), boxBodySize);

    return mParseFunc(data.ptr(), boxBodySize, &mUserData);
}
std::shared_ptr<Mp4BoxData> UserDefineBox::getData(std::shared_ptr<Mp4BoxData> src) const
{
    if (nullptr == mDataFunc)
        return src;

    return mDataFunc(mUserData);
}

std::string CommonBox::getBoxTypeStr() const
{
    return boxType2Str(mBoxType);
}

std::vector<std::shared_ptr<Mp4Box>> CommonBox::getContainBoxes() const
{
    if (mInvalid)
        return {};
    std::vector<std::shared_ptr<Mp4Box>> res;
    std::copy(mContainBoxes.begin(), mContainBoxes.end(), std::back_inserter(res));
    return res;
};