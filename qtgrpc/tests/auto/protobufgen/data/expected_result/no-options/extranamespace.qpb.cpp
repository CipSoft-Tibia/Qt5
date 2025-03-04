/* This file is autogenerated. DO NOT CHANGE. All changes will be lost */

#include "extranamespace.qpb.h"
#include <QtProtobuf/qprotobufserializer.h>
#include <cmath>

namespace qtprotobufnamespace::tests {

class EmptyMessage_QtProtobufData : public QSharedData
{
public:
    EmptyMessage_QtProtobufData()
        : QSharedData()
    {
    }

    EmptyMessage_QtProtobufData(const EmptyMessage_QtProtobufData &other)
        : QSharedData(other)
    {
    }

};

EmptyMessage::~EmptyMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 1> qt_protobuf_EmptyMessage_uint_data;
    const char qt_protobuf_EmptyMessage_char_data[40];
} qt_protobuf_EmptyMessage_metadata {
    // data
    {
        0, /* = version */
        0, /* = num fields */
        1, /* = field number offset */
        1, /* = property index offset */
        1, /* = field flags offset */
        38, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        39, /* = end-of-string-marker */
        // Field numbers:
        // Property indices:
        // Field flags:
    },
    // char_data
    /* metadata char_data: */
    "qtprotobufnamespace.tests.EmptyMessage\0" /* = full message name */
    /* field char_data: */
    ""
};

const QtProtobufPrivate::QProtobufPropertyOrdering EmptyMessage::propertyOrdering = {
    &qt_protobuf_EmptyMessage_metadata.data
};

void EmptyMessage::registerTypes()
{
    qRegisterMetaType<EmptyMessage>();
    qRegisterMetaType<EmptyMessageRepeated>();
}

EmptyMessage::EmptyMessage()
    : QProtobufMessage(&EmptyMessage::staticMetaObject),
      dptr(new EmptyMessage_QtProtobufData)
{
}

