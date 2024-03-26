// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugutil_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmlbinding_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmldebugservice_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qqmlenginedebugclient_p.h>

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlproperty.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuick/qquickitem.h>

#include <QtNetwork/qhostaddress.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>

#define QVERIFYOBJECT(statement) \
    do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__)) {\
    return QQmlEngineDebugObjectReference();\
    }\
    } while (0)

class NonScriptProperty : public QObject {
    Q_OBJECT
    Q_PROPERTY(int nonScriptProp READ nonScriptProp WRITE setNonScriptProp NOTIFY nonScriptPropChanged SCRIPTABLE false)
public:
    int nonScriptProp() const { return 0; }
    void setNonScriptProp(int) {}
signals:
    void nonScriptPropChanged();
};
QML_DECLARE_TYPE(NonScriptProperty)

class CustomTypes : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QModelIndex modelIndex READ modelIndex)
public:
    CustomTypes(QObject *parent = nullptr) : QObject(parent) {}

    QModelIndex modelIndex() { return QModelIndex(); }
};

class JsonTest : public QObject
{
   Q_OBJECT
   Q_PROPERTY(QJsonObject data READ data WRITE setData NOTIFY dataChanged)

public:
   JsonTest(QObject *parent = nullptr) : QObject(parent)
   {
      m_data["foo"] = QJsonValue(12);
      m_data["ttt"] = QJsonArray({4, 5, 4, 3, 2});
      m_data["a"] = QJsonValue(QJsonValue::Null);
      m_data["b"] = QJsonValue(QJsonValue::Undefined);
      m_data["c"] = QJsonValue("fffff");
   }

   QJsonObject data() const { return m_data; }

signals:
   void dataChanged(const QJsonObject &data);

public slots:
   void setData(const QJsonObject &data)
   {
       if (data != m_data) {
           m_data = data;
           emit dataChanged(data);
       }
   }

private:
   QJsonObject m_data;
};


class tst_QQmlEngineDebugService : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQmlEngineDebugService()
         : QQmlDataTest(QT_QMLTEST_DATADIR)
         , m_conn(nullptr)
         , m_engine(nullptr)
         , m_dbg(nullptr)
         , m_rootItem(nullptr)
    {
        qmlRegisterType<NonScriptProperty>("Test", 1, 0, "NonScriptPropertyElement");
        qmlRegisterType<CustomTypes>("Backend", 1, 0, "CustomTypes");
        qmlRegisterType<JsonTest>("JsonTest", 1, 0, "JsonTest");
        QTest::ignoreMessage(QtDebugMsg, "QML Debugger: Waiting for connection on port 3768...");
    }

private:
    QQmlEngineDebugObjectReference findRootObject(int context = 0,
                                           bool recursive = false);
    QQmlEngineDebugPropertyReference findProperty(
            const QList<QQmlEngineDebugPropertyReference> &props,
            const QString &name) const;

    void recursiveObjectTest(QObject *o,
                             const QQmlEngineDebugObjectReference &oref,
                             bool recursive) const;

    void getContexts();

    std::unique_ptr<QQmlDebugConnection> m_conn;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QQmlContext> m_context;

    std::vector<std::unique_ptr<QObject>> m_components;

    QQmlEngineDebugClient *m_dbg;
    QQuickItem *m_rootItem;

private slots:
    void initTestCase() override;

    void watch_property();
    void watch_object();
    void watch_expression();
    void watch_expression_data();
    void watch_context();
    void watch_file();
    void debuggerCrashOnAttach();

    void queryAvailableEngines();
    void queryRootContexts();
    void queryObject();
    void queryObject_data();
    void queryObjectsForLocation();
    void queryObjectsForLocation_data();
    void queryExpressionResult();
    void queryExpressionResult_data();
    void queryExpressionResultInRootContext();
    void queryExpressionResultBC();
    void queryExpressionResultBC_data();

    void setBindingForObject();
    void resetBindingForObject();
    void setMethodBody();
    void queryObjectTree();
    void setBindingInStates();

    void regression_QTCREATORBUG_7451();
    void queryObjectWithNonStreamableTypes();
    void jsonData();
    void asynchronousCreate();
    void invalidContexts();
    void createObjectOnDestruction();
    void fetchValueType();
};

