/* This file is autogenerated. DO NOT CHANGE. All changes will be lost */

#ifndef QPROTOBUF_ANNOTATION_H
#define QPROTOBUF_ANNOTATION_H

#include <QtProtobuf/qprotobufmessage.h>
#include <QtProtobuf/qprotobufobject.h>
#include <QtProtobuf/qprotobuflazymessagepointer.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

#include <QtCore/qmetatype.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>

#include <memory>


namespace qtprotobufnamespace::tests {
class AnnotatedMessage1;
using AnnotatedMessage1Repeated = QList<AnnotatedMessage1>;
namespace AnnotatedMessage1_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage1_QtProtobufNested

class AnnotatedMessage2;
using AnnotatedMessage2Repeated = QList<AnnotatedMessage2>;
namespace AnnotatedMessage2_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage2_QtProtobufNested

class AnnotatedMessage3;
using AnnotatedMessage3Repeated = QList<AnnotatedMessage3>;
namespace AnnotatedMessage3_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage3_QtProtobufNested

class AnnotatedMessage4;
using AnnotatedMessage4Repeated = QList<AnnotatedMessage4>;
namespace AnnotatedMessage4_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage4_QtProtobufNested

class AnnotatedMessage5;
using AnnotatedMessage5Repeated = QList<AnnotatedMessage5>;
namespace AnnotatedMessage5_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage5_QtProtobufNested

class AnnotatedMessage6;
using AnnotatedMessage6Repeated = QList<AnnotatedMessage6>;
namespace AnnotatedMessage6_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage6_QtProtobufNested

class AnnotatedMessage7;
using AnnotatedMessage7Repeated = QList<AnnotatedMessage7>;
namespace AnnotatedMessage7_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage7_QtProtobufNested

class AnnotatedMessage8;
using AnnotatedMessage8Repeated = QList<AnnotatedMessage8>;
namespace AnnotatedMessage8_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage8_QtProtobufNested

class AnnotatedMessage9;
using AnnotatedMessage9Repeated = QList<AnnotatedMessage9>;
namespace AnnotatedMessage9_QtProtobufNested {
enum class QtProtobufFieldEnum;
} // namespace AnnotatedMessage9_QtProtobufNested


/* test annotation */
class AnnotatedMessage1_QtProtobufData;
class AnnotatedMessage1 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage1)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage1_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage1();
    ~AnnotatedMessage1();
    AnnotatedMessage1(const AnnotatedMessage1 &other);
    AnnotatedMessage1 &operator =(const AnnotatedMessage1 &other);
    AnnotatedMessage1(AnnotatedMessage1 &&other) noexcept;
    AnnotatedMessage1 &operator =(AnnotatedMessage1 &&other) noexcept;
    bool operator ==(const AnnotatedMessage1 &other) const;
    bool operator !=(const AnnotatedMessage1 &other) const;

    /**
     * \brief Field annotation
     */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage1_QtProtobufData> dptr;
};
namespace AnnotatedMessage1_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage1_QtProtobufNested

/* test annotation */
class AnnotatedMessage2_QtProtobufData;
class AnnotatedMessage2 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage2)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage2_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage2();
    ~AnnotatedMessage2();
    AnnotatedMessage2(const AnnotatedMessage2 &other);
    AnnotatedMessage2 &operator =(const AnnotatedMessage2 &other);
    AnnotatedMessage2(AnnotatedMessage2 &&other) noexcept;
    AnnotatedMessage2 &operator =(AnnotatedMessage2 &&other) noexcept;
    bool operator ==(const AnnotatedMessage2 &other) const;
    bool operator !=(const AnnotatedMessage2 &other) const;

    /*!
     * \brief Field annotation
     */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage2_QtProtobufData> dptr;
};
namespace AnnotatedMessage2_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage2_QtProtobufNested

/* test annotation */
class AnnotatedMessage3_QtProtobufData;
class AnnotatedMessage3 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage3)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage3_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage3();
    ~AnnotatedMessage3();
    AnnotatedMessage3(const AnnotatedMessage3 &other);
    AnnotatedMessage3 &operator =(const AnnotatedMessage3 &other);
    AnnotatedMessage3(AnnotatedMessage3 &&other) noexcept;
    AnnotatedMessage3 &operator =(AnnotatedMessage3 &&other) noexcept;
    bool operator ==(const AnnotatedMessage3 &other) const;
    bool operator !=(const AnnotatedMessage3 &other) const;

    /*
     * Field annotation
     * Field annotation secondline
     */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage3_QtProtobufData> dptr;
};
namespace AnnotatedMessage3_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage3_QtProtobufNested

/* test annotation */
class AnnotatedMessage4_QtProtobufData;
class AnnotatedMessage4 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage4)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage4_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage4();
    ~AnnotatedMessage4();
    AnnotatedMessage4(const AnnotatedMessage4 &other);
    AnnotatedMessage4 &operator =(const AnnotatedMessage4 &other);
    AnnotatedMessage4(AnnotatedMessage4 &&other) noexcept;
    AnnotatedMessage4 &operator =(AnnotatedMessage4 &&other) noexcept;
    bool operator ==(const AnnotatedMessage4 &other) const;
    bool operator !=(const AnnotatedMessage4 &other) const;

    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage4_QtProtobufData> dptr;
};
namespace AnnotatedMessage4_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage4_QtProtobufNested

