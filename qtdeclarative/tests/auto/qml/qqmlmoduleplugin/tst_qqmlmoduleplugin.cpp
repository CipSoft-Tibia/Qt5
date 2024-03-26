// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <qdir.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonarray.h>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QCborMap>
#include <QCborValue>
#endif

#include <QtQuickShapes/private/qquickshapesglobal_p.h>

#if defined(Q_OS_MAC)
// For _PC_CASE_SENSITIVE
#include <unistd.h>
#endif

#include <QtQuickTestUtils/private/testhttpserver_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

// Note: this test does not use module identifier directives in the qmldir files, because
// it would result in repeated attempts to insert types into the same namespace.
// This occurs because type registration is process-global, while the test
// cases should really be run in proper per-process isolation.

class tst_qqmlmoduleplugin : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlmoduleplugin();

private slots:
    void initTestCase() override;
    void importsPlugin();
    void importsPlugin_data();
    void importsMixedQmlCppPlugin();
    void incorrectPluginCase();
    void importPluginWithQmlFile();
    void remoteImportWithQuotedUrl();
    void remoteImportWithUnquotedUri();
    void versionNotInstalled();
    void versionNotInstalled_data();
    void implicitQmldir();
    void implicitQmldir_data();
    void importsNested();
    void importsNested_data();
    void importLocalModule();
    void importLocalModule_data();
    void importStrictModule();
    void importStrictModule_data();
    void importProtectedModule();
    void importVersionedModule();
    void importsChildPlugin();
    void importsChildPlugin2();
    void importsChildPlugin21();
    void parallelPluginImport();
    void multiSingleton();
    void optionalPlugin();
    void moduleFromQrc();

private:
    QString m_importsDirectory;
    QString m_dataImportsDirectory;
};

class PluginThatWaits : public QQmlExtensionPlugin
{
public:
    static QByteArray metaData;

    static QMutex initializeEngineEntered;
    static QWaitCondition waitingForInitializeEngineEntry;
    static QMutex leavingInitializeEngine;
    static QWaitCondition waitingForInitializeEngineLeave;

    void registerTypes(const char *uri) override
    {
        qmlRegisterModule(uri, 1, 0);
    }

    void initializeEngine(QQmlEngine *, const char *) override
    {
        initializeEngineEntered.lock();
        leavingInitializeEngine.lock();
        waitingForInitializeEngineEntry.wakeOne();
        initializeEngineEntered.unlock();
        waitingForInitializeEngineLeave.wait(&leavingInitializeEngine);
        leavingInitializeEngine.unlock();
    }
};
QByteArray PluginThatWaits::metaData;
QMutex PluginThatWaits::initializeEngineEntered;
QWaitCondition PluginThatWaits::waitingForInitializeEngineEntry;
QMutex PluginThatWaits::leavingInitializeEngine;
QWaitCondition PluginThatWaits::waitingForInitializeEngineLeave;

class SecondStaticPlugin : public QQmlExtensionPlugin
{
public:
    static QByteArray metaData;

    void registerTypes(const char *uri) override
    {
        qmlRegisterModule(uri, 1, 0);
    }
};
QByteArray SecondStaticPlugin::metaData;

template <typename PluginType>
void registerStaticPlugin(const char *uri)
{
    auto instanceFunctor = []() {
        static PluginType plugin;
        return static_cast<QObject*>(&plugin);
    };

    QJsonObject md;
    md.insert(QStringLiteral("IID"), QQmlExtensionInterface_iid);
    QJsonArray uris;
    uris.append(uri);
    md.insert(QStringLiteral("uri"), uris);

    PluginType::metaData.append(char(1)); // current version
    PluginType::metaData.append(char(QT_VERSION_MAJOR));
    PluginType::metaData.append(char(QT_VERSION_MINOR));
    PluginType::metaData.append(char(qPluginArchRequirements()));
    PluginType::metaData.append(QCborValue(QCborMap::fromJsonObject(md)).toCbor());

    auto rawMetaDataFunctor = []() -> QPluginMetaData {
        return {reinterpret_cast<const uchar *>(PluginType::metaData.constData()), size_t(PluginType::metaData.size())};
    };
    QStaticPlugin plugin(instanceFunctor, rawMetaDataFunctor);
    qRegisterStaticPluginFunction(plugin);
};

