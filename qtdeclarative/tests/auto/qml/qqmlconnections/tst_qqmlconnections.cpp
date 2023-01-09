/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <private/qqmlconnections_p.h>
#include <private/qquickitem_p.h>
#include "../../shared/util.h"
#include <QtQml/qqmlscriptstring.h>

class tst_qqmlconnections : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlconnections();

private slots:
    void defaultValues();
    void properties();

    void connection_data() { prefixes(); }
    void connection();

    void trimming_data() { prefixes(); }
    void trimming();

    void targetChanged_data() { prefixes(); };
    void targetChanged();

    void unknownSignals_data();
    void unknownSignals();

    void errors_data();
    void errors();

    void rewriteErrors_data() { prefixes(); }
    void rewriteErrors();

    void singletonTypeTarget_data() { prefixes(); }
    void singletonTypeTarget();

    void enableDisable_QTBUG_36350_data() { prefixes(); }
    void enableDisable_QTBUG_36350();

    void disabledAtStart_data() { prefixes(); }
    void disabledAtStart();

    void clearImplicitTarget_data() { prefixes(); }
    void clearImplicitTarget();
    void onWithoutASignal();

    void noAcceleratedGlobalLookup_data() { prefixes(); }
    void noAcceleratedGlobalLookup();

    void bindToPropertyWithUnderscoreChangeHandler();

private:
    QQmlEngine engine;
    void prefixes();
};

tst_qqmlconnections::tst_qqmlconnections()
{
}

void tst_qqmlconnections::prefixes()
{
    QTest::addColumn<QString>("prefix");
    QTest::newRow("functions") << QString("functions");
    QTest::newRow("bindings") << QString("bindings");
}

void tst_qqmlconnections::defaultValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-connection3.qml"));
    QQmlConnections *item = qobject_cast<QQmlConnections*>(c.create());

    QVERIFY(item != nullptr);
    QVERIFY(!item->target());

    delete item;
}

void tst_qqmlconnections::properties()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-connection2.qml"));
    QQmlConnections *item = qobject_cast<QQmlConnections*>(c.create());

    QVERIFY(item != nullptr);

    QVERIFY(item != nullptr);
    QCOMPARE(item->target(), item);

    delete item;
}

void tst_qqmlconnections::connection()
{
    QFETCH(QString, prefix);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/test-connection.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(c.create());

    QVERIFY(item != nullptr);

    QCOMPARE(item->property("tested").toBool(), false);
    QCOMPARE(item->width(), 50.);
    emit item->setWidth(100.);
    QCOMPARE(item->width(), 100.);
    QCOMPARE(item->property("tested").toBool(), true);

    delete item;
}

void tst_qqmlconnections::trimming()
{
    QFETCH(QString, prefix);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/trimming.qml"));
    QObject *object = c.create();

    QVERIFY(object != nullptr);

    QCOMPARE(object->property("tested").toString(), QString(""));
    int index = object->metaObject()->indexOfSignal("testMe(int,QString)");
    QMetaMethod method = object->metaObject()->method(index);
    method.invoke(object,
                  Qt::DirectConnection,
                  Q_ARG(int, 5),
                  Q_ARG(QString, "worked"));
    QCOMPARE(object->property("tested").toString(), QString("worked5"));

    delete object;
}

// Confirm that target can be changed by one of our signal handlers
void tst_qqmlconnections::targetChanged()
{
    QFETCH(QString, prefix);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/connection-targetchange.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(c.create());
    QVERIFY(item != nullptr);

    QQmlConnections *connections = item->findChild<QQmlConnections*>("connections");
    QVERIFY(connections);

    QQuickItem *item1 = item->findChild<QQuickItem*>("item1");
    QVERIFY(item1);

    item1->setWidth(200);

    QQuickItem *item2 = item->findChild<QQuickItem*>("item2");
    QVERIFY(item2);
    QCOMPARE(connections->target(), item2);

    // If we don't crash then we're OK

    delete item;
}

void tst_qqmlconnections::unknownSignals_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("error");

    QTest::newRow("functions/basic")    << "functions/connection-unknownsignals.qml" << ":6:30: QML Connections: Detected function \"onFooBar\" in Connections element. This is probably intended to be a signal handler but no signal of the target matches the name.";
    QTest::newRow("functions/parent")   << "functions/connection-unknownsignals-parent.qml" << ":4:30: QML Connections: Detected function \"onFooBar\" in Connections element. This is probably intended to be a signal handler but no signal of the target matches the name.";
    QTest::newRow("functions/ignored")  << "functions/connection-unknownsignals-ignored.qml" << ""; // should be NO error
    QTest::newRow("functions/notarget") << "functions/connection-unknownsignals-notarget.qml" << ""; // should be NO error

    QTest::newRow("bindings/basic")    << "bindings/connection-unknownsignals.qml" << ":6:30: QML Connections: Cannot assign to non-existent property \"onFooBar\"";
    QTest::newRow("bindings/parent")   << "bindings/connection-unknownsignals-parent.qml" << ":4:30: QML Connections: Cannot assign to non-existent property \"onFooBar\"";
    QTest::newRow("bindings/ignored")  << "bindings/connection-unknownsignals-ignored.qml" << ""; // should be NO error
    QTest::newRow("bindings/notarget") << "bindings/connection-unknownsignals-notarget.qml" << ""; // should be NO error
}

