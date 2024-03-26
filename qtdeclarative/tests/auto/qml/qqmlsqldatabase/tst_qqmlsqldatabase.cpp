// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquicktext_p.h>
#include <private/qqmlengine_p.h>
#include <QtCore/qcryptographichash.h>
#include <QtSql/qsqldatabase.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlsqldatabase : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlsqldatabase()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
        qApp->setApplicationName("tst_qqmlsqldatabase");
        qApp->setOrganizationName("QtProject");
        qApp->setOrganizationDomain("www.qt-project.org");
        engine = new QQmlEngine;
    }

    ~tst_qqmlsqldatabase()
    {
        delete engine;
    }

private slots:
    void initTestCase() override;

    void checkDatabasePath();

    void testQml_data();
    void testQml();
    void testQml_cleanopen_data();
    void testQml_cleanopen();
    void totalDatabases();
    void upgradeDatabase();

    void cleanupTestCase();

private:
    QString dbDir() const;
    QQmlEngine *engine;
};

void removeRecursive(const QString& dirname)
{
    QDir dir(dirname);
    QFileInfoList entries(dir.entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot));
    for (int i = 0; i < entries.size(); ++i)
        if (entries[i].isDir())
            removeRecursive(entries[i].filePath());
        else
            dir.remove(entries[i].fileName());
    QDir().rmdir(dirname);
}

void tst_qqmlsqldatabase::initTestCase()
{
    if (engine->offlineStoragePath().isEmpty())
        QSKIP("offlineStoragePath is empty, skip this test.");
    QQmlDataTest::initTestCase();
    removeRecursive(dbDir());
    QDir().mkpath(dbDir());
}

void tst_qqmlsqldatabase::cleanupTestCase()
{
    if (engine->offlineStoragePath().isEmpty())
        QSKIP("offlineStoragePath is empty, skip this test.");
    removeRecursive(dbDir());
}

QString tst_qqmlsqldatabase::dbDir() const
{
    static QString tmpd = QDir::tempPath()+"/tst_qqmlsqldatabase_output-"
        + QDateTime::currentDateTime().toString(QLatin1String("yyyyMMddhhmmss"));
    return tmpd;
}

void tst_qqmlsqldatabase::checkDatabasePath()
{
    if (engine->offlineStoragePath().isEmpty())
        QSKIP("offlineStoragePath is empty, skip this test.");

    // Check default storage path (we can't use it since we don't want to mess with user's data)
    QVERIFY(engine->offlineStoragePath().contains("tst_qqmlsqldatabase"));
    QVERIFY(engine->offlineStoragePath().contains("OfflineStorage"));
}

static const int total_databases_created_by_tests = 13;
void tst_qqmlsqldatabase::testQml_data()
{
    QTest::addColumn<QString>("jsfile"); // The input file

    // Each test should use a newly named DB to avoid inter-test dependencies
    QTest::newRow("creation") << "creation.js";
    QTest::newRow("creation-a") << "creation-a.js";
    QTest::newRow("creation") << "creation.js";
    QTest::newRow("error-creation") << "error-creation.js"; // re-uses above DB
    QTest::newRow("changeversion") << "changeversion.js";
    QTest::newRow("readonly") << "readonly.js";
    QTest::newRow("readonly-error") << "readonly-error.js";
    QTest::newRow("selection") << "selection.js";
    QTest::newRow("selection-bindnames") << "selection-bindnames.js";
    QTest::newRow("iteration") << "iteration.js";
    QTest::newRow("iteration-forwardonly") << "iteration-forwardonly.js";
    QTest::newRow("error-a") << "error-a.js";
    QTest::newRow("error-notransaction") << "error-notransaction.js";
    QTest::newRow("error-outsidetransaction") << "error-outsidetransaction.js"; // reuse above
    QTest::newRow("reopen1") << "reopen1.js";
    QTest::newRow("reopen2") << "reopen2.js"; // re-uses above DB
    QTest::newRow("null-values") << "nullvalues.js";

    // If you add a test, you should usually use a new database in the
    // test - in which case increment total_databases_created_by_tests above.
}

void tst_qqmlsqldatabase::testQml()
{
    if (engine->offlineStoragePath().isEmpty())
        QSKIP("offlineStoragePath is empty, skip this test.");

    // Tests QML SQL Database support with tests
    // that have been validated against Webkit.
    //
    QFETCH(QString, jsfile);

    QString qml=
        "import QtQuick 2.0\n"
        "import \""+jsfile+"\" as JS\n"
        "Text { text: JS.test() }";

    engine->setOfflineStoragePath(dbDir());
    QQmlComponent component(engine);
    component.setData(qml.toUtf8(), testFileUrl("empty.qml")); // just a file for relative local imports
    QVERIFY(!component.isError());
    QQuickText *text = qobject_cast<QQuickText*>(component.create());
    QVERIFY(text != nullptr);
    QCOMPARE(text->text(),QString("passed"));
}

void tst_qqmlsqldatabase::testQml_cleanopen_data()
{
    QTest::addColumn<QString>("jsfile"); // The input file
    QTest::newRow("reopen1") << "reopen1.js";
    QTest::newRow("reopen2") << "reopen2.js";
    QTest::newRow("error-creation") << "error-creation.js"; // re-uses creation DB
}

void tst_qqmlsqldatabase::testQml_cleanopen()
{
    if (engine->offlineStoragePath().isEmpty())
        QSKIP("offlineStoragePath is empty, skip this test.");

    // Same as testQml, but clean connections between tests,
    // making it more like the tests are running in new processes.
    testQml();

    engine->collectGarbage();

    foreach (QString dbname, QSqlDatabase::connectionNames()) {
        QSqlDatabase::removeDatabase(dbname);
    }
}

void tst_qqmlsqldatabase::totalDatabases()
{
    if (engine->offlineStoragePath().isEmpty())
        QSKIP("offlineStoragePath is empty, skip this test.");

    QCOMPARE(QDir(dbDir()+"/Databases").entryInfoList(QDir::Files|QDir::NoDotAndDotDot).size(), total_databases_created_by_tests*2);
}

void tst_qqmlsqldatabase::upgradeDatabase()
{
    QQmlComponent component(engine, testFile("changeVersion.qml"));
    QVERIFY(component.isReady());

    QObject *object = component.create();
    QVERIFY(object);
    QVERIFY(object->property("version").toString().isEmpty());

    QVERIFY(QMetaObject::invokeMethod(object, "create"));
    QCOMPARE(object->property("version").toString(), QLatin1String("2"));

    QVERIFY(QMetaObject::invokeMethod(object, "upgrade"));
    QCOMPARE(object->property("version").toString(), QLatin1String("22"));
}

QTEST_MAIN(tst_qqmlsqldatabase)

#include "tst_qqmlsqldatabase.moc"
