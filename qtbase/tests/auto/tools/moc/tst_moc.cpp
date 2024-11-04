// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2020 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <stdio.h>
#include <qobject.h>
#include <qmetaobject.h>
#include <qjsondocument.h>
#include <qregularexpression.h>
#include <qtyperevision.h>

#include <private/qobject_p.h>

#include "using-namespaces.h"
#include "assign-namespace.h"
#include "no-keywords.h"
#include "single_function_keyword.h"
#include "backslash-newlines.h"
#include "slots-with-void-template.h"
#include "qinvokable.h"
// msvc and friends crap out on it
#if !defined(Q_CC_GNU) || defined(Q_OS_WIN)
#define SKIP_NEWLINE_TEST
#endif
#if !defined(SKIP_NEWLINE_TEST)
#include "os9-newlines.h"
// msvc and friends crap out on this file too,
// it seems to contain Mac 9 EOLs, and not windows EOLs.
#include "win-newlines.h"
#endif
#include "escapes-in-string-literals.h"
#include "cstyle-enums.h"

#if defined(PARSE_BOOST)
#include "parse-boost.h"
#endif
#include "cxx11-enums.h"
#include "cxx11-final-classes.h"
#include "cxx11-explicit-override-control.h"
#include "cxx11-trailing-return.h"

#include "parse-defines.h"
#include "related-metaobjects-in-namespaces.h"
#include "related-metaobjects-in-gadget.h"
#include "related-metaobjects-name-conflict.h"

#include "non-gadget-parent-class.h"
#include "grand-parent-gadget-class.h"
#include "namespace.h"
#include "cxx17-namespaces.h"
#include "cxx-attributes.h"

#include "moc_include.h"
#include "pointery_to_incomplete.h"
#include "fwdclass1.h"
#include "fwdclass2.h"
#include "fwdclass3.h"

#include "signal-with-default-arg.h"

#include "qmlmacro.h"

#include "tech-preview.h"

using namespace Qt::StringLiterals;

#ifdef Q_MOC_RUN
// check that moc can parse these constructs, they are being used in Windows winsock2.h header
#define STRING_HASH_HASH(x) ("foo" ## x ## "bar")
const char *string_hash_hash = STRING_HASH_HASH("baz");
#endif

#if defined(Q_MOC_RUN) || __cplusplus > 202002L
/* Check that nested inline namespaces are at least not causing moc to break.
   Check it even outside of C++20 mode as moc gets passed the  wrong __cplusplus version
   and also to increase coverage, given how few C++20 configurations exist in the CI at the time
   of writing this comment.
*/
namespace A::inline B {}
namespace A {
   namespace B::inline C {}
}
#endif


namespace TokenStartingWithNumber
{
Q_NAMESPACE

#define FOR_EACH_ITEM( CALL ) \
  CALL( EXAMPLE ) \
  CALL( 123_EXAMPLE ) \
  CALL( OTHER_EXAMPLE )

enum FooItems
{

#define ENUM_ITEM(NAME, ...) FOO ## NAME,
  FOR_EACH_ITEM( ENUM_ITEM )
};

Q_ENUM_NS(FooItems)
}

Q_DECLARE_METATYPE(const QMetaObject*);

#define TESTEXPORTMACRO Q_DECL_EXPORT

#if !defined(Q_MOC_RUN) && !defined(Q_NOREPLY)
# define Q_NOREPLY
#endif

struct TagTest : QObject {
    Q_OBJECT

    Q_INVOKABLE Q_NOREPLY inline int test() {return 0;}
public slots:
    Q_NOREPLY virtual inline void pamOpen(int){}
};


namespace TestNonQNamespace {

struct TestGadget {
    Q_GADGET
    Q_CLASSINFO("key", "value")
public:
    enum class TestGEnum1 {
        Key1 = 11,
        Key2
    };
    Q_ENUM(TestGEnum1)

    enum class TestGEnum2 {
        Key1 = 17,
        Key2
    };
    Q_ENUM(TestGEnum2)

    enum TestGEnum3: quint8 {
        Key1 = 23,
        Key2
    };
    Q_ENUM(TestGEnum3)
};

}

namespace TestQNamespace {
    Q_NAMESPACE
    enum class TestEnum1 {
        Key1 = 11,
        Key2
    };
    Q_ENUM_NS(TestEnum1)

    enum class TestEnum2 {
        Key1 = 17,
        Key2
    };
    Q_ENUM_NS(TestEnum2)

    enum TestEnum3: qint8 {
        Key1 = 23,
        Key2
    };
    Q_ENUM_NS(TestEnum3)

    // try to dizzy moc by adding a struct in between
    struct TestGadget {
        Q_GADGET
    public:
        enum class TestGEnum1 {
            Key1 = 13,
            Key2
        };
        enum class TestGEnum2 {
            Key1 = 23,
            Key2
        };
        enum TestGEnum3: qint16 {
            Key1 = 33,
            Key2
        };
        Q_ENUM(TestGEnum1)
        Q_ENUM(TestGEnum2)
        Q_ENUM(TestGEnum3)
    };

    struct TestGadgetExport {
        Q_GADGET_EXPORT(TESTEXPORTMACRO)
        Q_CLASSINFO("key", "exported")
    public:
        enum class TestGeEnum1 {
            Key1 = 20,
            Key2
        };
        Q_ENUM(TestGeEnum1)
        enum class TestGeEnum2 {
            Key1 = 23,
            Key2
        };
        Q_ENUM(TestGeEnum2)
        enum TestGeEnum3: quint16 {
            Key1 = 26,
            Key2
        };
        Q_ENUM(TestGeEnum3)

    };

    enum class TestFlag1 {
        None = 0,
        Flag1 = 1,
        Flag2 = 2,
        Any = Flag1 | Flag2
    };
    Q_FLAG_NS(TestFlag1)

    enum class TestFlag2 {
        None = 0,
        Flag1 = 4,
        Flag2 = 8,
        Any = Flag1 | Flag2
    };
    Q_FLAG_NS(TestFlag2)
}

namespace TestSameEnumNamespace {
    Q_NAMESPACE

    enum class TestSameEnumNamespace {
        Key1 = 1,
        Key2 = 2,
    };
    Q_ENUM_NS(TestSameEnumNamespace)
}

namespace TestNestedSameEnumNamespace {
namespace a {
    Q_NAMESPACE
    // enum class with the same name as the enclosing nested namespace
    enum class a {
        Key11 = 11,
        Key12 = 12,
    };
    Q_ENUM_NS(a)
}
}

namespace TestExportNamespace {
    Q_NAMESPACE_EXPORT(TESTEXPORTMACRO)
    enum class MyEnum {
        Key1, Key2
    };
    Q_ENUM_NS(MyEnum)
}

QT_USE_NAMESPACE

template <bool b> struct QTBUG_31218 {};
struct QTBUG_31218_Derived : QTBUG_31218<-1<0> {};

#if defined(Q_MOC_RUN)
 class QTBUG_45790 : Bug() { };
#endif

class CreatableGadget
{
    Q_GADGET
public:
    Q_INVOKABLE CreatableGadget()
    {
        CreatableGadget::qt_static_metacall((QObject*)this, QMetaObject::ReadProperty, -1, nullptr);
    }
};

CreatableGadget creatableGadget; // Force the compiler to use the constructor

struct ParentWithSignalWithArgument : QObject {
    Q_OBJECT
    Q_PROPERTY(int i READ i WRITE setI NOTIFY iChanged)

public:
    int i() const {return 0;}
    void setI(int) {}

signals:
    void iChanged(int);
};

struct SignalWithArgumentInParent : ParentWithSignalWithArgument
{
    Q_OBJECT
    Q_PROPERTY(int otherI READ i WRITE setI NOTIFY iChanged)
};

struct MyStruct {};
struct MyStruct2 {};

struct SuperClass {};

// Try to avoid inserting for instance a comment with a quote between the following line and the Q_OBJECT
// That will make the test give a false positive.
const char* test_multiple_number_of_escapes =   "\\\"";
namespace MyNamespace
{
    class TestSuperClass : public QObject
    {
        Q_OBJECT
        public:
            inline TestSuperClass() {}
    };
}

namespace String
{
    typedef QString Type;
}

namespace Int
{
    typedef int Type;
}

typedef struct {
    int doNotConfuseMoc;
} OldStyleCStruct;

namespace {

    class GadgetInUnnamedNS
    {
        Q_GADGET
        Q_PROPERTY(int x READ x WRITE setX)
        Q_PROPERTY(int y READ y WRITE setY)
    public:
        explicit GadgetInUnnamedNS(int x, int y) : m_x(x), m_y(y) {}
        int x() const { return m_x; }
        int y() const { return m_y; }
        void setX(int x) { m_x = x; }
        void setY(int y) { m_y = y; }

    private:
        int m_x, m_y;
    };

    class ObjectInUnnamedNS : public QObject
    {
        Q_OBJECT
    public:
        explicit ObjectInUnnamedNS(QObject *parent = nullptr) : QObject(parent) {}
    };

}

class Sender : public QObject
{
    Q_OBJECT

public:
    void sendValue(const String::Type& value)
    {
        emit send(value);
    }
    void sendValue(const Int::Type& value)
    {
        emit send(value);
    }

    bool operator< ( const Sender & ) const { /* QTBUG-36834 */ return true;}
signals:
    void send(const String::Type&);
    void send(const Int::Type&);
};

class Receiver : public QObject
{
    Q_OBJECT
public:
    Receiver() : stringCallCount(0), intCallCount(0) {}

    int stringCallCount;
    int intCallCount;

public slots:
    void receive(const String::Type&) { stringCallCount++; }
    void receive(const Int::Type&)    { intCallCount++; }
};

#define MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES

#define DONT_CONFUSE_MOC(klass) klass
#define DONT_CONFUSE_MOC_EVEN_MORE(klass, dummy, dummy2) klass

Q_DECLARE_METATYPE(MyStruct)
Q_DECLARE_METATYPE(MyStruct*)

namespace myNS {
    struct Points
    {
        Points() : p1(0xBEEF), p2(0xBABE) { }
        int p1, p2;
    };
}

Q_DECLARE_METATYPE(myNS::Points)

class TestClassinfoWithEscapes: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("escaped", "\"bar\"")
    Q_CLASSINFO("\"escaped\"", "foo")
    Q_CLASSINFO("cpp c*/omment", "f/*oo")
    Q_CLASSINFO("endswith\\", "Or?\?/")
    Q_CLASSINFO("newline\n inside\n", "Or \r")
    Q_CLASSINFO("\xffz", "\0012")
public slots:
    void slotWithAReallyLongName(int)
    { }
};

#define CLASSINFO_VAARGS(...) Q_CLASSINFO("classinfo_va_args", #__VA_ARGS__)
class TestClassinfoFromVaArgs : public QObject
{
    Q_OBJECT
    CLASSINFO_VAARGS(a, b, c, d)
};
#undef CLASSINFO_VAARGS

struct ForwardDeclaredStruct;

struct StructQObject : public QObject
{
    Q_OBJECT
public:
    void foo(struct ForwardDeclaredStruct *);
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wunused-variable")
void StructQObject::foo(struct ForwardDeclaredStruct *)
{
    struct Inner {
        bool field;
    };

    Q_DECL_UNUSED_MEMBER struct Inner unusedVariable;
}
QT_WARNING_POP

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wignored-qualifiers")
QT_WARNING_DISABLE_GCC("-Wignored-qualifiers")

using ObjectCRef = const QObject &;

class TestClass : public MyNamespace::TestSuperClass, public DONT_CONFUSE_MOC(MyStruct),
                  public DONT_CONFUSE_MOC_EVEN_MORE(MyStruct2, dummy, ignored)
{
    Q_OBJECT
    Q_CLASSINFO("help", QT_TR_NOOP("Opening this will let you configure something"))
    Q_PROPERTY(short int shortIntProperty READ shortIntProperty)
    Q_PROPERTY(unsigned short int unsignedShortIntProperty READ unsignedShortIntProperty)
    Q_PROPERTY(signed short int signedShortIntProperty READ signedShortIntProperty)
    Q_PROPERTY(long int longIntProperty READ longIntProperty)
    Q_PROPERTY(unsigned long int unsignedLongIntProperty READ unsignedLongIntProperty)
    Q_PROPERTY(signed long int signedLongIntProperty READ signedLongIntProperty)
    Q_PROPERTY(long double longDoubleProperty READ longDoubleProperty)
    Q_PROPERTY(myNS::Points points READ points WRITE setPoints)

    Q_CLASSINFO("Multi"
                "line",
                ""
                "This is a "
                "multiline Q_CLASSINFO"
                "")

    // a really really long string that we have to cut into pieces in the generated stringdata
    // table, otherwise msvc craps out
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.kde.KCookieServer\" >\n"
"    <method name=\"findCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"cookies\" />\n"
"    </method>\n"
"    <method name=\"findDomains\" >\n"
"      <arg direction=\"out\" type=\"as\" name=\"domains\" />\n"
"    </method>\n"
"    <method name=\"findCookies\" >\n"
"      <arg direction=\"in\" type=\"ai\" name=\"fields\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"domain\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"fqdn\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"path\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\" />\n"
"      <arg direction=\"out\" type=\"as\" name=\"cookies\" />\n"
"      <annotation value=\"QList&lt;int>\" name=\"com.trolltech.QtDBus.QtTypeName.In0\" />\n"
"    </method>\n"
"    <method name=\"findDOMCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"cookies\" />\n"
"    </method>\n"
"    <method name=\"addCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"ay\" name=\"cookieHeader\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\"  />\n"
"    </method>\n"
"    <method name=\"deleteCookie\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"domain\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"fqdn\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"path\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\" />\n"
"    </method>\n"
"    <method name=\"deleteCookiesFromDomain\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"domain\" />\n"
"    </method>\n"
"    <method name=\"deleteSessionCookies\" >\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"    </method>\n"
"    <method name=\"deleteSessionCookiesFor\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"fqdn\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"    </method>\n"
"    <method name=\"deleteAllCookies\" />\n"
"    <method name=\"addDOMCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"ay\" name=\"cookieHeader\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"    </method>\n"
"    <method name=\"setDomainAdvice\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"advice\" />\n"
"    </method>\n"
"    <method name=\"getDomainAdvice\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"advice\" />\n"
"    </method>\n"
"    <method name=\"reloadPolicy\" />\n"
"    <method name=\"shutdown\" />\n"
"  </interface>\n"
        "")

public:
    inline TestClass() {}

private slots:
    inline void dummy1() MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES {}
    inline void dummy2() MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES const {}
    inline void dummy3() const MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES {}

    void slotWithULongLong(unsigned long long) {}
    void slotWithULongLongP(unsigned long long*) {}
    void slotWithULong(unsigned long) {}
    void slotWithLongLong(long long) {}
    void slotWithLong(long) {}

    void slotWithColonColonType(::Int::Type) {}

    TestClass &slotWithReferenceReturnType() { return *this; }

#if (0 && 1) || 1
    void expressionEvaluationShortcut1() {}
#endif
#if (1 || 0) && 0
#else
    void expressionEvaluationShortcut2() {}
#endif

public slots:
    void slotWithArray(const double[3]) {}
    void slotWithNamedArray(const double namedArray[3]) { Q_UNUSED(namedArray); }
    void slotWithMultiArray(const double[3][4]) {}

    short int shortIntProperty() { return 0; }
    unsigned short int unsignedShortIntProperty() { return 0; }
    signed short int signedShortIntProperty() { return 0; }
    long int longIntProperty() { return 0; }
    unsigned long int unsignedLongIntProperty() { return 0; }
    signed long int signedLongIntProperty() { return 0; }
    long double longDoubleProperty() { return 0.0; }

    myNS::Points points() { return m_points; }
    void setPoints(myNS::Points points) { m_points = points; }

signals:
    void signalWithArray(const double[3]);
    void signalWithNamedArray(const double namedArray[3]);
    void signalWithIterator(QList<QUrl>::iterator);
    void signalWithListPointer(QList<QUrl>*); //QTBUG-31002

private slots:
    // for tst_Moc::preprocessorConditionals
#if 0
    void invalidSlot() {}
#else
    void slotInElse() {}
#endif

#if 1
    void slotInIf() {}
#else
    void invalidSlot() {}
#endif

#if 0
    void invalidSlot() {}
#elif 0
#else
    void slotInLastElse() {}
#endif

#if 0
    void invalidSlot() {}
#elif 1
    void slotInElif() {}
#else
    void invalidSlot() {}
#endif

    friend class Receiver; // task #85783
signals:
    friend class Sender; // task #85783

#define MACRO_DEFINED

#if !(defined MACRO_UNDEF || defined MACRO_DEFINED) || 1
    void signalInIf1();
#else
    void doNotExist();
#endif
#if !(!defined MACRO_UNDEF || !defined MACRO_DEFINED) && 1
    void doNotExist();
#else
    void signalInIf2();
#endif
#if !(!defined (MACRO_DEFINED) || !defined (MACRO_UNDEF)) && 1
    void doNotExist();
#else
    void signalInIf3();
#endif

# //QTBUG-22717
 # /*  */
#

 # \

//
public slots:
    void const slotWithSillyConst() {}
    void slotTakingCRefViaTypedef(ObjectCRef o) { this->setObjectName(o.objectName()); }

public:
    Q_INVOKABLE void const slotWithSillyConst2() {}
    Q_INVOKABLE QObject& myInvokableReturningRef()
    { return *this; }
    Q_INVOKABLE const QObject& myInvokableReturningConstRef() const
    { return *this; }


    // that one however should be fine
public slots:
    void slotWithVoidStar(void *) {}

private:
     myNS::Points m_points;

#ifdef Q_MOC_RUN
    int xx = 11'11; // digit separator must not confuse moc (QTBUG-59351)
    int xx = 0b11'11; // digit separator in a binary literal must not confuse moc (QTBUG-75656)
#endif

private slots:
     inline virtual void blub1() {}
     virtual inline void blub2() {}
};

QT_WARNING_POP

// quick test to verify that moc handles the L suffix
// correctly in the preprocessor
#if 2000L < 1
#else
class PropertyTestClass : public QObject
{
    Q_OBJECT
public:

    enum TestEnum { One, Two, Three };

    Q_ENUM(TestEnum)
};
#endif

class PropertyUseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PropertyTestClass::TestEnum foo READ foo)
public:

    inline PropertyTestClass::TestEnum foo() const { return PropertyTestClass::One; }
};

class EnumSourceClass : public QObject
{
    Q_OBJECT

public:
    enum TestEnum { Value = 37 };

    Q_ENUM(TestEnum)
};

class EnumUserClass : public QObject
{
    Q_OBJECT

public:
    Q_ENUMS(EnumSourceClass::TestEnum)
};

