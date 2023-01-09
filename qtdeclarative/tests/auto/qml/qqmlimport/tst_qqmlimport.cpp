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

#include <QtTest/QtTest>
#include <QQmlApplicationEngine>
#include <QQmlAbstractUrlInterceptor>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlengine_p.h>
#include "../../shared/util.h"

class tst_QQmlImport : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void importPathOrder();
    void testDesignerSupported();
    void uiFormatLoading();
    void completeQmldirPaths_data();
    void completeQmldirPaths();
    void interceptQmldir();
    void singletonVersionResolution();
    void removeDynamicPlugin();
    void cleanup();
};

void tst_QQmlImport::cleanup()
{
    QQmlImports::setDesignerSupportRequired(false);
}

void tst_QQmlImport::testDesignerSupported()
{
    QQuickView *window = new QQuickView();
    window->engine()->addImportPath(directory());

    window->setSource(testFileUrl("testfile_supported.qml"));
    QVERIFY(window->errors().isEmpty());

    window->setSource(testFileUrl("testfile_unsupported.qml"));
    QVERIFY(window->errors().isEmpty());

    QQmlImports::setDesignerSupportRequired(true);

    //imports are cached so we create a new window
    delete window;
    window = new QQuickView();

    window->engine()->addImportPath(directory());
    window->engine()->clearComponentCache();

    window->setSource(testFileUrl("testfile_supported.qml"));
    QVERIFY(window->errors().isEmpty());

    QString warningString("%1:30:1: module does not support the designer \"MyPluginUnsupported\" \n     import MyPluginUnsupported 1.0\r \n     ^ ");
#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
    warningString.remove('\r');
#endif
    warningString = warningString.arg(testFileUrl("testfile_unsupported.qml").toString());
    QTest::ignoreMessage(QtWarningMsg, warningString.toLocal8Bit());
    window->setSource(testFileUrl("testfile_unsupported.qml"));
    QVERIFY(!window->errors().isEmpty());

    delete window;
}

