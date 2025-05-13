#ifndef _MP4_BOX_DATA_TABLE_H_
#define _MP4_BOX_DATA_TABLE_H_

#include "Mp4BoxDataTypes.h"
#include <functional>

class Mp4BoxDataTable : public Mp4BoxDataBase
{
public:
    Mp4BoxDataTable() : Mp4BoxDataBase(MP4_BOX_DATA_TYPE_TABLE) {}

    virtual std::shared_ptr<Mp4BoxData> tableAddColumn(const std::string &columnName) override;

    virtual size_t                            tableGetColumnCount() const override;
    virtual std::string                       tableGetColumnName(size_t idx) const override;
    virtual size_t                            tableGetRowCount() const override;
    virtual std::shared_ptr<const Mp4BoxData> tableGetRow(size_t idx) const override;
    std::shared_ptr<const Mp4BoxData>         operator[](uint64_t idx) const override;

    virtual std::string toString() const override;
    virtual std::string toHexString() const override;

    uint64_t     size() const override;
    virtual void tableSetCallbacks(
        const std::function<uint64_t(const void *)>                                              &getRowCountCallback,
        const std::function<std::shared_ptr<const Mp4BoxData>(const void *, uint64_t)>           &getRowCallback,
        const std::function<std::shared_ptr<const Mp4BoxData>(const void *, uint64_t, uint64_t)> &getCellCallback,
        const void                                                                               *userData) override;

private:
    std::string toStringInternal(bool bHex) const;

private:
    std::vector<std::string> mColumnsName;

    std::function<uint64_t(const void *)> mGetRowCountCallback;

    std::function<std::shared_ptr<const Mp4BoxData>(const void *, uint64_t, uint64_t)> mGetCellCallback;
    std::function<std::shared_ptr<const Mp4BoxData>(const void *, uint64_t)>           mGetRowCallback;
    const void                                                                        *mCallbackData = nullptr;
};

#endif // _MP4_BOX_DATA_TABLE_H_