class CtorTestClass : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE CtorTestClass(QObject *parent = nullptr);

    CtorTestClass(int foo);

    inline Q_INVOKABLE CtorTestClass(const QString &str)
        { m_str = str; }

    QString m_str;

protected:
    CtorTestClass(int foo, int bar, int baz);
private:
    CtorTestClass(float, float) {}
};

CtorTestClass::CtorTestClass(QObject *parent)
    : QObject(parent) {}

CtorTestClass::CtorTestClass(int, int, int) {}

class PrivatePropertyTest;

class tst_Moc : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool user1 READ user1 USER true )
    Q_PROPERTY(bool user2 READ user2 USER false)
    Q_PROPERTY(QString member1 MEMBER sMember)
    Q_PROPERTY(QString member2 MEMBER sMember READ member2)
    Q_PROPERTY(QString member3 MEMBER sMember WRITE setMember3)
    Q_PROPERTY(QString member4 MEMBER sMember NOTIFY member4Changed)
    Q_PROPERTY(QString member5 MEMBER sMember NOTIFY member5Changed)
    Q_PROPERTY(QString member6 MEMBER sConst CONSTANT)
    Q_PROPERTY(QString sub1 MEMBER (sub.m_string))
    Q_PROPERTY(QString sub2 READ (sub.string) WRITE (sub.setString))

public:
    inline tst_Moc() : sConst("const") {}

private slots:
    void initTestCase();

    void dontStripNamespaces();
    void oldStyleCasts();
    void warnOnExtraSignalSlotQualifiaction();
    void uLongLong();
    void inputFileNameWithDotsButNoExtension();
    void userProperties();
    void supportConstSignals();
    void task87883();
    void multilineComments();
    void classinfoWithEscapes();
    void classinfoFromVaArgs();
    void trNoopInClassInfo();
    void ppExpressionEvaluation();
    void arrayArguments();
    void preprocessorConditionals();
    void blackslashNewlines();
    void slotWithSillyConst();
    void slotTakingCRefViaTypedef();
    void testExtraData();
    void testExtraDataForEnum();
    void namespaceTypeProperty();
    void slotsWithVoidTemplate();
    void structQObject();
    void namespacedFlags();
    void warnOnMultipleInheritance();
    void ignoreOptionClashes();
    void forgottenQInterface();
    void os9Newline();
    void winNewline();
    void escapesInStringLiterals();
    void frameworkSearchPath();
    void cstyleEnums();
    void defineMacroViaCmdline();
    void defineMacroViaForcedInclude();
    void defineMacroViaForcedIncludeRelative();
    void environmentIncludePaths_data();
    void environmentIncludePaths();
    void specifyMetaTagsFromCmdline();
    void invokable();
    void singleFunctionKeywordSignalAndSlot();
    void templateGtGt();
    void qprivateslots();
    void qprivateproperties();
    void anonymousProperties();
    void warnOnPropertyWithoutREAD();
    void constructors();
    void typenameWithUnsigned();
    void warnOnVirtualSignal();
    void QTBUG5590_dummyProperty();
    void QTBUG12260_defaultTemplate();
    void notifyError();
    void QTBUG17635_invokableAndProperty();
    void revisions();
    void warnings_data();
    void warnings();
    void privateClass();
    void cxx11Enums_data();
    void cxx11Enums();
    void cxx11TrailingReturn();
    void returnRefs();
    void memberProperties_data();
    void memberProperties();
    void memberProperties2();
    void privateSignalConnection();
    void finalClasses_data();
    void finalClasses();
    void explicitOverrideControl_data();
    void explicitOverrideControl();
    void overloadedAddressOperator();
    void autoPropertyMetaTypeRegistration();
    void autoMethodArgumentMetaTypeRegistration();
    void autoSignalSpyMetaTypeRegistration();
    void parseDefines();
    void preprocessorOnly();
    void unterminatedFunctionMacro();
    void QTBUG32933_relatedObjectsDontIncludeItself();
    void writeEnumFromUnrelatedClass();
    void relatedMetaObjectsWithinNamespaces();
    void relatedMetaObjectsInGadget();
    void relatedMetaObjectsNameConflict_data();
    void relatedMetaObjectsNameConflict();
    void strignLiteralsInMacroExtension();
    void unnamedNamespaceObjectsAndGadgets();
    void veryLongStringData();
    void gadgetHierarchy();
    void optionsFileError_data();
    void optionsFileError();
    void testQNamespace();
    void testNestedQNamespace();
    void cxx17Namespaces();
    void cxxAttributes();
    void mocJsonOutput();
    void mocInclude();
    void requiredProperties();
    void qpropertyMembers();
    void observerMetaCall();
    void setQPRopertyBinding();
    void privateQPropertyShim();
    void readWriteThroughBindable();
    void invokableCtors();
    void virtualInlineTaggedSlot();
    void tokenStartingWithNumber();

signals:
    void sigWithUnsignedArg(unsigned foo);
    void sigWithSignedArg(signed foo);
    void sigWithConstSignedArg(const signed foo);
    void sigWithVolatileConstSignedArg(volatile const signed foo);
    void sigWithCustomType(const MyStruct);
    void constSignal1() const;
    void constSignal2(int arg) const;
    void member4Changed();
    void member5Changed(const QString &newVal);
    void sigWithDefaultArg(int i = 12);

private:
    bool user1() { return true; };
    bool user2() { return false; };
    template <class T> void revisions_T();
    QString member2() const { return sMember; }
    void setMember3( const QString &sVal ) { sMember = sVal; }

private:
    QString m_moc;
    QString m_sourceDirectory;
    QString qtIncludePath;
    class PrivateClass;
    QString sMember;
    const QString sConst;
    PrivatePropertyTest *pPPTest;

    struct {
        QString m_string;
        void setString(const QString &s) { m_string = s; }
        QString string() { return m_string; }
    } sub;

};

#define VERIFY_NO_ERRORS(proc) do { \
        auto &&p = proc; \
        const QByteArray stderr = p.readAllStandardError(); \
        QVERIFY2(stderr.isEmpty(), stderr.data()); \
        QCOMPARE(p.exitCode(), 0); \
    } while (false)


void tst_Moc::initTestCase()
{
    QString binpath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    QString qtpaths = QString("%1/qtpaths").arg(binpath);
    QString libexecPath = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath);
    m_moc = QString("%1/moc").arg(libexecPath);

    const QString testHeader = QFINDTESTDATA("backslash-newlines.h");
    QVERIFY(!testHeader.isEmpty());
    m_sourceDirectory = QFileInfo(testHeader).absolutePath();
#if defined(Q_OS_UNIX) && QT_CONFIG(process)
    QProcess proc;
    proc.start(qtpaths, QStringList() << "-query" << "QT_INSTALL_HEADERS");
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray output = proc.readAllStandardOutput();
    QVERIFY(!output.isEmpty());
    qtIncludePath = QString::fromLocal8Bit(output).trimmed();
    QFileInfo fi(qtIncludePath);
    QVERIFY(fi.exists());
    QVERIFY(fi.isDir());
#endif
}

void tst_Moc::dontStripNamespaces()
{
    Sender sender;
    Receiver receiver;

    connect(&sender, SIGNAL(send(const String::Type &)),
            &receiver, SLOT(receive(const String::Type &)));
    connect(&sender, SIGNAL(send(const Int::Type &)),
            &receiver, SLOT(receive(const Int::Type &)));

    sender.sendValue(String::Type("Hello"));
    QCOMPARE(receiver.stringCallCount, 1);
    QCOMPARE(receiver.intCallCount, 0);
    sender.sendValue(Int::Type(42));
    QCOMPARE(receiver.stringCallCount, 1);
    QCOMPARE(receiver.intCallCount, 1);
}

void tst_Moc::oldStyleCasts()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    proc.start(m_moc, QStringList(m_sourceDirectory + QStringLiteral("/oldstyle-casts.h")));
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-Wold-style-cast" << "-I" << "."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-fPIC" << "-std=c++1z" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::warnOnExtraSignalSlotQualifiaction()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/extraqualification.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":18:1: warning: Function declaration Test::badFunctionDeclaration contains extra qualification. Ignoring as signal or slot.\n") +
                header + QString(":21:1: warning: parsemaybe: Function declaration Test::anotherOne contains extra qualification. Ignoring as signal or slot.\n"));
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

void tst_Moc::uLongLong()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    int idx = mobj->indexOfSlot("slotWithULong(ulong)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithULongLong(unsigned long long)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithULongLong(qulonglong)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithULongLongP(qulonglong*)");
    QVERIFY(idx != -1);

    idx = mobj->indexOfSlot("slotWithLong(long)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithLongLong(long long)");
    QVERIFY(idx != -1);
}

void tst_Moc::inputFileNameWithDotsButNoExtension()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    proc.setWorkingDirectory(m_sourceDirectory + QStringLiteral("/task71021"));
    proc.start(m_moc, QStringList("../Header"));
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-I" << ".."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-fPIC" << "-std=c++1z" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::userProperties()
{
    const QMetaObject *mobj = metaObject();
    QMetaProperty property = mobj->property(mobj->indexOfProperty("user1"));
    QVERIFY(property.isValid());
    QVERIFY(property.isUser());

    property = mobj->property(mobj->indexOfProperty("user2"));
    QVERIFY(property.isValid());
    QVERIFY(!property.isUser());
}

void tst_Moc::supportConstSignals()
{
    QSignalSpy spy1(this, SIGNAL(constSignal1()));
    QVERIFY(spy1.isEmpty());
    emit constSignal1();
    QCOMPARE(spy1.size(), 1);

    QSignalSpy spy2(this, SIGNAL(constSignal2(int)));
    QVERIFY(spy2.isEmpty());
    emit constSignal2(42);
    QCOMPARE(spy2.size(), 1);
    QCOMPARE(spy2.at(0).at(0).toInt(), 42);
}

#include "task87883.h"

void tst_Moc::task87883()
{
    QVERIFY(Task87883::staticMetaObject.className());
}

#include "c-comments.h"

void tst_Moc::multilineComments()
{
    QVERIFY(IfdefedClass::staticMetaObject.className());
}

void tst_Moc::classinfoWithEscapes()
{
    const QMetaObject *mobj = &TestClassinfoWithEscapes::staticMetaObject;
    QCOMPARE(mobj->methodCount() - mobj->methodOffset(), 1);

    QCOMPARE(mobj->classInfoCount(), 6);
    QCOMPARE(mobj->classInfo(2).name(), "cpp c*/omment");
    QCOMPARE(mobj->classInfo(2).value(), "f/*oo");
    QCOMPARE(mobj->classInfo(3).name(), "endswith\\");
    QCOMPARE(mobj->classInfo(3).value(), "Or?\?/");
    QCOMPARE(mobj->classInfo(4).name(), "newline\n inside\n");
    QCOMPARE(mobj->classInfo(4).value(), "Or \r");
    QCOMPARE(mobj->classInfo(5).name(), "\xff" "z");
    QCOMPARE(mobj->classInfo(5).value(), "\001" "2");

    QMetaMethod mm = mobj->method(mobj->methodOffset());
    QCOMPARE(mm.methodSignature(), QByteArray("slotWithAReallyLongName(int)"));
}

void tst_Moc::classinfoFromVaArgs()
{
    const QMetaObject *mobj = &TestClassinfoFromVaArgs::staticMetaObject;

    QCOMPARE(mobj->classInfoCount(), 1);
    QCOMPARE(mobj->classInfo(0).name(), "classinfo_va_args");
    QCOMPARE(mobj->classInfo(0).value(), "a,b,c,d");
}

void tst_Moc::trNoopInClassInfo()
{
    TestClass t;
    const QMetaObject *mobj = t.metaObject();
    QVERIFY(mobj);
    QCOMPARE(mobj->classInfoCount(), 3);
    QCOMPARE(mobj->indexOfClassInfo("help"), 0);
    QCOMPARE(QString(mobj->classInfo(0).value()), QString("Opening this will let you configure something"));
}

void tst_Moc::ppExpressionEvaluation()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    int idx = mobj->indexOfSlot("expressionEvaluationShortcut1()");
    QVERIFY(idx != -1);

    idx = mobj->indexOfSlot("expressionEvaluationShortcut2()");
    QVERIFY(idx != -1);
}

void tst_Moc::arrayArguments()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("slotWithArray(const double[3])") != -1);
    QVERIFY(mobj->indexOfSlot("slotWithNamedArray(const double[3])") != -1);
    QVERIFY(mobj->indexOfSlot("slotWithMultiArray(const double[3][4])") != -1);
    QVERIFY(mobj->indexOfSignal("signalWithArray(const double[3])") != -1);
    QVERIFY(mobj->indexOfSignal("signalWithNamedArray(const double[3])") != -1);
}

void tst_Moc::preprocessorConditionals()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("slotInElse()") != -1);
    QVERIFY(mobj->indexOfSlot("slotInIf()") != -1);
    QVERIFY(mobj->indexOfSlot("slotInLastElse()") != -1);
    QVERIFY(mobj->indexOfSlot("slotInElif()") != -1);
    QVERIFY(mobj->indexOfSignal("signalInIf1()") != -1);
    QVERIFY(mobj->indexOfSignal("signalInIf2()") != -1);
    QVERIFY(mobj->indexOfSignal("signalInIf3()") != -1);
    QCOMPARE(mobj->indexOfSignal("doNotExist()"), -1);
}

void tst_Moc::blackslashNewlines()
{
    BackslashNewlines tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("works()") != -1);
    QCOMPARE(mobj->indexOfSlot("buggy()"), -1);
}

void tst_Moc::slotWithSillyConst()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("slotWithSillyConst()") != -1);
    QVERIFY(mobj->indexOfMethod("slotWithSillyConst2()") != -1);
    QVERIFY(mobj->indexOfSlot("slotWithVoidStar(void*)") != -1);
}

void tst_Moc::slotTakingCRefViaTypedef()
{
    TestClass tst;
    QObject obj;
    obj.setObjectName("works");
    QMetaObject::invokeMethod(&tst, "slotTakingCRefViaTypedef", Q_ARG(ObjectCRef, obj));
    QCOMPARE(obj.objectName(), "works");
}

void tst_Moc::testExtraData()
{
    const QMetaObject *mobj = &PropertyTestClass::staticMetaObject;
    QCOMPARE(mobj->enumeratorCount(), 1);
    QCOMPARE(QByteArray(mobj->enumerator(0).name()), QByteArray("TestEnum"));

    mobj = &PropertyUseClass::staticMetaObject;
    const int idx = mobj->indexOfProperty("foo");
    QVERIFY(idx != -1);
    const QMetaProperty prop = mobj->property(idx);
    QVERIFY(prop.isValid());
    QVERIFY(prop.isEnumType());
    const QMetaEnum en = prop.enumerator();
    QCOMPARE(QByteArray(en.name()), QByteArray("TestEnum"));
}

// QTBUG-20639 - Accept non-local enums for QML signal/slot parameters.
void tst_Moc::testExtraDataForEnum()
{
    const QMetaObject *mobjSource = &EnumSourceClass::staticMetaObject;
    QCOMPARE(mobjSource->enumeratorCount(), 1);
    QCOMPARE(QByteArray(mobjSource->enumerator(0).name()), QByteArray("TestEnum"));

    const QMetaObject *mobjUser = &EnumUserClass::staticMetaObject;
    QCOMPARE(mobjUser->enumeratorCount(), 0);

    const auto *objects = mobjUser->d.relatedMetaObjects;
    QVERIFY(objects);
    QCOMPARE(objects[0], mobjSource);
    QVERIFY(!objects[1]);
}

void tst_Moc::namespaceTypeProperty()
{
    qRegisterMetaType<myNS::Points>("myNS::Points");
    TestClass tst;
    QByteArray ba = QByteArray("points");
    QVariant v = tst.property(ba);
    QVERIFY(v.isValid());
    myNS::Points p = qvariant_cast<myNS::Points>(v);
    QCOMPARE(p.p1, 0xBEEF);
    QCOMPARE(p.p2, 0xBABE);
    p.p1 = 0xCAFE;
    p.p2 = 0x1EE7;
    QVERIFY(tst.setProperty(ba, QVariant::fromValue(p)));
    myNS::Points pp = qvariant_cast<myNS::Points>(tst.property(ba));
    QCOMPARE(p.p1, pp.p1);
    QCOMPARE(p.p2, pp.p2);
}

void tst_Moc::slotsWithVoidTemplate()
{
    SlotsWithVoidTemplateTest test;
    QVERIFY(QObject::connect(&test, SIGNAL(myVoidSignal(void)),
                             &test, SLOT(dummySlot(void))));
    QVERIFY(QObject::connect(&test, SIGNAL(mySignal(const TestTemplate<void> &)),
                             &test, SLOT(anotherSlot(const TestTemplate<void> &))));
    QVERIFY(QObject::connect(&test, SIGNAL(myVoidSignal2()),
                             &test, SLOT(dummySlot2())));
}

void tst_Moc::structQObject()
{
    StructQObject o;
    QCOMPARE(QByteArray(o.metaObject()->className()), QByteArray("StructQObject"));
}

#include "namespaced-flags.h"

Q_DECLARE_METATYPE(QList<Foo::Bar::Flags>);

void tst_Moc::namespacedFlags()
{
    Foo::Baz baz;
    Foo::Bar bar;

    bar.setFlags(Foo::Bar::Read | Foo::Bar::Write);
    QVERIFY(baz.flags() != bar.flags());

    const QVariant v = bar.property("flags");
    QVERIFY(v.isValid());
    QVERIFY(baz.setProperty("flags", v));
    QCOMPARE(baz.flags(), bar.flags());

    QList<Foo::Bar::Flags> l;
    l << baz.flags();
    QVariant v2 = baz.setProperty("flagsList", QVariant::fromValue(l));
    QCOMPARE(l, baz.flagsList());
    QCOMPARE(l, qvariant_cast<QList<Foo::Bar::Flags> >(baz.property("flagsList")));
}