tst_qqmlmoduleplugin::tst_qqmlmoduleplugin()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qqmlmoduleplugin::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_importsDirectory = QFINDTESTDATA(QStringLiteral("imports"));
    QVERIFY2(QFileInfo(m_importsDirectory).isDir(),
             qPrintable(QString::fromLatin1("Imports directory '%1' does not exist.").arg(m_importsDirectory)));
    m_dataImportsDirectory = directory() + QStringLiteral("/imports");
    QVERIFY2(QFileInfo(m_dataImportsDirectory).isDir(),
             qPrintable(QString::fromLatin1("Imports directory '%1' does not exist.").arg(m_dataImportsDirectory)));

    registerStaticPlugin<PluginThatWaits>("moduleWithWaitingPlugin");
    registerStaticPlugin<SecondStaticPlugin>("moduleWithStaticPlugin");
}

#define VERIFY_ERRORS(errorfile) \
    if (!errorfile) { \
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty()) \
            qWarning() << "Unexpected Errors:" << component.errors(); \
        QVERIFY(!component.isError()); \
        QVERIFY(component.errors().isEmpty()); \
    } else { \
        QString verify_errors_file_name = testFile(errorfile); \
        QFile file(verify_errors_file_name); \
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text)); \
        QByteArray data = file.readAll(); \
        file.close(); \
        QList<QByteArray> expected = data.split('\n'); \
        expected.removeAll(QByteArray("")); \
        QList<QQmlError> errors = component.errors(); \
        QList<QByteArray> actual; \
        for (int ii = 0; ii < errors.count(); ++ii) { \
            const QQmlError &error = errors.at(ii); \
            QByteArray errorStr = QByteArray::number(error.line()) + ':' +  \
                                  QByteArray::number(error.column()) + ':' + \
                                  error.description().toUtf8(); \
            actual << errorStr; \
        } \
        if (qgetenv("DEBUG") != "" && expected != actual) { \
            qWarning() << "Expected:" << expected << "Actual:" << actual; \
        } \
        if (qgetenv("QDECLARATIVELANGUAGE_UPDATEERRORS") != "" && expected != actual) {\
            QFile file(testFile(errorfile)); \
            QVERIFY(file.open(QIODevice::WriteOnly)); \
            for (int ii = 0; ii < actual.count(); ++ii) { \
                file.write(actual.at(ii)); file.write("\n"); \
            } \
            file.close(); \
        } else { \
            QCOMPARE(actual, expected); \
        } \
    }

void tst_qqmlmoduleplugin::importsPlugin()
{
    QFETCH(QString, suffix);
    QFETCH(QString, qmlFile);

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, qPrintable(QString("plugin%1 created").arg(suffix)));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(QString("import%1 worked").arg(suffix)));
    QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlPluginType' does not contain a module identifier directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(qmlFile));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::importsPlugin_data()
{
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("qmlFile");

    QTest::newRow("1.0") << "" << "works.qml";
    QTest::newRow("2.0") << "2" << "works2.qml";
    QTest::newRow("2.1") << "2.1" << "works21.qml";
    QTest::newRow("2.2") << "2.2" << "works22.qml";
}

void tst_qqmlmoduleplugin::incorrectPluginCase()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QQmlComponent component(&engine, testFileUrl(QStringLiteral("incorrectCase.qml")));

    QList<QQmlError> errors = component.errors();
    QCOMPARE(errors.size(), 1);

    QString expectedError = QLatin1String("module \"org.qtproject.WrongCase\" plugin \"PluGin\" not found");

#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
    bool caseSensitive = true;
#if defined(Q_OS_MAC)
    int res = pathconf(QDir::currentPath().toLatin1().constData(), _PC_CASE_SENSITIVE);
    if (res == -1)
        QSKIP("Could not establish case sensitivity of file system");
    caseSensitive = res != 0 && res != -1;
#ifdef QT_DEBUG
    QString libname = "libPluGin_debug.dylib";
#else
    QString libname = "libPluGin.dylib";
#endif
#elif defined(Q_OS_WIN32)
    caseSensitive = false;
    QString libname = "PluGin.dll";
