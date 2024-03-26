// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <private/qqmlpropertycache_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmlcontextdata_p.h>
#include <QCryptographicHash>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlpropertycache : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlpropertycache() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void properties();
    void propertiesDerived();
    void revisionedProperties();
    void methods();
    void methodsDerived();
    void signalHandlers();
    void signalHandlersDerived();
    void passForeignEnums();
    void passQGadget();
    void metaObjectSize_data();
    void metaObjectSize();
    void metaObjectChecksum();
    void metaObjectsForRootElements();
    void derivedGadgetMethod();
    void restrictRegistrationVersion();
    void rejectOverriddenFinal();

private:
    QQmlEngine engine;
};

class BaseGadget
{
    Q_GADGET
    QML_ANONYMOUS
public:
    Q_INVOKABLE QString stringValue() { return QLatin1String("base"); }
};

class DerivedGadget : public BaseGadget
{
    Q_GADGET
    QML_ANONYMOUS
public:
    Q_INVOKABLE QString stringValue() { return QLatin1String("derived"); }
};

class GadgetUser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(BaseGadget base READ base CONSTANT)
    Q_PROPERTY(DerivedGadget derived READ derived CONSTANT)
    Q_PROPERTY(QString baseString READ baseString WRITE setBaseString NOTIFY baseStringChanged)
    Q_PROPERTY(QString derivedString READ derivedString WRITE setDerivedString NOTIFY derivedStringChanged)
    QML_ELEMENT

public:
    BaseGadget base() const { return m_base; }
    DerivedGadget derived() const { return m_derived; }
    QString baseString() const { return m_baseString; }
    QString derivedString() const { return m_derivedString; }

public slots:
    void setBaseString(QString baseString)
    {
        if (m_baseString == baseString)
            return;

        m_baseString = baseString;
        emit baseStringChanged(m_baseString);
    }

    void setDerivedString(QString derivedString)
    {
        if (m_derivedString == derivedString)
            return;

        m_derivedString = derivedString;
        emit derivedStringChanged(m_derivedString);
    }

signals:
    void baseStringChanged(QString baseString);
    void derivedStringChanged(QString derivedString);

private:
    BaseGadget m_base;
    DerivedGadget m_derived;
    QString m_baseString;
    QString m_derivedString;
};

class BaseObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int propertyA READ propertyA NOTIFY propertyAChanged)
    Q_PROPERTY(QString propertyB READ propertyB NOTIFY propertyBChanged)
    Q_PROPERTY(int highVersion READ highVersion WRITE setHighVersion NOTIFY highVersionChanged REVISION(4, 0))
    Q_PROPERTY(int finalProp READ finalProp CONSTANT FINAL)

public:
    BaseObject(QObject *parent = nullptr) : QObject(parent) {}

    int propertyA() const { return 0; }
    QString propertyB() const { return QString(); }
    int highVersion() const { return m_highVersion; }
    int finalProp() const { return 8; }

public Q_SLOTS:
    void slotA() {}

    void setHighVersion(int highVersion)
    {
        if (m_highVersion == highVersion)
            return;

        m_highVersion = highVersion;
        emit highVersionChanged();
    }

Q_SIGNALS:
    void propertyAChanged();
    void propertyBChanged();
    void signalA();
    void highVersionChanged();

private:
    int m_highVersion = 0;
};

class DerivedObject : public BaseObject
{
    Q_OBJECT
    Q_PROPERTY(int propertyC READ propertyC NOTIFY propertyCChanged)
    Q_PROPERTY(QString propertyD READ propertyD NOTIFY propertyDChanged)
    Q_PROPERTY(int propertyE READ propertyE NOTIFY propertyEChanged REVISION 1)
    Q_PROPERTY(int finalProp READ finalProp CONSTANT) // bad!
public:
    DerivedObject(QObject *parent = nullptr) : BaseObject(parent) {}

