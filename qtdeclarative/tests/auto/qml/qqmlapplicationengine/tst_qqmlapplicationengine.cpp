// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlApplicationEngine>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QRegularExpression>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#include <QDebug>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlapplicationengine : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlapplicationengine() : QQmlDataTest(QT_QMLTEST_DATADIR) {}


private slots:
    void initTestCase() override;
    void basicLoading();
    void testNonResolvedPath();
    void application_data();
    void application();
    void applicationProperties();
    void removeObjectsWhenDestroyed();
    void loadTranslation_data();
    void loadTranslation();
    void translationChange();
    void setInitialProperties();
    void failureToLoadTriggersWarningSignal();
    void errorWhileCreating();

private:
    QString buildDir;
    QString srcDir;
};

void tst_qqmlapplicationengine::initTestCase()
{
    qputenv("QT_MESSAGE_PATTERN", ""); // don't let it modify the debug output from testapp
    buildDir = QDir::currentPath();
    QQmlDataTest::initTestCase(); //Changes current path to src dir
    srcDir = QDir::currentPath();
}

void tst_qqmlapplicationengine::basicLoading()
{
    int size = 0;

    auto test = std::make_unique<QQmlApplicationEngine>(testFileUrl("basicTest.qml"));
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QSignalSpy objectCreated(test.get(), &QQmlApplicationEngine::objectCreated);
    test->load(testFileUrl("basicTest.qml"));
    QCOMPARE(objectCreated.size(), size);//one less than rootObjects().size() because we missed the first one
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QByteArray testQml("import QtQml 2.0; QtObject{property bool success: true; property TestItem t: TestItem{}}");
    test->loadData(testQml, testFileUrl("dynamicTest.qml"));
    QCOMPARE(objectCreated.size(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    test->loadFromModule("QtQuick", "Rectangle");
    QCOMPARE(objectCreated.size(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QCOMPARE(test->rootObjects()[size -1]->metaObject()->className(), "QQuickRectangle");
}

// make sure we resolve a relative URL to an absolute one, otherwise things
// will break.
void tst_qqmlapplicationengine::testNonResolvedPath()
{
#if defined(Q_OS_INTEGRITY)
    QSKIP("INTEGRITY stores QML files in resources, and the path to a resource cannot be relative in this case");
#endif

#ifdef Q_OS_ANDROID
    QSKIP("Android stores QML files in resources, and the path to a resource cannot be relative in this case");
#endif
    {
        // NOTE NOTE NOTE! Missing testFileUrl is *WANTED* here! We want a
        // non-resolved URL.
        QQmlApplicationEngine test("data/nonResolvedLocal.qml");
        QCOMPARE(test.rootObjects().size(), 1);
        QVERIFY(test.rootObjects()[0]);
        QVERIFY(test.rootObjects()[0]->property("success").toBool());
    }
    {
        QQmlApplicationEngine test;
        // NOTE NOTE NOTE! Missing testFileUrl is *WANTED* here! We want a
        // non-resolved URL.
        test.load("data/nonResolvedLocal.qml");
        QCOMPARE(test.rootObjects().size(), 1);
        QVERIFY(test.rootObjects()[0]);
        QVERIFY(test.rootObjects()[0]->property("success").toBool());
    }
}

void tst_qqmlapplicationengine::application_data()
{
#ifdef Q_OS_ANDROID
    QSKIP("Cannot launch external process on Android");
#endif
    QTest::addColumn<QByteArray>("qmlFile");
    QTest::addColumn<QByteArray>("expectedStdErr");

    QTest::newRow("delayed quit") << QByteArray("delayedQuit.qml")
                                  << QByteArray("qml: Start: delayedQuit.qml\nqml: End\n");
    QTest::newRow("delayed exit") << QByteArray("delayedExit.qml")
                                  << QByteArray("qml: Start: delayedExit.qml\nqml: End\n");
    QTest::newRow("immediate quit") << QByteArray("immediateQuit.qml")
                                    << QByteArray("qml: End: immediateQuit.qml\n");
    QTest::newRow("immediate exit") << QByteArray("immediateExit.qml")
                                    << QByteArray("qml: End: immediateExit.qml\n");
}

void tst_qqmlapplicationengine::application()
{
    /* This test batches together some tests about running an external application
       written with QQmlApplicationEngine. The application tests the following functionality
       which is easier to do by watching a separate process:
       - Loads relative paths from the working directory
       - Quits when quit is called
       - Exits when exit is called
       - Emits aboutToQuit after quit is called
       - Has access to application command line arguments

       Note that checking the output means that on builds with extra debugging, this might fail with a false positive.
       Also the testapp is automatically built and installed in shadow builds, so it does NOT use testData
   */

    QFETCH(QByteArray, qmlFile);
    QFETCH(QByteArray, expectedStdErr);

#if QT_CONFIG(process)
    QDir::setCurrent(buildDir);
    QProcess *testProcess = new QProcess(this);
#ifdef Q_OS_QNX
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_FORCE_STDERR_LOGGING", "1"); // QTBUG-76546
    testProcess->setProcessEnvironment(env);
#endif
    QStringList args;
    args << qmlFile; // QML file passed as an argument is going to be run by testapp.
    testProcess->start(QLatin1String("testapp/testapp"), args);
    QVERIFY(testProcess->waitForFinished(5000));
    QCOMPARE(testProcess->exitCode(), 0);
    QByteArray testStdOut = testProcess->readAllStandardOutput();
    QByteArray testStdErr = testProcess->readAllStandardError();
#ifdef Q_OS_WIN
    expectedStdErr.replace('\n', QByteArray("\r\n"));
#endif
    QCOMPARE(testStdOut, QByteArray(""));
    QVERIFY2(QString(testStdErr).endsWith(QString(expectedStdErr)),
             QByteArray("\nExpected ending:\n") + expectedStdErr
             + QByteArray("\nActual output:\n") + testStdErr);
    delete testProcess;
    QDir::setCurrent(srcDir);
#else // process
    QSKIP("No process support");
#endif // process
}

void tst_qqmlapplicationengine::applicationProperties()
{
    const QString originalName = QCoreApplication::applicationName();
    const QString originalVersion = QCoreApplication::applicationVersion();
    const QString originalOrganization = QCoreApplication::organizationName();
    const QString originalDomain = QCoreApplication::organizationDomain();
    QString firstName = QLatin1String("Test A");
    QString firstVersion = QLatin1String("0.0A");
    QString firstOrganization = QLatin1String("Org A");
    QString firstDomain = QLatin1String("a.org");
    QString secondName = QLatin1String("Test B");
    QString secondVersion = QLatin1String("0.0B");
    QString secondOrganization = QLatin1String("Org B");
    QString secondDomain = QLatin1String("b.org");

    QCoreApplication::setApplicationName(firstName);
    QCoreApplication::setApplicationVersion(firstVersion);
    QCoreApplication::setOrganizationName(firstOrganization);
    QCoreApplication::setOrganizationDomain(firstDomain);

    QQmlApplicationEngine *test = new QQmlApplicationEngine(testFileUrl("applicationTest.qml"));
    QObject* root = test->rootObjects().at(0);
    QVERIFY(root);
    QCOMPARE(root->property("originalName").toString(), firstName);
    QCOMPARE(root->property("originalVersion").toString(), firstVersion);
    QCOMPARE(root->property("originalOrganization").toString(), firstOrganization);
    QCOMPARE(root->property("originalDomain").toString(), firstDomain);
    QCOMPARE(root->property("currentName").toString(), secondName);
    QCOMPARE(root->property("currentVersion").toString(), secondVersion);
    QCOMPARE(root->property("currentOrganization").toString(), secondOrganization);
    QCOMPARE(root->property("currentDomain").toString(), secondDomain);
    QCOMPARE(QCoreApplication::applicationName(), secondName);
    QCOMPARE(QCoreApplication::applicationVersion(), secondVersion);
    QCOMPARE(QCoreApplication::organizationName(), secondOrganization);
    QCOMPARE(QCoreApplication::organizationDomain(), secondDomain);

    QObject* application = root->property("applicationInstance").value<QObject*>();
    QVERIFY(application);
    QSignalSpy nameChanged(application, SIGNAL(nameChanged()));
    QSignalSpy versionChanged(application, SIGNAL(versionChanged()));
    QSignalSpy organizationChanged(application, SIGNAL(organizationChanged()));
    QSignalSpy domainChanged(application, SIGNAL(domainChanged()));

    QCoreApplication::setApplicationName(originalName);
    QCoreApplication::setApplicationVersion(originalVersion);
    QCoreApplication::setOrganizationName(originalOrganization);
    QCoreApplication::setOrganizationDomain(originalDomain);

    QCOMPARE(nameChanged.size(), 1);
    QCOMPARE(versionChanged.size(), 1);
    QCOMPARE(organizationChanged.size(), 1);
    QCOMPARE(domainChanged.size(), 1);

    delete test;
}

void tst_qqmlapplicationengine::removeObjectsWhenDestroyed()
{
    QScopedPointer<QQmlApplicationEngine> test(new QQmlApplicationEngine);
    QVERIFY(test->rootObjects().isEmpty());

    QSignalSpy objectCreated(test.data(), SIGNAL(objectCreated(QObject*,QUrl)));
    test->load(testFileUrl("basicTest.qml"));
    QCOMPARE(objectCreated.size(), 1);

    QSignalSpy objectDestroyed(test->rootObjects().first(), SIGNAL(destroyed()));
    test->rootObjects().first()->deleteLater();
    objectDestroyed.wait();
    QCOMPARE(objectDestroyed.size(), 1);
    QCOMPARE(test->rootObjects().size(), 0);
}

void tst_qqmlapplicationengine::loadTranslation_data()
{
    QTest::addColumn<QUrl>("qmlUrl");
    QTest::addColumn<QString>("translation");

    QTest::newRow("local file") << testFileUrl("loadTranslation.qml")
                                  << QStringLiteral("translated");
    QTest::newRow("qrc") << QUrl(QLatin1String("qrc:///data/loadTranslation.qml"))
                                  << QStringLiteral("translated");
}

void tst_qqmlapplicationengine::loadTranslation()
{
    QFETCH(QUrl, qmlUrl);
    QFETCH(QString, translation);

    QQmlApplicationEngine test(qmlUrl);
    QVERIFY(!test.rootObjects().isEmpty());

    QObject *rootObject = test.rootObjects().first();
    QVERIFY(rootObject);

    QCOMPARE(rootObject->property("translation").toString(), translation);
}

void tst_qqmlapplicationengine::translationChange()
{
    if (QLocale().language() == QLocale::SwissGerman) {
        QSKIP("Skipping this when running under the Swiss locale as we would always load translation.");
    }

    QQmlApplicationEngine engine(testFileUrl("loadTranslation.qml"));

    QCOMPARE(engine.uiLanguage(), QLocale().bcp47Name());

    QObject *rootObject = engine.rootObjects().first();
    QVERIFY(rootObject);

    QCOMPARE(rootObject->property("translation").toString(), "translated");

    engine.setUiLanguage("de_CH");
    QCOMPARE(rootObject->property("translation").toString(), QString::fromUtf8("Gr\u00FCezi"));

    engine.setUiLanguage(QString());
    QCOMPARE(rootObject->property("translation").toString(), "translate it");
}

void tst_qqmlapplicationengine::setInitialProperties()
{
    QQmlApplicationEngine test {};
    {
        test.setInitialProperties(QVariantMap{{"success", false}});
        test.load(testFileUrl("basicTest.qml"));
        QVERIFY(!test.rootObjects().empty());
        QCOMPARE(test.rootObjects().first()->property("success").toBool(), false);
    }
    {
        test.setInitialProperties({{"success", true}});
        test.load(testFileUrl("basicTest.qml"));
        QCOMPARE(test.rootObjects().size(), 2);
        QCOMPARE(test.rootObjects().at(1)->property("success").toBool(), true);
    }
}

Q_DECLARE_METATYPE(QList<QQmlError>) // for signalspy below

void tst_qqmlapplicationengine::failureToLoadTriggersWarningSignal()
{
    auto url = testFileUrl("invalid.qml");
    qRegisterMetaType<QList<QQmlError>>();
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QQmlApplicationEngine failed to load component");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         QRegularExpression(QRegularExpression::escape(url.toString()) + QLatin1Char('*')));
    QQmlApplicationEngine test;
    QSignalSpy warningObserver(&test, &QQmlApplicationEngine::warnings);
    test.load(url);
    QTRY_COMPARE(warningObserver.size(), 1);
}

void tst_qqmlapplicationengine::errorWhileCreating()
{
    auto url = testFileUrl("requiredViolation.qml");
    QQmlApplicationEngine test;
    QSignalSpy observer(&test, &QQmlApplicationEngine::objectCreated);
    QSignalSpy failureObserver(&test, &QQmlApplicationEngine::objectCreationFailed);

    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "QQmlApplicationEngine failed to create component");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, qPrintable(QStringLiteral("%1:5:5: Required property foo was not initialized").arg(testFileUrl("Required.qml").toString())));

    test.load(url);

    QTRY_COMPARE(observer.size(), 1);
    QCOMPARE(failureObserver.size(), 1);
    QCOMPARE(failureObserver.first().first(), url);
    QList<QVariant> args = observer.takeFirst();
    QVERIFY(args.at(0).isNull());
    QCOMPARE(args.at(1).toUrl(), url);
}

QTEST_MAIN(tst_qqmlapplicationengine)

#include "tst_qqmlapplicationengine.moc"
