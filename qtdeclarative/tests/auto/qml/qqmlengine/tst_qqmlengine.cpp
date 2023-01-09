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

#include "../../shared/util.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QDir>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QDebug>
#include <QBuffer>
#include <QCryptographicHash>
#include <QQmlComponent>
#include <QQmlNetworkAccessManagerFactory>
#include <QQmlExpression>
#include <QQmlIncubationController>
#include <QTemporaryDir>
#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>
#include <QQmlAbstractUrlInterceptor>

class tst_qqmlengine : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlengine() {}

private slots:
    void initTestCase() override;
    void rootContext();
    void networkAccessManager();
    void synchronousNetworkAccessManager();
    void baseUrl();
    void contextForObject();
    void offlineStoragePath();
    void offlineDatabaseStoragePath();
    void clearComponentCache();
    void trimComponentCache();
    void trimComponentCache_data();
    void repeatedCompilation();
    void failedCompilation();
    void failedCompilation_data();
    void outputWarningsToStandardError();
    void objectOwnership();
    void multipleEngines();
    void qtqmlModule_data();
    void qtqmlModule();
    void urlInterceptor_data();
    void urlInterceptor();
    void qmlContextProperties();
    void testGCCorruption();
    void testGroupedPropertyRevisions();
    void componentFromEval();
    void qrcUrls();
    void cppSignalAndEval();
    void singletonInstance();
    void aggressiveGc();
    void cachedGetterLookup_qtbug_75335();
    void createComponentOnSingletonDestruction();
    void uiLanguage();

public slots:
    QObject *createAQObjectForOwnershipTest ()
    {
        static QObject *ptr = new QObject();
        return ptr;
    }

private:
    QTemporaryDir m_tempDir;
};

void tst_qqmlengine::initTestCase()
{
    QVERIFY2(m_tempDir.isValid(), qPrintable(m_tempDir.errorString()));
    QQmlDataTest::initTestCase();
}

void tst_qqmlengine::rootContext()
{
    QQmlEngine engine;

    QVERIFY(engine.rootContext());

    QCOMPARE(engine.rootContext()->engine(), &engine);
    QVERIFY(!engine.rootContext()->parentContext());
}

class NetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    NetworkAccessManagerFactory() : manager(nullptr) {}

    QNetworkAccessManager *create(QObject *parent) {
        manager = new QNetworkAccessManager(parent);
        return manager;
    }

    QNetworkAccessManager *manager;
};

void tst_qqmlengine::networkAccessManager()
{
    QQmlEngine *engine = new QQmlEngine;

    // Test QQmlEngine created manager
    QPointer<QNetworkAccessManager> manager = engine->networkAccessManager();
    QVERIFY(manager != nullptr);
    delete engine;

    // Test factory created manager
    engine = new QQmlEngine;
    NetworkAccessManagerFactory factory;
    engine->setNetworkAccessManagerFactory(&factory);
    QCOMPARE(engine->networkAccessManagerFactory(), &factory);
    QNetworkAccessManager *engineNam = engine->networkAccessManager(); // calls NetworkAccessManagerFactory::create()
    QCOMPARE(engineNam, factory.manager);
    delete engine;
}

class ImmediateReply : public QNetworkReply {

    Q_OBJECT

public:
    ImmediateReply() {
        setFinished(true);
    }
    virtual qint64 readData(char* , qint64 ) {
        return 0;
    }
    virtual void abort() { }
};

class ImmediateManager : public QNetworkAccessManager {

    Q_OBJECT

public:
    ImmediateManager(QObject *parent = nullptr) : QNetworkAccessManager(parent) {
    }

    QNetworkReply *createRequest(Operation, const QNetworkRequest & , QIODevice * outgoingData = nullptr) {
        Q_UNUSED(outgoingData);
        return new ImmediateReply;
    }
};

class ImmediateFactory : public QQmlNetworkAccessManagerFactory {

public:
    QNetworkAccessManager *create(QObject *) {
        return new ImmediateManager;
    }
};

void tst_qqmlengine::synchronousNetworkAccessManager()
{
    ImmediateFactory factory;
    QQmlEngine engine;
    engine.setNetworkAccessManagerFactory(&factory);
    QQmlComponent c(&engine, QUrl("myScheme://test.qml"));
    // reply is finished, so should not be in loading state.
    QVERIFY(!c.isLoading());
}


void tst_qqmlengine::baseUrl()
{
    QQmlEngine engine;

    QUrl cwd = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());

    QCOMPARE(engine.baseUrl(), cwd);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd.resolved(QUrl("main.qml")));

    QDir dir = QDir::current();
    dir.cdUp();
    QVERIFY(dir != QDir::current());
    QDir::setCurrent(dir.path());
    QCOMPARE(QDir::current(), dir);

    QUrl cwd2 = QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
    QCOMPARE(engine.baseUrl(), cwd2);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd2.resolved(QUrl("main.qml")));

    engine.setBaseUrl(cwd);
    QCOMPARE(engine.baseUrl(), cwd);
    QCOMPARE(engine.rootContext()->resolvedUrl(QUrl("main.qml")), cwd.resolved(QUrl("main.qml")));


    const QString testPath = QDir::currentPath() + QLatin1String("/");
    const QString rootPath = QDir::rootPath();
    engine.setBaseUrl(QUrl());

    // Check that baseUrl returns a url to a localFile
    QCOMPARE(engine.baseUrl().toLocalFile(), testPath);

    QDir::setCurrent(QDir::rootPath());

    // Make sure this also works when in the rootPath
    QCOMPARE(engine.baseUrl().toLocalFile(), rootPath);
}

