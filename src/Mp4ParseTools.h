#ifndef _MP4_PARSE_TOOLS_H_
#define _MP4_PARSE_TOOLS_H_

#include <functional>
#include <string>
#include "Mp4Types.h"

#ifdef __linux
    #include <byteswap.h>
#else

    #define bswap_16(n) (((n) << 8) | ((n) >> 8))

    #define bswap_32(n)                                                                     \
        ((((n) << 24) & 0xff000000) | (((n) << 8) & 0x00ff0000) | (((n) >> 8) & 0x0000ff00) \
         | (((n) >> 24) & 0x000000ff))

    #define bswap_64(n) ((uint64_t)bswap_32((uint32_t)(n)) << 32 | bswap_32((uint32_t)((n) >> 32)))

#endif

#define MP4_LOG(loglevel, fmt, ...)                                                                 \
    do                                                                                              \
    {                                                                                               \
        char logBuffer[1024] = {0};                                                                 \
        snprintf(logBuffer, sizeof(logBuffer), "[%s:%d]: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
        gLogCallback(loglevel, logBuffer);                                                          \
    } while (0)

#define MP4_ERR(fmt, ...)                               \
    do                                                  \
    {                                                   \
        MP4_LOG(MP4_LOG_LEVEL_ERR, fmt, ##__VA_ARGS__); \
    } while (0)

#define MP4_WARN(fmt, ...)                               \
    do                                                   \
    {                                                    \
        MP4_LOG(MP4_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__); \
    } while (0)

#define MP4_INFO(fmt, ...)                               \
    do                                                   \
    {                                                    \
        MP4_LOG(MP4_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__); \
    } while (0)

#define MP4_DBG(fmt, ...)                               \
    do                                                  \
    {                                                   \
        MP4_LOG(MP4_LOG_LEVEL_DBG, fmt, ##__VA_ARGS__); \
    } while (0)

#define MP4_PARSE_ERR(fmt, ...)                                     \
    do                                                              \
    {                                                               \
        MP4_ERR(fmt, ##__VA_ARGS__);                                \
        char logBuffer[1024] = {0};                                 \
        snprintf(logBuffer, sizeof(logBuffer), fmt, ##__VA_ARGS__); \
        mErrors.push(logBuffer);                                    \
    } while (0)

#define CHECK_RET(func)                               \
    do                                                \
    {                                                 \
        int _ret = func;                              \
        if (_ret < 0)                                 \
        {                                             \
            MP4_ERR("%s fail ret=%d\n", #func, _ret); \
            return _ret;                              \
        }                                             \
    } while (0)

std::string getTimeString(time_t utcTime); // time in mp4 is based on 1904

struct BinaryData
{
    BinaryData() : length(0) {}
    explicit BinaryData(uint64_t len);
    virtual ~BinaryData() {}

    void create(uint64_t len);

    uint8_t *ptr() const;

    uint64_t                   length;
    std::shared_ptr<uint8_t[]> data;
};

std::string data2hex(const BinaryData &data, int truncateLen = 16);
std::string data2hex(const void *buffer, uint64_t bufferSize, int truncateLen = 16);

extern std::function<void(MP4_LOG_LEVEL_E, const char *)> gLogCallback;
std::string hexString(uint32_t val);

struct BinaryFileReader
{
public:
    BinaryFileReader();

    explicit BinaryFileReader(std::string &fn) : BinaryFileReader() { open(fn); }

    ~BinaryFileReader() { close(); }

    int  open(std::string &fn);
    int  close();
    bool isOpened() const { return mFileHandle != nullptr; }

    const std::string &getFileFullPath() const { return mFileFullPath; };
    const std::string &getFileName() const { return mFileName; }
    const std::string &getFileBaseName() const { return mBaseName; };
    const std::string &getFileExtension() const { return mExtension; };
    uint64_t           getCursorPos() const { return mReadPos; };
    uint64_t           getFileSize() const { return fileSize; }

    uint64_t read(void *buf, uint64_t len);

    uint64_t readAt(uint64_t pos, void *buf,
                    uint64_t len); // read len bytes from pos, then back to the readPos before

    uint64_t readStill(void *buf, uint64_t len); // read len bytes, but not changing readPos

    std::string readStr(uint64_t max_len);

    uint64_t readData(void *buf, uint16_t bufLen, uint16_t dataLen, bool reverse);

    uint8_t  readU8();
    int8_t   readS8();
    uint16_t readU16(bool reverse);
    int16_t  readS16(bool reverse);
    uint32_t readU32(bool reverse);
    int32_t  readS32(bool reverse);
    uint64_t readU64(bool reverse);
    int64_t  readS64(bool reverse);

    uint64_t readUnsigned(uint16_t bytes, bool reverse);
    int64_t  readSigned(uint16_t bytes, bool reverse);
    uint64_t setCursor(uint64_t pos);
    uint64_t skip(uint64_t len);

private:
    uint64_t setFileCursor(uint64_t absolutePos);
    int      checkBuffer(uint64_t readPos, uint64_t readSize);

private:
    std::string mFileFullPath;
    std::string mFileName;
    std::string mDrive;
    std::string mDir;
    std::string mBaseName;
    std::string mExtension;

    enum ErrCode
    {
        ERR_FILE_NOT_EXIST = -1,
        ERR_FILE_IS_DIR    = -2,
        ERR_FILE_OPEN_FAIL = -3,
    };

    FILE *mFileHandle = NULL;

    uint64_t fileSize = 0;
    uint64_t mReadPos = 0;

    static const uint64_t      mBufferSize = 1024 * 1024;
    std::unique_ptr<uint8_t[]> mReadBuffer;
    uint64_t                   mBufferStartOffset = 0;
    uint64_t                   mBufferContainSize = 0;
};

struct BitsReader
{
    uint8_t *buf;
    uint32_t sizeBits = 0;
    uint32_t ptr      = 0;
    bool     err      = false;
    BitsReader()      = delete;
    BitsReader(void *buf, uint32_t sizeBytes)
    {
        this->buf = (uint8_t *)buf;
        sizeBits  = sizeBytes * 8;
        ptr       = 0;
    }
    ~BitsReader() {}

    uint8_t  readBit();
    uint32_t readBit(int bits_cnt);
    uint32_t readGolomb();
};

struct BitsWriter
{
    uint8_t *buf;
    uint32_t sizeBits = 0;
    uint32_t ptr      = 0;
    BitsWriter()      = delete;
    BitsWriter(void *buf, uint32_t sizeBytes)
    {
        this->buf = (uint8_t *)buf;
        sizeBits  = sizeBytes * 8;
        ptr       = 0;
    }
    ~BitsWriter() {}

    void writeBit(uint8_t val);
    void writBits(int bits_cnt, uint64_t val);
};
#endif