QQmlEngineDebugObjectReference tst_QQmlEngineDebugService::findRootObject(
        int context, bool recursive)
{
    bool success = false;
    m_dbg->queryAvailableEngines(&success);
    QVERIFYOBJECT(success);
    QVERIFYOBJECT(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QVERIFYOBJECT(m_dbg->engines().size());
    m_dbg->queryRootContexts(m_dbg->engines()[0], &success);
    QVERIFYOBJECT(success);
    QVERIFYOBJECT(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QVERIFYOBJECT(m_dbg->rootContext().contexts.size());
    QVERIFYOBJECT(m_dbg->rootContext().contexts.last().objects.size());
    int count = m_dbg->rootContext().contexts.size();
    recursive ? m_dbg->queryObjectRecursive(m_dbg->rootContext().contexts[count - context - 1].objects[0],
                                            &success) :
                m_dbg->queryObject(m_dbg->rootContext().contexts[count - context - 1].objects[0], &success);
    QVERIFYOBJECT(success);
    QVERIFYOBJECT(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    return m_dbg->object();
}

QQmlEngineDebugPropertyReference tst_QQmlEngineDebugService::findProperty(
        const QList<QQmlEngineDebugPropertyReference> &props, const QString &name) const
{
    for (const QQmlEngineDebugPropertyReference &p : props) {
        if (p.name == name)
            return p;
    }
    return QQmlEngineDebugPropertyReference();
}

void tst_QQmlEngineDebugService::recursiveObjectTest(
        QObject *o, const QQmlEngineDebugObjectReference &oref, bool recursive) const
{
    const QMetaObject *meta = o->metaObject();

    QCOMPARE(oref.debugId, QQmlDebugService::idForObject(o));
    QCOMPARE(oref.name, o->objectName());
    QCOMPARE(oref.className, QQmlMetaType::prettyTypeName(o));
    QCOMPARE(oref.contextDebugId, QQmlDebugService::idForObject(
                 qmlContext(o)));

    const QObjectList &children = o->children();
    for (int i=0; i<children.size(); i++) {
        QObject *child = children[i];
        if (!qmlContext(child))
            continue;
        int debugId = QQmlDebugService::idForObject(child);
        QVERIFY(debugId >= 0);

        QQmlEngineDebugObjectReference cref;
        for (const QQmlEngineDebugObjectReference &ref : oref.children) {
            QVERIFY(!ref.className.isEmpty());
            if (ref.debugId == debugId) {
                cref = ref;
                break;
            }
        }
        QVERIFY(cref.debugId >= 0);

        if (recursive)
            recursiveObjectTest(child, cref, true);
    }

    for (const QQmlEngineDebugPropertyReference &p : oref.properties) {
        QCOMPARE(p.objectDebugId, QQmlDebugService::idForObject(o));

        // signal properties are fake - they are generated from QQmlAbstractBoundSignal children
        if (p.name.startsWith("on") && p.name.size() > 2 && p.name[2].isUpper()) {
            QString signal = p.value.toString();
            QQmlBoundSignalExpression *expr = QQmlPropertyPrivate::signalExpression(QQmlProperty(o, p.name));
            QVERIFY(expr && expr->expression() == signal);
            QVERIFY(p.valueTypeName.isEmpty());
            QVERIFY(p.binding.isEmpty());
            QVERIFY(!p.hasNotifySignal);
            continue;
        }

        QMetaProperty pmeta = meta->property(meta->indexOfProperty(p.name.toUtf8().constData()));

        QCOMPARE(p.name, QString::fromUtf8(pmeta.name()));

        if (pmeta.userType() == QMetaType::QObjectStar) {
            const QQmlEngineDebugObjectReference ref
                    = qvariant_cast<QQmlEngineDebugObjectReference>(p.value);
            QObject *pobj = qvariant_cast<QObject *>(pmeta.read(o));
            if (pobj) {
                if (pobj->objectName().isEmpty())
                    QCOMPARE(ref.name, QString("<unnamed object>"));
                else
                    QCOMPARE(ref.name, pobj->objectName());
            } else {
                QCOMPARE(ref.name, QString("<unknown value>"));
            }
        } else if (pmeta.userType() < QMetaType::User && pmeta.userType() != QMetaType::QVariant) {
            const QVariant expected = pmeta.read(o);
            QVariant value = p.value;
            QMetaType expectedType = expected.metaType();
            if (value != expected && p.value.canConvert(expectedType))
               value.convert(expectedType);
            QVERIFY2(value == expected, QString::fromLatin1("%1 != %2. Details: %3/%4/%5/%6")
                     .arg(p.value.toString(), expected.toString(), p.name, p.valueTypeName)
                     .arg(pmeta.userType()).arg(pmeta.userType()).toUtf8());
        }

        if (p.name == "parent")
            QVERIFY(p.valueTypeName == "QGraphicsObject*" ||
                    p.valueTypeName == "QQuickItem*");
        else
            QCOMPARE(p.valueTypeName, QString::fromUtf8(pmeta.typeName()));

        QQmlAbstractBinding *binding =
                QQmlPropertyPrivate::binding(
                    QQmlProperty(o, p.name));
        if (binding)
            QCOMPARE(binding->expression(), p.binding);

        QCOMPARE(p.hasNotifySignal, pmeta.hasNotifySignal());

        QVERIFY(pmeta.isValid());
    }
}

void tst_QQmlEngineDebugService::getContexts()
{
    bool success = false;

    m_dbg->queryAvailableEngines(&success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QList<QQmlEngineDebugEngineReference> engines = m_dbg->engines();
    QCOMPARE(engines.size(), 1);
    m_dbg->queryRootContexts(engines.first(), &success);

    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
}

void tst_QQmlEngineDebugService::initTestCase()
{
    QQmlDataTest::initTestCase();

    m_components.clear();
    m_engine = std::make_unique<QQmlEngine>(this);

    // The contents of these files was previously hardcoded as QList<QByteArray>
    // directly in this test, but that fails on Android, because the required
    // dependencies are not deployed. When the contents is moved to separate
    // files, qmlimportscanner is capable of providing all the necessary
    // dependencies.
    // Note that the order of the files in this list matters! The test-cases
    // expect Qml components to be created is certain order.
    constexpr const char *fileNames[] = {
        "complexItem.qml",
        "emptyItem.qml",
        "itemWithFunctions.qml",
        "rectangleWithTransitions.qml",
        "customTypes.qml",
        "jsonTest.qml",
        "debuggerCrashOnAttach.qml"
    };

    for (auto file : fileNames) {
        QQmlComponent component(m_engine.get(), testFileUrl(file));
        QVERIFY(component.isReady());  // fails if bad syntax
        m_components.push_back(std::unique_ptr<QObject>(component.create()));
    }
    m_rootItem = qobject_cast<QQuickItem*>(m_components.at(0).get());

    // add an extra context to test for multiple contexts
    m_context = std::make_unique<QQmlContext>(m_engine->rootContext(), this);
    m_context->setObjectName("tst_QQmlDebug_childContext");

    m_conn = std::make_unique<QQmlDebugConnection>(this);
    m_conn->connectToHost("127.0.0.1", 3768);

    bool ok = m_conn->waitForConnected();
    QVERIFY(ok);
    m_dbg = new QQmlEngineDebugClient(m_conn.get());
    const QList<QQmlDebugClient *> others = QQmlDebugTest::createOtherClients(m_conn.get());
    QTRY_COMPARE(m_dbg->state(), QQmlEngineDebugClient::Enabled);
    for (QQmlDebugClient *other : others)
        QCOMPARE(other->state(), QQmlDebugClient::Unavailable);
    qDeleteAll(others);
}

void tst_QQmlEngineDebugService::setMethodBody()
{
    bool success;
    QQmlEngineDebugObjectReference obj = findRootObject(2);
    QVERIFY(!obj.className.isEmpty());

    QObject *root = m_components.at(2).get();
    // Without args
    {
    QVariant rv;
    QVERIFY(QMetaObject::invokeMethod(root, "myMethodNoArgs", Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariant, rv)));
    QCOMPARE(rv, QVariant(qreal(3)));


    QVERIFY(m_dbg->setMethodBody(obj.debugId, "myMethodNoArgs", "return 7",
                                 &success));
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QVERIFY(QMetaObject::invokeMethod(root, "myMethodNoArgs", Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariant, rv)));
    QCOMPARE(rv, QVariant(qreal(7)));
    }

    // With args
    {
    QVariant rv;
    QVERIFY(QMetaObject::invokeMethod(root, "myMethod", Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariant, rv), Q_ARG(QVariant, QVariant(19))));
    QCOMPARE(rv, QVariant(qreal(28)));

    QVERIFY(m_dbg->setMethodBody(obj.debugId, "myMethod", "return a + 7",
                                 &success));
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QVERIFY(QMetaObject::invokeMethod(root, "myMethod", Qt::DirectConnection,
                                      Q_RETURN_ARG(QVariant, rv), Q_ARG(QVariant, QVariant(19))));
    QCOMPARE(rv, QVariant(qreal(26)));
    }
}

void tst_QQmlEngineDebugService::watch_property()
{
    QQmlEngineDebugObjectReference obj = findRootObject();
    QVERIFY(!obj.className.isEmpty());
    QQmlEngineDebugPropertyReference prop = findProperty(obj.properties, "width");

    bool success;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->addWatch(prop, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->addWatch(QQmlEngineDebugPropertyReference(), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), false);

    quint32 id = m_dbg->addWatch(prop, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    QSignalSpy spy(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant)));

    int origWidth = m_rootItem->property("width").toInt();
    m_rootItem->setProperty("width", origWidth*2);

    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant))));
    QCOMPARE(spy.size(), 1);

    m_dbg->removeWatch(id, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    // restore original value and verify spy doesn't get additional signal since watch has been removed
    m_rootItem->setProperty("width", origWidth);
    QTest::qWait(100);
    QCOMPARE(spy.size(), 1);

    QCOMPARE(spy.at(0).at(0).value<QByteArray>(), prop.name.toUtf8());
    QCOMPARE(spy.at(0).at(1).value<QVariant>(), QVariant::fromValue(origWidth*2));
}