void tst_qqmlengine::contextForObject()
{
    QQmlEngine *engine = new QQmlEngine;

    // Test null-object
    QVERIFY(!QQmlEngine::contextForObject(nullptr));

    // Test an object with no context
    QObject object;
    QVERIFY(!QQmlEngine::contextForObject(&object));

    // Test setting null-object
    QQmlEngine::setContextForObject(nullptr, engine->rootContext());

    // Test setting null-context
    QQmlEngine::setContextForObject(&object, nullptr);

    // Test setting context
    QQmlEngine::setContextForObject(&object, engine->rootContext());
    QCOMPARE(QQmlEngine::contextForObject(&object), engine->rootContext());

    QQmlContext context(engine->rootContext());

    // Try changing context
    QTest::ignoreMessage(QtWarningMsg, "QQmlEngine::setContextForObject(): Object already has a QQmlContext");
    QQmlEngine::setContextForObject(&object, &context);
    QCOMPARE(QQmlEngine::contextForObject(&object), engine->rootContext());

    // Delete context
    delete engine; engine = nullptr;
    QVERIFY(!QQmlEngine::contextForObject(&object));
}

void tst_qqmlengine::offlineStoragePath()
{
    // Without these set, QDesktopServices::storageLocation returns
    // strings with extra "//" at the end. We set them to ignore this problem.
    qApp->setApplicationName("tst_qqmlengine");
    qApp->setOrganizationName("QtProject");
    qApp->setOrganizationDomain("www.qt-project.org");

    QQmlEngine engine;

    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    QCOMPARE(dataLocation.isEmpty(), engine.offlineStoragePath().isEmpty());

    QDir dir(dataLocation);
    dir.mkpath("QML");
    dir.cd("QML");
    dir.mkpath("OfflineStorage");
    dir.cd("OfflineStorage");

    QCOMPARE(QDir::fromNativeSeparators(engine.offlineStoragePath()), dir.path());

    engine.setOfflineStoragePath(QDir::homePath());
    QCOMPARE(engine.offlineStoragePath(), QDir::homePath());
}

void tst_qqmlengine::offlineDatabaseStoragePath()
{
    // Without these set, QDesktopServices::storageLocation returns
    // strings with extra "//" at the end. We set them to ignore this problem.
    qApp->setApplicationName("tst_qqmlengine");
    qApp->setOrganizationName("QtProject");
    qApp->setOrganizationDomain("www.qt-project.org");

    QQmlEngine engine;
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    const QString databaseName = QLatin1String("foo");
    QString databaseLocation = engine.offlineStorageDatabaseFilePath(databaseName);
    QCOMPARE(dataLocation.isEmpty(), databaseLocation.isEmpty());

    QDir dir(dataLocation);
    dir.mkpath("QML");
    dir.cd("QML");
    dir.mkpath("OfflineStorage");
    dir.cd("OfflineStorage");
    dir.mkpath("Databases");
    dir.cd("Databases");
    QCOMPARE(QFileInfo(databaseLocation).dir().path(), dir.path());

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(databaseName.toUtf8());
    QCOMPARE(databaseLocation, QDir::toNativeSeparators(dir.filePath(QLatin1String(md5.result().toHex()))));
}

void tst_qqmlengine::clearComponentCache()
{
    QQmlEngine engine;

    const QString fileName = m_tempDir.filePath(QStringLiteral("temp.qml"));
    const QUrl fileUrl = QUrl::fromLocalFile(fileName);

    // Create original qml file
    {
        QFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("import QtQuick 2.0\nQtObject {\nproperty int test: 10\n}\n");
        file.close();
    }

    // Test "test" property
    {
        QQmlComponent component(&engine, fileUrl);
        QObject *obj = component.create();
        QVERIFY(obj != nullptr);
        QCOMPARE(obj->property("test").toInt(), 10);
        delete obj;
    }

    // Modify qml file
    {
        // On macOS with HFS+ the precision of file times is measured in seconds, so to ensure that
        // the newly written file has a modification date newer than an existing cache file, we must
        // wait.
        // Similar effects of lacking precision have been observed on some Linux systems.
        QThread::sleep(1);

        QFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("import QtQuick 2.0\nQtObject {\nproperty int test: 11\n}\n");
        file.close();
    }

    // Test cache hit
    {
        QQmlComponent component(&engine, fileUrl);
        QObject *obj = component.create();
        QVERIFY(obj != nullptr);
        QCOMPARE(obj->property("test").toInt(), 10);
        delete obj;
    }

    // Clear cache
    engine.clearComponentCache();

    // Test cache refresh
    {
        QQmlComponent component(&engine, fileUrl);
        QObject *obj = component.create();
        QVERIFY(obj != nullptr);
        QCOMPARE(obj->property("test").toInt(), 11);
        delete obj;
    }

    // Regular Synchronous loading will leave us with an event posted
    // to the gui thread and an extra refcount that will only be dropped after the
    // event delivery. Call sendPostedEvents() to get rid of it so that
    // the temporary directory can be removed.
    QCoreApplication::sendPostedEvents();
}