void tst_Moc::warnOnMultipleInheritance()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    QStringList args;
    const QString header = m_sourceDirectory + QStringLiteral("/warn-on-multiple-qobject-subclasses.h");
    args << "-I" << qtIncludePath + "/QtGui" << header;
    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":18:1: warning: Class Bar inherits from two QObject subclasses QWindow and Foo. This is not supported!\n"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::ignoreOptionClashes()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    QStringList args;
    const QString header = m_sourceDirectory + QStringLiteral("/interface-from-include.h");
    const QString includeDir = m_sourceDirectory + "/Test.framework/Headers";
    // given --ignore-option-clashes, -pthread should be ignored, but the -I path should not be.
    args << "--ignore-option-clashes" << "-pthread" << "-I" << includeDir << "-fno-builtin" << header;
    proc.start(m_moc, args);
    bool finished = proc.waitForFinished();
    if (!finished)
        qWarning("waitForFinished failed. QProcess error: %d", (int)proc.error());
    QVERIFY(finished);
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();

    // If -pthread wasn't ignored, it was parsed as a prefix of "thread/", which breaks compilation.
    QStringList gccArgs;
    gccArgs << "-c" << "-x" << "c++" << "-I" << ".."
         << "-I" << qtIncludePath << "-I" << includeDir << "-o" << "/dev/null"
         << "-fPIC" << "-std=c++1z" <<  "-";
    proc.start("gcc", gccArgs);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::forgottenQInterface()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    QStringList args;
    const QString header = m_sourceDirectory + QStringLiteral("/forgotten-qinterface.h");
    args << "-I" << qtIncludePath + "/QtCore" << header;
    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":20:1: warning: Class Test implements the interface MyInterface but does not list it in Q_INTERFACES. qobject_cast to MyInterface will not work!\n"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::os9Newline()
{
#if !defined(SKIP_NEWLINE_TEST)
    const QMetaObject &mo = Os9Newlines::staticMetaObject;
    QVERIFY(mo.indexOfSlot("testSlot()") != -1);
    QFile f(m_sourceDirectory + QStringLiteral("/os9-newlines.h"));
    QVERIFY(f.open(QIODevice::ReadOnly)); // no QIODevice::Text!
    QByteArray data = f.readAll();
    f.close();
    QVERIFY(!data.contains('\n'));
    QVERIFY(data.contains('\r'));
#endif
}

void tst_Moc::winNewline()
{
#if !defined(SKIP_NEWLINE_TEST)
    const QMetaObject &mo = WinNewlines::staticMetaObject;
    QVERIFY(mo.indexOfSlot("testSlot()") != -1);
    QFile f(m_sourceDirectory + QStringLiteral("/win-newlines.h"));
    QVERIFY(f.open(QIODevice::ReadOnly)); // no QIODevice::Text!
    QByteArray data = f.readAll();
    f.close();
    for (int i = 0; i < data.size(); ++i) {
        if (data.at(i) == QLatin1Char('\r')) {
            QVERIFY(i < data.size() - 1);
            ++i;
            QCOMPARE(data.at(i), '\n');
        } else {
            QVERIFY(data.at(i) != '\n');
        }
    }
#endif
}

void tst_Moc::escapesInStringLiterals()
{
    const QMetaObject &mo = StringLiterals::staticMetaObject;
    QCOMPARE(mo.classInfoCount(), 3);

    int idx = mo.indexOfClassInfo("Test");
    QVERIFY(idx != -1);
    QMetaClassInfo info = mo.classInfo(idx);
    QCOMPARE(QByteArray(info.value()),
             QByteArray("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\x53"));

    QVERIFY(idx != -1);
    idx = mo.indexOfClassInfo("Test2");
    info = mo.classInfo(idx);
    QCOMPARE(QByteArray(info.value()),
             QByteArray("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\123"));

    QVERIFY(idx != -1);
    idx = mo.indexOfClassInfo("Test3");
    info = mo.classInfo(idx);
    QCOMPARE(QByteArray(info.value()),
             QByteArray("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nb"));
}

void tst_Moc::frameworkSearchPath()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && QT_CONFIG(process)
    QStringList args;
    args << "-F" << m_sourceDirectory + QStringLiteral("/.")
         << m_sourceDirectory + QStringLiteral("/interface-from-framework.h")
         ;

    QProcess proc;
    proc.start(m_moc, args);
    bool finished = proc.waitForFinished();
    if (!finished)
        qWarning("waitForFinished failed. QProcess error: %d", (int)proc.error());
    QVERIFY(finished);
    VERIFY_NO_ERRORS(proc);
#else
    QSKIP("Only tested/relevant on unixy platforms");
#endif
}

void tst_Moc::cstyleEnums()
{
    const QMetaObject &obj = CStyleEnums::staticMetaObject;
    QCOMPARE(obj.enumeratorCount(), 2);
    QMetaEnum metaEnum = obj.enumerator(0);
    QCOMPARE(metaEnum.name(), "Baz");
    QCOMPARE(metaEnum.keyCount(), 2);
    QCOMPARE(metaEnum.key(0), "Foo");
    QCOMPARE(metaEnum.key(1), "Bar");

    QMetaEnum metaEnum2 = obj.enumerator(1);
    QCOMPARE(metaEnum2.name(), "Baz2");
    QCOMPARE(metaEnum2.keyCount(), 2);
    QCOMPARE(metaEnum2.key(0), "Foo2");
    QCOMPARE(metaEnum2.key(1), "Bar2");
}

void tst_Moc::templateGtGt()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    proc.start(m_moc, QStringList(m_sourceDirectory + QStringLiteral("/template-gtgt.h")));
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

void tst_Moc::defineMacroViaCmdline()
{
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;

    QStringList args;
    args << "-DFOO";
    args << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

void tst_Moc::defineMacroViaForcedInclude()
{
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;

    QStringList args;
    args << "--include" << m_sourceDirectory + QLatin1String("/subdir/extradefines.h");
    args << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

void tst_Moc::defineMacroViaForcedIncludeRelative()
{
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;

    QStringList args;
    args << "--include" << QStringLiteral("extradefines.h") << "-I" + m_sourceDirectory + "/subdir";
    args << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on unix/gcc");
#endif
}


void tst_Moc::environmentIncludePaths_data()
{
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QTest::addColumn<QString>("cmdline");
    QTest::addColumn<QString>("varname");

    QTest::newRow("INCLUDE") << "--compiler-flavor=msvc" << "INCLUDE";
    QTest::newRow("CPATH1") << QString() << "CPATH";
    QTest::newRow("CPATH2") << "--compiler-flavor=unix" << "CPATH";
    QTest::newRow("CPLUS_INCLUDE_PATH1") << QString() << "CPLUS_INCLUDE_PATH";
    QTest::newRow("CPLUS_INCLUDE_PATH2") << "--compiler-flavor=unix" << "CPLUS_INCLUDE_PATH";
#endif
}

void tst_Moc::environmentIncludePaths()
{
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QFETCH(QString, cmdline);
    QFETCH(QString, varname);

    QStringList args;
    if (!cmdline.isEmpty())
        args << cmdline;
    args << "--include" << QStringLiteral("extradefines.h")
         << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("INCLUDE");
    env.remove("CPATH");
    env.remove("CPLUS_INCLUDE_PATH");
    env.insert(varname, m_sourceDirectory + "/subdir");

    QProcess proc;
    proc.setProcessEnvironment(env);
    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

// tst_Moc::specifyMetaTagsFromCmdline()
// plugin_metadata.h contains a plugin which we register here. Since we're not building this
// application as a plugin, we need to copy some of the initializer code found in qplugin.h:
extern "C" Q_DECL_EXPORT QObject *qt_plugin_instance();
extern "C" Q_DECL_EXPORT QPluginMetaData qt_plugin_query_metadata_v2();
class StaticPluginInstance{
public:
    StaticPluginInstance() {
        QStaticPlugin plugin(qt_plugin_instance, qt_plugin_query_metadata_v2);
        qRegisterStaticPluginFunction(plugin);
    }
};
static StaticPluginInstance staticInstance;

void tst_Moc::specifyMetaTagsFromCmdline() {
    const auto staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &plugin : staticPlugins) {
        const QString iid = plugin.metaData().value(QLatin1String("IID")).toString();
        if (iid == QLatin1String("test.meta.tags")) {
            const QJsonArray metaTagsUriList = plugin.metaData().value("uri").toArray();
            QCOMPARE(metaTagsUriList.size(), 2);

            // The following uri-s are set in the pro file using
            // -Muri=com.company.app -Muri=com.company.app.private
            QCOMPARE(metaTagsUriList[0].toString(), QLatin1String("com.company.app"));
            QCOMPARE(metaTagsUriList[1].toString(), QLatin1String("com.company.app.private"));
            return;
        }
    }
    QFAIL("Could not find plugin with IID 'test.meta.tags'");
}

void tst_Moc::invokable()
{
    const int fooIndex = 4;
    {
        const QMetaObject &mobj = InvokableBeforeReturnType::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 5);
        QCOMPARE(mobj.method(fooIndex).methodSignature(), QByteArray("foo()"));
    }

    {
        const QMetaObject &mobj = InvokableBeforeInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QCOMPARE(mobj.method(fooIndex).methodSignature(), QByteArray("foo()"));
        QCOMPARE(mobj.method(fooIndex + 1).methodSignature(), QByteArray("bar()"));
    }
}

void tst_Moc::singleFunctionKeywordSignalAndSlot()
{
    const int mySignalIndex = 4;
    {
        const QMetaObject &mobj = SingleFunctionKeywordBeforeReturnType::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QCOMPARE(mobj.method(mySignalIndex).methodSignature(), QByteArray("mySignal()"));
        QCOMPARE(mobj.method(mySignalIndex + 1).methodSignature(), QByteArray("mySlot()"));
    }

    {
        const QMetaObject &mobj = SingleFunctionKeywordBeforeInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QCOMPARE(mobj.method(mySignalIndex).methodSignature(), QByteArray("mySignal()"));
        QCOMPARE(mobj.method(mySignalIndex + 1).methodSignature(), QByteArray("mySlot()"));
    }

    {
        const QMetaObject &mobj = SingleFunctionKeywordAfterInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QCOMPARE(mobj.method(mySignalIndex).methodSignature(), QByteArray("mySignal()"));
        QCOMPARE(mobj.method(mySignalIndex + 1).methodSignature(), QByteArray("mySlot()"));
    }
}

#include "qprivateslots.h"

void tst_Moc::qprivateslots()
{
    TestQPrivateSlots tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("_q_privateslot()") != -1);
    QVERIFY(mobj->indexOfMethod("method1()") != -1); //tast204730
}

class PrivatePropertyTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo)
    Q_PRIVATE_PROPERTY(d, int bar READ bar WRITE setBar)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, int plop READ plop WRITE setPlop)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d_func(), int baz READ baz WRITE setBaz)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub MEMBER mBlub)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub2 MEMBER mBlub READ blub)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub3 MEMBER mBlub WRITE setBlub)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub4 MEMBER mBlub NOTIFY blub4Changed)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub5 MEMBER mBlub NOTIFY blub5Changed)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub6 MEMBER mConst CONSTANT)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, int zap READ zap WRITE setZap BINDABLE bindableZap)
    class MyDPointer {
    public:
        MyDPointer() : mConst("const"), mBar(0), mPlop(0) {}
        int bar() { return mBar ; }
        void setBar(int value) { mBar = value; }
        int plop() { return mPlop ; }
        void setPlop(int value) { mPlop = value; }
        int baz() { return mBaz ; }
        void setBaz(int value) { mBaz = value; }
        QString blub() const { return mBlub; }
        void setBlub(const QString &value) { mBlub = value; }
        int zap() { return mZap; }
        void setZap(int zap) { mZap = zap; }
        QBindable<int> bindableZap() { return QBindable<int>(&mZap); }
        QString mBlub;
        const QString mConst;
    private:
        int mBar;
        int mPlop;
        int mBaz;
        QProperty<int> mZap;
    };
public:
    PrivatePropertyTest(QObject *parent = nullptr) : QObject(parent), mFoo(0), d (new MyDPointer) {}
    int foo() { return mFoo ; }
    void setFoo(int value) { mFoo = value; }
    MyDPointer *d_func() {return d.data();}
    const MyDPointer *d_func() const {return d.data();}
signals:
    void blub4Changed();
    void blub5Changed(const QString &newBlub);
private:
    int mFoo;
    QScopedPointer<MyDPointer> d;
};


void tst_Moc::qprivateproperties()
{
    PrivatePropertyTest test;

    test.setProperty("foo", 1);
    QCOMPARE(test.property("foo"), QVariant::fromValue(1));

    test.setProperty("bar", 2);
    QCOMPARE(test.property("bar"), QVariant::fromValue(2));

    test.setProperty("plop", 3);
    QCOMPARE(test.property("plop"), QVariant::fromValue(3));

    test.setProperty("baz", 4);
    QCOMPARE(test.property("baz"), QVariant::fromValue(4));

    QMetaProperty zap = test.metaObject()->property(test.metaObject()->indexOfProperty("zap"));
    QVERIFY(zap.isValid());
    QVERIFY(zap.isBindable());
    auto zapBindable = zap.bindable(&test);
    QVERIFY(zapBindable.isBindable());
}


class AnonymousPropertyTest1 : public QObject
{
    Q_OBJECT
    QT_ANONYMOUS_PROPERTY(int READ foo WRITE setFoo)
public:
    int foo() { return mFoo ; }
    void setFoo(int value) { mFoo = value; }

private:
    int mFoo = 0;
};

class AnonymousPropertyTest2 : public QObject
{
    Q_OBJECT
    QT_ANONYMOUS_PRIVATE_PROPERTY(d, int READ bar WRITE setBar)

    class MyDPointer {
    public:
        int bar() { return mBar ; }
        void setBar(int value) { mBar = value; }
    private:
        int mBar = 0;
    };

public:
    AnonymousPropertyTest2(QObject *parent = nullptr) : QObject(parent), d (new MyDPointer) {}
    MyDPointer *d_func() {return d.data();}
    const MyDPointer *d_func() const {return d.data();}

private:
    QScopedPointer<MyDPointer> d;
};

void tst_Moc::anonymousProperties()
{
    AnonymousPropertyTest1 test1;

    test1.setProperty("", 17);
    QCOMPARE(test1.property(""), QVariant::fromValue(17));

    AnonymousPropertyTest2 test2;

    test2.setProperty("", 27);
    QCOMPARE(test2.property(""), QVariant::fromValue(27));
}

void tst_Moc::warnOnPropertyWithoutREAD()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/warn-on-property-without-read.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":11:1: warning: Property declaration foo has neither an associated QProperty<> member, nor a READ accessor function nor an associated MEMBER variable. The property will be invalid.\n"));
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

void tst_Moc::constructors()
{
    const QMetaObject *mo = &CtorTestClass::staticMetaObject;
    QCOMPARE(mo->constructorCount(), 3);
    {
        QMetaMethod mm = mo->constructor(0);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.methodSignature(), QByteArray("CtorTestClass(QObject*)"));
        QCOMPARE(mm.typeName(), "");
        QList<QByteArray> paramNames = mm.parameterNames();
        QCOMPARE(paramNames.size(), 1);
        QCOMPARE(paramNames.at(0), QByteArray("parent"));
        QList<QByteArray> paramTypes = mm.parameterTypes();
        QCOMPARE(paramTypes.size(), 1);
        QCOMPARE(paramTypes.at(0), QByteArray("QObject*"));
    }
    {
        QMetaMethod mm = mo->constructor(1);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.methodSignature(), QByteArray("CtorTestClass()"));
        QCOMPARE(mm.typeName(), "");
        QCOMPARE(mm.parameterNames().size(), 0);
        QCOMPARE(mm.parameterTypes().size(), 0);
    }
    {
        QMetaMethod mm = mo->constructor(2);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.methodSignature(), QByteArray("CtorTestClass(QString)"));
        QCOMPARE(mm.typeName(), "");
        QList<QByteArray> paramNames = mm.parameterNames();
        QCOMPARE(paramNames.size(), 1);
        QCOMPARE(paramNames.at(0), QByteArray("str"));
        QList<QByteArray> paramTypes = mm.parameterTypes();
        QCOMPARE(paramTypes.size(), 1);
        QCOMPARE(paramTypes.at(0), QByteArray("QString"));
    }

    QCOMPARE(mo->indexOfConstructor("CtorTestClass(QObject*)"), 0);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass()"), 1);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass(QString)"), 2);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass2(QObject*)"), -1);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass(float,float)"), -1);

    QScopedPointer<QObject> o1(mo->newInstance());
    QVERIFY(o1 != 0);
    QCOMPARE(o1->parent(), (QObject*)0);
    QVERIFY(qobject_cast<CtorTestClass*>(o1.data()) != 0);

    QObject *o2 = mo->newInstance(Q_ARG(QObject*, o1.data()));
    QVERIFY(o2 != 0);
    QCOMPARE(o2->parent(), o1.data());

    QString str = QString::fromLatin1("hello");
    QScopedPointer<QObject> o3(mo->newInstance(Q_ARG(QString, str)));
    QVERIFY(o3 != 0);
    QCOMPARE(qobject_cast<CtorTestClass*>(o3.data())->m_str, str);

    {
        //explicit constructor
        QObject *o = QObject::staticMetaObject.newInstance();
        QVERIFY(o);
        delete o;
    }
}

#include "task234909.h"

#include "task240368.h"

void tst_Moc::typenameWithUnsigned()
{
    TypenameWithUnsigned tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("a(uint)") != -1);
    QVERIFY(mobj->indexOfSlot("b(uint)") != -1);
    QVERIFY(mobj->indexOfSlot("c(uint*)") != -1);
    QVERIFY(mobj->indexOfSlot("d(uint*)") != -1);
    QVERIFY(mobj->indexOfSlot("e(uint&)") != -1);
    QVERIFY(mobj->indexOfSlot("f(uint&)") != -1);
    QVERIFY(mobj->indexOfSlot("g(unsigned1)") != -1);
    QVERIFY(mobj->indexOfSlot("h(unsigned1)") != -1);
    QVERIFY(mobj->indexOfSlot("i(uint,unsigned1)") != -1);
    QVERIFY(mobj->indexOfSlot("j(unsigned1,uint)") != -1);
    QVERIFY(mobj->indexOfSlot("k(unsignedQImage)") != -1);
    QVERIFY(mobj->indexOfSlot("l(unsignedQImage)") != -1);
}

void tst_Moc::warnOnVirtualSignal()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/pure-virtual-signals.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header + QString(":13:1: warning: Signals cannot be declared virtual\n") +
                         header + QString(":15:1: warning: Signals cannot be declared virtual\n"));
#else
    QSKIP("Only tested on unix/gcc");
#endif
}

class QTBUG5590_DummyObject: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dummy)
};

class QTBUG5590_PropertyObject: public QTBUG5590_DummyObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(int value2 READ value2 WRITE setValue2)

    public:
        QTBUG5590_PropertyObject() :  m_value(85), m_value2(40) { }
        int value() const { return m_value; }
        void setValue(int value) { m_value = value; }
        int value2() const { return m_value2; }
        void setValue2(int value) { m_value2 = value; }
    private:
        int m_value, m_value2;
};

