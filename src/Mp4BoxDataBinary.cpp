
#include <iomanip>
#include <sstream>

#include "Mp4BoxDataBinary.h"

using std::string;

string Mp4BoxDataBinary::toString() const
{
    return toHexString();
}
string Mp4BoxDataBinary::toHexString() const
{
    if (!mGetSizeCallback || !mGetDataCallback)
        std::stringstream ss;

    uint64_t size = mGetSizeCallback(mUserData);
    if (size == 0)
        return "";

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint64_t i = 0; i < size; i++)
    {
        uint8_t data = mGetDataCallback(i, mUserData);
        ss << std::setw(2) << static_cast<int>(data);
        if (i < size - 1)
            ss << " ";
    }
    return ss.str();
}

void Mp4BoxDataBinary::binarySetCallbacks(
    const std::function<uint64_t(const void *userData)>                 &getSizeCallback,
    const std::function<uint8_t(uint64_t offset, const void *userData)> &getDataCallback, const void *userData)
{
    mGetSizeCallback = getSizeCallback;
    mGetDataCallback = getDataCallback;
    mUserData        = userData;
}

uint64_t Mp4BoxDataBinary::binaryGetSize() const
{
    if (mGetSizeCallback)
        return mGetSizeCallback(mUserData);

    return 0;
}

uint8_t Mp4BoxDataBinary::binaryGetData(uint64_t offset) const
{
    if (mGetDataCallback)
        return mGetDataCallback(offset, mUserData);

    return 0;
}

uint64_t Mp4BoxDataBinary::size() const
{
    return binaryGetSize();
}