struct ComponentCacheFunctions : public QObject, public QQmlIncubationController
{
    Q_OBJECT
public:
    QQmlEngine *engine;

    ComponentCacheFunctions(QQmlEngine &e) : engine(&e) {}

    Q_INVOKABLE void trim()
    {
        // Wait for any pending deletions to occur
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();

        // There might be JS function objects around that hold a last ref to the compilation unit that's
        // keeping the type compilation data (CompilationUnit) around. Let's collect them as well so that
        // trim works well.
        engine->collectGarbage();

        engine->trimComponentCache();
    }

    Q_INVOKABLE bool isTypeLoaded(QString file)
    {
        return QQmlEnginePrivate::get(engine)->isTypeLoaded(tst_qqmlengine::instance()->testFileUrl(file));
    }

    Q_INVOKABLE bool isScriptLoaded(QString file)
    {
        return QQmlEnginePrivate::get(engine)->isScriptLoaded(tst_qqmlengine::instance()->testFileUrl(file));
    }

    Q_INVOKABLE void beginIncubation()
    {
        startTimer(0);
    }

    Q_INVOKABLE void waitForIncubation()
    {
        while (incubatingObjectCount() > 0) {
            QCoreApplication::processEvents();
        }
    }

private:
    virtual void timerEvent(QTimerEvent *)
    {
        incubateFor(1000);
    }
};

void tst_qqmlengine::trimComponentCache()
{
    QFETCH(QString, file);

    QQmlEngine engine;
    ComponentCacheFunctions componentCache(engine);
    engine.setIncubationController(&componentCache);

    QQmlComponent component(&engine, testFileUrl(file));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {"componentCache", QVariant::fromValue(&componentCache)}
    }));
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("success").toBool(), true);
}

void tst_qqmlengine::trimComponentCache_data()
{
    QTest::addColumn<QString>("file");

    // The various tests here are for two types of components: those that are
    // empty apart from their inherited elements, and those that define new properties.
    // For each there are five types of composition: extension, aggregation,
    // aggregation via component, property and object-created-via-transient-component.
    foreach (const QString &test, (QStringList() << "EmptyComponent"
                                                 << "VMEComponent"
                                                 << "EmptyExtendEmptyComponent"
                                                 << "VMEExtendEmptyComponent"
                                                 << "EmptyExtendVMEComponent"
                                                 << "VMEExtendVMEComponent"
                                                 << "EmptyAggregateEmptyComponent"
                                                 << "VMEAggregateEmptyComponent"
                                                 << "EmptyAggregateVMEComponent"
                                                 << "VMEAggregateVMEComponent"
                                                 << "EmptyPropertyEmptyComponent"
                                                 << "VMEPropertyEmptyComponent"
                                                 << "EmptyPropertyVMEComponent"
                                                 << "VMEPropertyVMEComponent"
                                                 << "VMETransientEmptyComponent"
                                                 << "VMETransientVMEComponent")) {
        // For these cases, we first test that the component instance keeps the components
        // referenced, and then that the instantiated object keeps the components referenced
        for (int i = 1; i <= 2; ++i) {
            QString name(QString("%1-%2").arg(test).arg(i));
            QString file(QString("test%1.%2.qml").arg(test).arg(i));
            QTest::newRow(name.toLatin1().constData()) << file;
        }
    }

    // Test that a transient component is correctly referenced
    QTest::newRow("TransientComponent-1") << "testTransientComponent.1.qml";
    QTest::newRow("TransientComponent-2") << "testTransientComponent.2.qml";

    // Test that components can be reloaded after unloading
    QTest::newRow("ReloadComponent") << "testReloadComponent.qml";

    // Test that components are correctly referenced when dynamically loaded
    QTest::newRow("LoaderComponent") << "testLoaderComponent.qml";

    // Test that components are correctly referenced when incubated
    QTest::newRow("IncubatedComponent") << "testIncubatedComponent.qml";

    // Test that a top-level omponents is correctly referenced
    QTest::newRow("TopLevelComponent") << "testTopLevelComponent.qml";

    // TODO:
    // Test that scripts are unloaded when no longer referenced
    QTest::newRow("ScriptComponent") << "testScriptComponent.qml";
}

void tst_qqmlengine::repeatedCompilation()
{
    QQmlEngine engine;

    for (int i = 0; i < 100; ++i) {
        engine.collectGarbage();
        engine.trimComponentCache();

        QQmlComponent component(&engine, testFileUrl("repeatedCompilation.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QCOMPARE(object->property("success").toBool(), true);
    }
}

void tst_qqmlengine::failedCompilation()
{
    QFETCH(QString, file);

    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl(file));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QQmlComponent: Component is not ready");
    QVERIFY(!component.isReady());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object.isNull());

    engine.collectGarbage();
    engine.trimComponentCache();
    engine.clearComponentCache();
}

void tst_qqmlengine::failedCompilation_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("Invalid URL") << "failedCompilation.does.not.exist.qml";
    QTest::newRow("Invalid content") << "failedCompilation.1.qml";
}