    int propertyC() const { return 0; }
    QString propertyD() const { return QString(); }
    int propertyE() const { return 0; }
    int finalProp() const { return 9; }

public Q_SLOTS:
    void slotB() {}

Q_SIGNALS:
    void propertyCChanged();
    void propertyDChanged();
    Q_REVISION(1) void propertyEChanged();
    void signalB();
};

const QQmlPropertyData *cacheProperty(const QQmlPropertyCache::ConstPtr &cache, const char *name)
{
    return cache->property(QLatin1String(name), nullptr, nullptr);
}

void tst_qqmlpropertycache::properties()
{
    QQmlEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr cache = QQmlPropertyCache::createStandalone(metaObject);
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cache, "propertyA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyA"));

    QVERIFY((data = cacheProperty(cache, "propertyB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyB"));

    QVERIFY((data = cacheProperty(cache, "propertyC")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyC"));

    QVERIFY((data = cacheProperty(cache, "propertyD")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyD"));
}

void tst_qqmlpropertycache::propertiesDerived()
{
    QQmlEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr parentCache
            = QQmlPropertyCache::createStandalone(&BaseObject::staticMetaObject);
    QQmlPropertyCache::ConstPtr cache
            = parentCache->copyAndAppend(object.metaObject(), QTypeRevision());
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cache, "propertyA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyA"));

    QVERIFY((data = cacheProperty(cache, "propertyB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyB"));

    QVERIFY((data = cacheProperty(cache, "propertyC")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyC"));

    QVERIFY((data = cacheProperty(cache, "propertyD")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfProperty("propertyD"));
}

void tst_qqmlpropertycache::revisionedProperties()
{
    // Check that if you create a QQmlPropertyCache from a QMetaObject together
    // with an explicit revision, the cache will then, and only then, report a
    // property with a matching revision as available.
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr cacheWithoutVersion(
                QQmlPropertyCache::createStandalone(metaObject));
    QQmlPropertyCache::ConstPtr cacheWithVersion(
                QQmlPropertyCache::createStandalone(
                    metaObject, QTypeRevision::fromMinorVersion(1)));
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cacheWithoutVersion, "propertyE")));
    QCOMPARE(cacheWithoutVersion->isAllowedInRevision(data), false);
    QCOMPARE(cacheWithVersion->isAllowedInRevision(data), true);
}

void tst_qqmlpropertycache::methods()
{
    QQmlEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr cache(QQmlPropertyCache::createStandalone(metaObject));
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cache, "slotA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("slotA()"));

    QVERIFY((data = cacheProperty(cache, "slotB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("slotB()"));

    QVERIFY((data = cacheProperty(cache, "signalA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalA()"));

    QVERIFY((data = cacheProperty(cache, "signalB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalB()"));

    QVERIFY((data = cacheProperty(cache, "propertyAChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY((data = cacheProperty(cache, "propertyBChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY((data = cacheProperty(cache, "propertyCChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY((data = cacheProperty(cache, "propertyDChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyDChanged()"));
}

void tst_qqmlpropertycache::methodsDerived()
{
    QQmlEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr parentCache(
                QQmlPropertyCache::createStandalone(&BaseObject::staticMetaObject));
    QQmlPropertyCache::ConstPtr cache
            = parentCache->copyAndAppend(object.metaObject(), QTypeRevision {});
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cache, "slotA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("slotA()"));

    QVERIFY((data = cacheProperty(cache, "slotB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("slotB()"));

    QVERIFY((data = cacheProperty(cache, "signalA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalA()"));

    QVERIFY((data = cacheProperty(cache, "signalB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalB()"));

    QVERIFY((data = cacheProperty(cache, "propertyAChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY((data = cacheProperty(cache, "propertyBChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY((data = cacheProperty(cache, "propertyCChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY((data = cacheProperty(cache, "propertyDChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyDChanged()"));
}

void tst_qqmlpropertycache::signalHandlers()
{
    QQmlEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr cache(QQmlPropertyCache::createStandalone(metaObject));
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cache, "onSignalA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalA()"));

    QVERIFY((data = cacheProperty(cache, "onSignalB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalB()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyAChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyBChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyCChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyDChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyDChanged()"));
}

void tst_qqmlpropertycache::signalHandlersDerived()
{
    QQmlEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QQmlPropertyCache::ConstPtr parentCache(
                QQmlPropertyCache::createStandalone(&BaseObject::staticMetaObject));
    QQmlPropertyCache::ConstPtr cache
            = parentCache->copyAndAppend(object.metaObject(), QTypeRevision{});
    const QQmlPropertyData *data;

    QVERIFY((data = cacheProperty(cache, "onSignalA")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalA()"));

    QVERIFY((data = cacheProperty(cache, "onSignalB")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("signalB()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyAChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyBChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyCChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY((data = cacheProperty(cache, "onPropertyDChanged")));
    QCOMPARE(data->coreIndex(), metaObject->indexOfMethod("propertyDChanged()"));
}

class MyEnum : public QObject
 {
    Q_OBJECT
 public:
     enum Option1Flag {
         Option10 = 0,
         Option1A = 1,
         Option1B = 2,
         Option1C = 4,
         Option1D = 8,
         Option1E = 16,
         Option1F = 32,
         Option1AD = Option1A | Option1D,
     };
     Q_DECLARE_FLAGS(Option1, Option1Flag)
     Q_FLAG(Option1)

     enum ShortEnum: quint16 {
         Short0  = 0,
         Short8  = 0xff,
         Short16 = 0xffff
     };
     Q_ENUM(ShortEnum);
};

class MyData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyEnum::Option1 opt1 READ opt1 WRITE setOpt1 NOTIFY opt1Changed)
    Q_PROPERTY(MyEnum::ShortEnum opt2 READ opt2 WRITE setOpt2 NOTIFY opt2Changed)
public:
    MyEnum::Option1 opt1() const { return m_opt1;  }
    MyEnum::ShortEnum opt2() const { return m_opt2;  }

signals:
    void opt1Changed(MyEnum::Option1 opt1);
    void opt2Changed(MyEnum::ShortEnum opt2);

public slots:
    void setOpt1(MyEnum::Option1 opt1)
    {
        QCOMPARE(opt1, MyEnum::Option1AD);
        if (opt1 != m_opt1) {
            m_opt1 = opt1;
            emit opt1Changed(opt1);
        }
    }

    void setOpt2(MyEnum::ShortEnum opt2)
    {
        QCOMPARE(opt2, MyEnum::Short16);
        if (opt2 != m_opt2) {
            m_opt2 = opt2;
            emit opt2Changed(opt2);
        }
    }

    void setOption1(MyEnum::Option1 opt1) { setOpt1(opt1); }
    void setOption2(MyEnum::ShortEnum opt2) { setOpt2(opt2); }

private:
    MyEnum::Option1 m_opt1 = MyEnum::Option10;
    MyEnum::ShortEnum m_opt2 = MyEnum::Short8;
};

void tst_qqmlpropertycache::passForeignEnums()
{
    qmlRegisterType<MyEnum>("example", 1, 0, "MyEnum");
    qmlRegisterType<MyData>("example", 1, 0, "MyData");

    MyEnum myenum;
    MyData data;

    engine.rootContext()->setContextProperty("myenum", &myenum);
    engine.rootContext()->setContextProperty("mydata", &data);

    QQmlComponent component(&engine, testFile("foreignEnums.qml"));
    QVERIFY(component.isReady());

    QScopedPointer<QObject> obj(component.create(engine.rootContext()));
    QVERIFY(!obj.isNull());
    QCOMPARE(data.opt1(), MyEnum::Option1AD);
    QCOMPARE(data.opt2(), MyEnum::Short16);
}

Q_DECLARE_METATYPE(MyEnum::Option1)
Q_DECLARE_METATYPE(MyEnum::ShortEnum)

QT_BEGIN_NAMESPACE
class SimpleGadget
{
    Q_GADGET
    Q_PROPERTY(bool someProperty READ someProperty)
public:
    bool someProperty() const { return true; }
};

// Avoids NeedsCreation and NeedsDestruction flags
Q_DECLARE_TYPEINFO(SimpleGadget, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

class GadgetEmitter : public QObject
{
    Q_OBJECT
signals:
    void emitGadget(SimpleGadget);
};

void tst_qqmlpropertycache::passQGadget()
{
    qRegisterMetaType<SimpleGadget>();

    GadgetEmitter emitter;
    engine.rootContext()->setContextProperty("emitter", &emitter);
    QQmlComponent component(&engine, testFile("passQGadget.qml"));
    QVERIFY(component.isReady());

    QScopedPointer<QObject> obj(component.create(engine.rootContext()));
    QVariant before = obj->property("result");
    QVERIFY(before.isNull());
    emit emitter.emitGadget(SimpleGadget());
    QVariant after = obj->property("result");
    QCOMPARE(after.typeId(), QMetaType::Bool);
    QVERIFY(after.toBool());
}

class TestClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop READ prop WRITE setProp NOTIFY propChanged)
    int m_prop;

public:
    enum MyEnum {
        First, Second
    };
    Q_ENUM(MyEnum)

    Q_CLASSINFO("Foo", "Bar")

    TestClass() {}

    int prop() const
    {
        return m_prop;
    }

public slots:
    void setProp(int prop)
    {
        if (m_prop == prop)
            return;

        m_prop = prop;
        emit propChanged(prop);
    }
signals:
    void propChanged(int prop);
};

class TestClassWithParameters : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void slotWithArguments(int firstArg) {
        Q_UNUSED(firstArg);
    }
};

class TestClassWithClassInfo : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("Key", "Value")
};

#include "tst_qqmlpropertycache.moc"

#define ARRAY_SIZE(arr) \
    int(sizeof(arr) / sizeof(arr[0]))

template <typename T, typename = void>
struct SizeofOffsetsAndSizes_helper
{
    static constexpr size_t value = sizeof(T::offsetsAndSize); // old moc
};

template <typename T>
struct SizeofOffsetsAndSizes_helper<T, std::void_t<decltype(T::offsetsAndSizes)>>
{
    static constexpr size_t value = sizeof(T::offsetsAndSizes); // new moc
};

template <typename T>
constexpr size_t sizeofOffsetsAndSizes(const T &)
{
    return SizeofOffsetsAndSizes_helper<T>::value;
}

#define TEST_CLASS(Class) \
    QTest::newRow(#Class) \
            << &Class::staticMetaObject << ARRAY_SIZE(qt_meta_data_CLASS##Class##ENDCLASS) \
            << int(sizeofOffsetsAndSizes(qt_meta_stringdata_CLASS##Class##ENDCLASS) / (sizeof(uint) * 2))

Q_DECLARE_METATYPE(const QMetaObject*);

void tst_qqmlpropertycache::metaObjectSize_data()
{
    QTest::addColumn<const QMetaObject*>("metaObject");
    QTest::addColumn<int>("expectedFieldCount");
    QTest::addColumn<int>("expectedStringCount");

    TEST_CLASS(TestClass);
    TEST_CLASS(TestClassWithParameters);
    TEST_CLASS(TestClassWithClassInfo);
}

void tst_qqmlpropertycache::metaObjectSize()
{
    QFETCH(const QMetaObject *, metaObject);
    QFETCH(int, expectedFieldCount);
    QFETCH(int, expectedStringCount);

    int size = 0;
    int stringDataSize = 0;
    bool valid = QQmlPropertyCache::determineMetaObjectSizes(*metaObject, &size, &stringDataSize);
    QVERIFY(valid);

    QCOMPARE(size, expectedFieldCount - 1); // Remove trailing zero field until fixed in moc.
    QCOMPARE(stringDataSize, expectedStringCount);
}

void tst_qqmlpropertycache::metaObjectChecksum()
{
    QMetaObjectBuilder builder;
    builder.setClassName("Test");
    builder.addClassInfo("foo", "bar");

    QCryptographicHash hash(QCryptographicHash::Md5);

    QScopedPointer<QMetaObject, QScopedPointerPodDeleter> mo(builder.toMetaObject());
    QVERIFY(!mo.isNull());

    QVERIFY(QQmlPropertyCache::addToHash(hash, *mo.data()));
    QByteArray initialHash = hash.result();
    QVERIFY(!initialHash.isEmpty());
    hash.reset();

    {
        QVERIFY(QQmlPropertyCache::addToHash(hash, *mo.data()));
        QByteArray nextHash = hash.result();
        QVERIFY(!nextHash.isEmpty());
        hash.reset();
        QCOMPARE(initialHash, nextHash);
    }

    builder.addProperty("testProperty", "int", -1);

    mo.reset(builder.toMetaObject());
    {
        QVERIFY(QQmlPropertyCache::addToHash(hash, *mo.data()));
        QByteArray nextHash = hash.result();
        QVERIFY(!nextHash.isEmpty());
        hash.reset();
        QVERIFY(initialHash != nextHash);
    }
}

void tst_qqmlpropertycache::metaObjectsForRootElements()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("noDuckType.qml"));
    QVERIFY(c.isReady());
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("result").toString(), QString::fromLatin1("good"));
}

void tst_qqmlpropertycache::derivedGadgetMethod()
{
    metaObjectsForRootElements();

    qmlRegisterTypesAndRevisions<BaseGadget, DerivedGadget, GadgetUser>("Test.PropertyCache", 1);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("derivedGadgetMethod.qml"));
    QVERIFY(c.isReady());
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());
    GadgetUser *gadgetUser = qobject_cast<GadgetUser *>(obj.data());
    QVERIFY(gadgetUser);
    QCOMPARE(gadgetUser->baseString(), QString::fromLatin1("base"));
    QCOMPARE(gadgetUser->derivedString(), QString::fromLatin1("derived"));
}

void tst_qqmlpropertycache::restrictRegistrationVersion()
{
    qmlRegisterTypesAndRevisions<BaseObject>("Test.PropertyCache", 3);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("highVersion.qml"));
    QVERIFY(c.isError());
}

void tst_qqmlpropertycache::rejectOverriddenFinal()
{
    qmlRegisterTypesAndRevisions<BaseObject>("Test.PropertyCache", 3);
    QQmlEngine engine;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                             "Final member finalProp is overridden in class BaseObject_QML_.* "
                             "The override won't be used."));

    QQmlComponent c(&engine, testFileUrl("finalProp.qml"));
    QVERIFY2(!c.isError(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                             "finalProp.qml:15: TypeError: Property 'finalProp' of object "
                             "BaseObject_QML_.* is not a function"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                             "finalProp.qml:11: TypeError: Property 'finalProp' of object "
                             "BaseObject_QML_.* is not a function"));

    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("a").toInt(), 8);

    DerivedObject *derived = new DerivedObject(o.data());
    QTest::ignoreMessage(QtWarningMsg,
                         "Final member finalProp is overridden in class DerivedObject. "
                         "The override won't be used.");
    o->setProperty("obj", QVariant::fromValue(derived));
    QCOMPARE(derived->finalProp(), 9);

    // rejects override of final property
    QCOMPARE(o->property("a").toInt(), 8);

    // Cannot override final property with method, either
    QCOMPARE(o->property("b").toInt(), 8);

    // Cannot call the method overridding a final property
    QCOMPARE(o->property("c").toInt(), 0);
}

QTEST_MAIN(tst_qqmlpropertycache)