void tst_QQmlImport::uiFormatLoading()
{
    int size = 0;

    QQmlApplicationEngine *test = new QQmlApplicationEngine(testFileUrl("TestForm.ui.qml"));
    test->addImportPath(directory());
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QSignalSpy objectCreated(test, SIGNAL(objectCreated(QObject*,QUrl)));
    test->load(testFileUrl("TestForm.ui.qml"));
    QCOMPARE(objectCreated.count(), size);//one less than rootObjects().size() because we missed the first one
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QByteArray testQml("import QtQml 2.0; QtObject{property bool success: true; property TestForm t: TestForm{}}");
    test->loadData(testQml, testFileUrl("dynamicTestForm.ui.qml"));
    QCOMPARE(objectCreated.count(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    test->load(testFileUrl("openTestFormFromDir.qml"));
    QCOMPARE(objectCreated.count(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    test->load(testFileUrl("openTestFormFromQmlDir.qml"));
    QCOMPARE(objectCreated.count(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    delete test;
}

void tst_QQmlImport::importPathOrder()
{
#ifdef Q_OS_ANDROID
    QSKIP("QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath) returns bogus path on Android, but its nevertheless unusable.");
#endif
    QStringList expectedImportPaths;
    QString appDirPath = QCoreApplication::applicationDirPath();
    QString qml2Imports = QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
#ifdef Q_OS_WIN
    // The drive letter has a different case as QQmlImport will
    // cause it to be converted after passing through QUrl
    appDirPath[0] = appDirPath[0].toUpper();
    qml2Imports[0] = qml2Imports[0].toUpper();
#endif
    expectedImportPaths << appDirPath
                        << QLatin1String("qrc:/qt-project.org/imports")
                        << qml2Imports;
    QQmlEngine engine;
    QCOMPARE(expectedImportPaths, engine.importPathList());

    // Add an import path
    engine.addImportPath(QT_QMLTEST_DATADIR);
    expectedImportPaths.prepend(QT_QMLTEST_DATADIR);
    QCOMPARE(expectedImportPaths, engine.importPathList());
}

Q_DECLARE_METATYPE(QQmlImports::ImportVersion)

void tst_QQmlImport::completeQmldirPaths_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QStringList>("basePaths");
    QTest::addColumn<int>("majorVersion");
    QTest::addColumn<int>("minorVersion");
    QTest::addColumn<QStringList>("expectedPaths");

    QTest::newRow("QtQml") << "QtQml" << (QStringList() << "qtbase/qml/" << "path/to/qml") << 2 << 7
                           << (QStringList() << "qtbase/qml/QtQml.2.7/qmldir" << "path/to/qml/QtQml.2.7/qmldir"
                                             << "qtbase/qml/QtQml.2/qmldir" << "path/to/qml/QtQml.2/qmldir"
                                             << "qtbase/qml/QtQml/qmldir" << "path/to/qml/QtQml/qmldir");

    QTest::newRow("QtQml.Models") << "QtQml.Models" << QStringList("qtbase/qml/") << 2 << 2
                                  << (QStringList() << "qtbase/qml/QtQml/Models.2.2/qmldir" << "qtbase/qml/QtQml.2.2/Models/qmldir"
                                                    << "qtbase/qml/QtQml/Models.2/qmldir" << "qtbase/qml/QtQml.2/Models/qmldir"
                                                    << "qtbase/qml/QtQml/Models/qmldir");

    QTest::newRow("org.qt-project.foo.bar") << "org.qt-project.foo.bar" << QStringList("qtbase/qml/") << 0 << 1
                                            << (QStringList() << "qtbase/qml/org/qt-project/foo/bar.0.1/qmldir" << "qtbase/qml/org/qt-project/foo.0.1/bar/qmldir" << "qtbase/qml/org/qt-project.0.1/foo/bar/qmldir" << "qtbase/qml/org.0.1/qt-project/foo/bar/qmldir"
                                                              << "qtbase/qml/org/qt-project/foo/bar.0/qmldir" << "qtbase/qml/org/qt-project/foo.0/bar/qmldir" << "qtbase/qml/org/qt-project.0/foo/bar/qmldir" << "qtbase/qml/org.0/qt-project/foo/bar/qmldir"
                                                              << "qtbase/qml/org/qt-project/foo/bar/qmldir");
}

void tst_QQmlImport::completeQmldirPaths()
{
    QFETCH(QString, uri);
    QFETCH(QStringList, basePaths);
    QFETCH(int, majorVersion);
    QFETCH(int, minorVersion);
    QFETCH(QStringList, expectedPaths);

    QCOMPARE(QQmlImports::completeQmldirPaths(uri, basePaths, majorVersion, minorVersion), expectedPaths);
}

class QmldirUrlInterceptor : public QQmlAbstractUrlInterceptor {
public:
    QUrl intercept(const QUrl &url, DataType type) override
    {
        if (type != UrlString && !url.isEmpty() && url.isValid()) {
            QString str = url.toString(QUrl::None);
            return str.replace(QStringLiteral("$(INTERCEPT)"), QStringLiteral("intercepted"));
        }
        return url;
    }
};

void tst_QQmlImport::interceptQmldir()
{
    QQmlEngine engine;
    QmldirUrlInterceptor interceptor;
    engine.setUrlInterceptor(&interceptor);

    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("interceptQmldir.qml"));
    QVERIFY(component.isReady());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
}

// QTBUG-77102
void tst_QQmlImport::singletonVersionResolution()
{
    QQmlEngine engine;
    engine.addImportPath(testFile("QTBUG-77102/imports"));
    {
        // Singleton with higher version is simply ignored when importing lower version of plugin
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("QTBUG-77102/main.0.9.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
    }
    {
        // but the singleton is not accessible
        QQmlComponent component(&engine);
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, QRegularExpression {".*ReferenceError: MySettings is not defined$"} );
        component.loadUrl(testFileUrl("QTBUG-77102/main.0.9.fail.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
    }
    {
        // unless a version which is high enough is imported
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("QTBUG-77102/main.1.0.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        auto item = qobject_cast<QQuickItem*>(obj.get());
        QCOMPARE(item->width(), 50);
    }
    {
        // or when there is no number because we are importing from a path
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("QTBUG-77102/main.nonumber.qml"));
        QVERIFY(component.isReady());
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        auto item = qobject_cast<QQuickItem*>(obj.get());
        QCOMPARE(item->width(), 50);
    }
}

void tst_QQmlImport::removeDynamicPlugin()
{
    qmlClearTypeRegistrations();
    QQmlEngine engine;
    {
        // Load something that adds a dynamic plugin
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtTest 1.0; TestResult{}"), QUrl());
        QVERIFY(component.isReady());
    }
    QQmlImportDatabase *imports = &QQmlEnginePrivate::get(&engine)->importDatabase;
    const QStringList &plugins = imports->dynamicPlugins();
    QVERIFY(!plugins.isEmpty());
    for (const QString &plugin : plugins)
        QVERIFY(imports->removeDynamicPlugin(plugin));
    QVERIFY(imports->dynamicPlugins().isEmpty());
    qmlClearTypeRegistrations();
}


QTEST_MAIN(tst_QQmlImport)

#include "tst_qqmlimport.moc"