void tst_qqmlengine::outputWarningsToStandardError()
{
    QQmlEngine engine;

    QCOMPARE(engine.outputWarningsToStandardError(), true);

    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0; QtObject { property int a: undefined }", QUrl());

    QVERIFY(c.isReady());

    QQmlTestMessageHandler messageHandler;

    QObject *o = c.create();

    QVERIFY(o != nullptr);
    delete o;

    QCOMPARE(messageHandler.messages().count(), 1);
    QCOMPARE(messageHandler.messages().at(0), QLatin1String("<Unknown File>:1:32: Unable to assign [undefined] to int"));
    messageHandler.clear();

    engine.setOutputWarningsToStandardError(false);
    QCOMPARE(engine.outputWarningsToStandardError(), false);

    o = c.create();

    QVERIFY(o != nullptr);
    delete o;

    QVERIFY2(messageHandler.messages().isEmpty(), qPrintable(messageHandler.messageString()));
}

void tst_qqmlengine::objectOwnership()
{
    {
    QCOMPARE(QQmlEngine::objectOwnership(nullptr), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(nullptr, QQmlEngine::JavaScriptOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(nullptr), QQmlEngine::CppOwnership);
    }

    {
    QObject o;
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::CppOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::JavaScriptOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::JavaScriptOwnership);
    QQmlEngine::setObjectOwnership(&o, QQmlEngine::CppOwnership);
    QCOMPARE(QQmlEngine::objectOwnership(&o), QQmlEngine::CppOwnership);
    }

    {
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.0; QtObject { property QtObject object: QtObject {} }", QUrl());

    QObject *o = c.create();
    QVERIFY(o != nullptr);

    QCOMPARE(QQmlEngine::objectOwnership(o), QQmlEngine::CppOwnership);

    QObject *o2 = qvariant_cast<QObject *>(o->property("object"));
    QCOMPARE(QQmlEngine::objectOwnership(o2), QQmlEngine::JavaScriptOwnership);

    delete o;
    }
    {
        QObject *ptr = createAQObjectForOwnershipTest();
        QSignalSpy spy(ptr, SIGNAL(destroyed()));
        {
            QQmlEngine engine;
            QQmlComponent c(&engine);
            QQmlEngine::setObjectOwnership(ptr, QQmlEngine::JavaScriptOwnership);
            c.setData("import QtQuick 2.0; Item { required property QtObject test; property int data: test.createAQObjectForOwnershipTest() ? 0 : 1 }", QUrl());
            QVERIFY(c.isReady());
            QObject *o = c.createWithInitialProperties( {{"test", QVariant::fromValue(this)}} );
            QVERIFY(o != nullptr);
        }
        QTRY_VERIFY(spy.count());
    }
    {
        QObject *ptr = new QObject();
        QSignalSpy spy(ptr, SIGNAL(destroyed()));
        {
            QQmlEngine engine;
            QQmlComponent c(&engine);
            QQmlEngine::setObjectOwnership(ptr, QQmlEngine::JavaScriptOwnership);
            c.setData("import QtQuick 2.0; QtObject { required property QtObject test; property var object: { var i = test; test ? 0 : 1 }  }", QUrl());
            QVERIFY(c.isReady());
            QObject *o = c.createWithInitialProperties({{"test", QVariant::fromValue(ptr)}});
            QVERIFY(o != nullptr);
            QQmlProperty testProp(o, "test");
            testProp.write(QVariant::fromValue<QObject*>(nullptr));
        }
        QTRY_VERIFY(spy.count());
    }
}

// Test an object can be accessed by multiple engines
void tst_qqmlengine::multipleEngines()
{
    QObject o;
    o.setObjectName("TestName");

    // Simultaneous engines
    {
        QQmlEngine engine1;
        QQmlEngine engine2;
        engine1.rootContext()->setContextProperty("object", &o);
        engine2.rootContext()->setContextProperty("object", &o);

        QQmlExpression expr1(engine1.rootContext(), nullptr, QString("object.objectName"));
        QQmlExpression expr2(engine2.rootContext(), nullptr, QString("object.objectName"));

        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
        QCOMPARE(expr2.evaluate().toString(), QString("TestName"));
    }

    // Serial engines
    {
        QQmlEngine engine1;
        engine1.rootContext()->setContextProperty("object", &o);
        QQmlExpression expr1(engine1.rootContext(), nullptr, QString("object.objectName"));
        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
    }
    {
        QQmlEngine engine1;
        engine1.rootContext()->setContextProperty("object", &o);
        QQmlExpression expr1(engine1.rootContext(), nullptr, QString("object.objectName"));
        QCOMPARE(expr1.evaluate().toString(), QString("TestName"));
    }
}

