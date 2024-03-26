// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBINARYJSON_P_H
#define QBINARYJSON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qbinaryjson.h"

#include <private/qbinaryjsonvalue_p.h>
#include <private/qendian_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

/*
  This defines a binary data structure for Json data. The data structure is optimised for fast reading
  and minimum allocations. The whole data structure can be mmap'ed and used directly.

  In most cases the binary structure is not as space efficient as a utf8 encoded text representation, but
  much faster to access.

  The size requirements are:

  String:
    Latin1 data: 2 bytes header + string.length()
    Full Unicode: 4 bytes header + 2*(string.length())

  Values: 4 bytes + size of data (size can be 0 for some data)
    bool: 0 bytes
    double: 8 bytes (0 if integer with less than 27bits)
    string: see above
    array: size of array
    object: size of object
  Array: 12 bytes + 4*length + size of Value data
  Object: 12 bytes + 8*length + size of Key Strings + size of Value data

  For an example such as

    {                                           // object: 12 + 5*8                   = 52
         "firstName": "John",                   // key 12, value 8                    = 20
         "lastName" : "Smith",                  // key 12, value 8                    = 20
         "age"      : 25,                       // key 8, value 0                     = 8
         "address"  :                           // key 12, object below               = 140
         {                                      // object: 12 + 4*8
             "streetAddress": "21 2nd Street",  // key 16, value 16
             "city"         : "New York",       // key 8, value 12
             "state"        : "NY",             // key 8, value 4
             "postalCode"   : "10021"           // key 12, value 8
         },                                     // object total: 128
         "phoneNumber":                         // key: 16, value array below         = 172
         [                                      // array: 12 + 2*4 + values below: 156
             {                                  // object 12 + 2*8
               "type"  : "home",                // key 8, value 8
               "number": "212 555-1234"         // key 8, value 16
             },                                 // object total: 68
             {                                  // object 12 + 2*8
               "type"  : "fax",                 // key 8, value 8
               "number": "646 555-4567"         // key 8, value 16
             }                                  // object total: 68
         ]                                      // array total: 156
    }                                           // great total:                         412 bytes

    The uncompressed text file used roughly 500 bytes, so in this case we end up using about
    the same space as the text representation.

    Other measurements have shown a slightly bigger binary size than a compact text
    representation where all possible whitespace was stripped out.
*/

namespace QBinaryJsonPrivate {

class Array;
class Object;
class Value;
class Entry;

template<typename T>
using q_littleendian = QLEInteger<T>;

using qle_short = q_littleendian<short>;
using qle_ushort = q_littleendian<unsigned short>;
using qle_int = q_littleendian<int>;
using qle_uint = q_littleendian<unsigned int>;

template<typename... Accessors>
using qle_bitfield = QLEIntegerBitfieldUnion<uint, Accessors...>;

template<int pos, int width>
using qle_bitfield_accessor
        = QSpecialIntegerAccessor<QLittleEndianStorageType<uint>, pos, width>;

template<int pos, int width>
using qle_signedbitfield_accessor
        = QSpecialIntegerAccessor<QLittleEndianStorageType<uint>, pos, width, int>;

using offset = qle_uint;

// round the size up to the next 4 byte boundary
inline uint alignedSize(uint size) { return (size + 3) & ~3; }

const int MaxLatin1Length = 0x7fff;

static inline bool useCompressed(QStringView s)
{
    if (s.size() > MaxLatin1Length)
        return false;
    return QtPrivate::isLatin1(s);
}

static inline bool useCompressed(QLatin1String s)
{
    return s.size() <= MaxLatin1Length;
}

static inline uint qStringSize(const QString &string, bool compress)
{
    uint l = 2 + string.size();
    if (!compress)
        l *= 2;
    return alignedSize(l);
}

// returns INT_MAX if it can't compress it into 28 bits
static inline int compressedNumber(double d)
{
    // this relies on details of how ieee floats are represented
    const int exponent_off = 52;
    const quint64 fraction_mask = 0x000fffffffffffffULL;
    const quint64 exponent_mask = 0x7ff0000000000000ULL;

    quint64 val;
    memcpy (&val, &d, sizeof(double));
    int exp = (int)((val & exponent_mask) >> exponent_off) - 1023;
    if (exp < 0 || exp > 25)
        return std::numeric_limits<int>::max();

    quint64 non_int = val & (fraction_mask >> exp);
    if (non_int)
        return std::numeric_limits<int>::max();

    bool neg = (val >> 63) != 0;
    val &= fraction_mask;
    val |= ((quint64)1 << 52);
    int res = (int)(val >> (52 - exp));
    return neg ? -res : res;
}

class Latin1String;

class String
{
public:
    explicit String(const char *data) : d(reinterpret_cast<const Data *>(data)) {}