void tst_Moc::QTBUG5590_dummyProperty()
{
    QTBUG5590_PropertyObject o;
    QCOMPARE(o.property("value").toInt(), 85);
    QCOMPARE(o.property("value2").toInt(), 40);
    o.setProperty("value", 32);
    QCOMPARE(o.value(), 32);
    o.setProperty("value2", 82);
    QCOMPARE(o.value2(), 82);
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wignored-qualifiers")
QT_WARNING_DISABLE_GCC("-Wignored-qualifiers")
class QTBUG7421_ReturnConstTemplate: public QObject
{ Q_OBJECT
public slots:
        const QList<int> returnConstTemplate1() { return QList<int>(); }
        QList<int> const returnConstTemplate2() { return QList<int>(); }
        const int returnConstInt() { return 0; }
        const QString returnConstString(const QString s) { return s; }
        QString const returnConstString2( QString const s) { return s; }
};
QT_WARNING_POP

struct science_constant {};
struct science_const {};
struct constconst {};
struct const_ {};

class QTBUG9354_constInName: public QObject
{ Q_OBJECT
public slots:
    void slotChooseScientificConst0(science_constant const &) {};
    void foo(science_const const &) {};
    void foo(constconst const &) {};
    void foo(constconst *) {};
    void foo(const_ *) {};
};


template<typename T1, typename T2>
class TestTemplate2
{
};

class QTBUG11647_constInTemplateParameter : public QObject
{ Q_OBJECT
public slots:
    void testSlot(TestTemplate2<const int, const short*>) {}
    void testSlot2(TestTemplate2<int, short const * const >) {}
    void testSlot3(TestTemplate2<TestTemplate2 < const int, const short* > const *,
                                TestTemplate2< TestTemplate2 < void, int > , unsigned char *> > ) {}

signals:
    void testSignal(TestTemplate2<const int, const short*>);
};

class QTBUG12260_defaultTemplate_Object : public QObject
{ Q_OBJECT
public slots:
    void doSomething(QHash<QString, QVariant> = QHash<QString, QVariant>() ) {}
    void doSomethingElse(QSharedPointer<QVarLengthArray<QString, (16 >> 2)> >
            = QSharedPointer<QVarLengthArray<QString, (16 >> 2)> >() ) {}

    void doAnotherThing(bool = (1 < 3), bool = (1 > 4)) {}

    void performSomething(QList<QList<QString>> = QList<QList<QString>>(8 < 1),
                          QHash<int, QList<QString>> = QHash<int, QList<QString>>()) {}
};


void tst_Moc::QTBUG12260_defaultTemplate()
{
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doSomething(QHash<QString,QVariant>)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doAnotherThing(bool,bool)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doSomethingElse(QSharedPointer<QVarLengthArray<QString,(16>>2)>>)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("performSomething(QList<QList<QString>>,QHash<int,QList<QString>>)") != -1);
}

void tst_Moc::notifyError()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/error-on-wrong-notify.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-I" << "."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-fPIC" << "-std=c++1z" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 1);
    const QString gccOutput = QString::fromLocal8Bit(proc.readAllStandardError());
    QVERIFY(gccOutput.contains(QLatin1String("error")));
    QVERIFY(gccOutput.contains(QLatin1String("fooChanged")));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

class QTBUG_17635_InvokableAndProperty : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(int numberOfEggs READ numberOfEggs)
    Q_PROPERTY(int numberOfChickens READ numberOfChickens)
    Q_INVOKABLE QString getEgg(int index) { Q_UNUSED(index); return QString::fromLatin1("Egg"); }
    Q_INVOKABLE QString getChicken(int index) { Q_UNUSED(index); return QString::fromLatin1("Chicken"); }
    int numberOfEggs() { return 2; }
    int numberOfChickens() { return 4; }
};

void tst_Moc::QTBUG17635_invokableAndProperty()
{
    //Moc used to fail parsing Q_INVOKABLE if they were dirrectly following a Q_PROPERTY;
    QTBUG_17635_InvokableAndProperty mc;
    QString val;
    QMetaObject::invokeMethod(&mc, "getEgg", Q_RETURN_ARG(QString, val), Q_ARG(int, 10));
    QCOMPARE(val, QString::fromLatin1("Egg"));
    QMetaObject::invokeMethod(&mc, "getChicken", Q_RETURN_ARG(QString, val), Q_ARG(int, 10));
    QCOMPARE(val, QString::fromLatin1("Chicken"));
    QVERIFY(mc.metaObject()->indexOfProperty("numberOfEggs") != -1);
    QVERIFY(mc.metaObject()->indexOfProperty("numberOfChickens") != -1);
}

// If changed, update VersionTestNotify below
class VersionTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ foo)
    Q_PROPERTY(int prop2 READ foo REVISION 2)
    Q_PROPERTY(int prop514 READ foo REVISION(5, 14))

public:
    int foo() const { return 0; }

    Q_INVOKABLE void method1() {}
    Q_INVOKABLE Q_REVISION(4) void method2() {}
    Q_INVOKABLE Q_REVISION(6, 0) void method60() {}

    enum TestEnum { One, Two };
    Q_ENUM(TestEnum);


public slots:
    void slot1() {}
    Q_REVISION(3) void slot2() {}
    Q_REVISION(6, 1) void slot61() {}

signals:
    void signal1();
    Q_REVISION(5) void signal2();
    Q_REVISION(6, 2) void signal62();

public slots Q_REVISION(6):
    void slot3() {}
    void slot4() {}

public slots Q_REVISION(5, 12):
    void slot512() {}

signals Q_REVISION(7):
    void signal3();
    void signal4();

signals Q_REVISION(5, 15):
    void signal515();
};

// If changed, update VersionTest above
class VersionTestNotify : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ foo NOTIFY fooChanged)
    Q_PROPERTY(int prop2 READ foo REVISION 2)
    Q_PROPERTY(int prop514 READ foo REVISION(5, 14))

public:
    int foo() const { return 0; }

    Q_INVOKABLE void method1() {}
    Q_INVOKABLE Q_REVISION(4) void method2() {}
    Q_INVOKABLE Q_REVISION(6, 0) void method60() {}

    enum TestEnum { One, Two };
    Q_ENUM(TestEnum);

public slots:
    void slot1() {}
    Q_REVISION(3) void slot2() {}
    Q_REVISION(6, 1) void slot61() {}

signals:
    void fooChanged();
    void signal1();
    Q_REVISION(5) void signal2();
    Q_REVISION(6, 2) void signal62();

public slots Q_REVISION(6):
    void slot3() {}
    void slot4() {}

public slots Q_REVISION(5, 12):
    void slot512() {}

signals Q_REVISION(7):
    void signal3();
    void signal4();

signals Q_REVISION(5, 15):
    void signal515();
};