#endif
    if (!caseSensitive) {
        expectedError = QLatin1String("File name case mismatch for \"")
            + QDir(m_importsDirectory).filePath("org/qtproject/WrongCase/" + libname)
            + QLatin1Char('"');
    }
#endif

    QCOMPARE(errors.at(0).description(), expectedError);
}

void tst_qqmlmoduleplugin::importPluginWithQmlFile()
{
    QString path = m_importsDirectory;

    // QTBUG-16885: adding an import path with a lower-case "c:" causes assert failure
    // (this only happens if the plugin includes pure QML files)
    #ifdef Q_OS_WIN
        QVERIFY(path.at(0).isUpper() && path.at(1) == QLatin1Char(':'));
        path = path.at(0).toLower() + path.mid(1);
    #endif

    QQmlEngine engine;
    engine.addImportPath(path);

    QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestPluginWithQmlFile' does not contain a module identifier directive - it cannot be protected from external registrations.");

    QQmlComponent component(&engine, testFileUrl(QStringLiteral("pluginWithQmlFile.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlmoduleplugin::remoteImportWithQuotedUrl()
{
    ThreadedTestHTTPServer server(m_dataImportsDirectory);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    const QString qml = "import \"" + server.urlString("/org/qtproject/PureQmlModule") + "\" \nComponentA { width: 300; ComponentB{} }";
    component.setData(qml.toUtf8(), QUrl());

    QTRY_COMPARE(component.status(), QQmlComponent::Ready);
    QObject *object = component.create();
    QCOMPARE(object->property("width").toInt(), 300);
    QVERIFY(object != nullptr);
    delete object;

    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
}

void tst_qqmlmoduleplugin::remoteImportWithUnquotedUri()
{
    TestHTTPServer server;
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
    server.serveDirectory(m_dataImportsDirectory);

    QQmlEngine engine;
    engine.addImportPath(m_dataImportsDirectory);
    QQmlComponent component(&engine);
    component.setData("import org.qtproject.PureQmlModule 1.0 \nComponentA { width: 300; ComponentB{} }", QUrl());


    QTRY_COMPARE(component.status(), QQmlComponent::Ready);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("width").toInt(), 300);
    delete object;

    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
}

static QByteArray msgComponentError(const QQmlComponent &c, const QQmlEngine *engine /* = nullptr */)
{
    QString result;
    const QList<QQmlError> errors = c.errors();
    QTextStream str(&result);
    str << "Component '" << c.url().toString() << "' has " << errors.size() << " errors: '";
    for (int i = 0; i < errors.size(); ++i) {
        if (i)
            str << ", '";
        str << errors.at(i).toString() << '\'';
    }
    if (!engine) {
        if (QQmlContext *context = c.creationContext())
            engine = context->engine();
    }
    if (engine) {
        str << " Import paths: (" << engine->importPathList().join(QStringLiteral(", "))
            << ") Plugin paths: (" << engine->pluginPathList().join(QStringLiteral(", "))
            << ')';
    }
    return result.toLocal8Bit();
}

// QTBUG-17324

void tst_qqmlmoduleplugin::importsMixedQmlCppPlugin()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlMixedPluginType' does not contain a module identifier directive - it cannot be protected from external registrations.");

    {
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("importsMixedQmlCppPlugin.qml")));

    QObject *o = component.create();
    QVERIFY2(o != nullptr, msgComponentError(component, &engine));
    QCOMPARE(o->property("test").toBool(), true);
    delete o;
    }

    {
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("importsMixedQmlCppPlugin.2.qml")));

    QObject *o = component.create();
    QVERIFY2(o != nullptr, msgComponentError(component, &engine));
    QCOMPARE(o->property("test").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    delete o;
    }


}

void tst_qqmlmoduleplugin::versionNotInstalled_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    QTest::newRow("versionNotInstalled") << "versionNotInstalled.qml" << "versionNotInstalled.errors.txt";
    QTest::newRow("versionNotInstalled") << "versionNotInstalled.2.qml" << "versionNotInstalled.2.errors.txt";
}

void tst_qqmlmoduleplugin::versionNotInstalled()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    static int count = 0;
    if (++count == 1)
        QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlVersionPluginType' does not contain a module identifier directive - it cannot be protected from external registrations.");

    QQmlComponent component(&engine, testFileUrl(file));
    VERIFY_ERRORS(errorFile.toLatin1().constData());
}