EmptyMessage::EmptyMessage(const EmptyMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
EmptyMessage &EmptyMessage::operator =(const EmptyMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
EmptyMessage::EmptyMessage(EmptyMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
EmptyMessage &EmptyMessage::operator =(EmptyMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool EmptyMessage::operator ==(const EmptyMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other);
}

bool EmptyMessage::operator !=(const EmptyMessage &other) const
{
    return !this->operator ==(other);
}


class SimpleStringMessage_QtProtobufData : public QSharedData
{
public:
    SimpleStringMessage_QtProtobufData()
        : QSharedData()
    {
    }

    SimpleStringMessage_QtProtobufData(const SimpleStringMessage_QtProtobufData &other)
        : QSharedData(other),
          m_testFieldString(other.m_testFieldString)
    {
    }

    QString m_testFieldString;
};

SimpleStringMessage::~SimpleStringMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 5> qt_protobuf_SimpleStringMessage_uint_data;
    const char qt_protobuf_SimpleStringMessage_char_data[63];
} qt_protobuf_SimpleStringMessage_metadata {
    // data
    {
        0, /* = version */
        1, /* = num fields */
        2, /* = field number offset */
        3, /* = property index offset */
        4, /* = field flags offset */
        45, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        46, /* = testFieldString */
        62, /* = end-of-string-marker */
        // Field numbers:
        6, /* = testFieldString */
        // Property indices:
        0, /* = testFieldString */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = testFieldString */
    },
    // char_data
    /* metadata char_data: */
    "qtprotobufnamespace.tests.SimpleStringMessage\0" /* = full message name */
    /* field char_data: */
    "testFieldString\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering SimpleStringMessage::propertyOrdering = {
    &qt_protobuf_SimpleStringMessage_metadata.data
};

void SimpleStringMessage::registerTypes()
{
    qRegisterMetaType<SimpleStringMessage>();
    qRegisterMetaType<SimpleStringMessageRepeated>();
}

SimpleStringMessage::SimpleStringMessage()
    : QProtobufMessage(&SimpleStringMessage::staticMetaObject),
      dptr(new SimpleStringMessage_QtProtobufData)
{
}

SimpleStringMessage::SimpleStringMessage(const SimpleStringMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
SimpleStringMessage &SimpleStringMessage::operator =(const SimpleStringMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
SimpleStringMessage::SimpleStringMessage(SimpleStringMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
SimpleStringMessage &SimpleStringMessage::operator =(SimpleStringMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool SimpleStringMessage::operator ==(const SimpleStringMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && dptr->m_testFieldString == other.dptr->m_testFieldString;
}

bool SimpleStringMessage::operator !=(const SimpleStringMessage &other) const
{
    return !this->operator ==(other);
}

QString SimpleStringMessage::testFieldString() const
{
    return dptr->m_testFieldString;
}

void SimpleStringMessage::setTestFieldString(const QString &testFieldString)
{
    if (dptr->m_testFieldString != testFieldString) {
        dptr.detach();
        dptr->m_testFieldString = testFieldString;
    }
}


class ComplexMessage_QtProtobufData : public QSharedData
{
public:
    ComplexMessage_QtProtobufData()
        : QSharedData(),
          m_testFieldInt(0),
          m_testComplexField(nullptr)
    {
    }

    ComplexMessage_QtProtobufData(const ComplexMessage_QtProtobufData &other)
        : QSharedData(other),
          m_testFieldInt(other.m_testFieldInt),
          m_testComplexField(other.m_testComplexField
                                               ? new SimpleStringMessage(*other.m_testComplexField)
                                               : nullptr)
    {
    }

    QtProtobuf::int32 m_testFieldInt;
    QtProtobufPrivate::QProtobufLazyMessagePointer<SimpleStringMessage> m_testComplexField;
};

ComplexMessage::~ComplexMessage() = default;

static constexpr struct {
    QtProtobufPrivate::QProtobufPropertyOrdering::Data data;
    const std::array<uint, 9> qt_protobuf_ComplexMessage_uint_data;
    const char qt_protobuf_ComplexMessage_char_data[72];
} qt_protobuf_ComplexMessage_metadata {
    // data
    {
        0, /* = version */
        2, /* = num fields */
        3, /* = field number offset */
        5, /* = property index offset */
        7, /* = field flags offset */
        40, /* = message full name length */
    },
    // uint_data
    {
        // JSON name offsets:
        41, /* = testFieldInt */
        54, /* = testComplexField */
        71, /* = end-of-string-marker */
        // Field numbers:
        1, /* = testFieldInt */
        2, /* = testComplexField */
        // Property indices:
        0, /* = testFieldInt */
        1, /* = testComplexField */
        // Field flags:
        QtProtobufPrivate::NoFlags, /* = testFieldInt */
        QtProtobufPrivate::NoFlags, /* = testComplexField */
    },
    // char_data
    /* metadata char_data: */
    "qtprotobufnamespace.tests.ComplexMessage\0" /* = full message name */
    /* field char_data: */
    "testFieldInt\0testComplexField\0"
};

const QtProtobufPrivate::QProtobufPropertyOrdering ComplexMessage::propertyOrdering = {
    &qt_protobuf_ComplexMessage_metadata.data
};

void ComplexMessage::registerTypes()
{
    qRegisterMetaType<ComplexMessage>();
    qRegisterMetaType<ComplexMessageRepeated>();
}

ComplexMessage::ComplexMessage()
    : QProtobufMessage(&ComplexMessage::staticMetaObject),
      dptr(new ComplexMessage_QtProtobufData)
{
}

ComplexMessage::ComplexMessage(const ComplexMessage &other)
    : QProtobufMessage(other),
      dptr(other.dptr)
{
}
ComplexMessage &ComplexMessage::operator =(const ComplexMessage &other)
{
    QProtobufMessage::operator=(other);
    dptr = other.dptr;
    return *this;
}
ComplexMessage::ComplexMessage(ComplexMessage &&other) noexcept
    : QProtobufMessage(std::move(other)),
      dptr(std::move(other.dptr))
{
}
ComplexMessage &ComplexMessage::operator =(ComplexMessage &&other) noexcept
{
    QProtobufMessage::operator=(std::move(other));
    dptr.swap(other.dptr);
    return *this;
}
bool ComplexMessage::operator ==(const ComplexMessage &other) const
{
    return QProtobufMessage::isEqual(*this, other)
        && dptr->m_testFieldInt == other.dptr->m_testFieldInt
        && (dptr->m_testComplexField == other.dptr->m_testComplexField
            || *dptr->m_testComplexField == *other.dptr->m_testComplexField);
}

bool ComplexMessage::operator !=(const ComplexMessage &other) const
{
    return !this->operator ==(other);
}

QtProtobuf::int32 ComplexMessage::testFieldInt() const
{
    return dptr->m_testFieldInt;
}

SimpleStringMessage *ComplexMessage::testComplexField_p() const
{
    return dptr->m_testComplexField ? dptr->m_testComplexField.get() : nullptr;
}

bool ComplexMessage::hasTestComplexField() const
{
    return dptr->m_testComplexField.operator bool();
}

SimpleStringMessage &ComplexMessage::testComplexField()
{
    dptr.detach();
    return *dptr->m_testComplexField;
}
const SimpleStringMessage &ComplexMessage::testComplexField() const
{
    return *dptr->m_testComplexField;
}

void ComplexMessage::clearTestComplexField()
{
    if (dptr->m_testComplexField) {
        dptr.detach();
        dptr->m_testComplexField.reset();
    }
}

void ComplexMessage::setTestFieldInt(const QtProtobuf::int32 &testFieldInt)
{
    if (dptr->m_testFieldInt != testFieldInt) {
        dptr.detach();
        dptr->m_testFieldInt = testFieldInt;
    }
}

void ComplexMessage::setTestComplexField_p(SimpleStringMessage *testComplexField)
{
    if (dptr->m_testComplexField.get() != testComplexField) {
        dptr.detach();
        dptr->m_testComplexField.reset(testComplexField);
    }
}

void ComplexMessage::setTestComplexField(const SimpleStringMessage &testComplexField)
{
    if (*dptr->m_testComplexField != testComplexField) {
        dptr.detach();
        *dptr->m_testComplexField = testComplexField;
    }
}

} // namespace qtprotobufnamespace::tests

#include "moc_extranamespace.qpb.cpp"