template <class T>
void tst_Moc::revisions_T()
{
    int idx = T::staticMetaObject.indexOfProperty("prop1");
    QCOMPARE(T::staticMetaObject.property(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfProperty("prop2");
    QCOMPARE(T::staticMetaObject.property(idx).revision(),
             QTypeRevision::fromMinorVersion(2).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfProperty("prop514");
    QCOMPARE(T::staticMetaObject.property(idx).revision(),
             QTypeRevision::fromVersion(5, 14).toEncodedVersion<int>());

    idx = T::staticMetaObject.indexOfMethod("method1()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfMethod("method2()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(4).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfMethod("method60()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromVersion(6, 0).toEncodedVersion<int>());

    idx = T::staticMetaObject.indexOfSlot("slot1()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfSlot("slot2()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(3).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfSlot("slot61()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromVersion(6, 1).toEncodedVersion<int>());

    idx = T::staticMetaObject.indexOfSlot("slot3()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(6).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfSlot("slot4()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(6).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfSlot("slot512()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromVersion(5, 12).toEncodedVersion<int>());

    idx = T::staticMetaObject.indexOfSignal("signal1()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfSignal("signal2()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(5).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfSignal("signal62()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromVersion(6, 2).toEncodedVersion<int>());

    idx = T::staticMetaObject.indexOfSignal("signal3()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(7).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfSignal("signal4()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromMinorVersion(7).toEncodedVersion<int>());
    idx = T::staticMetaObject.indexOfSignal("signal515()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(),
             QTypeRevision::fromVersion(5, 15).toEncodedVersion<int>());

    idx = T::staticMetaObject.indexOfEnumerator("TestEnum");
    QCOMPARE(T::staticMetaObject.enumerator(idx).keyCount(), 2);
    QCOMPARE(T::staticMetaObject.enumerator(idx).key(0), "One");
}

// test using both class that has properties with and without NOTIFY signals
void tst_Moc::revisions()
{
    revisions_T<VersionTest>();
    revisions_T<VersionTestNotify>();
}

void tst_Moc::warnings_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<int>("exitCode");
    QTest::addColumn<QString>("expectedStdOut");
    QTest::addColumn<QString>("expectedStdErr");

    // empty input should result in "no relevant classes" note
    QTest::newRow("No relevant classes")
        << QByteArray(" ")
        << QStringList()
        << 0
        << QString()
        << QString("standard input: note: No relevant classes found. No output generated.");

    // passing "-nn" should suppress "no relevant classes" note
    QTest::newRow("-nn")
        << QByteArray(" ")
        << (QStringList() << "-nn")
        << 0
        << QString()
        << QString();

    // passing "-nw" should also suppress "no relevant classes" note
    QTest::newRow("-nw")
        << QByteArray(" ")
        << (QStringList() << "-nw")
        << 0
        << QString()
        << QString();

    // This should output a warning
    QTest::newRow("Invalid property warning")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << QStringList()
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1:1: warning: Property declaration x has neither an associated QProperty<> member, nor a READ accessor function nor an associated MEMBER variable. The property will be invalid.");

    // This should output a warning
    QTest::newRow("Duplicate property warning")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x READ x) Q_PROPERTY(int x READ y) };")
        << QStringList()
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1:1: warning: The property 'x' is defined multiple times in class X.");

    // Passing "-nn" should NOT suppress the warning
    QTest::newRow("Invalid property warning with -nn")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << (QStringList() << "-nn")
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1:1: warning: Property declaration x has neither an associated QProperty<> member, nor a READ accessor function nor an associated MEMBER variable. The property will be invalid.");

    // Passing "-nw" should suppress the warning
    QTest::newRow("Invalid property warning with -nw")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << (QStringList() << "-nw")
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString();

    // This should output an error
    QTest::newRow("Does not inherit QObject")
        << QByteArray("class X { Q_OBJECT };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:1:1: error: Class contains Q_OBJECT macro but does not inherit from QObject");

    // "-nn" should not suppress the error
    QTest::newRow("Does not inherit QObject with -nn")
        << QByteArray("class X { Q_OBJECT };")
        << (QStringList() << "-nn")
        << 1
        << QString()
        << QString("standard input:1:1: error: Class contains Q_OBJECT macro but does not inherit from QObject");

    // "-nw" should not suppress the error
    QTest::newRow("Does not inherit QObject with -nw")
        << QByteArray("class X { Q_OBJECT };")
        << (QStringList() << "-nw")
        << 1
        << QString()
        << QString("standard input:1:1: error: Class contains Q_OBJECT macro but does not inherit from QObject");

    QTest::newRow("Warning on invalid macro")
        << QByteArray("#define Foo(a, b)\n class X : public QObject { Q_OBJECT  }; \n Foo(a) \n Foo(a,b,c) \n")
        << QStringList()
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString();

    QTest::newRow("Class declaration lacks Q_OBJECT macro.")
        << QByteArray("class X : public QObject \n { \n public slots: \n void foo() {} \n };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5:1: error: Class declaration lacks Q_OBJECT macro.");

    QTest::newRow("Namespace declaration lacks Q_NAMESPACE macro.")
        << QByteArray("namespace X {\nQ_CLASSINFO(\"key\",\"value\")\nenum class MyEnum {Key1 = 1}\nQ_ENUMS(MyEnum)\n}\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:1:1: error: Namespace declaration lacks Q_NAMESPACE macro.");

    QTest::newRow("Wrong Q_ENUM context.")
        << QByteArray("namespace X {\nQ_NAMESPACE\n\nenum class MyEnum {Key1 = 1}\nQ_ENUM(MyEnum)\n}\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5:1: error: Q_ENUM can't be used in a Q_NAMESPACE, use Q_ENUM_NS instead");

    QTest::newRow("Wrong Q_FLAG context.")
        << QByteArray("namespace X {\nQ_NAMESPACE\n\nenum class MyEnum {Key1 = 1}\nQ_FLAG(MyEnum)\n}\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5:1: error: Q_FLAG can't be used in a Q_NAMESPACE, use Q_FLAG_NS instead");

    QTest::newRow("Wrong Q_ENUM_NS context.")
        << QByteArray("class X {\nQ_GADGET\n\nenum class MyEnum {Key1 = 1}\nQ_ENUM_NS(MyEnum)\n};\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5:1: error: Q_ENUM_NS can't be used in a Q_OBJECT/Q_GADGET, use Q_ENUM instead");

    QTest::newRow("Wrong Q_FLAG_NS context.")
        << QByteArray("class X {\nQ_GADGET\n\nenum class MyEnum {Key1 = 1}\nQ_FLAG_NS(MyEnum)\n};\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5:1: error: Q_FLAG_NS can't be used in a Q_OBJECT/Q_GADGET, use Q_FLAG instead");

    QTest::newRow("Invalid macro definition")
        << QByteArray("#define Foo(a, b, c) a b c #a #b #c a##b##c #d\n Foo(45, 42, 39);")
        << QStringList()
        << 1
        << QString("IGNORE_ALL_STDOUT")
        << QString(":2:1: error: '#' is not followed by a macro parameter");

    QTest::newRow("QTBUG-46210: crash on invalid macro invocation")
        << QByteArray("#define Foo(a, b, c) a b c #a #b #c a##b##c\n Foo(45);")
        << QStringList()
        << 1
        << QString("IGNORE_ALL_STDOUT")
        << QString(":2:1: error: Macro invoked with too few parameters for a use of '#'");

    QTest::newRow("QTBUG-54609: crash on invalid input")
        << QByteArray::fromBase64("EAkJCQkJbGFzcyBjbGFzcyBiYWkcV2kgTUEKcGYjZGVmaW5lIE1BKFEs/4D/FoQ=")
        << QStringList()
        << 1
        << QString("IGNORE_ALL_STDOUT")
        << QString(": error: Unexpected character in macro argument list.");

    QTest::newRow("Missing header warning")
        << QByteArray("class X : public QObject { Q_OBJECT };")
        << (QStringList() << QStringLiteral("--include") << QStringLiteral("doesnotexist.h"))
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QStringLiteral("Warning: Failed to resolve include \"doesnotexist.h\" for moc file <standard input>");

    QTest::newRow("QTBUG-54815: Crash on invalid input")
        << QByteArray("class M{(})F<{}d000000000000000#0")
        << QStringList()
        << 0
        << QString()
        << QString("standard input:1:1: note: No relevant classes found. No output generated.");

    QTest::newRow("Q_PLUGIN_METADATA: invalid file")
        << QByteArray("class X { \n Q_PLUGIN_METADATA(FILE \"does.not.exists\") \n };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:2:1: error: Plugin Metadata file \"does.not.exists\" does not exist. Declaration will be ignored");

    QTest::newRow("Auto-declared, missing trailing return")
        << QByteArray("class X { \n public slots: \n auto fun() { return 1; } };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:3:1: error: Function declared with auto as return type but missing trailing return type. Return type deduction is not supported.");

    QTest::newRow("Auto-declared, volatile auto as trailing return type")
        << QByteArray("class X { \n public slots: \n auto fun() -> volatile auto { return 1; } };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:3:1: error: Function declared with auto as return type but missing trailing return type. Return type deduction is not supported.");

    // We don't currently support the decltype keyword, so it's not the same error as above.
    // The test is just here to make sure this keeps generating an error until return type deduction
    // is supported.
    QTest::newRow("Auto-declared, decltype in trailing return type")
        << QByteArray("class X { \n public slots: \n auto fun() -> decltype(0+1) { return 1; } };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:3:1: error: Parse error at \"decltype\"");

    QTest::newRow("QTBUG-36367: report correct error location")
        << "class X { \n Q_PROPERTY(Foo* foo NONSENSE foo) \n };"_ba
        << QStringList()
        << 1
        << QString()
        << u"standard input:2:1: error: Parse error at \"NONSENSE\""_s;

#ifdef Q_OS_UNIX  // Limit to Unix because the error message is platform-dependent
    QTest::newRow("Q_PLUGIN_METADATA: unreadable file")
        << QByteArray("class X { \n Q_PLUGIN_METADATA(FILE \".\") \n };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:2:1: error: Plugin Metadata file \".\" could not be opened: file to open is a directory");
#endif
}

void tst_Moc::warnings()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
    QFETCH(QByteArray, input);
    QFETCH(QStringList, args);
    QFETCH(int, exitCode);
    QFETCH(QString, expectedStdOut);
    QFETCH(QString, expectedStdErr);

#ifdef Q_CC_MSVC
    // moc compiled with MSVC uses a different output format to match MSVC compiler style
    QRegularExpression lineNumberRe(":(-?\\d+):(\\d+)", QRegularExpression::InvertedGreedinessOption);
    expectedStdErr.replace(lineNumberRe, "(\\1:\\2)");
#endif

#if QT_CONFIG(process)
    QProcess proc;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_MESSAGE_PATTERN", "no qDebug or qWarning please");
    proc.setProcessEnvironment(env);

    proc.start(m_moc, args);
    QVERIFY(proc.waitForStarted());

    QCOMPARE(proc.write(input), qint64(input.size()));

    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());

    QCOMPARE(proc.exitCode(), exitCode);
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

    // magic value "IGNORE_ALL_STDOUT" ignores stdout
    if (expectedStdOut != "IGNORE_ALL_STDOUT")
        QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed(), expectedStdOut);
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()).trimmed().remove('\r'), expectedStdErr);
#else
    QSKIP("Only tested if QProcess is available");
#endif
}

class tst_Moc::PrivateClass : public QObject {
    Q_PROPERTY(int someProperty READ someSlot WRITE someSlot2)
Q_OBJECT
Q_SIGNALS:
    void someSignal();
public Q_SLOTS:
    int someSlot() { return 1; }
    void someSlot2(int) {}
public:
    Q_INVOKABLE PrivateClass()  {}
};

void tst_Moc::privateClass()
{
    QCOMPARE(PrivateClass::staticMetaObject.indexOfConstructor("PrivateClass()"), 0);
    QVERIFY(PrivateClass::staticMetaObject.indexOfSignal("someSignal()") > 0);
}

void tst_Moc::cxx11Enums_data()
{
    QTest::addColumn<const QMetaObject *>("meta");
    QTest::addColumn<QByteArray>("typeName");
    QTest::addColumn<QByteArray>("enumName");
    QTest::addColumn<char>("prefix");
    QTest::addColumn<bool>("isScoped");
    QTest::addColumn<bool>("isTyped");

    const QMetaObject *meta1 = &CXX11Enums::staticMetaObject;
    const QMetaObject *meta2 = &CXX11Enums2::staticMetaObject;
    const QMetaObject *meta3 = &CXX11Enums3::staticMetaObject;

    QTest::newRow("EnumClass") << meta1 << QByteArray("EnumClass") << QByteArray("EnumClass") << 'A' << true << false;
    QTest::newRow("EnumClass 2") << meta2 << QByteArray("EnumClass") << QByteArray("EnumClass") << 'A' << true << false;
    QTest::newRow("EnumClass 3") << meta3 << QByteArray("EnumClass") << QByteArray("EnumClass") << 'A' << true << false;
    QTest::newRow("TypedEnum") << meta1 << QByteArray("TypedEnum") << QByteArray("TypedEnum") << 'B' << false << true;
    QTest::newRow("TypedEnum 2") << meta2 << QByteArray("TypedEnum") << QByteArray("TypedEnum") << 'B' << false << true;
    QTest::newRow("TypedEnum 3") << meta3 << QByteArray("TypedEnum") << QByteArray("TypedEnum") << 'B' << false << true;
    QTest::newRow("TypedEnumClass") << meta1 << QByteArray("TypedEnumClass") << QByteArray("TypedEnumClass") << 'C' << true << true;
    QTest::newRow("TypedEnumClass 2") << meta2 << QByteArray("TypedEnumClass") << QByteArray("TypedEnumClass") << 'C' << true << true;
    QTest::newRow("TypedEnumClass 3") << meta3 << QByteArray("TypedEnumClass") << QByteArray("TypedEnumClass") << 'C' << true << true;
    QTest::newRow("NormalEnum") << meta1 << QByteArray("NormalEnum") << QByteArray("NormalEnum") << 'D' << false << false;
    QTest::newRow("NormalEnum 2") << meta2 << QByteArray("NormalEnum") << QByteArray("NormalEnum") << 'D' << false << false;
    QTest::newRow("NormalEnum 3") << meta3 << QByteArray("NormalEnum") << QByteArray("NormalEnum") << 'D' << false << false;
    QTest::newRow("ClassFlags") << meta1 << QByteArray("ClassFlags") << QByteArray("ClassFlag") << 'F' << true << false;
    QTest::newRow("ClassFlags 2") << meta2 << QByteArray("ClassFlags") << QByteArray("ClassFlag") << 'F' << true << false;
    QTest::newRow("EnumStruct") << meta1 << QByteArray("EnumStruct") << QByteArray("EnumStruct") << 'G' << true << false;
    QTest::newRow("TypedEnumStruct") << meta1 << QByteArray("TypedEnumStruct") << QByteArray("TypedEnumStruct") << 'H' << true << true;
    QTest::newRow("StructFlags") << meta1 << QByteArray("StructFlags") << QByteArray("StructFlag") << 'I' << true << false;
}

void tst_Moc::cxx11Enums()
{
    QFETCH(const QMetaObject *,meta);
    QCOMPARE(meta->enumeratorOffset(), 0);

    QFETCH(QByteArray, typeName);
    QFETCH(QByteArray, enumName);
    QFETCH(char, prefix);
    QFETCH(bool, isScoped);
    QFETCH(bool, isTyped);

    int idx = meta->indexOfEnumerator(typeName);
    QVERIFY(idx != -1);
    QCOMPARE(meta->indexOfEnumerator(enumName), idx);

    const QMetaEnum metaEnum = meta->enumerator(idx);
    QCOMPARE(metaEnum.enclosingMetaObject(), meta);
    QCOMPARE(metaEnum.isValid(), true);
    QCOMPARE(metaEnum.keyCount(), 4);
    QCOMPARE(metaEnum.name(), typeName.constData());
    QCOMPARE(metaEnum.enumName(), enumName.constData());

    const QMetaType metaType = metaEnum.metaType();
    const bool isUnsigned = metaType.flags() & QMetaType::IsUnsignedEnumeration;
    if (isTyped) {
        QCOMPARE(size_t(metaType.sizeOf()), sizeof(char));
        QCOMPARE(isUnsigned, !std::is_signed_v<char>);
    } else if (isScoped) {
        QCOMPARE(size_t(metaType.sizeOf()), sizeof(int));
        QCOMPARE(isUnsigned, !std::is_signed_v<int>);
    } else {
        // underlying type is implementation defined
    }

    bool isFlag = metaEnum.isFlag();
    for (int i = 0; i < 4; i++) {
        QByteArray v = prefix + QByteArray::number(i);
        const int value = isFlag ? (1 << i) : i;
        QCOMPARE(metaEnum.keyToValue(v), value);
        QCOMPARE(metaEnum.valueToKey(value), v.constData());
    }
    QCOMPARE(metaEnum.isScoped(), isScoped);
}

void tst_Moc::cxx11TrailingReturn()
{
    CXX11TrailingReturn retClass;
    const QMetaObject *mobj = retClass.metaObject();
    QVERIFY(mobj->indexOfSlot("fun()") != -1);
    QVERIFY(mobj->indexOfSlot("arguments(int,char)") != -1);
    QVERIFY(mobj->indexOfSlot("inlineFunc(int)") != -1);
    QVERIFY(mobj->indexOfSlot("constRefReturn()") != -1);
    QVERIFY(mobj->indexOfSlot("constConstRefReturn()") != -1);
    QVERIFY(mobj->indexOfSignal("trailingSignalReturn(int)") != -1);
}

void tst_Moc::returnRefs()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfMethod("myInvokableReturningRef()") != -1);
    QVERIFY(mobj->indexOfMethod("myInvokableReturningConstRef()") != -1);
    // Those two functions are copied from the qscriptextqobject test in qtscript
    // they used to cause miscompilation of the moc generated file.
}

void tst_Moc::memberProperties_data()
{
    QTest::addColumn<int>("object");
    QTest::addColumn<QString>("property");
    QTest::addColumn<QString>("signal");
    QTest::addColumn<QString>("writeValue");
    QTest::addColumn<bool>("expectedWriteResult");
    QTest::addColumn<QString>("expectedReadResult");

    pPPTest = new PrivatePropertyTest( this );

    QTest::newRow("MEMBER property")
            << 0 << "member1" << "" << "abc" << true << "abc";
    QTest::newRow("MEMBER property with READ function")
            << 0 << "member2" << "" << "def" << true << "def";
    QTest::newRow("MEMBER property with WRITE function")
            << 0 << "member3" << "" << "ghi" << true << "ghi";
    QTest::newRow("MEMBER property with NOTIFY")
            << 0 << "member4" << "member4Changed()" << "lmn" << true << "lmn";
    QTest::newRow("MEMBER property with NOTIFY(value)")
            << 0 << "member5" << "member5Changed(const QString&)" << "opq" << true << "opq";
    QTest::newRow("MEMBER property with CONSTANT")
            << 0 << "member6" << "" << "test" << false << "const";
    QTest::newRow("private MEMBER property")
            << 1 << "blub" << "" << "abc" << true << "abc";
    QTest::newRow("private MEMBER property with READ function")
            << 1 << "blub2" << "" << "def" << true << "def";
    QTest::newRow("private MEMBER property with WRITE function")
            << 1 << "blub3" << "" << "ghi" << true << "ghi";
    QTest::newRow("private MEMBER property with NOTIFY")
            << 1 << "blub4" << "blub4Changed()" << "jkl" << true << "jkl";
    QTest::newRow("private MEMBER property with NOTIFY(value)")
            << 1 << "blub5" << "blub5Changed(const QString&)" << "mno" << true << "mno";
    QTest::newRow("private MEMBER property with CONSTANT")
            << 1 << "blub6" << "" << "test" << false << "const";
    QTest::newRow("sub1")
            << 0 << "sub1" << "" << "helloSub1" << true << "helloSub1";
    QTest::newRow("sub2")
            << 0 << "sub2" << "" << "helloSub2" << true << "helloSub2";
}

void tst_Moc::memberProperties()
{
    QFETCH(int, object);
    QFETCH(QString, property);
    QFETCH(QString, signal);
    QFETCH(QString, writeValue);
    QFETCH(bool, expectedWriteResult);
    QFETCH(QString, expectedReadResult);

    QObject *pObj = (object == 0) ? this : static_cast<QObject*>(pPPTest);

    QString sSignalDeclaration;
    if (!signal.isEmpty())
        sSignalDeclaration = QString(SIGNAL(%1)).arg(signal);
    else
        QTest::ignoreMessage(QtWarningMsg, "QSignalSpy: Not a valid signal, use the SIGNAL macro");
    QSignalSpy notifySpy(pObj, sSignalDeclaration.toLatin1().constData());

    int index = pObj->metaObject()->indexOfProperty(property.toLatin1().constData());
    QVERIFY(index != -1);
    QMetaProperty prop = pObj->metaObject()->property(index);

    QCOMPARE(prop.write(pObj, writeValue), expectedWriteResult);

    QVariant readValue = prop.read(pObj);
    QCOMPARE(readValue.toString(), expectedReadResult);

    if (!signal.isEmpty())
    {
        QCOMPARE(notifySpy.size(), 1);
        if (prop.notifySignal().parameterNames().size() > 0) {
            QList<QVariant> arguments = notifySpy.takeFirst();
            QCOMPARE(arguments.size(), 1);
            QCOMPARE(arguments.at(0).toString(), expectedReadResult);
        }

        notifySpy.clear();
        // a second write with the same value should not cause the signal to be emitted again
        QCOMPARE(prop.write(pObj, writeValue), expectedWriteResult);
        QCOMPARE(notifySpy.size(), 0);
    }
}

//this used to fail to compile
class ClassWithOneMember  : public QObject {
    Q_PROPERTY(int member MEMBER member)
    Q_OBJECT
public:
    int member;
};

void tst_Moc::memberProperties2()
{
    ClassWithOneMember o;
    o.member = 442;
    QCOMPARE(o.property("member").toInt(), 442);
    QVERIFY(o.setProperty("member", 6666));
    QCOMPARE(o.member, 6666);
}

class SignalConnectionTester : public QObject
{
    Q_OBJECT
public:
    SignalConnectionTester(QObject *parent = nullptr)
      : QObject(parent), testPassed(false)
    {

    }

public Q_SLOTS:
    void testSlot()
    {
      testPassed = true;
    }
    void testSlotWith1Arg(int i)
    {
      testPassed = i == 42;
    }
    void testSlotWith2Args(int i, const QString &s)
    {
      testPassed = i == 42 && s == "Hello";
    }

public:
    bool testPassed;
};

class ClassWithPrivateSignals : public QObject
{
    Q_OBJECT
public:
    ClassWithPrivateSignals(QObject *parent = nullptr)
      : QObject(parent)
    {

    }

    void emitPrivateSignals()
    {
        emit privateSignal1(QPrivateSignal());
        emit privateSignalWith1Arg(42, QPrivateSignal());
        emit privateSignalWith2Args(42, "Hello", QPrivateSignal());

        emit privateOverloadedSignal(QPrivateSignal());
        emit privateOverloadedSignal(42, QPrivateSignal());

        emit overloadedMaybePrivate();
        emit overloadedMaybePrivate(42, QPrivateSignal());
    }

Q_SIGNALS:
    void privateSignal1(QPrivateSignal);
    void privateSignalWith1Arg(int arg1, QPrivateSignal);
    void privateSignalWith2Args(int arg1, const QString &arg2, QPrivateSignal);

    void privateOverloadedSignal(QPrivateSignal);
    void privateOverloadedSignal(int, QPrivateSignal);

    void overloadedMaybePrivate();
    void overloadedMaybePrivate(int, QPrivateSignal);

};

class SubClassFromPrivateSignals : public ClassWithPrivateSignals
{
    Q_OBJECT
public:
    SubClassFromPrivateSignals(QObject *parent = nullptr)
      : ClassWithPrivateSignals(parent)
    {

    }

    void emitProtectedSignals()
    {
      // Compile test: All of this intentionally does not compile:
//         emit privateSignal1();
//         emit privateSignalWith1Arg(42);
//         emit privateSignalWith2Args(42, "Hello");
//
//         emit privateSignal1(QPrivateSignal());
//         emit privateSignalWith1Arg(42, QPrivateSignal());
//         emit privateSignalWith2Args(42, "Hello", QPrivateSignal());
//
//         emit privateSignal1(ClassWithPrivateSignals::QPrivateSignal());
//         emit privateSignalWith1Arg(42, ClassWithPrivateSignals::QPrivateSignal());
//         emit privateSignalWith2Args(42, "Hello", ClassWithPrivateSignals::QPrivateSignal());

//         emit privateOverloadedSignal();
//         emit privateOverloadedSignal(42);

//         emit overloadedMaybePrivate();
//         emit overloadedMaybePrivate(42);


    }
};

void tst_Moc::privateSignalConnection()
{
    // Function pointer connects. Matching signals and slots
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignal1, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&classWithPrivateSignals, "privateSignal1");
        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignal1, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&subClassFromPrivateSignals, "privateSignal1");
        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&classWithPrivateSignals, "privateSignalWith1Arg", Q_ARG(int, 42));
        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&subClassFromPrivateSignals, "privateSignalWith1Arg", Q_ARG(int, 42));
        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith2Args, &tester, &SignalConnectionTester::testSlotWith2Args);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&classWithPrivateSignals, "privateSignalWith2Args", Q_ARG(int, 42), Q_ARG(QString, "Hello"));
        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith2Args, &tester, &SignalConnectionTester::testSlotWith2Args);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&subClassFromPrivateSignals, "privateSignalWith2Args", Q_ARG(int, 42), Q_ARG(QString, "Hello"));
        QVERIFY(tester.testPassed);
    }


    // String based connects. Matching signals and slots
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignal1()), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignal1()), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlotWith1Arg(int)));

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlotWith1Arg(int)));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith2Args(int,QString)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith2Args(int,QString)));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }

    // Function pointer connects. Decayed slot arguments
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }

    // String based connects. Decayed slot arguments
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith1Arg(int)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith1Arg(int)));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlot()));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }

    // Overloaded private signals
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateOverloadedSignal()), &tester, SLOT(testSlot()));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateOverloadedSignal(int)), &tester, SLOT(testSlotWith1Arg(int)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    // We can't use function pointer connections to private signals which are overloaded because we would have to cast in this case to:
    //   static_cast<void (ClassWithPrivateSignals::*)(int, ClassWithPrivateSignals::QPrivateSignal)>(&ClassWithPrivateSignals::privateOverloadedSignal)
    // Which doesn't work as ClassWithPrivateSignals::QPrivateSignal is private.

    // Overload with either private or not private signals
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(overloadedMaybePrivate()), &tester, SLOT(testSlot()));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateOverloadedSignal(int)), &tester, SLOT(testSlotWith1Arg(int)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals,
                         static_cast<void (ClassWithPrivateSignals::*)()>(&ClassWithPrivateSignals::overloadedMaybePrivate),
                         &tester, &SignalConnectionTester::testSlot);
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    // We can't use function pointer connections to private signals which are overloaded because we would have to cast in this case to:
    //   static_cast<void (ClassWithPrivateSignals::*)(int, ClassWithPrivateSignals::QPrivateSignal)>(&ClassWithPrivateSignals::overloadedMaybePrivate)
    // Which doesn't work as ClassWithPrivateSignals::QPrivateSignal is private.

    // Connecting from one private signal to another
    {
        ClassWithPrivateSignals classWithPrivateSignals1;
        ClassWithPrivateSignals classWithPrivateSignals2;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals1, &ClassWithPrivateSignals::privateSignal1,
                         &classWithPrivateSignals2, &ClassWithPrivateSignals::privateSignal1);
        QObject::connect(&classWithPrivateSignals2, &ClassWithPrivateSignals::privateSignal1,
                         &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);
        classWithPrivateSignals1.emitPrivateSignals();
        QVERIFY(tester.testPassed);
    }
}

void tst_Moc::finalClasses_data()
{
    QTest::addColumn<QString>("className");
    QTest::addColumn<QString>("expected");

    QTest::newRow("FinalTestClassQt") << FinalTestClassQt::staticMetaObject.className() << "FinalTestClassQt";
    QTest::newRow("ExportedFinalTestClassQt") << ExportedFinalTestClassQt::staticMetaObject.className() << "ExportedFinalTestClassQt";
    QTest::newRow("ExportedFinalTestClassQtX") << ExportedFinalTestClassQtX::staticMetaObject.className() << "ExportedFinalTestClassQtX";

    QTest::newRow("FinalTestClassCpp11") << FinalTestClassCpp11::staticMetaObject.className() << "FinalTestClassCpp11";
    QTest::newRow("ExportedFinalTestClassCpp11") << ExportedFinalTestClassCpp11::staticMetaObject.className() << "ExportedFinalTestClassCpp11";
    QTest::newRow("ExportedFinalTestClassCpp11X") << ExportedFinalTestClassCpp11X::staticMetaObject.className() << "ExportedFinalTestClassCpp11X";

    QTest::newRow("SealedTestClass") << SealedTestClass::staticMetaObject.className() << "SealedTestClass";
    QTest::newRow("ExportedSealedTestClass") << ExportedSealedTestClass::staticMetaObject.className() << "ExportedSealedTestClass";
    QTest::newRow("ExportedSealedTestClassX") << ExportedSealedTestClassX::staticMetaObject.className() << "ExportedSealedTestClassX";
}

void tst_Moc::finalClasses()
{
    QFETCH(QString, className);
    QFETCH(QString, expected);

    QCOMPARE(className, expected);
}

void tst_Moc::explicitOverrideControl_data()
{
    QTest::addColumn<const QMetaObject*>("mo");

#define ADD(x) QTest::newRow(#x) << &x::staticMetaObject
    ADD(ExplicitOverrideControlFinalQt);
    ADD(ExplicitOverrideControlFinalCxx11);
    ADD(ExplicitOverrideControlSealed);
    ADD(ExplicitOverrideControlOverrideQt);
    ADD(ExplicitOverrideControlOverrideCxx11);
    ADD(ExplicitOverrideControlFinalQtOverrideQt);
    ADD(ExplicitOverrideControlFinalCxx11OverrideCxx11);
    ADD(ExplicitOverrideControlSealedOverride);
#undef ADD
}

void tst_Moc::explicitOverrideControl()
{
    QFETCH(const QMetaObject*, mo);

    QVERIFY(mo);
    QCOMPARE(mo->indexOfMethod("pureSlot0()"), mo->methodOffset() + 0);
    QCOMPARE(mo->indexOfMethod("pureSlot1()"), mo->methodOffset() + 1);
    QCOMPARE(mo->indexOfMethod("pureSlot2()"), mo->methodOffset() + 2);
    QCOMPARE(mo->indexOfMethod("pureSlot3()"), mo->methodOffset() + 3);
#if 0 // moc doesn't support volatile slots
    QCOMPARE(mo->indexOfMethod("pureSlot4()"), mo->methodOffset() + 4);
    QCOMPARE(mo->indexOfMethod("pureSlot5()"), mo->methodOffset() + 5);
    QCOMPARE(mo->indexOfMethod("pureSlot6()"), mo->methodOffset() + 6);
    QCOMPARE(mo->indexOfMethod("pureSlot7()"), mo->methodOffset() + 7);
    QCOMPARE(mo->indexOfMethod("pureSlot8()"), mo->methodOffset() + 8);
    QCOMPARE(mo->indexOfMethod("pureSlot9()"), mo->methodOffset() + 9);
#endif
}