// test that errors are reporting correctly for plugin loading and qmldir parsing
void tst_qqmlmoduleplugin::implicitQmldir_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    // parsing qmldir succeeds, but plugin specified in the qmldir file doesn't exist
    QTest::newRow("implicitQmldir") << "implicit1" << "temptest.qml" << "implicitQmldir.errors.txt";

    // parsing qmldir fails due to syntax errors, etc.
    QTest::newRow("implicitQmldir2") << "implicit2" << "temptest2.qml" << "implicitQmldir.2.errors.txt";
}
void tst_qqmlmoduleplugin::implicitQmldir()
{
    QFETCH(QString, directory);
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    QString importPath = testFile(directory);
    QString fileName = directory + QDir::separator() + file;
    QString errorFileName = directory + QDir::separator() + errorFile;
    QUrl testUrl = testFileUrl(fileName);

    QQmlEngine engine;
    engine.addImportPath(importPath);

    QQmlComponent component(&engine, testUrl);
    QList<QQmlError> errors = component.errors();
    VERIFY_ERRORS(errorFileName.toLatin1().constData());
    QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    QObject *obj = component.create();
    QVERIFY(!obj);
    delete obj;
}

void tst_qqmlmoduleplugin::importsNested_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("errorFile");

    // Note: no other test case should import the plugin used for this test, or the
    // wrong order test will pass spuriously
    QTest::newRow("wrongOrder") << "importsNested.1.qml" << "importsNested.1.errors.txt";
    QTest::newRow("missingImport") << "importsNested.3.qml" << "importsNested.3.errors.txt";
    QTest::newRow("invalidVersion") << "importsNested.4.qml" << "importsNested.4.errors.txt";
    QTest::newRow("correctOrder") << "importsNested.2.qml" << QString();
}
void tst_qqmlmoduleplugin::importsNested()
{
    QFETCH(QString, file);
    QFETCH(QString, errorFile);

    // Note: because imports are cached between test case data rows (and the plugins remain loaded),
    // these tests should really be run in new instances of the app...

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    if (!errorFile.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    }

    static int count = 0;
    if (++count == 1)
        QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlNestedPluginType' does not contain a module identifier directive - it cannot be protected from external registrations.");

    QQmlComponent component(&engine, testFile(file));
    QObject *obj = component.create();

    if (errorFile.isEmpty()) {
        if (qgetenv("DEBUG") != "" && !component.errors().isEmpty())
            qWarning() << "Unexpected Errors:" << component.errors();
        QVERIFY(obj);
        delete obj;
    } else {
        QList<QQmlError> errors = component.errors();
        VERIFY_ERRORS(errorFile.toLatin1().constData());
        QVERIFY(!obj);
    }
}

void tst_qqmlmoduleplugin::importLocalModule()
{
    QFETCH(QString, qml);
    QFETCH(int, majorVersion);
    QFETCH(int, minorVersion);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), testFileUrl("empty.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("majorVersion").value<int>(), majorVersion);
    QCOMPARE(object->property("minorVersion").value<int>(), minorVersion);
}

void tst_qqmlmoduleplugin::importLocalModule_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<int>("majorVersion");
    QTest::addColumn<int>("minorVersion");

    QTest::newRow("default version")
        << "import \"localModule\"\n"
           "TestComponent {}"
        << 2 << 0;

    QTest::newRow("specific version")
        << "import \"localModule\" 1.1\n"
           "TestComponent {}"
        << 1 << 1;

    QTest::newRow("lesser version")
        << "import \"localModule\" 1.0\n"
           "TestComponent {}"
        << 1 << 0;

    // Note: this does not match the behaviour of installed modules, which fail for this case:
    QTest::newRow("nonexistent version")
        << "import \"localModule\" 1.3\n"
           "TestComponent {}"
        << 1 << 1;

    QTest::newRow("high version")
        << "import \"localModule\" 2.0\n"
           "TestComponent {}"
        << 2 << 0;
}

void tst_qqmlmoduleplugin::importStrictModule()
{
    QFETCH(QString, qml);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QUrl url(testFileUrl("empty.qml"));

    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), url);

    if (error.isEmpty()) {
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
    } else {
        QVERIFY(!component.isReady());
        QCOMPARE(component.errors().size(), 1);
        QCOMPARE(component.errors().first().toString(), url.toString() + error);
    }
}

