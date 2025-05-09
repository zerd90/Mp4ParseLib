#include <sstream>
#include <iostream>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <string>
#include <filesystem>
#include "Mp4ParseTools.h"
#include "Mp4Parse.h"

namespace fs = std::filesystem;
using std::string;

static const char *gWeekString[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

string getTimeString(time_t utcTime) // based on 1904
{
    auto tm = gmtime(&utcTime);
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d %s", tm->tm_year + 1900 - 66 /* 1970-1904 */,
             tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, gWeekString[tm->tm_wday]);
    return buf;
}

BinaryData::BinaryData(uint64_t len) : data(new uint8_t[len])
{
    length = len;
}
void BinaryData::create(uint64_t len)
{
    data   = std::shared_ptr<uint8_t[]>(new uint8_t[len]);
    length = len;
}
uint8_t *BinaryData::ptr() const
{
    return this->data.get();
}

std::string data2hex(const BinaryData &data, int truncateLen)
{
    return data2hex(data.ptr(), data.length, truncateLen);
}

std::string data2hex(const void *buffer, uint64_t bufferSize, int truncateLen)
{
    std::string dataString("0x");
    for (uint64_t i = 0, im = MIN(truncateLen, bufferSize); i < im; i++)
    {
        char strBuffer[5] = {0};
        snprintf(strBuffer, sizeof(strBuffer), "%02X", ((uint8_t *)buffer)[i]);
        dataString += strBuffer;
    }
    if (bufferSize > truncateLen)
    {
        dataString.append("...");
    }
    return dataString;
}

#if defined(WIN32) || defined(_WIN32)
    #define fseek64 _fseeki64
    #define ftell64 _ftelli64
using file_stat64_t = struct _stat64;
    #define stat64 _stat64

#elif defined(__linux)
    #define fseek64 fseeko64
    #define ftell64 ftello64
using file_stat64_t = struct stat64;

#else // Mac
    #define fseek64 fseek
    #define ftell64 ftell
    #define stat64  stat
using file_stat64_t = struct stat;

#endif

std::function<void(MP4_LOG_LEVEL_E, const char *)> gLogCallback = defaultLogCallback;
void setMp4ParseLogCallback(std::function<void(MP4_LOG_LEVEL_E, const char *)> logCallback)
{
    if (nullptr == logCallback)
    {
        logCallback = defaultLogCallback;
    }
    else
    {
        gLogCallback = logCallback;
    }
}

string hexString(uint32_t val)
{
    std::stringstream ss;
    ss << "0x" << std::hex << val;
    return ss.str();
}

BinaryFileReader::BinaryFileReader()
{
    mReadBuffer = std::make_unique<uint8_t[]>(mBufferSize);
}

int BinaryFileReader::checkBuffer(uint64_t readPos, uint64_t readSize)
{
    if (readPos >= fileSize)
    {
        return -1;
    }

    uint64_t canReadSize = MIN(readSize, fileSize - readPos);

    if (mBufferStartOffset > readPos || readPos + canReadSize > mBufferStartOffset + mBufferContainSize)
    {
        mBufferContainSize = MIN(fileSize - readPos, mBufferSize);
        if (fseek64(mFileHandle, readPos, SEEK_SET) < 0)
        {
            MP4_ERR("seek to 0x%" PRIx64 " fail %s\n", readPos, strerror(errno));
            return -1;
        }
        if (fread(mReadBuffer.get(), 1, mBufferContainSize, mFileHandle) != mBufferContainSize)
        {
            MP4_ERR("read %s fail(%s), pos 0x%" PRIx64 ", size 0x%" PRIx64 "\n", mFileFullPath.c_str(), strerror(errno), readPos,
                    mBufferContainSize);
            return -1;
        }
        mBufferStartOffset = readPos;
        MP4_DBG("update buffer, pos=0x%" PRIx64 ", size=0x%" PRIx64 "\n", mBufferStartOffset, mBufferContainSize);
    }
    return 0;
}