class OverloadedAddressOperator : public QObject
{
   Q_OBJECT
public:
   void* operator&() { return nullptr; }
signals:
   void self(OverloadedAddressOperator&);
public slots:
    void assertSelf(OverloadedAddressOperator &o)
    {
        QCOMPARE(std::addressof(o), this);
        testResult = (std::addressof(o) == this);
    }
public:
    bool testResult = false;
};

void tst_Moc::overloadedAddressOperator()
{
    OverloadedAddressOperator o;
    OverloadedAddressOperator *p = std::addressof(o);
    QCOMPARE(&o, nullptr);
    QVERIFY(p);
    QObject::connect(p, &OverloadedAddressOperator::self, p, &OverloadedAddressOperator::assertSelf);
    emit o.self(o);
    QVERIFY(o.testResult);
}

class CustomQObject : public QObject
{
    Q_OBJECT
    Q_ENUMS(Number)
public:
    enum Number {
      Zero,
      One,
      Two
    };
    explicit CustomQObject(QObject *parent = nullptr)
      : QObject(parent)
    {
    }
};

Q_DECLARE_METATYPE(CustomQObject::Number)

typedef CustomQObject* CustomQObjectStar;
Q_DECLARE_METATYPE(CustomQObjectStar);

namespace SomeNamespace {

class NamespacedQObject : public QObject
{
    Q_OBJECT
public:
    explicit NamespacedQObject(QObject *parent = nullptr)
      : QObject(parent)
    {

    }
};

struct NamespacedNonQObject {};
}
Q_DECLARE_METATYPE(SomeNamespace::NamespacedNonQObject)

// Need different types for the invokable method tests because otherwise the registration
// done in the property test would interfere.

class CustomQObject2 : public QObject
{
    Q_OBJECT
    Q_ENUMS(Number)
public:
    enum Number {
      Zero,
      One,
      Two
    };
    explicit CustomQObject2(QObject *parent = nullptr)
      : QObject(parent)
    {
    }
};

Q_DECLARE_METATYPE(CustomQObject2::Number)

typedef CustomQObject2* CustomQObject2Star;
Q_DECLARE_METATYPE(CustomQObject2Star);

namespace SomeNamespace2 {

class NamespacedQObject2 : public QObject
{
    Q_OBJECT
public:
    explicit NamespacedQObject2(QObject *parent = nullptr)
      : QObject(parent)
    {

    }
};

struct NamespacedNonQObject2 {};
}
Q_DECLARE_METATYPE(SomeNamespace2::NamespacedNonQObject2)


struct CustomObject3 {};
struct CustomObject4 {};
struct CustomObject5 {};
struct CustomObject6 {};
struct CustomObject7 {};
struct CustomObject8 {};
struct CustomObject9 {};
struct CustomObject10 {};
struct CustomObject11 {};
struct CustomObject12 {};

Q_DECLARE_METATYPE(CustomObject3)
Q_DECLARE_METATYPE(CustomObject4)
Q_DECLARE_METATYPE(CustomObject5)
Q_DECLARE_METATYPE(CustomObject6)
Q_DECLARE_METATYPE(CustomObject7)
Q_DECLARE_METATYPE(CustomObject8)
Q_DECLARE_METATYPE(CustomObject9)
Q_DECLARE_METATYPE(CustomObject10)
Q_DECLARE_METATYPE(CustomObject11)
Q_DECLARE_METATYPE(CustomObject12)

class AutoRegistrationObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* object READ object CONSTANT)
    Q_PROPERTY(CustomQObject* customObject READ customObject CONSTANT)
    Q_PROPERTY(QSharedPointer<CustomQObject> customObjectP READ customObjectP CONSTANT)
    Q_PROPERTY(QWeakPointer<CustomQObject> customObjectWP READ customObjectWP CONSTANT)
    Q_PROPERTY(QPointer<CustomQObject> customObjectTP READ customObjectTP CONSTANT)
    Q_PROPERTY(QList<int> listInt READ listInt CONSTANT)
    Q_PROPERTY(QList<QVariant> listVariant READ listVariant CONSTANT)
    Q_PROPERTY(QList<CustomQObject*> listObject READ listObject CONSTANT)
    Q_PROPERTY(QList<QList<int>> listListInt READ listListInt CONSTANT)
    Q_PROPERTY(QList<QList<CustomQObject *>> listListObject READ listListObject CONSTANT)
    Q_PROPERTY(CustomQObject::Number enumValue READ enumValue CONSTANT)
    Q_PROPERTY(CustomQObjectStar customObjectTypedef READ customObjectTypedef CONSTANT)
    Q_PROPERTY(SomeNamespace::NamespacedQObject* customObjectNamespaced READ customObjectNamespaced CONSTANT)
    Q_PROPERTY(SomeNamespace::NamespacedNonQObject customNonQObjectNamespaced READ customNonQObjectNamespaced CONSTANT)
public:
    AutoRegistrationObject(QObject *parent = nullptr)
      : QObject(parent)
    {
    }

    QObject* object() const
    {
        return 0;
    }

    QSharedPointer<CustomQObject> customObjectP() const
    {
        return QSharedPointer<CustomQObject>();
    }

    QWeakPointer<CustomQObject> customObjectWP() const
    {
        return QWeakPointer<CustomQObject>();
    }

    QPointer<CustomQObject> customObjectTP() const
    {
        return QPointer<CustomQObject>();
    }

    CustomQObject* customObject() const
    {
        return 0;
    }

    QList<int> listInt() const
    {
        return QList<int>();
    }

    QList<QVariant> listVariant() const { return QList<QVariant>(); }

    QList<CustomQObject*> listObject() const
    {
        return QList<CustomQObject*>();
    }

    QList<QList<int>> listListInt() const { return QList<QList<int>>(); }

    QList<QList<CustomQObject *>> listListObject() const { return QList<QList<CustomQObject *>>(); }

    CustomQObject::Number enumValue() const
    {
        return CustomQObject::Zero;
    }

    CustomQObjectStar customObjectTypedef() const
    {
        return 0;
    }

    SomeNamespace::NamespacedQObject* customObjectNamespaced() const
    {
        return 0;
    }

    SomeNamespace::NamespacedNonQObject customNonQObjectNamespaced() const
    {
        return SomeNamespace::NamespacedNonQObject();
    }

public slots:
    void objectSlot(QObject*) {}
    void customObjectSlot(CustomQObject2*) {}
    void sharedPointerSlot(QSharedPointer<CustomQObject2>) {}
    void weakPointerSlot(QWeakPointer<CustomQObject2>) {}
    void trackingPointerSlot(QPointer<CustomQObject2>) {}
    void listIntSlot(QList<int>) {}
    void listVariantSlot(QList<QVariant>) { }
    void listCustomObjectSlot(QList<CustomQObject2*>) {}
    void listListIntSlot(QList<QList<int>>) { }
    void listListCustomObjectSlot(QList<QList<CustomQObject2 *>>) { }
    void enumSlot(CustomQObject2::Number) {}
    void typedefSlot(CustomQObject2Star) {}
    void namespacedQObjectSlot(SomeNamespace2::NamespacedQObject2*) {}
    void namespacedNonQObjectSlot(SomeNamespace2::NamespacedNonQObject2) {}

    void bu1(int, CustomObject3) {}
    void bu2(CustomObject4, int) {}
    void bu3(CustomObject5, CustomObject6) {}
    void bu4(CustomObject7, int, CustomObject8) {}
    void bu5(int, CustomObject9, CustomObject10) {}
    void bu6(int, CustomObject11, int) {}

    // these can't be registered, but they should at least compile
    void ref1(int&) {}
    void ref2(QList<int>&) {}
    void ref3(CustomQObject2&) {}
    void ref4(QSharedPointer<CustomQObject2>&) {}

signals:
    void someSignal(CustomObject12);
};

void tst_Moc::autoPropertyMetaTypeRegistration()
{
    AutoRegistrationObject aro;

    static const int numPropertiesUnderTest = 15;
    QList<int> propertyMetaTypeIds;
    propertyMetaTypeIds.reserve(numPropertiesUnderTest);

    const QMetaObject *metaObject = aro.metaObject();
    QCOMPARE(metaObject->propertyCount(), numPropertiesUnderTest);
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        const QMetaProperty prop = metaObject->property(i);
        propertyMetaTypeIds.append(prop.userType());
        QVariant var = prop.read(&aro);
        QVERIFY(var.isValid());
    }

    // Verify that QMetaProperty::userType gave us what we expected.
    QList<int> expectedMetaTypeIds = QList<int>()
            << QMetaType::QString // QObject::userType
            << QMetaType::QObjectStar // AutoRegistrationObject::object
            << qMetaTypeId<CustomQObject *>() // etc.
            << qMetaTypeId<QSharedPointer<CustomQObject>>()
            << qMetaTypeId<QWeakPointer<CustomQObject>>() << qMetaTypeId<QPointer<CustomQObject>>()
            << qMetaTypeId<QList<int>>() << qMetaTypeId<QList<QVariant>>()
            << qMetaTypeId<QList<CustomQObject *>>() << qMetaTypeId<QList<QList<int>>>()
            << qMetaTypeId<QList<QList<CustomQObject *>>>() << qMetaTypeId<CustomQObject::Number>()
            << qMetaTypeId<CustomQObjectStar>() << qMetaTypeId<SomeNamespace::NamespacedQObject *>()
            << qMetaTypeId<SomeNamespace::NamespacedNonQObject>();

    QCOMPARE(propertyMetaTypeIds, expectedMetaTypeIds);
}

template<typename T>
struct DefaultConstructor
{
  static inline T construct() { return T(); }
};

template<typename T>
struct DefaultConstructor<T*>
{
  static inline T* construct() { return 0; }
};

void tst_Moc::autoMethodArgumentMetaTypeRegistration()
{
    AutoRegistrationObject aro;

    QList<int> methodArgMetaTypeIds;

    const QMetaObject *metaObject = aro.metaObject();

    int i = metaObject->methodOffset(); // Start after QObject built-in slots;

    while (i < metaObject->methodCount()) {
        // Skip over signals so we start at the first slot.
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal)
            ++i;
        else
            break;

    }

#define TYPE_LOOP(TYPE) \
    { \
        const QMetaMethod method = metaObject->method(i); \
        for (int j = 0; j < method.parameterCount(); ++j) \
            methodArgMetaTypeIds.append(method.parameterType(j)); \
        QVERIFY(method.invoke(&aro, Q_ARG(TYPE, DefaultConstructor<TYPE>::construct()))); \
        ++i; \
    }

#define FOR_EACH_SLOT_ARG_TYPE(F)                                                                  \
    F(QObject *)                                                                                   \
    F(CustomQObject2 *)                                                                            \
    F(QSharedPointer<CustomQObject2>)                                                              \
    F(QWeakPointer<CustomQObject2>)                                                                \
    F(QPointer<CustomQObject2>)                                                                    \
    F(QList<int>)                                                                                  \
    F(QList<QVariant>)                                                                             \
    F(QList<CustomQObject2 *>)                                                                     \
    F(QList<QList<int>>)                                                                           \
    F(QList<QList<CustomQObject2 *>>)                                                              \
    F(CustomQObject2::Number)                                                                      \
    F(CustomQObject2Star)                                                                          \
    F(SomeNamespace2::NamespacedQObject2 *)                                                        \
    F(SomeNamespace2::NamespacedNonQObject2)

    // Note: mulit-arg slots are tested below.

    FOR_EACH_SLOT_ARG_TYPE(TYPE_LOOP)

#undef TYPE_LOOP
#undef FOR_EACH_SLOT_ARG_TYPE

    QList<int> expectedMetaTypeIds = QList<int>()
            << QMetaType::QObjectStar << qMetaTypeId<CustomQObject2 *>()
            << qMetaTypeId<QSharedPointer<CustomQObject2>>()
            << qMetaTypeId<QWeakPointer<CustomQObject2>>()
            << qMetaTypeId<QPointer<CustomQObject2>>() << qMetaTypeId<QList<int>>()
            << qMetaTypeId<QList<QVariant>>() << qMetaTypeId<QList<CustomQObject2 *>>()
            << qMetaTypeId<QList<QList<int>>>() << qMetaTypeId<QList<QList<CustomQObject2 *>>>()
            << qMetaTypeId<CustomQObject2::Number>() << qMetaTypeId<CustomQObject2Star>()
            << qMetaTypeId<SomeNamespace2::NamespacedQObject2 *>()
            << qMetaTypeId<SomeNamespace2::NamespacedNonQObject2>();

    QCOMPARE(methodArgMetaTypeIds, expectedMetaTypeIds);

    QList<int> methodMultiArgMetaTypeIds;

    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu1"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(int, 42), Q_ARG(CustomObject3, CustomObject3())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu2"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(CustomObject4, CustomObject4()), Q_ARG(int, 42)));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu3"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(CustomObject5, CustomObject5()), Q_ARG(CustomObject6, CustomObject6())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu4"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(CustomObject7, CustomObject7()), Q_ARG(int, 42), Q_ARG(CustomObject8, CustomObject8())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu5"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(int, 42), Q_ARG(CustomObject9, CustomObject9()), Q_ARG(CustomObject10, CustomObject10())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu6"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(int, 42), Q_ARG(CustomObject11, CustomObject11()), Q_ARG(int, 42)));
        ++i;
    }

    QList<int> expectedMultiMetaTypeIds = QList<int>()
            << QMetaType::Int << qMetaTypeId<CustomObject3>() << qMetaTypeId<CustomObject4>()
            << QMetaType::Int << qMetaTypeId<CustomObject5>() << qMetaTypeId<CustomObject6>()
            << qMetaTypeId<CustomObject7>() << QMetaType::Int << qMetaTypeId<CustomObject8>()
            << QMetaType::Int << qMetaTypeId<CustomObject9>() << qMetaTypeId<CustomObject10>()
            << QMetaType::Int << qMetaTypeId<CustomObject11>() << QMetaType::Int;

    QCOMPARE(methodMultiArgMetaTypeIds, expectedMultiMetaTypeIds);


}

void tst_Moc::autoSignalSpyMetaTypeRegistration()
{
    AutoRegistrationObject aro;

    QList<int> methodArgMetaTypeIds;

    const QMetaObject *metaObject = aro.metaObject();

    int i = metaObject->indexOfSignal(QMetaObject::normalizedSignature("someSignal(CustomObject12)"));

    QVERIFY(i > 0);

    QCOMPARE(QMetaType::fromName("CustomObject12").id(), (int)QMetaType::UnknownType);

    QSignalSpy spy(&aro, SIGNAL(someSignal(CustomObject12)));

    QVERIFY(QMetaType::fromName("CustomObject12").id() != QMetaType::UnknownType);
    QCOMPARE(QMetaType::fromName("CustomObject12").id(), qMetaTypeId<CustomObject12>());
}