void tst_qqmlconnections::unknownSignals()
{
    QFETCH(QString, file);
    QFETCH(QString, error);

    QUrl url = testFileUrl(file);
    if (!error.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, (url.toString() + error).toLatin1());
    } else {
        // QTest has no way to insist no message (i.e. fail)
    }

    QQmlEngine engine;
    QQmlComponent c(&engine, url);
    QObject *object = c.create();
    QVERIFY(object != nullptr);

    // check that connection is created (they are all runtime errors)
    QQmlConnections *connections = object->findChild<QQmlConnections*>("connections");
    QVERIFY(connections);

    if (file == "connection-unknownsignals-ignored.qml")
        QVERIFY(connections->ignoreUnknownSignals());

    delete object;
}

void tst_qqmlconnections::errors_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("error");

    QTest::newRow("no \"on\"") << "error-property.qml" << "Cannot assign to non-existent property \"fakeProperty\"";
    QTest::newRow("3rd letter lowercase") << "error-property2.qml" << "Cannot assign to non-existent property \"onfakeProperty\"";
    QTest::newRow("child object") << "error-object.qml" << "Connections: nested objects not allowed";
    QTest::newRow("grouped object") << "error-syntax.qml" << "Connections: syntax error";
}

void tst_qqmlconnections::errors()
{
    QFETCH(QString, file);
    QFETCH(QString, error);

    QUrl url = testFileUrl(file);

    QQmlEngine engine;
    QQmlComponent c(&engine, url);
    QVERIFY(c.isError());
    QList<QQmlError> errors = c.errors();
    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.at(0).description(), error);
}

class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ran READ ran WRITE setRan)

public:
    TestObject(QObject *parent = nullptr) : QObject(parent), m_ran(false) {}
    ~TestObject() {}

    bool ran() const { return m_ran; }
    void setRan(bool arg) { m_ran = arg; }

signals:
    void unnamedArgumentSignal(int a, qreal, QString c);
    void signalWithGlobalName(int parseInt);

private:
    bool m_ran;
};

void tst_qqmlconnections::rewriteErrors()
{
    QFETCH(QString, prefix);
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl(prefix + "/rewriteError-unnamed.qml"));
        QTest::ignoreMessage(QtWarningMsg, (c.url().toString() + ":5:35: QML Connections: Signal uses unnamed parameter followed by named parameter.").toLatin1());
        TestObject *obj = qobject_cast<TestObject*>(c.create());
        QVERIFY(obj != nullptr);
        obj->unnamedArgumentSignal(1, .5, "hello");
        QCOMPARE(obj->ran(), false);

        delete obj;
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl(prefix + "/rewriteError-global.qml"));
        QTest::ignoreMessage(QtWarningMsg, (c.url().toString() + ":5:35: QML Connections: Signal parameter \"parseInt\" hides global variable.").toLatin1());
        TestObject *obj = qobject_cast<TestObject*>(c.create());
        QVERIFY(obj != nullptr);

        obj->signalWithGlobalName(10);
        QCOMPARE(obj->ran(), false);

        delete obj;
    }
}


class MyTestSingletonType : public QObject
{
Q_OBJECT
Q_PROPERTY(int intProp READ intProp WRITE setIntProp NOTIFY intPropChanged)

public:
    MyTestSingletonType(QObject *parent = nullptr) : QObject(parent), m_intProp(0), m_changeCount(0) {}
    ~MyTestSingletonType() {}

    Q_INVOKABLE int otherMethod(int val) { return val + 4; }

    int intProp() const { return m_intProp; }
    void setIntProp(int val)
    {
        if (++m_changeCount % 3 == 0) emit otherSignal();
        m_intProp = val; emit intPropChanged();
    }

signals:
    void intPropChanged();
    void otherSignal();

private:
    int m_intProp;
    int m_changeCount;
};

static QObject *module_api_factory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
   Q_UNUSED(engine)
   Q_UNUSED(scriptEngine)
   MyTestSingletonType *api = new MyTestSingletonType();
   return api;
}