void tst_qqmlengine::qtqmlModule_data()
{
    QTest::addColumn<QUrl>("testFile");
    QTest::addColumn<QString>("expectedError");
    QTest::addColumn<QStringList>("expectedWarnings");

    QTest::newRow("import QtQml of correct version (2.0)")
            << testFileUrl("qtqmlModule.1.qml")
            << QString()
            << QStringList();

    QTest::newRow("import QtQml of incorrect version (3.0)")
            << testFileUrl("qtqmlModule.2.qml")
            << QString(testFileUrl("qtqmlModule.2.qml").toString() + QLatin1String(":1 module \"QtQml\" version 3.0 is not installed\n"))
            << QStringList();

    QTest::newRow("import QtQml of incorrect version (1.0)")
            << testFileUrl("qtqmlModule.3.qml")
            << QString(testFileUrl("qtqmlModule.3.qml").toString() + QLatin1String(":1 module \"QtQml\" version 1.0 is not installed\n"))
            << QStringList();

    QTest::newRow("import QtQml of incorrect version (2.50)")
            << testFileUrl("qtqmlModule.4.qml")
            << QString(testFileUrl("qtqmlModule.4.qml").toString() + QLatin1String(":1 module \"QtQml\" version 2.50 is not installed\n"))
            << QStringList();

    QTest::newRow("QtQml 2.0 module provides Component, QtObject, Connections, Binding and Timer")
            << testFileUrl("qtqmlModule.5.qml")
            << QString()
            << QStringList();

    QTest::newRow("can import QtQml then QtQuick")
            << testFileUrl("qtqmlModule.6.qml")
            << QString()
            << QStringList();

    QTest::newRow("can import QtQuick then QtQml")
            << testFileUrl("qtqmlModule.7.qml")
            << QString()
            << QStringList();

    QTest::newRow("no import results in no QtObject availability")
            << testFileUrl("qtqmlModule.8.qml")
            << QString(testFileUrl("qtqmlModule.8.qml").toString() + QLatin1String(":4 QtObject is not a type\n"))
            << QStringList();

    QTest::newRow("importing QtQml only results in no Item availability")
            << testFileUrl("qtqmlModule.9.qml")
            << QString(testFileUrl("qtqmlModule.9.qml").toString() + QLatin1String(":4 Item is not a type\n"))
            << QStringList();
}

// Test that the engine registers the QtQml module
void tst_qqmlengine::qtqmlModule()
{
    QFETCH(QUrl, testFile);
    QFETCH(QString, expectedError);
    QFETCH(QStringList, expectedWarnings);

    foreach (const QString &w, expectedWarnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(w));

    QQmlEngine e;
    QQmlComponent c(&e, testFile);
    if (expectedError.isEmpty()) {
        QObject *o = c.create();
        QVERIFY(o);
        delete o;
    } else {
        QCOMPARE(c.errorString(), expectedError);
    }
}

class CustomSelector : public QQmlAbstractUrlInterceptor
{
public:
    CustomSelector(const QUrl &base):m_base(base){}
    virtual QUrl intercept(const QUrl &url, QQmlAbstractUrlInterceptor::DataType d)
    {
        if ((url.scheme() != QStringLiteral("file") && url.scheme() != QStringLiteral("qrc"))
            || url.path().contains("QtQml"))
            return url;
        if (!m_interceptionPoints.contains(d))
            return url;

        if (url.path().endsWith("Test.2/qmldir")) {//Special case
            QUrl url = m_base;
            url.setPath(m_base.path() + "interception/module/intercepted/qmldir");
            return url;
        }
        // Special case: with 5.10 we always add the implicit import, so we need to explicitly handle this case now
        if (url.path().endsWith("intercepted/qmldir"))
            return url;

        QString alteredPath = url.path();
        int a = alteredPath.lastIndexOf('/');
        if (a < 0)
            a = 0;
        alteredPath.insert(a, QStringLiteral("/intercepted"));

        QUrl ret = url;
        ret.setPath(alteredPath);
        return ret;
    }
    QList<QQmlAbstractUrlInterceptor::DataType> m_interceptionPoints;
    QUrl m_base;
};

Q_DECLARE_METATYPE(QList<QQmlAbstractUrlInterceptor::DataType>);

