

#include "Mp4BoxDataArray.h"

using namespace std;

std::shared_ptr<Mp4BoxData> Mp4BoxDataArray::arrayAddItem(std::shared_ptr<Mp4BoxData> val)
{
    mArrayItems.push_back(val);
    return shared_from_this();
}

string Mp4BoxDataArray::toString() const
{
    string res;
    res += "[";
    for (uint64_t i = 0, im = mArrayItems.size(); i < im; ++i)
    {
        res += mArrayItems[i]->toString();
        if (i < im - 1)
            res += ", ";
    }
    res += "]";

    return res;
}

std::string Mp4BoxDataArray::toHexString() const
{
    string res;
    res += "[";
    for (uint64_t i = 0, im = mArrayItems.size(); i < im; ++i)
    {
        res += mArrayItems[i]->toHexString();
        if (i < im - 1)
            res += ", ";
    }
    res += "]";

    return res;
}

std::shared_ptr<const Mp4BoxData> Mp4BoxDataArray::arrayGetData(uint64_t idx) const
{
    if (idx >= mArrayItems.size())
        idx = mArrayItems.size() - 1;
    return mArrayItems[idx];
}

std::shared_ptr<const Mp4BoxData> Mp4BoxDataArray::operator[](uint64_t idx) const
{
    return arrayGetData(idx);
}

uint64_t Mp4BoxDataArray::size() const
{
    return mArrayItems.size();
}