int BinaryFileReader::open(std::string &newFileName)
{
    int   ret = -1;
    FILE *tmpFp;

    std::error_code     errCode;
    fs::directory_entry file(newFileName, errCode);
    if (errCode)
    {
        MP4_ERR("file %s fail %s\n", newFileName.c_str(), errCode.message().c_str());
        return -1;
    }
    auto status = file.status(errCode);
    if (errCode)
    {
        MP4_ERR("Get file Status Fail %s\n", errCode.message().c_str());
        return -1;
    }

    if (!is_regular_file(status))
    {
        MP4_ERR("%s not a regular file\n", newFileName.c_str());
        goto exit;
    }

    tmpFp = fopen(newFileName.c_str(), "rb");
    if (!tmpFp)
    {
        MP4_ERR("Can't open file %s\n", newFileName.c_str());
        goto exit;
    }

    close();

    mFileHandle = tmpFp;
    fileSize    = file.file_size(errCode);

    mReadPos = 0;

    mFileFullPath = newFileName;
    mFileName     = file.path().filename().string();
    mDrive        = file.path().root_path().string();
    mDir          = file.path().relative_path().string();
    mBaseName     = file.path().stem().string();
    mExtension    = file.path().extension().string();

    if (mReadBuffer)
    {
        mBufferContainSize = MIN(fileSize, mBufferSize);
        size_t readSize    = fread(mReadBuffer.get(), 1, mBufferContainSize, mFileHandle);
        if (readSize != mBufferContainSize)
        {
            MP4_ERR("read err %zu != 0x%" PRIx64 "\n", readSize, mBufferContainSize);
        }
        mBufferStartOffset = 0;
    }
    ret = 0;

exit:
    return ret;
}

int BinaryFileReader::close()
{
    if (!mFileHandle)
        return 0;

    int ret = fclose(mFileHandle);
    if (ret < 0)
    {
        MP4_ERR("%s close fail(%d)", mFileFullPath.c_str(), ret);
    }
    mFileHandle = nullptr;
    return ret;
}

uint64_t BinaryFileReader::read(void *buf, uint64_t len)
{
    if (!buf)
    {
        mReadPos = MIN(mReadPos + len, fileSize);
        return 0;
    }

    uint64_t rdSize = readStill(buf, len);

    mReadPos += rdSize;

    return rdSize;
}

uint64_t BinaryFileReader::readAt(uint64_t pos, void *buf, uint64_t len)
{
    setFileCursor(pos);
    uint64_t ret = fread(buf, 1, len, mFileHandle);

    return ret;
}

uint64_t BinaryFileReader::readStill(void *buf, uint64_t len)
{
    uint64_t readSize;

    if (len <= mBufferSize)
    {
        if (checkBuffer(mReadPos, len) < 0)
        {
            return 0;
        }

        readSize = MIN(len, mBufferContainSize - (mReadPos - mBufferStartOffset));
        memcpy(buf, mReadBuffer.get() + (mReadPos - mBufferStartOffset), len);
    }
    else
    {
        fseek64(mFileHandle, mReadPos, SEEK_SET);
        readSize = fread(buf, 1, len, mFileHandle);
    }

    return readSize;
}

string BinaryFileReader::readStr(uint64_t maxLen)
{
    string resStr;

    if (maxLen > fileSize - mReadPos)
    {
        maxLen = fileSize - mReadPos;
    }

    char     c      = 0;
    uint64_t maxEnd = mReadPos + maxLen;

    resStr.clear();
    while (1)
    {
        if (mReadPos >= maxEnd)
        {
            break;
        }
        read(&c, 1);

        if (c == 0)
            break;
        resStr.push_back(c);
    }

    return resStr;
}

uint64_t BinaryFileReader::readData(void *buf, uint16_t bufLen, uint16_t dataLen, bool reverse)
{
    uint64_t ret = 0;
    if (reverse)
    {
        int pos = bufLen - dataLen;
        ret     = read((uint8_t *)buf + pos, dataLen);
        if (ret < dataLen)
            return 0;

        switch (bufLen)
        {
            case 2:
                *(uint16_t *)buf = bswap_16(*(uint16_t *)buf);
                break;
            case 4:
                *(uint32_t *)buf = bswap_32(*(uint32_t *)buf);
                break;
            case 8:
                *(uint64_t *)buf = bswap_64(*(uint64_t *)buf);
                break;
            default:
                break;
        }
    }
    else
    {
        ret = read(buf, dataLen);
    }

    return ret;
}

uint8_t BinaryFileReader::readU8()
{
    uint8_t res = 0;
    read(&res, 1);
    return res;
}

int8_t BinaryFileReader::readS8()
{
    int8_t res = 0;
    read(&res, 1);
    return res;
}

uint16_t BinaryFileReader::readU16(bool reverse)
{
    uint16_t res = 0;
    read(&res, 2);
    if (reverse)
        res = bswap_16(res);
    return res;
}