    struct Data {
        qle_uint length;
        qle_ushort utf16[1];
    };
    const Data *d;

    uint byteSize() const { return sizeof(uint) + sizeof(ushort) * d->length; }
    bool isValid(uint maxSize) const
    {
        // Check byteSize() <= maxSize, avoiding integer overflow
        return maxSize >= sizeof(uint)
                && uint(d->length) <= (maxSize - sizeof(uint)) / sizeof(ushort);
    }

    static void copy(char *dest, QStringView str)
    {
        Data *data = reinterpret_cast<Data *>(dest);
        data->length = str.size();
        qToLittleEndian<quint16>(str.utf16(), str.size(), data->utf16);
        fillTrailingZeros(data);
    }

    static void fillTrailingZeros(Data *data)
    {
        if (data->length & 1)
            data->utf16[data->length] = 0;
    }

    bool operator ==(QStringView str) const
    {
        int slen = str.size();
        int l = d->length;
        if (slen != l)
            return false;
        const auto *s = reinterpret_cast<const ushort *>(str.utf16());
        const qle_ushort *a = d->utf16;
        const ushort *b = s;
        while (l-- && *a == *b)
            a++,b++;
        return (l == -1);
    }

    bool operator ==(const String &str) const
    {
        if (d->length != str.d->length)
            return false;
        return !memcmp(d->utf16, str.d->utf16, d->length * sizeof(ushort));
    }

    QString toString() const
    {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return QString(reinterpret_cast<const QChar *>(d->utf16), d->length);
#else
        const uint l = d->length;
        QString str(l, Qt::Uninitialized);
        QChar *ch = str.data();
        for (uint i = 0; i < l; ++i)
            ch[i] = QChar(d->utf16[i]);
        return str;
#endif
    }
};

class Latin1String
{
public:
    explicit Latin1String(const char *data) : d(reinterpret_cast<const Data *>(data)) {}

    struct Data {
        qle_ushort length;
        char latin1[1];
    };
    const Data *d;

    uint byteSize() const { return sizeof(ushort) + sizeof(char) * (d->length); }
    bool isValid(uint maxSize) const { return byteSize() <= maxSize; }

    static void copy(char *dest, QStringView src)
    {
        Data *data = reinterpret_cast<Data *>(dest);
        data->length = src.size(); // ### narrows from int to ushort
        auto *l = data->latin1;
        const auto srcData = src.utf16();
        for (qsizetype i = 0; i < src.size(); ++i)
            l[i] = char(srcData[i]);

        for (uint len = data->length; quintptr(l + len) & 0x3; ++len)
            l[len] = 0;
    }

    QLatin1String toQLatin1String() const noexcept { return QLatin1String(d->latin1, d->length); }
    QString toString() const { return QString::fromLatin1(d->latin1, d->length); }
};

static inline void copyString(char *dest, QStringView str, bool compress)
{
    if (compress)
        Latin1String::copy(dest, str);
    else
        String::copy(dest, str);
}

/*
 Base is the base class for both Object and Array. Both classes work more or less the same way.
 The class starts with a header (defined by the struct below), then followed by data (the data for
 values in the Array case and Entry's (see below) for objects.

 After the data a table follows (tableOffset points to it) containing Value objects for Arrays, and
 offsets from the beginning of the object to Entry's in the case of Object.

 Entry's in the Object's table are lexicographically sorted by key in the table(). This allows the usage
 of a binary search over the keys in an Object.
 */
class Base
{
private:
    using IsObjectAccessor = qle_bitfield_accessor<0, 1>;
    using LengthAccessor = qle_bitfield_accessor<1, 31>;
public:
    qle_uint size;
    qle_bitfield<IsObjectAccessor, LengthAccessor> isObjectAndLength;
    offset tableOffset;
    // content follows here

    void setIsObject() { isObjectAndLength.set<IsObjectAccessor>(1); }
    bool isObject() const { return !!isObjectAndLength.get<IsObjectAccessor>(); }

    void setIsArray() { isObjectAndLength.set<IsObjectAccessor>(0); }
    bool isArray() const { return !isObject(); }

