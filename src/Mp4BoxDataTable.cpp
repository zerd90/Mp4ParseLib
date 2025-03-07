
#include <vector>
#include "Mp4Defs.h"
#include "Mp4BoxDataTable.h"

using namespace std;

shared_ptr<const Mp4BoxData> Mp4BoxDataTable::tableGetRow(uint64_t idx) const
{
    return mTableItems[MIN(idx, mTableItems.size() - 1)];
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
    for (size_t idx = 0; idx < mTableItems.size(); idx++)
    {
        res += "{";
        auto &rowItem = *mTableItems[idx];
        for (size_t valIdx = 0; valIdx < mTableItems[idx]->size(); valIdx++)
        {
            if (bHex)
                res += rowItem[valIdx]->toHexString();
            else
                res += rowItem[valIdx]->toString();
            if (valIdx != rowItem.size() - 1)
                res += ", ";
        }
        res += "}";
        if (idx != mTableItems.size() - 1)
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

std::string Mp4BoxDataTable::tableGetColumnName(uint64_t idx) const
{
    return mColumnsName[MIN(idx, mColumnsName.size() - 1)];
}

std::shared_ptr<Mp4BoxData> Mp4BoxDataTable::tableAddRow(std::shared_ptr<Mp4BoxData> row)
{
    mTableItems.push_back(row);
    return shared_from_this();
}

std::shared_ptr<Mp4BoxData> Mp4BoxDataTable::tableAddColumn(const std::string &columnName)
{
    mColumnsName.push_back(columnName);
    return shared_from_this();
}

uint64_t Mp4BoxDataTable::size() const
{
    return mTableItems.size();
}

size_t Mp4BoxDataTable::tableGetRowCount() const
{
    return mTableItems.size();
}

size_t Mp4BoxDataTable::tableGetColumnCount() const
{
    return mColumnsName.size();
}