void tst_qqmlengine::urlInterceptor_data()
{
    QTest::addColumn<QUrl>("testFile");
    QTest::addColumn<QList<QQmlAbstractUrlInterceptor::DataType> >("interceptionPoint");
    QTest::addColumn<QString>("expectedFilePath");
    QTest::addColumn<QString>("expectedChildString");
    QTest::addColumn<QString>("expectedScriptString");
    QTest::addColumn<QString>("expectedResolvedUrl");
    QTest::addColumn<QString>("expectedAbsoluteUrl");

    QTest::newRow("InterceptTypes")
        << testFileUrl("interception/types/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::QmlFile << QQmlAbstractUrlInterceptor::JavaScriptFile << QQmlAbstractUrlInterceptor::UrlString)
        << testFileUrl("interception/types/intercepted/doesNotExist.file").toString()
        << QStringLiteral("intercepted")
        << QStringLiteral("intercepted")
        << testFileUrl("interception/types/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///intercepted/doesNotExist.file");

    QTest::newRow("InterceptQmlDir")
        << testFileUrl("interception/qmldir/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::QmldirFile << QQmlAbstractUrlInterceptor::UrlString)
        << testFileUrl("interception/qmldir/intercepted/doesNotExist.file").toString()
        << QStringLiteral("intercepted")
        << QStringLiteral("base file")
        << testFileUrl("interception/qmldir/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///intercepted/doesNotExist.file");

    QTest::newRow("InterceptModule")//just a Test{}, needs to intercept the module import for it to work
        << testFileUrl("interception/module/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::QmldirFile )
        << testFileUrl("interception/module/intercepted/doesNotExist.file").toString()
        << QStringLiteral("intercepted")
        << QStringLiteral("intercepted")
        << testFileUrl("interception/module/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///doesNotExist.file");

    QTest::newRow("InterceptStrings")
        << testFileUrl("interception/strings/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::UrlString)
        << testFileUrl("interception/strings/intercepted/doesNotExist.file").toString()
        << QStringLiteral("base file")
        << QStringLiteral("base file")
        << testFileUrl("interception/strings/intercepted/doesNotExist.file").toString()
        << QStringLiteral("file:///intercepted/doesNotExist.file");

    QTest::newRow("InterceptIncludes")
        << testFileUrl("interception/includes/urlInterceptor.qml")
        << (QList<QQmlAbstractUrlInterceptor::DataType>() << QQmlAbstractUrlInterceptor::JavaScriptFile)
        << testFileUrl("interception/includes/doesNotExist.file").toString()
        << QStringLiteral("base file")
        << QStringLiteral("intercepted include file")
        << testFileUrl("interception/includes/doesNotExist.file").toString()
        << QStringLiteral("file:///doesNotExist.file");
}

void tst_qqmlengine::urlInterceptor()
{

    QFETCH(QUrl, testFile);
    QFETCH(QList<QQmlAbstractUrlInterceptor::DataType>, interceptionPoint);
    QFETCH(QString, expectedFilePath);
    QFETCH(QString, expectedChildString);
    QFETCH(QString, expectedScriptString);
    QFETCH(QString, expectedResolvedUrl);
    QFETCH(QString, expectedAbsoluteUrl);

    QQmlEngine e;
    e.addImportPath(testFileUrl("interception/imports").url());
    CustomSelector cs(testFileUrl(""));
    cs.m_interceptionPoints = interceptionPoint;
    e.setUrlInterceptor(&cs);
    QQmlComponent c(&e, testFile); //Note that this can get intercepted too
    QObject *o = c.create();
    if (!o)
        qDebug() << c.errorString();
    QVERIFY(o);
    //Test a URL as a property initialization
    QCOMPARE(o->property("filePath").toString(), expectedFilePath);
    //Test a URL as a Type location
    QCOMPARE(o->property("childString").toString(), expectedChildString);
    //Test a URL as a Script location
    QCOMPARE(o->property("scriptString").toString(), expectedScriptString);
    //Test a URL as a resolveUrl() call
    QCOMPARE(o->property("resolvedUrl").toString(), expectedResolvedUrl);
    QCOMPARE(o->property("absoluteUrl").toString(), expectedAbsoluteUrl);
}

void tst_qqmlengine::qmlContextProperties()
{
    QQmlEngine e;

    QQmlComponent c(&e, testFileUrl("TypeofQmlProperty.qml"));
    QObject *o = c.create();
    if (!o) {
        qDebug() << c.errorString();
    }
    QVERIFY(o);
}

void tst_qqmlengine::testGCCorruption()
{
#ifdef SKIP_GCCORRUPTION_TEST
    QSKIP("Test too heavy for qemu");
#endif

    QQmlEngine e;

    QQmlComponent c(&e, testFileUrl("testGCCorruption.qml"));
    QObject *o = c.create();
    QVERIFY2(o, qPrintable(c.errorString()));
}

void tst_qqmlengine::testGroupedPropertyRevisions()
{
    QQmlEngine e;

    QQmlComponent c(&e, testFileUrl("testGroupedPropertiesRevision.1.qml"));
    QScopedPointer<QObject> object(c.create());
    QVERIFY2(object.data(), qPrintable(c.errorString()));
    QQmlComponent c2(&e, testFileUrl("testGroupedPropertiesRevision.2.qml"));
    QVERIFY(!c2.errorString().isEmpty());
}

void tst_qqmlengine::componentFromEval()
{
    QQmlEngine engine;
    const QUrl testUrl = testFileUrl("EmptyComponent.qml");
    QJSValue result = engine.evaluate("Qt.createComponent(\"" + testUrl.toString() + "\");");
    QPointer<QQmlComponent> component(qobject_cast<QQmlComponent*>(result.toQObject()));
    QVERIFY(!component.isNull());
    QScopedPointer<QObject> item(component->create());
    QVERIFY(!item.isNull());
}

void tst_qqmlengine::qrcUrls()
{
    QQmlEngine engine;
    QQmlEnginePrivate *pEngine = QQmlEnginePrivate::get(&engine);

    {
        QQmlRefPointer<QQmlTypeData> oneQml(pEngine->typeLoader.getType(QUrl("qrc:/qrcurls.qml")));
        QVERIFY(oneQml.data() != nullptr);
        QQmlRefPointer<QQmlTypeData> twoQml(pEngine->typeLoader.getType(QUrl("qrc:///qrcurls.qml")));
        QVERIFY(twoQml.data() != nullptr);
        QCOMPARE(oneQml.data(), twoQml.data());
    }

    {
        QQmlRefPointer<QQmlTypeData> oneJS(pEngine->typeLoader.getType(QUrl("qrc:/qrcurls.js")));
        QVERIFY(oneJS.data() != nullptr);
        QQmlRefPointer<QQmlTypeData> twoJS(pEngine->typeLoader.getType(QUrl("qrc:///qrcurls.js")));
        QVERIFY(twoJS.data() != nullptr);
        QCOMPARE(oneJS.data(), twoJS.data());
    }
}

class ObjectCaller : public QObject
{
    Q_OBJECT
signals:
    void doubleReply(const double a);
};

void tst_qqmlengine::cppSignalAndEval()
{
    ObjectCaller objectCaller;
    QQmlEngine engine;
    qmlRegisterSingletonInstance("Test", 1, 0, "CallerCpp", &objectCaller);
    QQmlComponent c(&engine);
    c.setData("import QtQuick 2.9\n"
              "import Test 1.0\n"
              "Item {\n"
              "    property var r: 0\n"
              "    Connections {\n"
              "        target: CallerCpp;\n"
              "        function onDoubleReply() {\n"
              "            eval('var z = 1');\n"
              "            r = a;\n"
              "        }\n"
              "    }\n"
              "}",
              QUrl(QStringLiteral("qrc:/main.qml")));
    QScopedPointer<QObject> object(c.create());
    QVERIFY(!object.isNull());
    emit objectCaller.doubleReply(1.1234);
    QCOMPARE(object->property("r"), 1.1234);
}

class CppSingleton : public QObject {
    Q_OBJECT
public:
    CppSingleton() {}

    static QObject *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        Q_UNUSED(jsEngine);
        return new CppSingleton();
    }
};

class JsSingleton : public QObject {
    Q_OBJECT
public:
    JsSingleton() {}

    static QJSValue create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        Q_UNUSED(qmlEngine);
        QJSValue value = jsEngine->newQObject(new JsSingleton());
        return value;
    }
};

class SomeQObjectClass : public QObject {
    Q_OBJECT
public:
    SomeQObjectClass() : QObject(nullptr){}
};

class Dayfly : public QObject
{
    Q_OBJECT
};

void tst_qqmlengine::singletonInstance()
{
    QQmlEngine engine;

    int cppSingletonTypeId = qmlRegisterSingletonType<CppSingleton>("Test", 1, 0, "CppSingleton", &CppSingleton::create);
    int jsValueSingletonTypeId = qmlRegisterSingletonType("Test", 1, 0, "JsSingleton", &JsSingleton::create);

    {
        // Cpp QObject singleton type
        QJSValue value = engine.singletonInstance<QJSValue>(cppSingletonTypeId);
        QVERIFY(!value.isUndefined());
        QVERIFY(value.isQObject());
        QObject *instance = value.toQObject();
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "CppSingleton");
    }

    {
        // QJSValue QObject singleton type
        QJSValue value = engine.singletonInstance<QJSValue>(jsValueSingletonTypeId);
        QVERIFY(!value.isUndefined());
        QVERIFY(value.isQObject());
        QObject *instance = value.toQObject();
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "JsSingleton");
    }

    {
        int data = 30;
        auto id = qmlRegisterSingletonType<CppSingleton>("Qt.test",1,0,"CapturingLambda",[data](QQmlEngine*, QJSEngine*){ // register qobject singleton with capturing lambda
                auto o = new CppSingleton;
                o->setProperty("data", data);
                return o;
        });
        QJSValue value = engine.singletonInstance<QJSValue>(id);
        QVERIFY(!value.isUndefined());
        QVERIFY(value.isQObject());
        QObject *instance = value.toQObject();
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "CppSingleton");
        QCOMPARE(instance->property("data"), data);
    }
    {
        qmlRegisterSingletonType<CppSingleton>("Qt.test",1,0,"NotAmbiguous", [](QQmlEngine* qeng, QJSEngine* jeng) -> QObject* {return CppSingleton::create(qeng, jeng);}); // test that overloads for qmlRegisterSingleton are not ambiguous
    }
    {
        // Register QObject* directly
        CppSingleton single;
        int id = qmlRegisterSingletonInstance("Qt.test", 1, 0, "CppOwned",
                                                                &single);
        QQmlEngine engine2;
        CppSingleton *singlePtr = engine2.singletonInstance<CppSingleton *>(id);
        QVERIFY(singlePtr);
        QCOMPARE(&single, singlePtr);
        QVERIFY(engine2.objectOwnership(singlePtr) == QQmlEngine::CppOwnership);
    }

    {
        CppSingleton single;
        QQmlEngine engineA;
        QQmlEngine engineB;
        int id = qmlRegisterSingletonInstance("Qt.test", 1, 0, "CppOwned", &single);
        auto singlePtr = engineA.singletonInstance<CppSingleton *>(id);
        QVERIFY(singlePtr);
        singlePtr = engineA.singletonInstance<CppSingleton *>(id); // accessing the singleton multiple times from the same engine is fine
        QVERIFY(singlePtr);
        QTest::ignoreMessage(QtMsgType::QtCriticalMsg, "<Unknown File>: qmlRegisterSingletonType(): \"CppOwned\" is not available because the callback function returns a null pointer.");
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: Singleton registered by registerSingletonInstance must only be accessed from one engine");
        QCOMPARE(&single, singlePtr);
        auto noSinglePtr = engineB.singletonInstance<CppSingleton *>(id);
        QVERIFY(!noSinglePtr);
    }

    {
        CppSingleton single;
        QThread newThread {};
        single.moveToThread(&newThread);
        QCOMPARE(single.thread(), &newThread);
        QQmlEngine engineB;
        int id = qmlRegisterSingletonInstance("Qt.test", 1, 0, "CppOwned", &single);
        QTest::ignoreMessage(QtMsgType::QtCriticalMsg, "<Unknown File>: qmlRegisterSingletonType(): \"CppOwned\" is not available because the callback function returns a null pointer.");
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: Registered object must live in the same thread as the engine it was registered with");
        auto noSinglePtr = engineB.singletonInstance<CppSingleton *>(id);
        QVERIFY(!noSinglePtr);
    }

    {
        // Invalid types
        QJSValue value;
        value = engine.singletonInstance<QJSValue>(-4711);
        QVERIFY(value.isUndefined());
        value = engine.singletonInstance<QJSValue>(1701);
        QVERIFY(value.isUndefined());
    }

    {
        // Valid, but non-singleton type
        int typeId = qmlRegisterType<CppSingleton>("Test", 1, 0, "NotASingleton");
        QJSValue value = engine.singletonInstance<QJSValue>(typeId);
        QVERIFY(value.isUndefined());
    }

    {
        // Cpp QObject singleton type
        CppSingleton *instance = engine.singletonInstance<CppSingleton*>(cppSingletonTypeId);
        QVERIFY(instance);
        QCOMPARE(instance->metaObject()->className(), "CppSingleton");
        QCOMPARE(instance, engine.singletonInstance<QJSValue>(cppSingletonTypeId).toQObject());
    }

    {
        // Wrong destination type
        SomeQObjectClass * instance = engine.singletonInstance<SomeQObjectClass*>(cppSingletonTypeId);
        QVERIFY(!instance);
    }

    {
        // deleted object
        auto dayfly = new Dayfly{};
        auto id = qmlRegisterSingletonInstance("Vanity", 1, 0, "Dayfly", dayfly);
        delete dayfly;
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: The registered singleton has already been deleted. Ensure that it outlives the engine.");
        QObject *instance = engine.singletonInstance<QObject*>(id);
        QVERIFY(!instance);
    }
}

