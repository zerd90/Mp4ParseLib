#include <vector>
#include "Mp4BoxData.h"
#include "Mp4Defs.h"
#include "Mp4BoxDataKeyValues.h"

using namespace std;
string Mp4BoxDataKeyValues::toString() const
{
    string res;

    res += "{";
    for (uint64_t i = 0, im = mKeyValues.size(); i < im; ++i)
    {
        res += mKeyValues[i].key;
        res += ": ";
        if (mKeyValues[i].value != nullptr)
            res += mKeyValues[i].value->toString();
        if (i < im - 1)
            res += ", ";
    }
    res += "}";

    return res;
}

std::string Mp4BoxDataKeyValues::toHexString() const
{
    string res;

    res += "{";
    for (uint64_t i = 0, im = mKeyValues.size(); i < im; ++i)
    {
        res += mKeyValues[i].key;
        res += ": ";
        if (mKeyValues[i].value != nullptr)
            res += mKeyValues[i].value->toHexString();
        if (i < im - 1)
            res += ", ";
    }
    res += "}";

    return res;
}

shared_ptr<const Mp4BoxData> Mp4BoxDataKeyValues::kvGetValue(const string &key) const
{
    if (mKeyValues.empty())
        return newObject(MP4_BOX_DATA_TYPE_BASIC);

    auto obj = std::find_if(mKeyValues.begin(), mKeyValues.end(), [&](const KeyValue &kv) { return kv.key == key; });
    if (obj != mKeyValues.end())
        return obj->value;
    return mKeyValues[0].value;
}

std::vector<std::string> Mp4BoxDataKeyValues::kvGetKeys() const
{
    std::vector<std::string> res;
    for (auto &kv : mKeyValues)
    {
        res.push_back(kv.key);
    }
    return res;
}

std::string Mp4BoxDataKeyValues::kvGetKey(uint64_t idx) const
{
    idx = MIN(idx, mKeyValues.size() - 1);
    return mKeyValues[idx].key;
}

shared_ptr<const Mp4BoxData> Mp4BoxDataKeyValues::operator[](const string &key) const
{
    return kvGetValue(key);
}

std::shared_ptr<Mp4BoxData> Mp4BoxDataKeyValues::kvAddPair(const std::string &key, std::shared_ptr<Mp4BoxData> val)
{
    KeyValue kv(key, val);
    mKeyValues.push_back(kv);
    return shared_from_this();
}

uint64_t Mp4BoxDataKeyValues::size() const
{
    return mKeyValues.size();
}
