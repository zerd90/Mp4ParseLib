#ifndef _MP4_BOX_DATA_BINARY_H_
#define _MP4_BOX_DATA_BINARY_H_

#include "Mp4BoxDataTypes.h"

class Mp4BoxDataBinary : public Mp4BoxDataBase
{
public:
    Mp4BoxDataBinary() : Mp4BoxDataBase(MP4_BOX_DATA_TYPE_BINARY) {}
    virtual ~Mp4BoxDataBinary() override {}

    std::string toString() const override;
    std::string toHexString() const override;

    virtual void
    binarySetCallbacks(const std::function<uint64_t(const void *userData)>                 &getSizeCallback,
                       const std::function<uint8_t(uint64_t offset, const void *userData)> &getDataCallback,
                       const void                                                          *userData) override;

    uint64_t binaryGetSize() const override;
    uint8_t  binaryGetData(uint64_t offset) const override;

    uint64_t size() const override;

private:
    std::function<uint64_t(const void *)>          mGetSizeCallback;
    std::function<uint8_t(uint64_t, const void *)> mGetDataCallback;
    const void                                    *mUserData;
};

#endif