void tst_QQmlEngineDebugService::watch_object()
{
    QQmlEngineDebugObjectReference obj = findRootObject();
    QVERIFY(!obj.className.isEmpty());

    bool success;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->addWatch(obj, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->addWatch(QQmlEngineDebugObjectReference(), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), false);

    quint32 id = m_dbg->addWatch(obj, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    QSignalSpy spy(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant)));

    int origWidth = m_rootItem->property("width").toInt();
    int origHeight = m_rootItem->property("height").toInt();

    m_rootItem->setProperty("width", origWidth*2);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant))));
    m_rootItem->setProperty("height", origHeight*2);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant))));

    QVERIFY(spy.size() > 0);

    int newWidth = -1;
    int newHeight = -1;
    for (int i=0; i<spy.size(); i++) {
        const QVariantList &values = spy[i];
        if (values[0].value<QByteArray>() == "width")
            newWidth = values[1].value<QVariant>().toInt();
        else if (values[0].value<QByteArray>() == "height")
            newHeight = values[1].value<QVariant>().toInt();

    }

    m_dbg->removeWatch(id, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    // since watch has been removed, restoring the original values should not trigger a valueChanged()
    spy.clear();
    m_rootItem->setProperty("width", origWidth);
    m_rootItem->setProperty("height", origHeight);
    QTest::qWait(100);
    QCOMPARE(spy.size(), 0);

    QCOMPARE(newWidth, origWidth * 2);
    QCOMPARE(newHeight, origHeight * 2);
}

void tst_QQmlEngineDebugService::watch_expression()
{
    QFETCH(QString, expr);
    QFETCH(int, increment);
    QFETCH(int, incrementCount);

    int origWidth = m_rootItem->property("width").toInt();

    QQmlEngineDebugObjectReference obj = findRootObject();
    QVERIFY(!obj.className.isEmpty());

    bool success;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->addWatch(obj, expr, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->addWatch(QQmlEngineDebugObjectReference(), expr, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), false);

    quint32 id = m_dbg->addWatch(obj, expr, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    QSignalSpy spy(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant)));

    int width = origWidth;
    for (int i=0; i<incrementCount+1; i++) {
        if (i > 0) {
            width += increment;
            m_rootItem->setProperty("width", width);
            QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(valueChanged(QByteArray,QVariant))));
        }
    }

    m_dbg->removeWatch(id, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    // restore original value and verify spy doesn't get a signal since watch has been removed
    m_rootItem->setProperty("width", origWidth);
    QTest::qWait(100);
    QCOMPARE(spy.size(), incrementCount);

    width = origWidth + increment;
    for (int i=0; i<spy.size(); i++) {
        width += increment;
        QCOMPARE(spy.at(i).at(1).value<QVariant>().toInt(), width);
    }
}

