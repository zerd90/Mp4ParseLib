#ifndef _MP4_BOX_DATA_ARRAY_H_
#define _MP4_BOX_DATA_ARRAY_H_

#include "Mp4BoxDataTypes.h"

class Mp4BoxDataArray : public Mp4BoxDataBase
{
public:
    Mp4BoxDataArray() : Mp4BoxDataBase(MP4_BOX_DATA_TYPE_ARRAY) {}

    virtual std::shared_ptr<Mp4BoxData> arrayAddItem(std::shared_ptr<Mp4BoxData> val) override;

    virtual std::shared_ptr<const Mp4BoxData> arrayGetData(uint64_t idx) const override;
    std::shared_ptr<const Mp4BoxData>         operator[](uint64_t idx) const override;

    std::string         toString() const override;
    virtual std::string toHexString() const override;
    uint64_t            size() const override;

private:
    std::vector<std::shared_ptr<Mp4BoxData>> mArrayItems;
};

#endif // _MP4_BOX_DATA_ARRAY_H_