void tst_qqmlmoduleplugin::importStrictModule_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("success")
        << "import org.qtproject.StrictModule 1.0\n"
           "MyPluginType {}"
        << QString()
        << QString();

    QTest::newRow("two strict modules with different major version")
        << "import org.qtproject.StrictModule 1.0\n"
           "import org.qtproject.StrictModule 2.0\n"
           "MyPluginType {}"
        << QString()
        << QString();

    QTest::newRow("old namespaced strict module")
        << "import org.qtproject.StrictModule 1.0 as Old\n"
           "import org.qtproject.StrictModule 2.0 as New\n"
           "Old.MyPluginType {}"
        << QString()
        << QString();

    QTest::newRow("new namespaced strict modules")
        << "import org.qtproject.StrictModule 1.0 as Old\n"
           "import org.qtproject.StrictModule 2.0 as New\n"
           "New.MyPluginType {}"
        << QString()
        << QString();

    QTest::newRow("non-strict clash")
        << "import org.qtproject.NonstrictModule 1.0\n"
           "MyPluginType {}"
        << "Module 'org.qtproject.NonstrictModule' does not contain a module identifier directive - it cannot be protected from external registrations."
        << ":1:1: Cannot install element 'MyPluginType' into protected module 'org.qtproject.StrictModule' version '1'";

    QTest::newRow("non-strict preemption")
        << "import org.qtproject.PreemptiveModule 1.0\n"
           "import org.qtproject.PreemptedStrictModule 1.0\n"
           "MyPluginType {}"
        << "Module 'org.qtproject.PreemptiveModule' does not contain a module identifier directive - it cannot be protected from external registrations."
        << ":2:1: Namespace 'org.qtproject.PreemptedStrictModule' has already been used for type registration";

    QTest::newRow("invalid namespace")
        << "import org.qtproject.InvalidNamespaceModule 1.0\n"
           "MyPluginType {}"
        << QString()
        << ":1:1: Module namespace 'org.qtproject.AwesomeModule' does not match import URI 'org.qtproject.InvalidNamespaceModule'";

    QTest::newRow("module directive must be first")
        << "import org.qtproject.InvalidFirstCommandModule 1.0\n"
           "MyPluginType {}"
        << QString()
        << ":1:1: module identifier directive must be the first directive in a qmldir file";
}

void tst_qqmlmoduleplugin::importProtectedModule()
{
    //TODO: More than basic test (test errors,test inverse works...)
    qmlRegisterType<QObject>("org.qtproject.ProtectedModule", 1, 0, "TestType");
    qmlProtectModule("org.qtproject.ProtectedModule", 1);

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QUrl url(testFileUrl("empty.qml"));

    QString qml = "import org.qtproject.ProtectedModule 1.0\n TestType {}\n";
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), url);
    //If plugin is loaded due to import, should assert
    QScopedPointer<QObject> object(component.create());
    //qDebug() << component.errorString();
    QVERIFY(object != nullptr);
}

void tst_qqmlmoduleplugin::importVersionedModule()
{
    qmlRegisterType<QObject>("org.qtproject.VersionedModule", 1, 0, "TestType");
    qmlRegisterModule("org.qtproject.VersionedModule", 1, 1);

    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);

    QUrl url(testFileUrl("empty.qml"));

    QQmlComponent component(&engine);
    component.setData("import org.qtproject.VersionedModule 1.0\n TestType {}\n", url);
    QScopedPointer<QObject> object10(component.create());
    QVERIFY(!object10.isNull());

    component.setData("import org.qtproject.VersionedModule 1.1\n TestType {}\n", url);
    QScopedPointer<QObject> object11(component.create());
    QVERIFY(!object11.isNull());

    component.setData("import org.qtproject.VersionedModule 1.2\n TestType {}\n", url);
    QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    QScopedPointer<QObject> object12(component.create());
    QVERIFY(object12.isNull());
    QCOMPARE(component.errorString(), QString("%1:1 module \"org.qtproject.VersionedModule\" version 1.2 is not installed\n").arg(url.toString()));
}