void tst_QQmlEngineDebugService::watch_expression_data()
{
    QTest::addColumn<QString>("expr");
    QTest::addColumn<int>("increment");
    QTest::addColumn<int>("incrementCount");

    QTest::newRow("width") << "width" << 0 << 0;
    QTest::newRow("width+10") << "width + 10" << 10 << 5;
}

void tst_QQmlEngineDebugService::watch_context()
{
    QQmlEngineDebugContextReference c;
    QTest::ignoreMessage(QtWarningMsg, "QQmlEngineDebugClient::addWatch(): Not implemented");
    bool success;
    m_dbg->addWatch(c, QString(), &success);
    QVERIFY(!success);
}

void tst_QQmlEngineDebugService::watch_file()
{
    QQmlEngineDebugFileReference f;
    QTest::ignoreMessage(QtWarningMsg, "QQmlEngineDebugClient::addWatch(): Not implemented");
    bool success;
    m_dbg->addWatch(f, &success);
    QVERIFY(!success);
}

void tst_QQmlEngineDebugService::queryAvailableEngines()
{
    bool success;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->queryAvailableEngines(&success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->queryAvailableEngines(&success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    // TODO test multiple engines
    const QList<QQmlEngineDebugEngineReference> engines = m_dbg->engines();
    QCOMPARE(engines.size(), 1);

    for (const QQmlEngineDebugEngineReference &e : engines) {
        QCOMPARE(e.debugId, QQmlDebugService::idForObject(m_engine.get()));
        QCOMPARE(e.name, m_engine->objectName());
    }
}

void tst_QQmlEngineDebugService::queryRootContexts()
{
    bool success;
    m_dbg->queryAvailableEngines(&success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QVERIFY(m_dbg->engines().size());
    const QQmlEngineDebugEngineReference engine =  m_dbg->engines()[0];

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->queryRootContexts(engine, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->queryRootContexts(engine, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QQmlContext *actualContext = m_engine->rootContext();
    QQmlEngineDebugContextReference context = m_dbg->rootContext();
    QCOMPARE(context.debugId, QQmlDebugService::idForObject(actualContext));
    QCOMPARE(context.name, actualContext->objectName());

    // root context query sends only root object data - it doesn't fill in
    // the children or property info
    QCOMPARE(context.objects.size(), 0);
    QCOMPARE(context.contexts.size(), 8);
    QVERIFY(context.contexts[0].debugId >= 0);
    QCOMPARE(context.contexts[0].name, QString("tst_QQmlDebug_childContext"));
}

void tst_QQmlEngineDebugService::queryObject()
{
    QFETCH(bool, recursive);

    bool success;

    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    recursive ? unconnected->queryObjectRecursive(rootObject, &success) : unconnected->queryObject(rootObject, &success);
    QVERIFY(!success);
    delete unconnected;

    recursive ? m_dbg->queryObjectRecursive(rootObject, &success) : m_dbg->queryObject(rootObject, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    const QQmlEngineDebugObjectReference obj = m_dbg->object();
    QVERIFY(!obj.className.isEmpty());

    // check source as defined in main()
    QQmlEngineDebugFileReference source = obj.source;
    QCOMPARE(source.url, testFileUrl("complexItem.qml"));
    QCOMPARE(source.lineNumber, 6); // because of license header
    QCOMPARE(source.columnNumber, 1);

    // generically test all properties, children and childrens' properties
    recursiveObjectTest(m_rootItem, obj, recursive);

    if (recursive) {
        for (const QQmlEngineDebugObjectReference &child : obj.children) {
            QVERIFY(!child.className.isEmpty());
            QVERIFY(child.properties.size() > 0);
        }

        QQmlEngineDebugObjectReference rect;
        QQmlEngineDebugObjectReference text;
        for (const QQmlEngineDebugObjectReference &child : obj.children) {
            QVERIFY(!child.className.isEmpty());
            if (child.className == "Rectangle")
                rect = child;
            else if (child.className == "Text")
                text = child;
        }

        // test specific property values
        QCOMPARE(findProperty(rect.properties, "width").value, QVariant::fromValue(500));
        QCOMPARE(findProperty(rect.properties, "height").value, QVariant::fromValue(600));
        QVariant expected = findProperty(rect.properties, "color").value;
        expected.convert(QMetaType::fromType<QColor>());
        QCOMPARE(expected , QVariant::fromValue(QColor("blue")));
    } else {
        for (const QQmlEngineDebugObjectReference &child : obj.children) {
            QVERIFY(!child.className.isEmpty());
            QCOMPARE(child.properties.size(), 0);
        }
    }
}

void tst_QQmlEngineDebugService::queryObject_data()
{
    QTest::addColumn<bool>("recursive");

    QTest::newRow("non-recursive") << false;
    QTest::newRow("recursive") << true;
}

void tst_QQmlEngineDebugService::queryObjectsForLocation()
{
    QFETCH(bool, recursive);

    bool success;

    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());

    const QString fileName = QFileInfo(rootObject.source.url.toString()).fileName();
    int lineNumber = rootObject.source.lineNumber;
    int columnNumber = rootObject.source.columnNumber;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    recursive ? unconnected->queryObjectsForLocationRecursive(fileName, lineNumber,
                                                              columnNumber, &success)
              : unconnected->queryObjectsForLocation(fileName, lineNumber,
                                                     columnNumber, &success);
    QVERIFY(!success);
    delete unconnected;

    recursive ? m_dbg->queryObjectsForLocationRecursive(fileName, lineNumber,
                                                      columnNumber, &success)
              : m_dbg->queryObjectsForLocation(fileName, lineNumber,
                                             columnNumber, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QCOMPARE(m_dbg->objects().size(), 1);
    const QQmlEngineDebugObjectReference obj = m_dbg->objects().first();
    QVERIFY(!obj.className.isEmpty());

    // check source as defined in main()
    QQmlEngineDebugFileReference source = obj.source;
    QCOMPARE(source.url, testFileUrl(fileName));
    QCOMPARE(source.lineNumber, lineNumber);
    QCOMPARE(source.columnNumber, columnNumber);

    // generically test all properties, children and childrens' properties
    recursiveObjectTest(m_rootItem, obj, recursive);

    if (recursive) {
        for (const QQmlEngineDebugObjectReference &child : obj.children) {
            QVERIFY(!child.className.isEmpty());
            QVERIFY(child.properties.size() > 0);
        }

        QQmlEngineDebugObjectReference rect;
        QQmlEngineDebugObjectReference text;
        for (const QQmlEngineDebugObjectReference &child : obj.children) {
            QVERIFY(!child.className.isEmpty());
            if (child.className == "Rectangle")
                rect = child;
            else if (child.className == "Text")
                text = child;
        }

        // test specific property values
        QCOMPARE(findProperty(rect.properties, "width").value, QVariant::fromValue(500));
        QCOMPARE(findProperty(rect.properties, "height").value, QVariant::fromValue(600));
        QVariant expected = findProperty(rect.properties, "color").value;
        QMetaType colorMetatype = QMetaType::fromType<QColor>();
        QVERIFY(expected.canConvert(colorMetatype));
        expected.convert(colorMetatype);
        QCOMPARE(expected , QVariant::fromValue(QColor("blue")));
    } else {
        for (const QQmlEngineDebugObjectReference &child : obj.children) {
            QVERIFY(!child.className.isEmpty());
            QCOMPARE(child.properties.size(), 0);
        }
    }
}

void tst_QQmlEngineDebugService::queryObjectsForLocation_data()
{
    QTest::addColumn<bool>("recursive");

    QTest::newRow("non-recursive") << false;
    QTest::newRow("recursive") << true;
}

void tst_QQmlEngineDebugService::regression_QTCREATORBUG_7451()
{
    const QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    int contextId = rootObject.contextDebugId;
    QQmlContext *context = qobject_cast<QQmlContext *>(QQmlDebugService::objectForId(contextId));
    QQmlComponent component(context->engine());
    QByteArray content;
    content.append("import QtQuick 2.0\n"
               "Text {"
               "y: 10\n"
               "text: \"test\"\n"
               "}");
    component.setData(content, rootObject.source.url);
    QObject *object = component.create(context);
    QVERIFY(object);
    int idNew = QQmlDebugService::idForObject(object);
    QVERIFY(idNew >= 0);

    const QString fileName = QFileInfo(rootObject.source.url.toString()).fileName();
    int lineNumber = rootObject.source.lineNumber;
    int columnNumber = rootObject.source.columnNumber;
    bool success = false;

    m_dbg->queryObjectsForLocation(fileName, lineNumber,
                                        columnNumber, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    for (const QQmlEngineDebugObjectReference &child : rootObject.children) {
        QVERIFY(!child.className.isEmpty());
        success = false;
        lineNumber = child.source.lineNumber;
        columnNumber = child.source.columnNumber;
        m_dbg->queryObjectsForLocation(fileName, lineNumber,
                                       columnNumber, &success);
        QVERIFY(success);
        QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    }

    delete object;
    QObject *deleted = QQmlDebugService::objectForId(idNew);
    QVERIFY(!deleted);

    lineNumber = rootObject.source.lineNumber;
    columnNumber = rootObject.source.columnNumber;
    success = false;
    m_dbg->queryObjectsForLocation(fileName, lineNumber,
                                   columnNumber, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    for (const QQmlEngineDebugObjectReference &child : rootObject.children) {
        QVERIFY(!child.className.isEmpty());
        success = false;
        lineNumber = child.source.lineNumber;
        columnNumber = child.source.columnNumber;
        m_dbg->queryObjectsForLocation(fileName, lineNumber,
                                       columnNumber, &success);
        QVERIFY(success);
        QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    }
}

void tst_QQmlEngineDebugService::queryObjectWithNonStreamableTypes()
{
    bool success;

    QQmlEngineDebugObjectReference rootObject = findRootObject(4, true);
    QVERIFY(!rootObject.className.isEmpty());

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->queryObject(rootObject, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->queryObject(rootObject, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QQmlEngineDebugObjectReference obj = m_dbg->object();
    QVERIFY(!obj.className.isEmpty());

    QCOMPARE(findProperty(obj.properties, "modelIndex").value,
             QVariant(QLatin1String("QModelIndex()")));
}

void tst_QQmlEngineDebugService::jsonData()
{
    bool success;

    QQmlEngineDebugObjectReference rootObject = findRootObject(5, true);
    QVERIFY(!rootObject.className.isEmpty());

    m_dbg->queryObject(rootObject, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QQmlEngineDebugObjectReference obj = m_dbg->object();
    QVERIFY(!obj.className.isEmpty());

    QCOMPARE(findProperty(obj.properties, "data").value,
             QJsonDocument::fromJson("{\"a\":null,\"c\":\"fffff\",\"foo\":12,\"ttt\":[4,5,4,3,2]}")
             .toVariant());
}

void tst_QQmlEngineDebugService::queryExpressionResult()
{
    QFETCH(QString, expr);
    QFETCH(QVariant, result);

    int objectId = findRootObject().debugId;

    bool success;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->queryExpressionResult(objectId, expr, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->queryExpressionResult(objectId, expr, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QCOMPARE(m_dbg->resultExpr(), result);
}

void tst_QQmlEngineDebugService::queryExpressionResult_data()
{
    QTest::addColumn<QString>("expr");
    QTest::addColumn<QVariant>("result");

    QTest::newRow("width + 50") << "width + 50" << QVariant::fromValue(60);
    QTest::newRow("blueRect.width") << "blueRect.width" << QVariant::fromValue(500);
    QTest::newRow("bad expr") << "aeaef" << QVariant::fromValue(QString("<undefined>"));
    QTest::newRow("QObject*") << "varObj" << QVariant::fromValue(QString("<unnamed object>"));
    QTest::newRow("list of QObject*") << "varObjList" << QVariant::fromValue(QVariantList() << QVariant(QString("<unnamed object>")));
    QVariantMap map;
    map.insert(QLatin1String("rect"), QVariant(QLatin1String("<unnamed object>")));
    QTest::newRow("varObjMap") << "varObjMap" << QVariant::fromValue(map);
    QTest::newRow("simpleVar") << "simpleVar" << QVariant::fromValue(10.05);
}

void tst_QQmlEngineDebugService::queryExpressionResultInRootContext()
{
    bool success = false;
    m_dbg->queryAvailableEngines(&success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QVERIFY(m_dbg->engines().size());

    const QString exp = QLatin1String("1");
    m_dbg->queryExpressionResult(-1, exp, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QCOMPARE(m_dbg->resultExpr().toString(), exp);
}

void tst_QQmlEngineDebugService::queryExpressionResultBC()
{
    QFETCH(QString, expr);
    QFETCH(QVariant, result);

    int objectId = findRootObject().debugId;

    bool success;

    QQmlEngineDebugClient *unconnected = new QQmlEngineDebugClient(nullptr);
    unconnected->queryExpressionResultBC(objectId, expr, &success);
    QVERIFY(!success);
    delete unconnected;

    m_dbg->queryExpressionResultBC(objectId, expr, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    QCOMPARE(m_dbg->resultExpr(), result);
}

void tst_QQmlEngineDebugService::queryExpressionResultBC_data()
{
    QTest::addColumn<QString>("expr");
    QTest::addColumn<QVariant>("result");

    QTest::newRow("width + 50") << "width + 50" << QVariant::fromValue(60);
    QTest::newRow("blueRect.width") << "blueRect.width" << QVariant::fromValue(500);
    QTest::newRow("bad expr") << "aeaef" << QVariant::fromValue(QString("<undefined>"));
    QTest::newRow("QObject*") << "varObj" << QVariant::fromValue(QString("<unnamed object>"));
    QTest::newRow("list of QObject*") << "varObjList" << QVariant::fromValue(QVariantList() << QVariant(QString("<unnamed object>")));
    QVariantMap map;
    map.insert(QLatin1String("rect"), QVariant(QLatin1String("<unnamed object>")));
    QTest::newRow("varObjMap") << "varObjMap" << QVariant::fromValue(map);
    QTest::newRow("simpleVar") << "simpleVar" << QVariant::fromValue(10.05);
}

void tst_QQmlEngineDebugService::setBindingForObject()
{
    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    QVERIFY(rootObject.debugId != -1);
    QQmlEngineDebugPropertyReference widthPropertyRef = findProperty(rootObject.properties, "width");

    QCOMPARE(widthPropertyRef.value, QVariant(10));
    QCOMPARE(widthPropertyRef.binding, QString());

    bool success;
    //
    // set literal
    //
    m_dbg->setBindingForObject(rootObject.debugId, "width", "15", true,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    widthPropertyRef =  findProperty(rootObject.properties, "width");

    QCOMPARE(widthPropertyRef.value, QVariant(15));
    QCOMPARE(widthPropertyRef.binding, QString());

    //
    // set expression
    //
    m_dbg->setBindingForObject(rootObject.debugId, "width", "height", false,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    widthPropertyRef =  findProperty(rootObject.properties, "width");

    QCOMPARE(widthPropertyRef.value, QVariant(20));
    QEXPECT_FAIL("", "Cannot retrieve text for a binding (QTBUG-37273)", Continue);
    QCOMPARE(widthPropertyRef.binding, QString("height"));

    //
    // set handler
    //
    rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    QCOMPARE(rootObject.children.size(), 5); // Rectangle, Text, MouseArea, Component.onCompleted, NonScriptPropertyElement
    QQmlEngineDebugObjectReference mouseAreaObject = rootObject.children.at(2);
    QVERIFY(!mouseAreaObject.className.isEmpty());
    m_dbg->queryObjectRecursive(mouseAreaObject, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    mouseAreaObject = m_dbg->object();
    QCOMPARE(mouseAreaObject.className, QString("MouseArea"));

    QQmlEngineDebugPropertyReference onEnteredRef = findProperty(mouseAreaObject.properties, "onEntered");

    QCOMPARE(onEnteredRef.name, QString("onEntered"));
    // Sorry, can't do that anymore: QCOMPARE(onEnteredRef.value,  QVariant("{ console.log('hello') }"));
    QCOMPARE(onEnteredRef.value,  QVariant("function() { [native code] }"));

    m_dbg->setBindingForObject(mouseAreaObject.debugId, "onEntered",
                               "{console.log('hello, world') }", false,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    mouseAreaObject = rootObject.children.at(2);
    QVERIFY(!mouseAreaObject.className.isEmpty());
    m_dbg->queryObjectRecursive(mouseAreaObject, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    mouseAreaObject = m_dbg->object();
    QVERIFY(!mouseAreaObject.className.isEmpty());
    onEnteredRef = findProperty(mouseAreaObject.properties, "onEntered");
    QCOMPARE(onEnteredRef.name, QString("onEntered"));
    QCOMPARE(onEnteredRef.value, QVariant("function() { [native code] }"));
}

void tst_QQmlEngineDebugService::resetBindingForObject()
{
    QQmlEngineDebugObjectReference rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    QVERIFY(rootObject.debugId != -1);
    QQmlEngineDebugPropertyReference widthPropertyRef = findProperty(rootObject.properties, "width");

    bool success = false;

    m_dbg->setBindingForObject(rootObject.debugId, "width", "15", true,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    //
    // reset
    //
    m_dbg->resetBindingForObject(rootObject.debugId, "width", &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    widthPropertyRef =  findProperty(rootObject.properties, "width");

    QCOMPARE(widthPropertyRef.value, QVariant(0));
    QCOMPARE(widthPropertyRef.binding, QString());

    //
    // reset nested property
    //
    success = false;
    m_dbg->resetBindingForObject(rootObject.debugId, "font.bold", &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    rootObject = findRootObject();
    QVERIFY(!rootObject.className.isEmpty());
    QQmlEngineDebugPropertyReference boldPropertyRef =  findProperty(rootObject.properties, "font.bold");

    QCOMPARE(boldPropertyRef.value.toBool(), false);
    QCOMPARE(boldPropertyRef.binding, QString());
}

void tst_QQmlEngineDebugService::setBindingInStates()
{
    // Check if changing bindings of propertychanges works

    const int sourceIndex = 3;

    QQmlEngineDebugObjectReference obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QVERIFY(obj.debugId != -1);
    QVERIFY(obj.children.size() >= 2);
    bool success;
    // We are going to switch state a couple of times, we need to get rid of the transition before
    m_dbg->queryExpressionResult(obj.debugId,QString("transitions = []"), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));


    // check initial value of the property that is changing
    m_dbg->queryExpressionResult(obj.debugId,QString("state=\"state1\""), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(),200);


    m_dbg->queryExpressionResult(obj.debugId,QString("state=\"\""),
                                              &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));


    obj = findRootObject(sourceIndex, true);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(),100);


    // change the binding
    QQmlEngineDebugObjectReference state = obj.children[1];
    QCOMPARE(state.className, QString("State"));
    QVERIFY(state.children.size() > 0);

    QQmlEngineDebugObjectReference propertyChange = state.children[0];
    QVERIFY(!propertyChange.className.isEmpty());
    QVERIFY(propertyChange.debugId != -1);

    m_dbg->setBindingForObject(propertyChange.debugId, "width",QVariant(300),true,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    // check properties changed in state
    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(),100);


    m_dbg->queryExpressionResult(obj.debugId,QString("state=\"state1\""), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(),300);

    // check changing properties of base state from within a state
    m_dbg->setBindingForObject(obj.debugId,"width","height*2",false,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    m_dbg->setBindingForObject(obj.debugId,"height","200",true,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(),300);

    m_dbg->queryExpressionResult(obj.debugId,QString("state=\"\""), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(), 400);

    //  reset binding while in a state
    m_dbg->queryExpressionResult(obj.debugId,QString("state=\"state1\""), &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(), 300);

    m_dbg->resetBindingForObject(propertyChange.debugId, "width", &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(), 400);

    // re-add binding
    m_dbg->setBindingForObject(propertyChange.debugId, "width", "300", true,
                               QString(), -1, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);

    obj = findRootObject(sourceIndex);
    QVERIFY(!obj.className.isEmpty());
    QCOMPARE(findProperty(obj.properties,"width").value.toInt(), 300);
}

void tst_QQmlEngineDebugService::queryObjectTree()
{
    const int sourceIndex = 3;

    QQmlEngineDebugObjectReference obj = findRootObject(sourceIndex, true);
    QVERIFY(!obj.className.isEmpty());
    QVERIFY(obj.debugId != -1);
    QVERIFY(obj.children.size() >= 2);

    // check state
    QQmlEngineDebugObjectReference state = obj.children[1];
    QCOMPARE(state.className, QString("State"));
    QVERIFY(state.children.size() > 0);

    QQmlEngineDebugObjectReference propertyChange = state.children[0];
    QVERIFY(!propertyChange.className.isEmpty());
    QVERIFY(propertyChange.debugId != -1);

    QQmlEngineDebugPropertyReference propertyChangeTarget = findProperty(propertyChange.properties,"target");
    QCOMPARE(propertyChangeTarget.objectDebugId, propertyChange.debugId);

    QQmlEngineDebugObjectReference targetReference = qvariant_cast<QQmlEngineDebugObjectReference>(propertyChangeTarget.value);
    QVERIFY(!targetReference.className.isEmpty());
    QCOMPARE(targetReference.debugId, -1);
    QCOMPARE(targetReference.name, QString("<unnamed object>"));

    // check transition
    QQmlEngineDebugObjectReference transition = obj.children[0];
    QCOMPARE(transition.className, QString("Transition"));
    QCOMPARE(findProperty(transition.properties,"from").value.toString(), QString("*"));
    QCOMPARE(findProperty(transition.properties,"to").value, findProperty(state.properties,"name").value);
    QVERIFY(transition.children.size() > 0);

    QQmlEngineDebugObjectReference animation = transition.children[0];
    QVERIFY(!animation.className.isEmpty());
    QVERIFY(animation.debugId != -1);

    QQmlEngineDebugPropertyReference animationTarget = findProperty(animation.properties,"target");
    QCOMPARE(animationTarget.objectDebugId, animation.debugId);

    targetReference = qvariant_cast<QQmlEngineDebugObjectReference>(animationTarget.value);
    QVERIFY(!targetReference.className.isEmpty());
    QCOMPARE(targetReference.debugId, -1);
    QCOMPARE(targetReference.name, QString("<unnamed object>"));

    QCOMPARE(findProperty(animation.properties,"property").value.toString(), QString("width"));
    QCOMPARE(findProperty(animation.properties,"duration").value.toInt(), 100);
}

void tst_QQmlEngineDebugService::asynchronousCreate() {
    QQmlEngineDebugObjectReference object;
    auto connection = connect(m_dbg, &QQmlEngineDebugClient::newObject, this, [&](int objectId) {
        object.debugId = objectId;
    });

    QByteArray asynchronousComponent = "import QtQuick 2.5\n"
                                       "Rectangle { id: asyncRect }";
    QQmlComponent component(m_engine.get());
    component.setData(asynchronousComponent, QUrl::fromLocalFile(""));
    QVERIFY(component.isReady());  // fails if bad syntax
    QQmlIncubator incubator(QQmlIncubator::Asynchronous);
    const auto guard = qScopeGuard([&](){ delete incubator.object(); });
    component.create(incubator);

    QVERIFY(m_dbg->object().idString != QLatin1String("asyncRect"));

    QTRY_VERIFY(object.debugId != -1);
    disconnect(connection);

    bool success = false;
    m_dbg->queryObject(object, &success);
    QVERIFY(success);

    QTRY_COMPARE(m_dbg->object().idString, QLatin1String("asyncRect"));
}

void tst_QQmlEngineDebugService::invalidContexts()
{
    getContexts();
    const int base = m_dbg->rootContext().contexts.size();
    QQmlContext context(m_engine.get());
    getContexts();
    QCOMPARE(m_dbg->rootContext().contexts.size(), base + 1);
    QQmlRefPointer<QQmlContextData> contextData = QQmlContextData::get(&context);
    contextData->invalidate();
    getContexts();
    QCOMPARE(m_dbg->rootContext().contexts.size(), base);
    QQmlRefPointer<QQmlContextData> rootData = QQmlContextData::get(m_engine->rootContext());
    const auto guard = qScopeGuard([this]() { initTestCase(); }); // Re-init to restore the contexts
    rootData->invalidate();
    getContexts();
    QCOMPARE(m_dbg->rootContext().contexts.size(), 0);
}

void tst_QQmlEngineDebugService::createObjectOnDestruction()
{
    QSignalSpy spy(m_dbg, SIGNAL(newObject(int)));
    QScopedPointer<QObject> o;
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(
                    "import QtQml 2.0;"
                    "QtObject {"
                        "property Component x:"
                            "Qt.createQmlObject('import QtQml 2.0; Component { QtObject { } }',"
                                                "this, 'x.qml');"
                        "Component.onDestruction: x.createObject(this, {});"
                    "}", QUrl::fromLocalFile("x.qml"));
        QVERIFY(component.isReady());
        o.reset(component.create());
        QVERIFY(!o.isNull());
        QTRY_COMPARE(spy.size(), 2);
    }
    // Doesn't crash and doesn't give us another signal for the object created on destruction.
    QTest::qWait(500);
    QCOMPARE(spy.size(), 2);
}

void tst_QQmlEngineDebugService::fetchValueType()
{
    QQmlApplicationEngine engine;
    engine.load(testFileUrl("fetchValueType.qml"));


    bool success = false;
    m_dbg->queryAvailableEngines(&success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QVERIFY(m_dbg->engines().size() > 1);

    QQmlEngineDebugObjectReference object;
    object.debugId = QQmlDebugService::idForObject(&engine);
    m_dbg->queryObjectRecursive(object, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));

}

void tst_QQmlEngineDebugService::debuggerCrashOnAttach() {
    QQmlEngineDebugObjectReference obj = findRootObject(6);
    QVERIFY(!obj.className.isEmpty());

    bool success;

    m_dbg->addWatch(obj, &success);
    QVERIFY(success);
    QVERIFY(QQmlDebugTest::waitForSignal(m_dbg, SIGNAL(result())));
    QCOMPARE(m_dbg->valid(), true);
}

int main(int argc, char *argv[])
{
    int _argc = argc + 1;
    QScopedArrayPointer<char *>_argv(new char*[_argc]);
    for (int i = 0; i < argc; ++i)
        _argv[i] = argv[i];
    char arg[] = "-qmljsdebugger=port:3768,services:QmlDebugger";
    _argv[_argc - 1] = arg;

    QGuiApplication app(_argc, _argv.data());
    tst_QQmlEngineDebugService tc;
    return QTest::qExec(&tc, _argc, _argv.data());
}

#include "tst_qqmlenginedebugservice.moc"