void tst_Moc::parseDefines()
{
    const QMetaObject *mo = &PD_NAMESPACE::PD_CLASSNAME::staticMetaObject;
    QCOMPARE(mo->className(), PD_SCOPED_STRING(PD_NAMESPACE, PD_CLASSNAME));
    QVERIFY(mo->indexOfSlot("voidFunction()") != -1);

    int index = mo->indexOfSlot("stringMethod()");
    QVERIFY(index != -1);
    QCOMPARE(mo->method(index).returnType(), int(QMetaType::QString));

    index = mo->indexOfSlot("combined1()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined2()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined3()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined4(int,int)");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined5()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined6()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("vararg1()");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg2(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg3(int,int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg4()");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg5(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg6(int,int)");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("INNERFUNCTION(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("inner_expanded(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("expanded_method(int)");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("conditionSlot()");
    QVERIFY(index != -1);

    int count = 0;
    for (int i = 0; i < mo->classInfoCount(); ++i) {
        QMetaClassInfo mci = mo->classInfo(i);
        if (!qstrcmp(mci.name(), "TestString")) {
            ++count;
            QVERIFY(!qstrcmp(mci.value(), "PD_CLASSNAME"));
        }
        if (!qstrcmp(mci.name(), "TestString2")) {
            ++count;
            QVERIFY(!qstrcmp(mci.value(), "ParseDefine"));
        }
        if (!qstrcmp(mci.name(), "TestString3")) {
            ++count;
            QVERIFY(!qstrcmp(mci.value(), "TestValue"));
        }
    }
    QCOMPARE(count, 3);

    index = mo->indexOfSlot("PD_DEFINE_ITSELF_SUFFIX(int)");
    QVERIFY(index != -1);

    index = mo->indexOfSignal("cmdlineSignal(QMap<int,int>)");
    QVERIFY(index != -1);

    index = mo->indexOfSignal("signalQTBUG55853()");
    QVERIFY(index != -1);
}

void tst_Moc::preprocessorOnly()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    proc.start(m_moc, QStringList() << "-E" << m_sourceDirectory + QStringLiteral("/pp-dollar-signs.h"));
    QVERIFY(proc.waitForFinished());
    VERIFY_NO_ERRORS(proc);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());

    QVERIFY(mocOut.contains("$$ = parser->createFoo()"));
#else
    QSKIP("Only tested on unix/gcc");
#endif
}


void tst_Moc::unterminatedFunctionMacro()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && defined(Q_CC_GNU) && QT_CONFIG(process)
    QProcess proc;
    proc.start(m_moc, QStringList() << "-E" << m_sourceDirectory + QStringLiteral("/unterminated-function-macro.h"));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 1);
    QCOMPARE(proc.readAllStandardOutput(), QByteArray());
    QByteArray errorOutput = proc.readAllStandardError();
    QVERIFY(!errorOutput.isEmpty());
    QVERIFY(errorOutput.contains("missing ')' in macro usage"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

namespace QTBUG32933_relatedObjectsDontIncludeItself {
    namespace NS {
        class Obj : QObject {
            Q_OBJECT
            Q_PROPERTY(MyEnum p1 MEMBER member)
            Q_PROPERTY(Obj::MyEnum p2 MEMBER member)
            Q_PROPERTY(NS::Obj::MyEnum p3 MEMBER member)
            Q_PROPERTY(QTBUG32933_relatedObjectsDontIncludeItself::NS::Obj::MyEnum p4 MEMBER member)
            Q_ENUMS(MyEnum);
        public:
            enum MyEnum { Something, SomethingElse };
            MyEnum member;
        };
    }
}

void tst_Moc::QTBUG32933_relatedObjectsDontIncludeItself()
{
    const QMetaObject *mo = &QTBUG32933_relatedObjectsDontIncludeItself::NS::Obj::staticMetaObject;
    const auto *objects = mo->d.relatedMetaObjects;
    // the related objects should be empty because the enums is in the same object.
    QVERIFY(!objects);
}

class UnrelatedClass : public QObject
{
    Q_OBJECT
    Q_ENUMS(UnrelatedEnum)
public:
    enum UnrelatedEnum {
        UnrelatedInvalidValue = -1,
        UnrelatedValue = 42
    };
};

// The presence of this macro used to confuse moc and prevent
// UnrelatedClass from being listed in the related meta objects.
Q_DECLARE_METATYPE(UnrelatedClass::UnrelatedEnum)

class TestClassReferencingUnrelatedEnum : public QObject
{
    Q_OBJECT
    Q_PROPERTY(UnrelatedClass::UnrelatedEnum enumProperty READ enumProperty WRITE setEnumProperty)
public:
    TestClassReferencingUnrelatedEnum()
        : m_enumProperty(UnrelatedClass::UnrelatedInvalidValue)
    {}

    UnrelatedClass::UnrelatedEnum enumProperty() const {
        return m_enumProperty;
    }

    void setEnumProperty(UnrelatedClass::UnrelatedEnum arg) {
        m_enumProperty = arg;
    }

private:
    UnrelatedClass::UnrelatedEnum m_enumProperty;
};

void tst_Moc::writeEnumFromUnrelatedClass()
{
    TestClassReferencingUnrelatedEnum obj;
    QString enumValueAsString("UnrelatedValue");
    obj.setProperty("enumProperty", enumValueAsString);
    QCOMPARE(int(obj.enumProperty()), int(UnrelatedClass::UnrelatedValue));
}



void tst_Moc::relatedMetaObjectsWithinNamespaces()
{
    const QMetaObject *relatedMo = &QTBUG_2151::A::staticMetaObject;

    const QMetaObject *testMo = &QTBUG_2151::B::staticMetaObject;
    QVERIFY(testMo->d.relatedMetaObjects);
    QCOMPARE(testMo->d.relatedMetaObjects[0], relatedMo);
}

void tst_Moc::relatedMetaObjectsInGadget()
{
    const QMetaObject *relatedMo = &QTBUG_35657::A::staticMetaObject;

    const QMetaObject *testMo = &QTBUG_35657::B::staticMetaObject;
    QVERIFY(testMo->d.relatedMetaObjects);
    QCOMPARE(testMo->d.relatedMetaObjects[0], relatedMo);
}

void tst_Moc::relatedMetaObjectsNameConflict_data()
{
    typedef QList<const QMetaObject *> QMetaObjects;
    QTest::addColumn<const QMetaObject*>("dependingObject");
    QTest::addColumn<QMetaObjects>("relatedMetaObjects");

    //NS1
    const QMetaObject *n1gadget = &NS1::Gadget::staticMetaObject;
    const QMetaObject *n1object = &NS1::Object::staticMetaObject;
    const QMetaObject *n1nestedGadget = &NS1::Nested::Gadget::staticMetaObject;
    const QMetaObject *n1nestedObject = &NS1::Nested::Object::staticMetaObject;
    //N2
    const QMetaObject *n2gadget = &NS2::Gadget::staticMetaObject;
    const QMetaObject *n2object = &NS2::Object::staticMetaObject;
    const QMetaObject *n2nestedGadget = &NS2::Nested::Gadget::staticMetaObject;
    const QMetaObject *n2nestedObject = &NS2::Nested::Object::staticMetaObject;

    QTest::newRow("N1::dependingObject") << &NS1::DependingObject::staticMetaObject
                                        <<  (QMetaObjects() << n1gadget << n1object);
    QTest::newRow("N2::dependingObject") << &NS2::DependingObject::staticMetaObject
                                        <<  (QMetaObjects() << n2gadget << n2object);
    QTest::newRow("N1::dependingNestedObject") << &NS1::DependingNestedObject::staticMetaObject
                                        <<  (QMetaObjects() << n1nestedObject);
    QTest::newRow("N2::dependingNestedObject") << &NS2::DependingNestedObject::staticMetaObject
                                        <<  (QMetaObjects() << n2nestedObject);
    QTest::newRow("N1::dependingNestedGadget") << &NS1::DependingNestedGadget::staticMetaObject
                                        <<  (QMetaObjects() << n1nestedGadget);
    QTest::newRow("N2::dependingNestedGadget") << &NS2::DependingNestedGadget::staticMetaObject
                                        <<  (QMetaObjects() << n2nestedGadget);
}

void tst_Moc::relatedMetaObjectsNameConflict()
{
    typedef QList<const QMetaObject *> QMetaObjects;
    QFETCH(const QMetaObject*, dependingObject);
    QFETCH(const QMetaObjects, relatedMetaObjects);

    // load all specified metaobjects int a set
    QSet<const QMetaObject*> dependency;
    const auto *i = dependingObject->d.relatedMetaObjects;
    while (*i) {
        dependency.insert(*i);
        ++i;
    }

    // check if all required metaobjects are specified
    for (const QMetaObject *mo : relatedMetaObjects)
        QVERIFY(dependency.contains(mo));

    // check if no additional metaobjects ara specified
    QCOMPARE(dependency.size(), relatedMetaObjects.size());
}

class StringLiteralsInMacroExtension: public QObject
{
    Q_OBJECT
#define Macro(F) F " " F
    Q_CLASSINFO(Macro("String"), Macro("Literal"))
#undef Macro

#define Macro(F) F
    Q_CLASSINFO("String" Macro("!"), "Literal" Macro("!"))
    Q_CLASSINFO(Macro("!") "String", Macro("!") "Literal")
#undef Macro

#define Macro "foo"
    Q_CLASSINFO("String" Macro, "Literal" Macro)
    Q_CLASSINFO(Macro "String", Macro "Literal")
#undef Macro
};

void tst_Moc::strignLiteralsInMacroExtension()
{
    const QMetaObject *mobj = &StringLiteralsInMacroExtension::staticMetaObject;
    QCOMPARE(mobj->classInfoCount(), 5);

    QCOMPARE(mobj->classInfo(0).name(), "String String");
    QCOMPARE(mobj->classInfo(0).value(), "Literal Literal");

    QCOMPARE(mobj->classInfo(1).name(), "String!");
    QCOMPARE(mobj->classInfo(1).value(), "Literal!");

    QCOMPARE(mobj->classInfo(2).name(), "!String");
    QCOMPARE(mobj->classInfo(2).value(), "!Literal");

    QCOMPARE(mobj->classInfo(3).name(), "Stringfoo");
    QCOMPARE(mobj->classInfo(3).value(), "Literalfoo");

    QCOMPARE(mobj->classInfo(4).name(), "fooString");
    QCOMPARE(mobj->classInfo(4).value(), "fooLiteral");
}

class VeryLongStringData : public QObject
{
    Q_OBJECT

    #define repeat2(V) V V
    #define repeat4(V) repeat2(V) repeat2(V)
    #define repeat8(V) repeat4(V) repeat4(V)
    #define repeat16(V) repeat8(V) repeat8(V)
    #define repeat32(V) repeat16(V) repeat16(V)
    #define repeat64(V) repeat32(V) repeat32(V)
    #define repeat128(V) repeat64(V) repeat64(V)
    #define repeat256(V) repeat128(V) repeat128(V)
    #define repeat512(V) repeat256(V) repeat256(V)
    #define repeat1024(V) repeat512(V) repeat512(V)
    #define repeat2048(V) repeat1024(V) repeat1024(V)
    #define repeat4096(V) repeat2048(V) repeat2048(V)
    #define repeat8192(V) repeat4096(V) repeat4096(V)
    #define repeat16384(V) repeat8192(V) repeat8192(V)
    #define repeat32768(V) repeat16384(V) repeat16384(V)
    #define repeat65534(V) repeat32768(V) repeat16384(V) repeat8192(V) repeat4096(V) repeat2048(V) repeat1024(V) repeat512(V) repeat256(V) repeat128(V) repeat64(V) repeat32(V) repeat16(V) repeat8(V) repeat4(V) repeat2(V)

    Q_CLASSINFO("\1" "23\xff", "String with CRLF.\r\n")
    Q_CLASSINFO(repeat65534("n"), repeat65534("i"))
    Q_CLASSINFO(repeat65534("e"), repeat65534("r"))
    Q_CLASSINFO(repeat32768("o"), repeat32768("b"))
    Q_CLASSINFO(":", ")")

    #undef repeat2
    #undef repeat4
    #undef repeat8
    #undef repeat16
    #undef repeat32
    #undef repeat64
    #undef repeat128
    #undef repeat256
    #undef repeat512
    #undef repeat1024
    #undef repeat2048
    #undef repeat4096
    #undef repeat8192
    #undef repeat16384
    #undef repeat32768
    #undef repeat65534
};

void tst_Moc::unnamedNamespaceObjectsAndGadgets()
{
    // these just test very basic functionality of gadgets and objects
    // defined in unnamed namespaces.
    {
        GadgetInUnnamedNS gadget(21, 42);
        QCOMPARE(gadget.x(), 21);
        QCOMPARE(gadget.y(), 42);
        gadget.staticMetaObject.property(0).writeOnGadget(&gadget, 12);
        gadget.staticMetaObject.property(1).writeOnGadget(&gadget, 24);
        QCOMPARE(gadget.x(), 12);
        QCOMPARE(gadget.y(), 24);
    }

    {
        ObjectInUnnamedNS object;
        QObject *qObject = &object;
        QCOMPARE(static_cast<ObjectInUnnamedNS *>(qObject),
                 qobject_cast<ObjectInUnnamedNS *>(qObject));
    }
}

void tst_Moc::veryLongStringData()
{
    const QMetaObject *mobj = &VeryLongStringData::staticMetaObject;
    int startAt = 1;        // some other classinfo added to the beginning
    QCOMPARE(mobj->classInfoCount(), startAt + 4);

    QCOMPARE(mobj->classInfo(startAt + 0).name()[0], 'n');
    QCOMPARE(mobj->classInfo(startAt + 0).value()[0], 'i');
    QCOMPARE(mobj->classInfo(startAt + 1).name()[0], 'e');
    QCOMPARE(mobj->classInfo(startAt + 1).value()[0], 'r');
    QCOMPARE(mobj->classInfo(startAt + 2).name()[0], 'o');
    QCOMPARE(mobj->classInfo(startAt + 2).value()[0], 'b');
    QCOMPARE(mobj->classInfo(startAt + 3).name()[0], ':');
    QCOMPARE(mobj->classInfo(startAt + 3).value()[0], ')');

    QCOMPARE(strlen(mobj->classInfo(startAt + 0).name()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(startAt + 0).value()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(startAt + 1).name()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(startAt + 1).value()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(startAt + 2).name()), static_cast<size_t>(32768));
    QCOMPARE(strlen(mobj->classInfo(startAt + 2).value()), static_cast<size_t>(32768));
    QCOMPARE(strlen(mobj->classInfo(startAt + 3).name()), static_cast<size_t>(1));
    QCOMPARE(strlen(mobj->classInfo(startAt + 3).value()), static_cast<size_t>(1));
}

void tst_Moc::gadgetHierarchy()
{
    QCOMPARE(NonGadgetParent::Derived::staticMetaObject.superClass(), static_cast<const QMetaObject*>(nullptr));
    QCOMPARE(GrandParentGadget::DerivedGadget::staticMetaObject.superClass(), &GrandParentGadget::BaseGadget::staticMetaObject);
    QCOMPARE(GrandParentGadget::CRTPDerivedGadget::staticMetaObject.superClass(), &GrandParentGadget::BaseGadget::staticMetaObject);
}

void tst_Moc::optionsFileError_data()
{
    QTest::addColumn<QString>("optionsArgument");
    QTest::newRow("no filename") << QStringLiteral("@");
    QTest::newRow("nonexistent file") << QStringLiteral("@letshuntasnark");
}

void tst_Moc::optionsFileError()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if QT_CONFIG(process)
    QFETCH(QString, optionsArgument);
    QProcess p;
    p.start(m_moc, QStringList(optionsArgument));
    QVERIFY(p.waitForFinished());
    QCOMPARE(p.exitCode(), 1);
    QVERIFY(p.readAllStandardOutput().isEmpty());
    const QByteArray err = p.readAllStandardError();
    QVERIFY(err.contains("moc: "));
    QVERIFY(!err.contains("QCommandLineParser"));
#endif
}

static void checkEnum(const QMetaEnum &enumerator, const QByteArray &name,
                      const QList<QPair<QByteArray, int>> &keys,
                      const QMetaType underlyingType = QMetaType::fromType<int>())
{
    QCOMPARE(name, QByteArray{enumerator.name()});
    QCOMPARE(keys.size(), enumerator.keyCount());
    QCOMPARE(underlyingType, enumerator.metaType().underlyingType());
    for (int i = 0; i < enumerator.keyCount(); ++i) {
        QCOMPARE(keys[i].first, QByteArray{enumerator.key(i)});
        QCOMPARE(keys[i].second, enumerator.value(i));
    }
}

class EnumFromNamespaceClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FooNamespace::Enum1 prop READ prop CONSTANT)
public:
    FooNamespace::Enum1 prop() { return FooNamespace::Enum1::Key2; }
};

void tst_Moc::testNestedQNamespace()
{
    QCOMPARE(TestSameEnumNamespace::staticMetaObject.enumeratorCount(), 1);
    checkEnum(TestSameEnumNamespace::staticMetaObject.enumerator(0), "TestSameEnumNamespace",
                {{"Key1", 1}, {"Key2", 2}});
    QMetaEnum meta1 = QMetaEnum::fromType<TestSameEnumNamespace::TestSameEnumNamespace>();
    QVERIFY(meta1.isValid());
    QCOMPARE(meta1.name(), "TestSameEnumNamespace");
    QCOMPARE(meta1.enclosingMetaObject(), &TestSameEnumNamespace::staticMetaObject);
    QCOMPARE(meta1.keyCount(), 2);

    // QTBUG-112996
    QCOMPARE(TestNestedSameEnumNamespace::a::staticMetaObject.enumeratorCount(), 1);
    checkEnum(TestNestedSameEnumNamespace::a::staticMetaObject.enumerator(0), "a",
              {{"Key11", 11}, {"Key12", 12}});
    QMetaEnum meta2 = QMetaEnum::fromType<TestNestedSameEnumNamespace::a::a>();
    QVERIFY(meta2.isValid());
    QCOMPARE(meta2.name(), "a");
    QCOMPARE(meta2.enclosingMetaObject(), &TestNestedSameEnumNamespace::a::staticMetaObject);
    QCOMPARE(meta2.keyCount(), 2);
}

void tst_Moc::testQNamespace()
{
    QCOMPARE(TestQNamespace::staticMetaObject.enumeratorCount(), 5);
    checkEnum(TestQNamespace::staticMetaObject.enumerator(0), "TestEnum1",
                {{"Key1", 11}, {"Key2", 12}});
    checkEnum(TestQNamespace::staticMetaObject.enumerator(1), "TestEnum2",
                {{"Key1", 17}, {"Key2", 18}});
    checkEnum(TestQNamespace::staticMetaObject.enumerator(2), "TestEnum3",
                {{"Key1", 23}, {"Key2", 24}}, QMetaType::fromType<qint8>());
    checkEnum(TestQNamespace::staticMetaObject.enumerator(3), "TestFlag1",
                {{"None", 0}, {"Flag1", 1}, {"Flag2", 2}, {"Any", 1 | 2}});
    checkEnum(TestQNamespace::staticMetaObject.enumerator(4), "TestFlag2",
                {{"None", 0}, {"Flag1", 4}, {"Flag2", 8}, {"Any", 4 | 8}});

    QCOMPARE(TestQNamespace::TestGadget::staticMetaObject.enumeratorCount(), 3);
    checkEnum(TestQNamespace::TestGadget::staticMetaObject.enumerator(0), "TestGEnum1",
                {{"Key1", 13}, {"Key2", 14}});
    checkEnum(TestQNamespace::TestGadget::staticMetaObject.enumerator(1), "TestGEnum2",
                {{"Key1", 23}, {"Key2", 24}});
    checkEnum(TestQNamespace::TestGadget::staticMetaObject.enumerator(2), "TestGEnum3",
                {{"Key1", 33}, {"Key2", 34}}, QMetaType::fromType<qint16>());

    QCOMPARE(TestQNamespace::TestGadgetExport::staticMetaObject.enumeratorCount(), 3);
    checkEnum(TestQNamespace::TestGadgetExport::staticMetaObject.enumerator(0), "TestGeEnum1",
                {{"Key1", 20}, {"Key2", 21}});
    checkEnum(TestQNamespace::TestGadgetExport::staticMetaObject.enumerator(1), "TestGeEnum2",
                {{"Key1", 23}, {"Key2", 24}});
    checkEnum(TestQNamespace::TestGadgetExport::staticMetaObject.enumerator(2), "TestGeEnum3",
                {{"Key1", 26}, {"Key2", 27}}, QMetaType::fromType<quint16>());

    QMetaEnum meta = QMetaEnum::fromType<TestQNamespace::TestEnum1>();
    QVERIFY(meta.isValid());
    QCOMPARE(meta.name(), "TestEnum1");
    QCOMPARE(meta.enclosingMetaObject(), &TestQNamespace::staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);

    QCOMPARE(TestExportNamespace::staticMetaObject.enumeratorCount(), 1);
    checkEnum(TestExportNamespace::staticMetaObject.enumerator(0), "MyEnum",
        {{"Key1", 0}, {"Key2", 1}});

    QCOMPARE(FooNamespace::staticMetaObject.enumeratorCount(), 1);
    QCOMPARE(FooNamespace::FooNestedNamespace::staticMetaObject.enumeratorCount(), 2);
    QCOMPARE(FooNamespace::FooNestedNamespace::FooMoreNestedNamespace::staticMetaObject.enumeratorCount(), 1);

    EnumFromNamespaceClass obj;
    const QVariant prop = obj.property("prop");
    QCOMPARE(prop.userType(), QMetaType::fromType<FooNamespace::Enum1>().id());
    QCOMPARE(prop.toInt(), int(FooNamespace::Enum1::Key2));
}