    void setLength(uint length) { isObjectAndLength.set<LengthAccessor>(length); }
    uint length() const { return isObjectAndLength.get<LengthAccessor>(); }

    offset *table()
    {
        return reinterpret_cast<offset *>(reinterpret_cast<char *>(this) + tableOffset);
    }

    const offset *table() const
    {
        return reinterpret_cast<const offset *>(reinterpret_cast<const char *>(this) + tableOffset);
    }

    uint reserveSpace(uint dataSize, uint posInTable, uint numItems, bool replace);
};

class Object : public Base
{
public:
    const Entry *entryAt(uint i) const
    {
        return reinterpret_cast<const Entry *>(reinterpret_cast<const char *>(this) + table()[i]);
    }

    Entry *entryAt(uint i)
    {
        return reinterpret_cast<Entry *>(reinterpret_cast<char *>(this) + table()[i]);
    }

    uint indexOf(QStringView key, bool *exists) const;
    QJsonObject toJsonObject() const;
    bool isValid(uint maxSize) const;
};

class Array : public Base
{
public:
    const Value *at(uint i) const { return reinterpret_cast<const Value *>(table() + i); }
    Value *at(uint i) { return reinterpret_cast<Value *>(table() + i); }

    QJsonArray toJsonArray() const;
    bool isValid(uint maxSize) const;
};

class Value
{
private:
    using TypeAccessor = qle_bitfield_accessor<0, 3>;
    using LatinOrIntValueAccessor = qle_bitfield_accessor<3, 1>;
    using LatinKeyAccessor = qle_bitfield_accessor<4, 1>;
    using ValueAccessor = qle_bitfield_accessor<5, 27>;
    using IntValueAccessor = qle_signedbitfield_accessor<5, 27>;

    qle_bitfield<
            TypeAccessor,
            LatinOrIntValueAccessor,
            LatinKeyAccessor,
            ValueAccessor,
            IntValueAccessor
            > m_data;


    int intValue() const { return m_data.get<IntValueAccessor>(); }

public:
    enum {
        MaxSize = (1 << 27) - 1
    };

    inline const char *data(const Base *b) const
    {
        return reinterpret_cast<const char *>(b) + value();
    }

    uint usedStorage(const Base *b) const;

    bool toBoolean() const
    {
        Q_ASSERT(type() == QJsonValue::Bool);
        return value() != 0;
    }

    double toDouble(const Base *b) const
    {
        Q_ASSERT(type() == QJsonValue::Double);
        if (isLatinOrIntValue())
            return intValue();

        auto i = qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(b) + value());
        double d;
        memcpy(&d, &i, sizeof(double));
        return d;
    }

    QString toString(const Base *b) const
    {
        return isLatinOrIntValue()
                ? asLatin1String(b).toString()
                : asString(b).toString();
    }

    String asString(const Base *b) const
    {
        Q_ASSERT(type() == QJsonValue::String && !isLatinOrIntValue());
        return String(data(b));
    }

    Latin1String asLatin1String(const Base *b) const
    {
        Q_ASSERT(type() == QJsonValue::String && isLatinOrIntValue());
        return Latin1String(data(b));
    }

    const Base *base(const Base *b) const
    {
        Q_ASSERT(type() == QJsonValue::Array || type() == QJsonValue::Object);
        return reinterpret_cast<const Base *>(data(b));
    }

    QJsonValue toJsonValue(const Base *b) const;
    bool isValid(const Base *b) const;

    static uint requiredStorage(const QBinaryJsonValue &v, bool *compressed);
    static uint valueToStore(const QBinaryJsonValue &v, uint offset);
    static void copyData(const QBinaryJsonValue &v, char *dest, bool compressed);

    void setIsLatinKey(bool isLatinKey) { m_data.set<LatinKeyAccessor>(isLatinKey); }
    bool isLatinKey() const { return m_data.get<LatinKeyAccessor>(); }

    void setIsLatinOrIntValue(bool v) { m_data.set<LatinOrIntValueAccessor>(v); }
    bool isLatinOrIntValue() const { return m_data.get<LatinOrIntValueAccessor>(); }

    void setType(uint type) { m_data.set<TypeAccessor>(type); }
    uint type() const { return m_data.get<TypeAccessor>(); }

    void setValue(uint value) { m_data.set<ValueAccessor>(value); }
    uint value() const { return m_data.get<ValueAccessor>(); }
};

class Entry {
public:
    Value value;
    // key
    // value data follows key

