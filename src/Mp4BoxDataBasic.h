#ifndef _MP4_BOX_DATA_BASIC_H_
#define _MP4_BOX_DATA_BASIC_H_

#include "Mp4BoxDataTypes.h"

template <typename T, typename U>
static inline constexpr bool decayEquiv = std::is_same_v<std::decay_t<T>, std::decay_t<U>>;

#define IS_SIGNED(type)                                                                                          \
    (decayEquiv<signed char, type> || decayEquiv<short, type> || decayEquiv<int, type> || decayEquiv<long, type> \
     || decayEquiv<long long, type>)

#define IS_UNSIGNED(type)                                                                                  \
    (decayEquiv<unsigned char, type> || decayEquiv<unsigned short, type> || decayEquiv<unsigned int, type> \
     || decayEquiv<unsigned long, type> || decayEquiv<unsigned long long, type>)

#define IS_REAL(type) (decayEquiv<float, type> || decayEquiv<double, type>)

#define IS_STRING(type) (decayEquiv<std::string, type>)
#define IS_CSTR(type)   (decayEquiv<char *, type> || decayEquiv<const char *, type>)

class Mp4BoxDataBasic : public Mp4BoxDataBase
{
private:
    struct
    {
        uint64_t    u64 = 0;
        int64_t     s64 = 0;
        double      f64 = 0;
        std::string str;
    } objectValue;

    bool bhex = false;

public:
    explicit Mp4BoxDataBasic(MP4_BOX_DATA_TYPE_E type) : Mp4BoxDataBase(type) {}
    template <typename T>
    explicit Mp4BoxDataBasic(T val, bool bhex = false);

    int64_t     basicGetValueS64() const override;
    int32_t     basicGetValueS32() const override;
    int16_t     basicGetValueS16() const override;
    int8_t      basicGetValueS8() const override;
    uint64_t    basicGetValueU64() const override;
    uint32_t    basicGetValueU32() const override;
    uint16_t    basicGetValueU16() const override;
    uint8_t     basicGetValueU8() const override;
    double      basicGetValueReal() const override;
    std::string basicGetValueStr() const override;

    virtual std::string toString() const override;
    virtual std::string toHexString() const override;
};

template <typename T>
Mp4BoxDataBasic::Mp4BoxDataBasic(T val, bool bhex) : Mp4BoxDataBase(MP4_BOX_DATA_TYPE_BASIC)
{
    static_assert(IS_SIGNED(T) || IS_UNSIGNED(T) || IS_REAL(T) || IS_STRING(T) || IS_CSTR(T) || decayEquiv<char, T>,
                  "type wrong");

    if constexpr (IS_SIGNED(T) || decayEquiv<char, T>)
    {
        mObjectType     = MP4_BOX_DATA_TYPE_SINT;
        objectValue.s64 = val;
        this->bhex      = bhex;
    }
    else if constexpr (IS_UNSIGNED(T))
    {
        mObjectType     = MP4_BOX_DATA_TYPE_UINT;
        objectValue.u64 = val;
        this->bhex      = bhex;
    }
    else if constexpr (IS_REAL(T))
    {
        mObjectType     = MP4_BOX_DATA_TYPE_REAL;
        objectValue.f64 = val;
        this->bhex      = bhex;
    }
    else if constexpr (IS_STRING(T) || IS_CSTR(T))
    {
        mObjectType     = MP4_BOX_DATA_TYPE_STR;
        objectValue.str = val;
    }
}

#endif // _MP4_BOX_DATA_BASIC_H_