void tst_Moc::cxx17Namespaces()
{
    QCOMPARE(CXX17Namespace::A::B::C::D::staticMetaObject.className(),
             "CXX17Namespace::A::B::C::D");
    QCOMPARE(CXX17Namespace::A::B::C::D::staticMetaObject.enumeratorCount(), 1);
    QCOMPARE(CXX17Namespace::A::B::C::D::staticMetaObject.enumerator(0).name(), "NamEn");
    QCOMPARE(QMetaEnum::fromType<CXX17Namespace::A::B::C::D::NamEn>().name(), "NamEn");
    QCOMPARE(QMetaEnum::fromType<CXX17Namespace::A::B::C::D::NamEn>().keyCount(), 1);
    QCOMPARE(QMetaEnum::fromType<CXX17Namespace::A::B::C::D::NamEn>().value(0), 4);

    QCOMPARE(CXX17Namespace::A::B::C::D::ClassInNamespace::staticMetaObject.className(),
             "CXX17Namespace::A::B::C::D::ClassInNamespace");
    QCOMPARE(CXX17Namespace::A::B::C::D::ClassInNamespace::staticMetaObject.enumeratorCount(), 1);
    QCOMPARE(CXX17Namespace::A::B::C::D::ClassInNamespace::staticMetaObject.enumerator(0).name(), "GadEn");
    QCOMPARE(QMetaEnum::fromType<CXX17Namespace::A::B::C::D::ClassInNamespace::GadEn>().name(), "GadEn");
    QCOMPARE(QMetaEnum::fromType<CXX17Namespace::A::B::C::D::ClassInNamespace::GadEn>().keyCount(), 1);
    QCOMPARE(QMetaEnum::fromType<CXX17Namespace::A::B::C::D::ClassInNamespace::GadEn>().value(0), 3);
}

void tst_Moc::cxxAttributes()
{
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    auto so = CppAttribute::staticMetaObject;
QT_WARNING_POP
    QCOMPARE(so.className(), "CppAttribute");
    QCOMPARE(so.enumeratorCount(), 0);
    QVERIFY(so.indexOfSignal("deprecatedSignal") != 1);
    for (auto a: {"deprecatedSlot", "deprecatedSlot2", "deprecatedReason", "deprecatedReasonWithLBRACK",
                  "deprecatedReasonWith2LBRACK", "deprecatedReasonWithRBRACK", "deprecatedReasonWith2RBRACK",
                  "slotWithArguments"
#if !defined(_MSC_VER) || _MSC_VER >= 1912
                  , "noreturnSlot", "noreturnSlot2", "returnInt", "noreturnDeprecatedSlot",
                  "noreturnSlot3"
#endif
                  }) {
        QVERIFY(so.indexOfSlot(a) != 1);
    }

    QCOMPARE(TestQNamespaceDeprecated::staticMetaObject.enumeratorCount(), 2);
    checkEnum(TestQNamespaceDeprecated::staticMetaObject.enumerator(0), "TestEnum1",
                {{"Key1", 11}, {"Key2", 12}, {"Key3", 13}, {"Key4", 14}, {"Key5", 15}, {"Key6", 16},
                 {"Key7", 17}});
    checkEnum(TestQNamespaceDeprecated::staticMetaObject.enumerator(1), "TestFlag1",
                {{"None", 0}, {"Flag1", 1}, {"Flag2", 2}, {"Flag3", 3}, {"Any", 1 | 2 | 3}});

    QCOMPARE(TestQNamespaceDeprecated::TestGadget::staticMetaObject.enumeratorCount(), 1);
    checkEnum(TestQNamespaceDeprecated::TestGadget::staticMetaObject.enumerator(0), "TestGEnum1",
                {{"Key1", 13}, {"Key2", 14}, {"Key3", 15}});

    QMetaEnum meta = QMetaEnum::fromType<TestQNamespaceDeprecated::TestEnum1>();
    QVERIFY(meta.isValid());
    QCOMPARE(meta.name(), "TestEnum1");
    QCOMPARE(meta.enclosingMetaObject(), &TestQNamespaceDeprecated::staticMetaObject);
    QCOMPARE(meta.keyCount(), 7);
}

void tst_Moc::mocJsonOutput()
{
    const auto readFile = [](const QString &fileName) {
        QFile f(fileName);
        f.open(QIODevice::ReadOnly);
        return QJsonDocument::fromJson(f.readAll());
    };

    QString actualFile = QStringLiteral(":/allmocs.json");
    QString expectedFile = QStringLiteral(":/allmocs_baseline.json");
    if (!QFile::exists(actualFile)) {
        // TODO: necessary with cmake as we cannot generate the qrc file soon enough
        auto const appDir = QCoreApplication::applicationDirPath();
        actualFile = appDir + QDir::separator() + QLatin1String("./allmocs.json");
        expectedFile = appDir + QDir::separator() + QLatin1String("./allmocs_baseline.json");
    }

    QVERIFY2(QFile::exists(actualFile), qPrintable(actualFile));
    QVERIFY2(QFile::exists(expectedFile), qPrintable(expectedFile));

    QJsonDocument actualOutput = readFile(actualFile);
    QJsonDocument expectedOutput = readFile(expectedFile);

    const auto showPotentialDiff = [](const QJsonDocument &actual, const QJsonDocument &expected) -> QByteArray {
#if defined(Q_OS_UNIX)
        QByteArray actualStr = actual.toJson();
        QByteArray expectedStr = expected.toJson();

        QTemporaryFile actualFile;
        if (!actualFile.open())
            return "Error opening actual temp file";
        actualFile.write(actualStr);
        actualFile.flush();

        QTemporaryFile expectedFile;
        if (!expectedFile.open())
            return "Error opening expected temp file";
        expectedFile.write(expectedStr);
        expectedFile.flush();

        QProcess diffProc;
        diffProc.setProgram("diff");
        diffProc.setArguments(QStringList() << "-ub" << expectedFile.fileName() << actualFile.fileName());
        diffProc.start();
        if (!diffProc.waitForStarted())
            return "Error waiting for diff process to start.";
        if (!diffProc.waitForFinished())
            return "Error waiting for diff process to finish.";
        return diffProc.readAllStandardOutput();
#else
        Q_UNUSED(actual);
        Q_UNUSED(expected);
        return "Cannot launch diff. Please check allmocs.json and allmocs_baseline.json on disk.";
#endif
    };

    QVERIFY2(actualOutput == expectedOutput, showPotentialDiff(actualOutput, expectedOutput).constData());
}

void TestFwdProperties::setProp1(const FwdClass1 &v)
{
    prop1.reset(new FwdClass1(v));
}
void TestFwdProperties::setProp2(const FwdClass2 &v)
{
    prop2.reset(new FwdClass2(v));
}
void TestFwdProperties::setProp3(const FwdClass3 &v)
{
    prop3.reset(new FwdClass3(v));
}
TestFwdProperties::~TestFwdProperties() {}

Q_DECLARE_METATYPE(FwdClass1);

void tst_Moc::mocInclude()
{
    TestFwdProperties obj;
    obj.setProperty("prop1", QVariant::fromValue(FwdClass1 { 45 }));
    QCOMPARE(obj.prop1->x, 45);
}

class RequiredTest :public QObject
{
    Q_OBJECT

    Q_PROPERTY(int required MEMBER m_required REQUIRED)
    Q_PROPERTY(int notRequired MEMBER m_notRequired)

private:
    int m_required;
    int m_notRequired;
};

void tst_Moc::requiredProperties()
{
    QMetaObject mo = RequiredTest::staticMetaObject;
    QMetaProperty required = mo.property(mo.indexOfProperty("required"));
    QVERIFY(required.isValid());
    QVERIFY(required.isRequired());
    QMetaProperty notRequired = mo.property(mo.indexOfProperty("notRequired"));
    QVERIFY(notRequired.isValid());
    QVERIFY(!notRequired.isRequired());
}

class ClassWithQPropertyMembers : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int publicProperty MEMBER publicProperty BINDABLE bindablePublicProperty
               NOTIFY publicPropertyChanged)
    Q_PROPERTY(int privateExposedProperty MEMBER privateExposedProperty)
public:

signals:
    void publicPropertyChanged();

public:
    QBindable<int> bindablePublicProperty() { return QBindable<int>(&publicProperty); }
    Q_OBJECT_BINDABLE_PROPERTY(ClassWithQPropertyMembers, int, publicProperty, &ClassWithQPropertyMembers::publicPropertyChanged);
    QProperty<int> notExposed;


protected:
    QProperty<int> protectedProperty;

private:
    QProperty<int> privateProperty;
    QProperty<int> privateExposedProperty;
};

void tst_Moc::qpropertyMembers()
{
    const auto metaObject = &ClassWithQPropertyMembers::staticMetaObject;

    QCOMPARE(metaObject->propertyCount() - metaObject->superClass()->propertyCount(), 2);

    QCOMPARE(metaObject->indexOfProperty("notExposed"), -1);

    QMetaProperty prop = metaObject->property(metaObject->indexOfProperty("publicProperty"));
    QVERIFY(prop.isValid());

    QVERIFY(metaObject->property(metaObject->indexOfProperty("privateExposedProperty")).isValid());

    ClassWithQPropertyMembers instance;

    prop.write(&instance, 42);
    QCOMPARE(instance.publicProperty.value(), 42);

    QSignalSpy publicPropertySpy(&instance, SIGNAL(publicPropertyChanged()));

    instance.publicProperty.setValue(100);
    QCOMPARE(prop.read(&instance).toInt(), 100);
    QCOMPARE(publicPropertySpy.size(), 1);

    QCOMPARE(prop.metaType(), QMetaType(QMetaType::Int));

    QVERIFY(prop.notifySignal().isValid());
}



void tst_Moc::observerMetaCall()
{
    const auto metaObject = &ClassWithQPropertyMembers::staticMetaObject;
    QMetaProperty prop = metaObject->property(metaObject->indexOfProperty("publicProperty"));
    QVERIFY(prop.isValid());

    ClassWithQPropertyMembers instance;

    int observerCallCount = 0;


    auto observer = [&observerCallCount]() {
        ++observerCallCount;
    };

    auto bindable = prop.bindable(&instance);
    QVERIFY(bindable.isBindable());

    auto handler = bindable.onValueChanged(observer);

    QCOMPARE(observerCallCount, 0);
    instance.publicProperty.setValue(100);
    QCOMPARE(observerCallCount, 1);
    instance.publicProperty.setValue(101);
    QCOMPARE(observerCallCount, 2);
}



void tst_Moc::setQPRopertyBinding()
{
    const auto metaObject = &ClassWithQPropertyMembers::staticMetaObject;
    QMetaProperty prop = metaObject->property(metaObject->indexOfProperty("publicProperty"));
    QVERIFY(prop.isValid());

    ClassWithQPropertyMembers instance;

    bool bindingCalled = false;
    auto binding = Qt::makePropertyBinding([&bindingCalled]() {
        bindingCalled = true;
        return 42;
    });

    auto bindable = prop.bindable(&instance);
    QVERIFY(bindable.isBindable());
    bindable.setBinding(binding);

    QCOMPARE(instance.publicProperty.value(), 42);
    QVERIFY(bindingCalled); // but now it should've been called :)
}

class ClassWithPrivateQPropertyShim :public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(int testProperty READ testProperty WRITE setTestProperty
               BINDABLE bindableTestProperty NOTIFY testPropertyChanged)
    Q_PROPERTY(int testProperty2 READ testProperty2 WRITE setTestProperty2
               BINDABLE bindableTestProperty2)
    //Q_PROPERTY(d_func(), int, lazyTestProperty, setLazyTestProperty, NOTIFY lazyTestPropertyChanged)

signals:
    void testPropertyChanged();
    void lazyTestPropertyChanged();
public:

    int testProperty() const { return priv.testProperty; }
    void setTestProperty(int val) { priv.testProperty = val; }
    int testProperty2() const { return priv.testProperty2; }
    void setTestProperty2(int val) { priv.testProperty2 = val; }

    QBindable<int> bindableTestProperty() { return QBindable<int>(&priv.testProperty); }
    QBindable<int> bindableTestProperty2() { return QBindable<int>(&priv.testProperty2); }

    struct Private {
        Private(ClassWithPrivateQPropertyShim *pub)
            : q(pub)
        {}

        QBindingStorage bindingStorage;

        ClassWithPrivateQPropertyShim *q = nullptr;

        void onTestPropertyChanged() { q->testPropertyChanged(); }
        Q_OBJECT_BINDABLE_PROPERTY(Private, int, testProperty, &Private::onTestPropertyChanged);
        QProperty<int> testProperty2;
    };
    Private priv{this};

    Private *d_func() { return &priv; }
    const Private *d_func() const { return &priv; }
};

inline const QBindingStorage *qGetBindingStorage(const ClassWithPrivateQPropertyShim::Private *o)
{
    return &o->bindingStorage;
}
inline QBindingStorage *qGetBindingStorage(ClassWithPrivateQPropertyShim::Private *o)
{
    return &o->bindingStorage;
}

void tst_Moc::privateQPropertyShim()
{
    ClassWithPrivateQPropertyShim testObject;

    {
        auto metaObject = &ClassWithPrivateQPropertyShim::staticMetaObject;
        QMetaProperty prop = metaObject->property(metaObject->indexOfProperty("testProperty"));
        QVERIFY(prop.isValid());
        QVERIFY(prop.notifySignal().isValid());
    }

    testObject.priv.testProperty.setValue(42);
    QCOMPARE(testObject.property("testProperty").toInt(), 42);

    // Behave like a QProperty
    QVERIFY(!testObject.bindableTestProperty().hasBinding());
    testObject.bindableTestProperty().setBinding([]() { return 100; });
    QCOMPARE(testObject.testProperty(), 100);
    QVERIFY(testObject.bindableTestProperty().hasBinding());

    // Old style setter getters
    testObject.setTestProperty(400);
    QVERIFY(!testObject.bindableTestProperty().hasBinding());
    QCOMPARE(testObject.testProperty(), 400);

    // moc generates correct code for plain QProperty in PIMPL
    testObject.setTestProperty2(42);
    QCOMPARE(testObject.priv.testProperty2.value(), 42);
}


class BindableOnly : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int score BINDABLE scoreBindable READ default WRITE default)
public:
    BindableOnly(QObject *parent = nullptr)
        : QObject(parent)
        , m_score(4)
    {}
    QBindable<int> scoreBindable() { return QBindable<int>(&m_score); }
private:
    QProperty<int> m_score;
};

class BindableAndNotifyable : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int score BINDABLE scoreBindable NOTIFY scoreChanged READ default WRITE default)
public:
    BindableAndNotifyable(QObject *parent = nullptr)
        : QObject(parent)
        , m_score(4)
    {}
    QBindable<int> scoreBindable() { return QBindable<int>(&m_score); }
signals:
    void scoreChanged();
private:
    QProperty<int> m_score;
};

void tst_Moc::readWriteThroughBindable()
{
    {
        BindableOnly o;
        QCOMPARE(o.scoreBindable().value(), 4);
        QCOMPARE(o.property("score").toInt(), 4);
        o.scoreBindable().setValue(5);
        QCOMPARE(o.scoreBindable().value(), 5);
        QCOMPARE(o.property("score").toInt(), 5);
        const QMetaObject *mo = o.metaObject();
        const int i = mo->indexOfProperty("score");
        QVERIFY(i > 0);
        QMetaProperty p = mo->property(i);
        QCOMPARE(p.name(), "score");
        QVERIFY(p.isValid());
        QVERIFY(p.isWritable());
        QCOMPARE(p.read(&o), 5);
        QVERIFY(o.setProperty("score", 6));
        QCOMPARE(o.property("score").toInt(), 6);
        QVERIFY(p.write(&o, 7));
        QCOMPARE(p.read(&o), 7);
    }
    {
        BindableAndNotifyable o;
        QCOMPARE(o.scoreBindable().value(), 4);
        QCOMPARE(o.property("score").toInt(), 4);
        o.scoreBindable().setValue(5);
        QCOMPARE(o.scoreBindable().value(), 5);
        QCOMPARE(o.property("score").toInt(), 5);
        const QMetaObject *mo = o.metaObject();
        const int i = mo->indexOfProperty("score");
        QVERIFY(i > 0);
        QMetaProperty p = mo->property(i);
        QCOMPARE(p.name(), "score");
        QVERIFY(p.isValid());
        QVERIFY(p.isWritable());
        QCOMPARE(p.read(&o), 5);
        QVERIFY(o.setProperty("score", 6));
        QCOMPARE(o.property("score").toInt(), 6);
        QVERIFY(p.write(&o, 7));
        QCOMPARE(p.read(&o), 7);
    }
}

struct WithInvokableCtor
{
    Q_GADGET
    Q_PROPERTY(int thing MEMBER m_thing CONSTANT FINAL)
public:
    WithInvokableCtor() = default;
    Q_INVOKABLE WithInvokableCtor(int theThing) : m_thing(theThing) {}

    int m_thing = 10;
};

void tst_Moc::invokableCtors()
{
    const QMetaType metaType = QMetaType::fromType<WithInvokableCtor>();
    Q_ASSERT(metaType.sizeOf() > 0);
    const QMetaObject *metaObject = metaType.metaObject();
    QVERIFY(metaObject);

    QCOMPARE(metaObject->constructorCount(), 1);
    WithInvokableCtor *result = nullptr;
    const auto guard = qScopeGuard([&]() { delete result; });
    int argument = 17;
    void *a[] = { &result, &argument };
    metaObject->static_metacall(QMetaObject::CreateInstance, 0, a);
    QVERIFY(result);
    QCOMPARE(result->m_thing, 17);

    // Call dtor so that we're left with "uninitialized" memory.
    WithInvokableCtor result2;
    result2.~WithInvokableCtor();

    void *b[] = { &result2, &argument };
    metaObject->static_metacall(QMetaObject::ConstructInPlace, 0, b);
    QCOMPARE(result2.m_thing, 17);
}

void tst_Moc::virtualInlineTaggedSlot()
{
    auto mo = TagTest::staticMetaObject;
    auto idx = mo.indexOfMethod("pamOpen(int)");
    auto method = mo.method(idx);
    QVERIFY(method.isValid()); // fails!
    QCOMPARE(method.tag(), "Q_NOREPLY");
    idx = mo.indexOfMethod("test()");
    method = mo.method(idx);
    QVERIFY(method.isValid());
    QCOMPARE(method.tag(), "Q_NOREPLY");
    QCOMPARE(method.returnMetaType(), QMetaType::fromType<int>());
}

void tst_Moc::tokenStartingWithNumber()
{
    auto *mo  = &TokenStartingWithNumber::staticMetaObject;
    int index = mo->indexOfEnumerator("FooItems");
    QMetaEnum metaEnum = mo->enumerator(index);
    QVERIFY(metaEnum.isValid());
    QCOMPARE(metaEnum.keyCount(), 3);
}

QTEST_MAIN(tst_Moc)

// the generated code must compile with QT_NO_KEYWORDS
#undef signals
#undef slots
#undef emit

#include "tst_moc.moc"