void tst_qqmlengine::aggressiveGc()
{
    const QByteArray origAggressiveGc = qgetenv("QV4_MM_AGGRESSIVE_GC");
    qputenv("QV4_MM_AGGRESSIVE_GC", "true");
    {
        QQmlEngine engine; // freezing should not run into infinite recursion
        QJSValue obj = engine.newObject();
        QVERIFY(obj.isObject());
    }
    qputenv("QV4_MM_AGGRESSIVE_GC", origAggressiveGc);
}

void tst_qqmlengine::cachedGetterLookup_qtbug_75335()
{
    QQmlEngine engine;
    const QUrl testUrl = testFileUrl("CachedGetterLookup.qml");
    QQmlComponent component(&engine, testUrl);
    QVERIFY(component.isReady());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
}

class EvilSingleton : public QObject
{
    Q_OBJECT
public:
    QPointer<QQmlEngine> m_engine;
    EvilSingleton(QQmlEngine *engine) : m_engine(engine) {
        connect(this, &QObject::destroyed, this, [this]() {
            QQmlComponent component(m_engine);
            component.setData("import QtQml 2.0\nQtObject {}", QUrl("file://Stuff.qml"));
            QVERIFY(component.isReady());
            QScopedPointer<QObject> obj(component.create());
            QVERIFY(obj);
        });
    }
};