// QTBUG-20937
void tst_qqmlconnections::singletonTypeTarget()
{
    QFETCH(QString, prefix);
    qmlRegisterSingletonType<MyTestSingletonType>("MyTestSingletonType", 1, 0, "Api", module_api_factory);
    QQmlComponent component(&engine, testFileUrl(prefix + "/singletontype-target.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("moduleIntPropChangedCount").toInt(), 0);
    QCOMPARE(object->property("moduleOtherSignalCount").toInt(), 0);

    QMetaObject::invokeMethod(object, "setModuleIntProp");
    QCOMPARE(object->property("moduleIntPropChangedCount").toInt(), 1);
    QCOMPARE(object->property("moduleOtherSignalCount").toInt(), 0);

    QMetaObject::invokeMethod(object, "setModuleIntProp");
    QCOMPARE(object->property("moduleIntPropChangedCount").toInt(), 2);
    QCOMPARE(object->property("moduleOtherSignalCount").toInt(), 0);

    // the singleton Type emits otherSignal every 3 times the int property changes.
    QMetaObject::invokeMethod(object, "setModuleIntProp");
    QCOMPARE(object->property("moduleIntPropChangedCount").toInt(), 3);
    QCOMPARE(object->property("moduleOtherSignalCount").toInt(), 1);

    delete object;
}

void tst_qqmlconnections::enableDisable_QTBUG_36350()
{
    QFETCH(QString, prefix);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/test-connection.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(c.create());
    QVERIFY(item != nullptr);

    QQmlConnections *connections = item->findChild<QQmlConnections*>("connections");
    QVERIFY(connections);

    connections->setEnabled(false);
    QCOMPARE(item->property("tested").toBool(), false);
    QCOMPARE(item->width(), 50.);
    emit item->setWidth(100.);
    QCOMPARE(item->width(), 100.);
    QCOMPARE(item->property("tested").toBool(), false); //Should not have received signal to change property

    connections->setEnabled(true); //Re-enable the connectSignals()
    QCOMPARE(item->property("tested").toBool(), false);
    QCOMPARE(item->width(), 100.);
    emit item->setWidth(50.);
    QCOMPARE(item->width(), 50.);
    QCOMPARE(item->property("tested").toBool(), true); //Should have received signal to change property

    delete item;
}

void tst_qqmlconnections::disabledAtStart()
{
    QFETCH(QString, prefix);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/disabled-at-start.qml"));
    QObject * const object = c.create();

    QVERIFY(object != nullptr);

    QCOMPARE(object->property("tested").toBool(), false);
    const int index = object->metaObject()->indexOfSignal("testMe()");
    const QMetaMethod method = object->metaObject()->method(index);
    method.invoke(object, Qt::DirectConnection);
    QCOMPARE(object->property("tested").toBool(), false);

    delete object;
}

//QTBUG-56499
void tst_qqmlconnections::clearImplicitTarget()
{
    QFETCH(QString, prefix);
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/test-connection-implicit.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(c.create());

    QVERIFY(item != nullptr);

    // normal case: fire Connections
    item->setWidth(100.);
    QCOMPARE(item->property("tested").toBool(), true);

    item->setProperty("tested", false);
    // clear the implicit target
    QQmlConnections *connections = item->findChild<QQmlConnections*>();
    QVERIFY(connections);
    connections->setTarget(nullptr);

    // target cleared: no longer fire Connections
    item->setWidth(150.);
    QCOMPARE(item->property("tested").toBool(), false);

    delete item;
}

void tst_qqmlconnections::onWithoutASignal()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("connection-no-signal-name.qml"));
    QVERIFY(c.isError()); // Cannot assign to non-existent property "on" expected
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));
    QVERIFY(item == nullptr); // should parse error, and not give us an item (or crash).
}

class Proxy : public QObject
{
    Q_OBJECT
public:
    enum MyEnum { EnumValue = 20, AnotherEnumValue };
    Q_ENUM(MyEnum)

signals:
    void someSignal();
};

void tst_qqmlconnections::noAcceleratedGlobalLookup()
{
    QFETCH(QString, prefix);
    qRegisterMetaType<Proxy::MyEnum>();
    qmlRegisterType<Proxy>("test.proxy", 1, 0, "Proxy");
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl(prefix + "/override-proxy-type.qml"));
    QVERIFY(c.isReady());
    QScopedPointer<QObject> object(c.create());
    const QVariant val = object->property("testEnum");
    QCOMPARE(val.type(), QVariant::Int);
    QCOMPARE(val.toInt(), int(Proxy::EnumValue));
}

void tst_qqmlconnections::bindToPropertyWithUnderscoreChangeHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("underscore.qml"));
    QScopedPointer<QObject> root {component.create()};
    QVERIFY(root);
    QQmlProperty underscoreProperty(root.get(), "__underscore_property");
    QVERIFY(underscoreProperty.isValid());
    underscoreProperty.write(42);
    QVERIFY(root->property("sanityCheck").toBool());
    QVERIFY(root->property("success").toBool());
}

QTEST_MAIN(tst_qqmlconnections)

#include "tst_qqmlconnections.moc"
