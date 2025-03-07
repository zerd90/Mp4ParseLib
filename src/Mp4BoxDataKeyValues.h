#ifndef _MP4_BOX_DATA_KEY_VALUES_H_
#define _MP4_BOX_DATA_KEY_VALUES_H_

#include "Mp4BoxDataTypes.h"

class Mp4BoxDataKeyValues : public Mp4BoxDataBase
{
public:
    Mp4BoxDataKeyValues() : Mp4BoxDataBase(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS) {}

    std::vector<std::string>          kvGetKeys() const override;
    std::string                       kvGetKey(uint64_t idx) const override;
    std::shared_ptr<const Mp4BoxData> kvGetValue(const std::string &key) const override;
    std::shared_ptr<const Mp4BoxData> operator[](const std::string &key) const override;

    virtual std::shared_ptr<Mp4BoxData> kvAddPair(const std::string &key, std::shared_ptr<Mp4BoxData> val) override;

    virtual std::string toString() const override;
    virtual std::string toHexString() const override;

    uint64_t size() const override;

private:
    std::vector<KeyValue> mKeyValues;
};

#endif // _MP4_BOX_DATA_KEY_VALUES_H_