    uint size() const
    {
        uint s = sizeof(Entry);
        if (value.isLatinKey())
            s += shallowLatin1Key().byteSize();
        else
            s += shallowKey().byteSize();
        return alignedSize(s);
    }

    uint usedStorage(Base *b) const
    {
        return size() + value.usedStorage(b);
    }

    String shallowKey() const
    {
        Q_ASSERT(!value.isLatinKey());
        return String(reinterpret_cast<const char *>(this) + sizeof(Entry));
    }

    Latin1String shallowLatin1Key() const
    {
        Q_ASSERT(value.isLatinKey());
        return Latin1String(reinterpret_cast<const char *>(this) + sizeof(Entry));
    }

    QString key() const
    {
        return value.isLatinKey()
                ? shallowLatin1Key().toString()
                : shallowKey().toString();
    }

    bool isValid(uint maxSize) const
    {
        if (maxSize < sizeof(Entry))
            return false;
        maxSize -= sizeof(Entry);
        return value.isLatinKey()
                ? shallowLatin1Key().isValid(maxSize)
                : shallowKey().isValid(maxSize);
    }

    bool operator ==(QStringView key) const
    {
        return value.isLatinKey()
                ? (shallowLatin1Key().toQLatin1String() == key)
                : (shallowKey() == key);
    }

    bool operator >=(QStringView key) const
    {
        return value.isLatinKey()
                ? (shallowLatin1Key().toQLatin1String() >= key)
                : (shallowKey().toString() >= key);
    }
};

class Header {
public:
    qle_uint tag; // 'qbjs'
    qle_uint version; // 1
    Base *root() { return reinterpret_cast<Base *>(this + 1); }
    const Base *root() const { return reinterpret_cast<const Base *>(this + 1); }
};

class ConstData
{
    Q_DISABLE_COPY_MOVE(ConstData)
public:
    const uint alloc;
    union {
        const char *rawData;
        const Header *header;
    };

    ConstData(const char *raw, uint a) : alloc(a), rawData(raw) {}
    bool isValid() const;
    QJsonDocument toJsonDocument() const;
};

class MutableData
{
    Q_DISABLE_COPY_MOVE(MutableData)
public:
    QAtomicInt ref;
    uint alloc;
    union {
        char *rawData;
        Header *header;
    };
    uint compactionCounter : 31;

    MutableData(char *raw, uint a)
        : alloc(a), rawData(raw), compactionCounter(0)
    {
    }

    MutableData(uint reserved, QJsonValue::Type valueType)
        : rawData(nullptr), compactionCounter(0)
    {
        Q_ASSERT(valueType == QJsonValue::Array || valueType == QJsonValue::Object);

        alloc = sizeof(Header) + sizeof(Base) + reserved + sizeof(offset);
        header = reinterpret_cast<Header *>(malloc(alloc));
        Q_CHECK_PTR(header);
        header->tag = QJsonDocument::BinaryFormatTag;
        header->version = 1;
        Base *b = header->root();
        b->size = sizeof(Base);
        if (valueType == QJsonValue::Object)
            b->setIsObject();
        else
            b->setIsArray();
        b->tableOffset = sizeof(Base);
        b->setLength(0);
    }

    ~MutableData()
    {
        free(rawData);
    }

    MutableData *clone(const Base *b, uint reserve = 0)
    {
        uint size = sizeof(Header) + b->size;
        if (b == header->root() && ref.loadRelaxed() == 1 && alloc >= size + reserve)
            return this;

        if (reserve) {
            if (reserve < 128)
                reserve = 128;
            size = qMax(size + reserve, qMin(size *2, uint(Value::MaxSize)));
            if (size > Value::MaxSize) {
                qWarning("QJson: Document too large to store in data structure");
                return nullptr;
            }
        }
        char *raw = reinterpret_cast<char *>(malloc(size));
        Q_CHECK_PTR(raw);
        memcpy(raw + sizeof(Header), b, b->size);
        auto *h = reinterpret_cast<Header *>(raw);
        h->tag = QJsonDocument::BinaryFormatTag;
        h->version = 1;
        auto *d = new MutableData(raw, size);
        d->compactionCounter = (b == header->root()) ? compactionCounter : 0;
        return d;
    }

    char *takeRawData(uint *size)
    {
        *size = alloc;
        char *result = rawData;
        rawData = nullptr;
        alloc = 0;
        return result;
    }

    void compact();
};

} // namespace QBinaryJsonPrivate

Q_DECLARE_TYPEINFO(QBinaryJsonPrivate::Value, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QBINARYJSON_P_H
