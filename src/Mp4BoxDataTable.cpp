
#include <vector>
#include "Mp4Defs.h"
#include "Mp4BoxDataTable.h"

using namespace std;

shared_ptr<const Mp4BoxData> Mp4BoxDataTable::tableGetRow(size_t idx) const
{
    if (mGetRowCallback != nullptr)
        return mGetRowCallback(mCallbackData, idx);
    return nullptr;
}

std::shared_ptr<const Mp4BoxData> Mp4BoxDataTable::operator[](uint64_t idx) const
{
    return tableGetRow(idx);
}

string Mp4BoxDataTable::toStringInternal(bool bHex) const
{
    string res;

    res += "{";
    res += "\"Headers\":{";
    for (size_t idx = 0; idx < mColumnsName.size(); idx++)
    {
        res += mColumnsName[idx];
        if (idx != mColumnsName.size() - 1)
            res += ", ";
    }
    res += "}, Items:{";
    size_t rows = tableGetRowCount();
    for (size_t idx = 0; idx < rows; idx++)
    {
        res += "{";
        auto rowItem = tableGetRow(idx);
        for (size_t valIdx = 0; valIdx < rowItem->size(); valIdx++)
        {
            if (bHex)
                res += (*rowItem)[valIdx]->toHexString();
            else
                res += (*rowItem)[valIdx]->toString();
            if (valIdx != (*rowItem).size() - 1)
                res += ", ";
        }
        res += "}";
        if (idx != rows - 1)
            res += ", ";
    }
    res += "}}";
    return res;
}

string Mp4BoxDataTable::toString() const
{
    return toStringInternal(false);
}

std::string Mp4BoxDataTable::toHexString() const
{
    return toStringInternal(true);
}

std::string Mp4BoxDataTable::tableGetColumnName(size_t idx) const
{
    return mColumnsName[MIN(idx, mColumnsName.size() - 1)];
}

std::shared_ptr<Mp4BoxData> Mp4BoxDataTable::tableAddColumn(const std::string &columnName)
{
    mColumnsName.push_back(columnName);
    return shared_from_this();
}

uint64_t Mp4BoxDataTable::size() const
{
    return tableGetRowCount();
}

size_t Mp4BoxDataTable::tableGetRowCount() const
{
    if (mGetRowCountCallback == nullptr)
        return 0;
    return mGetRowCountCallback(mCallbackData);
}

size_t Mp4BoxDataTable::tableGetColumnCount() const
{
    return mColumnsName.size();
}
void Mp4BoxDataTable::tableSetCallbacks(
    const std::function<uint64_t(const void *)>                                              &getRowCountCallback,
    const std::function<std::shared_ptr<const Mp4BoxData>(const void *, uint64_t)>           &getRowCallback,
    const std::function<std::shared_ptr<const Mp4BoxData>(const void *, uint64_t, uint64_t)> &getCellCallback,
    const void                                                                               *userData)
{
    mGetRowCountCallback = getRowCountCallback;
    mGetRowCallback      = getRowCallback;
    mGetCellCallback     = getCellCallback;
    mCallbackData        = userData;
}