/*! \brief test annotation */
class AnnotatedMessage5_QtProtobufData;
class AnnotatedMessage5 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage5)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage5_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage5();
    ~AnnotatedMessage5();
    AnnotatedMessage5(const AnnotatedMessage5 &other);
    AnnotatedMessage5 &operator =(const AnnotatedMessage5 &other);
    AnnotatedMessage5(AnnotatedMessage5 &&other) noexcept;
    AnnotatedMessage5 &operator =(AnnotatedMessage5 &&other) noexcept;
    bool operator ==(const AnnotatedMessage5 &other) const;
    bool operator !=(const AnnotatedMessage5 &other) const;

    /* Field annotation */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage5_QtProtobufData> dptr;
};
namespace AnnotatedMessage5_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage5_QtProtobufNested

/** \brief test annotation */
class AnnotatedMessage6_QtProtobufData;
class AnnotatedMessage6 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage6)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage6_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage6();
    ~AnnotatedMessage6();
    AnnotatedMessage6(const AnnotatedMessage6 &other);
    AnnotatedMessage6 &operator =(const AnnotatedMessage6 &other);
    AnnotatedMessage6(AnnotatedMessage6 &&other) noexcept;
    AnnotatedMessage6 &operator =(AnnotatedMessage6 &&other) noexcept;
    bool operator ==(const AnnotatedMessage6 &other) const;
    bool operator !=(const AnnotatedMessage6 &other) const;

    /** Field annotation */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage6_QtProtobufData> dptr;
};
namespace AnnotatedMessage6_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage6_QtProtobufNested

/*!
 * \brief test annotation
 */
class AnnotatedMessage7_QtProtobufData;
class AnnotatedMessage7 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage7)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage7_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage7();
    ~AnnotatedMessage7();
    AnnotatedMessage7(const AnnotatedMessage7 &other);
    AnnotatedMessage7 &operator =(const AnnotatedMessage7 &other);
    AnnotatedMessage7(AnnotatedMessage7 &&other) noexcept;
    AnnotatedMessage7 &operator =(AnnotatedMessage7 &&other) noexcept;
    bool operator ==(const AnnotatedMessage7 &other) const;
    bool operator !=(const AnnotatedMessage7 &other) const;

    /*! Field annotation */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage7_QtProtobufData> dptr;
};
namespace AnnotatedMessage7_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage7_QtProtobufNested

/**
 * \brief test annotation
 */
class AnnotatedMessage8_QtProtobufData;
class AnnotatedMessage8 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage8)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage8_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage8();
    ~AnnotatedMessage8();
    AnnotatedMessage8(const AnnotatedMessage8 &other);
    AnnotatedMessage8 &operator =(const AnnotatedMessage8 &other);
    AnnotatedMessage8(AnnotatedMessage8 &&other) noexcept;
    AnnotatedMessage8 &operator =(AnnotatedMessage8 &&other) noexcept;
    bool operator ==(const AnnotatedMessage8 &other) const;
    bool operator !=(const AnnotatedMessage8 &other) const;

    /* Field annotation */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage8_QtProtobufData> dptr;
};
namespace AnnotatedMessage8_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage8_QtProtobufNested

/*
 * test annotation
 * test annotation secondline
 */
class AnnotatedMessage9_QtProtobufData;
class AnnotatedMessage9 : public QProtobufMessage
{
    Q_GADGET
    Q_PROTOBUF_OBJECT
    Q_DECLARE_PROTOBUF_SERIALIZERS(AnnotatedMessage9)
    Q_PROPERTY(QtProtobuf::sint32 testField READ testField WRITE setTestField SCRIPTABLE true)

public:
    using QtProtobufFieldEnum = AnnotatedMessage9_QtProtobufNested::QtProtobufFieldEnum;
    AnnotatedMessage9();
    ~AnnotatedMessage9();
    AnnotatedMessage9(const AnnotatedMessage9 &other);
    AnnotatedMessage9 &operator =(const AnnotatedMessage9 &other);
    AnnotatedMessage9(AnnotatedMessage9 &&other) noexcept;
    AnnotatedMessage9 &operator =(AnnotatedMessage9 &&other) noexcept;
    bool operator ==(const AnnotatedMessage9 &other) const;
    bool operator !=(const AnnotatedMessage9 &other) const;

    /* Field annotation */
    QtProtobuf::sint32 testField() const;
    void setTestField(const QtProtobuf::sint32 &testField);
    static void registerTypes();

private:
    QExplicitlySharedDataPointer<AnnotatedMessage9_QtProtobufData> dptr;
};
namespace AnnotatedMessage9_QtProtobufNested {
Q_NAMESPACE

enum class QtProtobufFieldEnum {
    TestFieldProtoFieldNumber = 1,
};
Q_ENUM_NS(QtProtobufFieldEnum)

} // namespace AnnotatedMessage9_QtProtobufNested
} // namespace qtprotobufnamespace::tests

Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage1)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage2)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage3)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage4)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage5)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage6)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage7)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage8)
Q_DECLARE_METATYPE(qtprotobufnamespace::tests::AnnotatedMessage9)
#endif // QPROTOBUF_ANNOTATION_H