void tst_qqmlengine::createComponentOnSingletonDestruction()
{
    qmlRegisterSingletonType<EvilSingleton>("foo.foo", 1, 0, "Singleton",
                                            [](QQmlEngine *engine, QJSEngine *) {
        return new EvilSingleton(engine);
    });

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("evilSingletonInstantiation.qml"));
    QVERIFY(component.isReady());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);
}

void tst_qqmlengine::uiLanguage()
{
    QQmlEngine engine;

    QObject::connect(&engine, &QJSEngine::uiLanguageChanged, [&engine]() {
        engine.retranslate();
    });

    QSignalSpy uiLanguageChangeSpy(&engine, SIGNAL(uiLanguageChanged()));

    QQmlComponent component(&engine, testFileUrl("uiLanguage.qml"));

    QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QVERIFY(engine.uiLanguage().isEmpty());
    QCOMPARE(object->property("numberOfTranslationBindingEvaluations").toInt(), 1);

    QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
    engine.setUiLanguage("TestLanguage");
    QCOMPARE(object->property("numberOfTranslationBindingEvaluations").toInt(), 2);
    QCOMPARE(object->property("chosenLanguage").toString(), "TestLanguage");


    QTest::ignoreMessage(QtMsgType::QtWarningMsg, (component.url().toString() + ":2:1: QML QtObject: Binding loop detected for property \"textToTranslate\"").toLatin1());
    engine.evaluate("Qt.uiLanguage = \"anotherLanguage\"");
    QCOMPARE(engine.uiLanguage(), QString("anotherLanguage"));
    QCOMPARE(object->property("numberOfTranslationBindingEvaluations").toInt(), 3);
    QCOMPARE(object->property("chosenLanguage").toString(), "anotherLanguage");
}

QTEST_MAIN(tst_qqmlengine)

#include "tst_qqmlengine.moc"
