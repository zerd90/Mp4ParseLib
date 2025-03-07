
#ifndef _DATA_STORE_H_
#define _DATA_STORE_H_

#include "Mp4Defs.h"
#include <assert.h>

#include <string>
#include <memory>
#include <vector>

typedef enum
{
    MP4_BOX_DATA_TYPE_SINT,
    MP4_BOX_DATA_TYPE_UINT,
    MP4_BOX_DATA_TYPE_REAL,
    MP4_BOX_DATA_TYPE_STR,
    MP4_BOX_DATA_TYPE_BASIC = MP4_BOX_DATA_TYPE_STR,
    MP4_BOX_DATA_TYPE_KEY_VALUE_PAIRS,
    MP4_BOX_DATA_TYPE_ARRAY,
    MP4_BOX_DATA_TYPE_TABLE,
    MP4_BOX_DATA_TYPE_UNKNOWN
} MP4_BOX_DATA_TYPE_E;

class Mp4BoxData : public std::enable_shared_from_this<Mp4BoxData>
{
public:
    virtual ~Mp4BoxData() {}
    virtual MP4_BOX_DATA_TYPE_E getDataType() const = 0;

    virtual int64_t     basicGetValueS64() const  = 0;
    virtual int32_t     basicGetValueS32() const  = 0;
    virtual int16_t     basicGetValueS16() const  = 0;
    virtual int8_t      basicGetValueS8() const   = 0;
    virtual uint64_t    basicGetValueU64() const  = 0;
    virtual uint32_t    basicGetValueU32() const  = 0;
    virtual uint16_t    basicGetValueU16() const  = 0;
    virtual uint8_t     basicGetValueU8() const   = 0;
    virtual double      basicGetValueReal() const = 0;
    virtual std::string basicGetValueStr() const  = 0;

    virtual std::string toString() const    = 0;
    virtual std::string toHexString() const = 0;

    virtual uint64_t size() const = 0; // for array / table / key value pairs

    // for key value pairs
    virtual std::shared_ptr<Mp4BoxData> kvAddPair(const std::string &key, std::shared_ptr<Mp4BoxData> val) = 0;
    std::shared_ptr<Mp4BoxData>         kvAddKey(const std::string &key, MP4_BOX_DATA_TYPE_E dataType)
    {
        auto newData = createData(dataType);
        kvAddPair(key, newData);
        return newData;
    }
    template <typename T>
    std::shared_ptr<Mp4BoxData> kvAddPair(const std::string &key, T val)
    {
        std::shared_ptr<Mp4BoxData> valObj = createBasicData(val);
        kvAddPair(key, valObj);
        return shared_from_this();
    }
    virtual std::vector<std::string>          kvGetKeys() const                        = 0;
    virtual std::string                       kvGetKey(uint64_t idx) const             = 0;
    virtual std::shared_ptr<const Mp4BoxData> kvGetValue(const std::string &key) const = 0;
    virtual std::shared_ptr<const Mp4BoxData> operator[](const std::string &key) const = 0;

    // for array
    virtual std::shared_ptr<Mp4BoxData> arrayAddItem(const std::shared_ptr<Mp4BoxData> val) = 0;
    template <typename T>
    std::shared_ptr<Mp4BoxData> arrayAddItem(T val)
    {
        std::shared_ptr<Mp4BoxData> valObj = createBasicData(val);
        arrayAddItem(valObj);
        return shared_from_this();
    }
    virtual std::shared_ptr<const Mp4BoxData> arrayGetData(uint64_t idx) const = 0;

    // for table
    virtual std::shared_ptr<Mp4BoxData> tableAddColumn(const std::string &columnName) = 0;
    template <typename... Args>
    void setColumnsName(Args... str)
    {
        (tableAddColumn(str), ...);
    }
    virtual std::shared_ptr<Mp4BoxData> tableAddRow(std::shared_ptr<Mp4BoxData> row) = 0;
    template <typename... Args>
    void tableAddRow(Args... args)
    {
        std::shared_ptr<Mp4BoxData> item = createArrayData();
        addRowValue(item, args...);
        tableAddRow(item);
    }
    virtual size_t                            tableGetColumnCount() const                = 0;
    virtual std::string                       tableGetColumnName(size_t columnIdx) const = 0;
    virtual size_t                            tableGetRowCount() const                   = 0;
    virtual std::shared_ptr<const Mp4BoxData> tableGetRow(size_t rowIdx) const           = 0;

    // for array / table
    virtual std::shared_ptr<const Mp4BoxData> operator[](uint64_t idx) const = 0;

    template <typename T>
    static std::shared_ptr<Mp4BoxData> createBasicData(T data);
    static std::shared_ptr<Mp4BoxData> createArrayData();
    static std::shared_ptr<Mp4BoxData> createKeyValuePairsData();
    static std::shared_ptr<Mp4BoxData> createTableData();

private:
    std::shared_ptr<Mp4BoxData> createData(MP4_BOX_DATA_TYPE_E dataType);
    void                        addRowValue(std::shared_ptr<Mp4BoxData> &valueArray) { MP4_UNUSED(valueArray); }

    template <typename T, typename... Args>
    void addRowValue(std::shared_ptr<Mp4BoxData> &valueArray, T val1, Args... vals)
    {
        std::shared_ptr<Mp4BoxData> curValue = createBasicData(val1);
        valueArray->arrayAddItem(curValue);
        addRowValue(valueArray, vals...);
    }
};

std::ostream &operator<<(std::ostream &os, const std::shared_ptr<Mp4BoxData> pobj);

#endif