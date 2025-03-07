#ifndef _MP4_BOX_DATA_TABLE_H_
#define _MP4_BOX_DATA_TABLE_H_

#include "Mp4BoxDataTypes.h"

class Mp4BoxDataTable : public Mp4BoxDataBase
{
public:
    Mp4BoxDataTable() : Mp4BoxDataBase(MP4_BOX_DATA_TYPE_TABLE) {}

    virtual std::shared_ptr<Mp4BoxData> tableAddColumn(const std::string &columnName) override;

    virtual std::shared_ptr<Mp4BoxData> tableAddRow(std::shared_ptr<Mp4BoxData> row) override;

    virtual size_t                            tableGetColumnCount() const override;
    virtual std::string                       tableGetColumnName(uint64_t idx) const override;
    virtual size_t                            tableGetRowCount() const override;
    virtual std::shared_ptr<const Mp4BoxData> tableGetRow(uint64_t idx) const override;
    std::shared_ptr<const Mp4BoxData>         operator[](uint64_t idx) const override;

    virtual std::string toString() const override;
    virtual std::string toHexString() const override;

    uint64_t size() const override;

private:
    std::string toStringInternal(bool bHex) const;

private:
    std::vector<std::string>                 mColumnsName;
    std::vector<std::shared_ptr<Mp4BoxData>> mTableItems; // vector of Mp4BoxDataArray
};

#endif // _MP4_BOX_DATA_TABLE_H_