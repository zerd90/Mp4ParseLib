#ifndef _MP4_BOX_DATA_TYPES_H_
#define _MP4_BOX_DATA_TYPES_H_

#include <string>
#include <vector>
#include <memory>

#include "Mp4BoxData.h"

class Mp4BoxDataBase;

class Mp4BoxDataBasic;
class Mp4BoxDataKeyValues;
class Mp4BoxDataArray;
class Mp4BoxDataTable;
struct KeyValue
{
    std::string                 key;
    std::shared_ptr<Mp4BoxData> value;

    KeyValue(const std::string &key, std::shared_ptr<Mp4BoxData> value) : key(key), value(value) {}
};

class Mp4BoxDataBase : public Mp4BoxData
{
public:
    Mp4BoxDataBase() = delete;
    explicit Mp4BoxDataBase(MP4_BOX_DATA_TYPE_E type) : mObjectType(type) {}
    virtual ~Mp4BoxDataBase() {}

    virtual MP4_BOX_DATA_TYPE_E getDataType() const override { return mObjectType; }
    virtual int64_t             basicGetValueS64() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual int32_t basicGetValueS32() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual int16_t basicGetValueS16() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual int8_t basicGetValueS8() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual uint64_t basicGetValueU64() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual uint32_t basicGetValueU32() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual uint16_t basicGetValueU16() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual uint8_t basicGetValueU8() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual double basicGetValueReal() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }
    virtual std::string basicGetValueStr() const override
    {
        assert(mObjectType <= MP4_BOX_DATA_TYPE_BASIC);
        return 0;
    }

    virtual uint64_t size() const override
    {
        assert(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS == mObjectType || MP4_BOX_DATA_TYPE_ARRAY == mObjectType
               || MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return 0;
    }

    virtual std::vector<std::string> kvGetKeys() const override
    {
        assert(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS == mObjectType);
        return std::vector<std::string>();
    }

    std::string kvGetKey(uint64_t idx) const override
    {
        assert(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS == mObjectType);
        return std::string();
    }

    virtual std::shared_ptr<const Mp4BoxData> kvGetValue(const std::string &key) const override
    {
        assert(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS == mObjectType);
        return shared_from_this();
    }

    virtual std::shared_ptr<const Mp4BoxData> operator[](const std::string &key) const override
    {
        return kvGetValue(key);
    }

    std::shared_ptr<const Mp4BoxData> arrayGetData(uint64_t idx) const override
    {
        assert(MP4_BOX_DATA_TYPE_ARRAY == mObjectType);
        return shared_from_this();
    }
    virtual std::shared_ptr<const Mp4BoxData> operator[](uint64_t idx) const override
    {
        assert(MP4_BOX_DATA_TYPE_ARRAY == mObjectType || MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return shared_from_this();
    }

    virtual std::shared_ptr<Mp4BoxData> arrayAddItem(std::shared_ptr<Mp4BoxData> val) override
    {
        assert(MP4_BOX_DATA_TYPE_ARRAY == mObjectType);
        return shared_from_this();
    }
    virtual std::shared_ptr<Mp4BoxData> kvAddPair(const std::string &key, std::shared_ptr<Mp4BoxData> val) override
    {
        assert(MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS == mObjectType);
        return shared_from_this();
    }

    virtual std::shared_ptr<Mp4BoxData> tableAddColumn(const std::string &columnName) override
    {
        assert(MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return shared_from_this();
    }
    virtual std::shared_ptr<Mp4BoxData> tableAddRow(std::shared_ptr<Mp4BoxData> row) override
    {
        assert(MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return shared_from_this();
    }
    virtual size_t tableGetColumnCount() const override
    {
        assert(MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return 0;
    }
    virtual std::string tableGetColumnName(size_t columnIdx) const override
    {
        assert(MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return std::string();
    }
    size_t tableGetRowCount() const override
    {
        assert(MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return 0;
    }
    std::shared_ptr<const Mp4BoxData> tableGetRow(size_t rowIdx) const override
    {
        assert(MP4_BOX_DATA_TYPE_TABLE == mObjectType);
        return nullptr;
    }

    friend std::ostream &operator<<(std::ostream &os, const Mp4BoxDataBase &obj);

protected:
    MP4_BOX_DATA_TYPE_E mObjectType = MP4_BOX_DATA_TYPE_UNKNOWN;
};

std::ostream &operator<<(std::ostream &os, const Mp4BoxDataBase &obj);

std::shared_ptr<Mp4BoxData> newObject(MP4_BOX_DATA_TYPE_E type);
template <typename T>
std::shared_ptr<Mp4BoxData> newObject(T val)
{
    return std::make_shared<Mp4BoxDataBasic>(val);
}
#endif // _MP4_BOX_DATA_TYPES_H_