/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
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
#include <QQmlApplicationEngine>
#include <QScopedPointer>
#include <QSignalSpy>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#include <QDebug>

class tst_qqmlapplicationengine : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlapplicationengine() {}


private slots:
    void initTestCase();
    void basicLoading();
    void testNonResolvedPath();
    void application_data();
    void application();
    void applicationProperties();
    void removeObjectsWhenDestroyed();
    void loadTranslation_data();
    void loadTranslation();

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

    QQmlApplicationEngine *test = new QQmlApplicationEngine(testFileUrl("basicTest.qml"));
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QSignalSpy objectCreated(test, SIGNAL(objectCreated(QObject*,QUrl)));
    test->load(testFileUrl("basicTest.qml"));
    QCOMPARE(objectCreated.count(), size);//one less than rootObjects().size() because we missed the first one
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QByteArray testQml("import QtQml 2.0; QtObject{property bool success: true; property TestItem t: TestItem{}}");
    test->loadData(testQml, testFileUrl("dynamicTest.qml"));
    QCOMPARE(objectCreated.count(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    delete test;
}

// make sure we resolve a relative URL to an absolute one, otherwise things
// will break.
void tst_qqmlapplicationengine::testNonResolvedPath()
{
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

    QCOMPARE(nameChanged.count(), 1);
    QCOMPARE(versionChanged.count(), 1);
    QCOMPARE(organizationChanged.count(), 1);
    QCOMPARE(domainChanged.count(), 1);

    delete test;
}

void tst_qqmlapplicationengine::removeObjectsWhenDestroyed()
{
    QScopedPointer<QQmlApplicationEngine> test(new QQmlApplicationEngine);
    QVERIFY(test->rootObjects().isEmpty());

    QSignalSpy objectCreated(test.data(), SIGNAL(objectCreated(QObject*,QUrl)));
    test->load(testFileUrl("basicTest.qml"));
    QCOMPARE(objectCreated.count(), 1);

    QSignalSpy objectDestroyed(test->rootObjects().first(), SIGNAL(destroyed()));
    test->rootObjects().first()->deleteLater();
    objectDestroyed.wait();
    QCOMPARE(objectDestroyed.count(), 1);
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

QTEST_MAIN(tst_qqmlapplicationengine)

#include "tst_qqmlapplicationengine.moc"