int16_t BinaryFileReader::readS16(bool reverse)
{
    int16_t res = 0;
    read(&res, 2);
    if (reverse)
        res = bswap_16(res);
    return res;
}

uint32_t BinaryFileReader::readU32(bool reverse)
{
    uint32_t res = 0;
    read(&res, 4);
    if (reverse)
        res = bswap_32(res);
    return res;
}

int32_t BinaryFileReader::readS32(bool reverse)
{
    int32_t res = 0;
    read(&res, 4);
    if (reverse)
        res = bswap_32(res);
    return res;
}

uint64_t BinaryFileReader::readU64(bool reverse)
{
    uint64_t res = 0;
    read(&res, 8);
    if (reverse)
        res = bswap_64(res);
    return res;
}

int64_t BinaryFileReader::readS64(bool reverse)
{
    int64_t res = 0;
    read(&res, 8);
    if (reverse)
        res = bswap_64(res);
    return res;
}

uint64_t BinaryFileReader::readUnsigned(uint16_t bytes, bool reverse)
{
    switch (bytes)
    {
        case 1:
            return readU8();
        case 2:
            return readU16(reverse);
        case 4:
            return readU32(reverse);
        case 8:
            return readU64(reverse);
        default:
            uint64_t res = 0;
            for (uint16_t i = 0; i < bytes; i++)
            {
                res = (res << 8) | readU8();
            }

            if (reverse)
            {
                res = bswap_64(res);
                res >>= ((8 - bytes) * 8);
            }
            return res;
    }
}

int64_t BinaryFileReader::readSigned(uint16_t bytes, bool reverse)
{
    switch (bytes)
    {
        case 1:
            return readS8();
        case 2:
            return readS16(reverse);
        case 4:
            return readS32(reverse);
        case 8:
            return readS64(reverse);
        default:
            int64_t res = 0;
            for (uint16_t i = 0; i < bytes; i++)
            {
                res = (res << 8) | readS8();
            }

            if (reverse)
            {
                res = bswap_64(res);
                res >>= ((8 - bytes) * 8);
            }
            return res;
    }
}

uint64_t BinaryFileReader::setCursor(uint64_t pos)
{
    mReadPos = MIN(pos, fileSize);
    return mReadPos;
}

uint64_t BinaryFileReader::skip(uint64_t len)
{
    return setCursor(mReadPos + len);
}

uint64_t BinaryFileReader::setFileCursor(uint64_t absolutePos)
{
    int ret = 0;
    (void)ret;
    uint64_t actualMove = MIN(absolutePos, fileSize);
    ret                 = fseek64(mFileHandle, actualMove, SEEK_SET);
    if (ret < 0)
    {
        return ret;
    }
    else
    {
        mReadPos = actualMove;
    }
    return actualMove;
}

uint8_t BitsReader::readBit()
{
    if (ptr >= sizeBits)
    {
        err = true;
        return 0;
    }

    uint32_t bytePtr = ptr / 8;
    uint8_t  byte    = *(buf + bytePtr);

    uint8_t bitsPtr = 7 - ptr % 8;
    uint8_t res     = (byte >> bitsPtr) & 0x1;

    ptr++;
    return res;
}

uint32_t BitsReader::readBit(int bitCount)
{
    uint32_t res = 0;
    for (int i = 0; i < bitCount; i++)
    {
        uint8_t bit = readBit();
        res         = (res << 1) + bit;
    }

    return res;
}

uint32_t BitsReader::readGolomb()
{
    int n;

    for (n = 0; readBit() == 0 && ptr < sizeBits; n++)
        ;

    return ((uint64_t)1 << n) + readBit(n) - 1;
}

void BitsWriter::writeBit(uint8_t val)
{
    if (ptr >= sizeBits)
        return;

    val = val & 0x1;

    uint32_t bytePtr = ptr / 8;
    uint8_t  byte    = *(buf + bytePtr);

    uint8_t bitPtr = 7 - ptr % 8;
    uint8_t res    = (byte & (~(0x01 << bitPtr))) + (val << bitPtr);

    *(buf + bytePtr) = res;
    ptr++;
}

void BitsWriter::writBits(int bitCount, uint64_t val)
{
    for (int i = 0; i < bitCount; i++)
    {
        uint8_t bit = (val >> (bitCount - 1 - i)) & 0x1;
        writeBit(bit);
    }
}