void tst_qqmlmoduleplugin::importsChildPlugin()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, "child plugin created");
    QTest::ignoreMessage(QtWarningMsg, "child import worked");
    QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlPluginType.ChildPlugin' does not contain a module identifier directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("child.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::importsChildPlugin2()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, "child plugin2 created");
    QTest::ignoreMessage(QtWarningMsg, "child import2 worked");
    QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlPluginType.ChildPlugin' does not contain a module identifier directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("child2.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::importsChildPlugin21()
{
    QQmlEngine engine;
    engine.addImportPath(m_importsDirectory);
    QTest::ignoreMessage(QtWarningMsg, "child plugin2.1 created");
    QTest::ignoreMessage(QtWarningMsg, "child import2.1 worked");
    QTest::ignoreMessage(QtWarningMsg, "Module 'org.qtproject.AutoTestQmlPluginType.ChildPlugin' does not contain a module identifier directive - it cannot be protected from external registrations.");
    QQmlComponent component(&engine, testFileUrl(QStringLiteral("child21.qml")));
    foreach (QQmlError err, component.errors())
        qWarning() << err;
    VERIFY_ERRORS(0);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toInt(),123);
    delete object;
}

void tst_qqmlmoduleplugin::parallelPluginImport()
{
    QMutexLocker locker(&PluginThatWaits::initializeEngineEntered);

    QThread worker;
    QObject::connect(&worker, &QThread::started, [&worker](){
        // Engines in separate threads are tricky, but as long as we do not create a graphical
        // object and move objects created by the engines across thread boundaries, this is safe.
        // At the same time this allows us to place the engine's loader thread into the position
        // where, without the fix for this bug, the global lock is acquired.
        QQmlEngine engineInThread;

        QQmlComponent component(&engineInThread);
        component.setData("import moduleWithWaitingPlugin 1.0\nimport QtQml 2.0\nQtObject {}",
                          QUrl());

        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());

        worker.quit();
    });
    worker.start();

    PluginThatWaits::waitingForInitializeEngineEntry.wait(&PluginThatWaits::initializeEngineEntered);

    {
        // After acquiring this lock, the engine in the other thread as well as its type loader
        // thread are blocked. However they should not hold the global plugin lock
        // qmlEnginePluginsWithRegisteredTypes()->mutex in qqmllimports.cpp, allowing for the load
        // of a component in a different engine with its own plugin to proceed.
        QMutexLocker continuationLock(&PluginThatWaits::leavingInitializeEngine);

        QQmlEngine secondEngine;
        QQmlComponent secondComponent(&secondEngine);
        secondComponent.setData("import moduleWithStaticPlugin 1.0\nimport QtQml 2.0\nQtObject {}",
                                QUrl());
        QScopedPointer<QObject> o(secondComponent.create());
        QVERIFY(!o.isNull());

        PluginThatWaits::waitingForInitializeEngineLeave.wakeOne();
    }

    worker.wait();
}

void tst_qqmlmoduleplugin::multiSingleton()
{
    QQmlEngine engine;
    QObject obj;
    qmlRegisterSingletonInstance("Test", 1, 0, "Tracker", &obj);
    engine.addImportPath(m_importsDirectory);
    QQmlComponent component(&engine, testFileUrl("multiSingleton.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(obj.objectName(), QLatin1String("first"));
    delete object;
}

void tst_qqmlmoduleplugin::optionalPlugin()
{
    // Force QtQuickShapes to be linked.
    volatile auto registration = &qml_register_types_QtQuick_Shapes;
    Q_UNUSED(registration);

    QQmlEngine engine;
    engine.setImportPathList({m_importsDirectory});
    QQmlComponent component(&engine);
    component.setData("import QtQuick.Shapes\nShapePath {}\n", QUrl());
    QScopedPointer<QObject> object10(component.create());
    QVERIFY(!object10.isNull());
}

void tst_qqmlmoduleplugin::moduleFromQrc()
{
    QQmlEngine engine;
    engine.setImportPathList({ QStringLiteral(":/foo/imports/"), m_dataImportsDirectory });
    QQmlComponent component(&engine);
    component.setData("import ModuleFromQrc\nFoo {}\n", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
}

QTEST_MAIN(tst_qqmlmoduleplugin)

#include "tst_qqmlmoduleplugin.moc"
