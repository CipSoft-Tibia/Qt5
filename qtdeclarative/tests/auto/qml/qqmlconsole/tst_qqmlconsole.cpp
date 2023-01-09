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
#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QLoggingCategory>
#include "../../shared/util.h"

class tst_qqmlconsole : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlconsole() {}

private slots:
    void logging();
    void categorized_logging();
    void tracing();
    void profiling();
    void testAssert();
    void exception();

private:
    QQmlEngine engine;
};

struct CustomObject {};

QDebug operator<<(QDebug dbg, const CustomObject &)
{
    return dbg << "MY OBJECT";
}

Q_DECLARE_METATYPE(CustomObject)

void tst_qqmlconsole::logging()
{
    QUrl testUrl = testFileUrl("logging.qml");

    QLoggingCategory loggingCategory("qml");
    QVERIFY(loggingCategory.isDebugEnabled());
    QVERIFY(loggingCategory.isWarningEnabled());
    QVERIFY(loggingCategory.isCriticalEnabled());

    QTest::ignoreMessage(QtDebugMsg, "console.debug");
    QTest::ignoreMessage(QtDebugMsg, "console.log");
    QTest::ignoreMessage(QtInfoMsg, "console.info");
    QTest::ignoreMessage(QtWarningMsg, "console.warn");
    QTest::ignoreMessage(QtCriticalMsg, "console.error");

    QTest::ignoreMessage(QtDebugMsg, "console.count: 1");
    QTest::ignoreMessage(QtDebugMsg, ": 1");
    QTest::ignoreMessage(QtDebugMsg, "console.count: 2");
    QTest::ignoreMessage(QtDebugMsg, ": 2");

    QTest::ignoreMessage(QtDebugMsg, "[1,2]");
    QTest::ignoreMessage(QtDebugMsg, "{\"a\":\"hello\",\"d\":1}");
    QTest::ignoreMessage(QtDebugMsg, "undefined");
    QTest::ignoreMessage(QtDebugMsg, "12");
    QTest::ignoreMessage(QtDebugMsg, "function e() { [native code] }");
    QTest::ignoreMessage(QtDebugMsg, "true");
    // Printing QML object prints out the class/type of QML object with the memory address
//    QTest::ignoreMessage(QtDebugMsg, "QtObject_QML_0(0xABCD..)");
//    QTest::ignoreMessage(QtDebugMsg, "[object Object]");
    QTest::ignoreMessage(QtDebugMsg, "1 pong! [object Object]");
    QTest::ignoreMessage(QtDebugMsg, "1 [ping,pong] [object Object] 2");
    QTest::ignoreMessage(QtDebugMsg, "[Hello,World]");
    QTest::ignoreMessage(QtDebugMsg, "QVariant(CustomObject, MY OBJECT)");
    QTest::ignoreMessage(QtDebugMsg, "[[1,2,3,[2,2,2,2],4],[5,6,7,8]]");

    QStringList stringList; stringList << QStringLiteral("Hello") << QStringLiteral("World");

    CustomObject customObject;
    QVERIFY(QMetaType::registerDebugStreamOperator<CustomObject>());

    QQmlComponent component(&engine, testUrl);
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {"customObject", QVariant::fromValue(customObject)},
            {"stringListProperty", stringList}
    }));
    QVERIFY(object != nullptr);
}

void tst_qqmlconsole::categorized_logging()
{
    QUrl testUrl = testFileUrl("categorized_logging.qml");
    QQmlTestMessageHandler messageHandler;
    messageHandler.setIncludeCategoriesEnabled(true);

    QLoggingCategory testCategory("qt.test");
    testCategory.setEnabled(QtDebugMsg, true);
    QVERIFY(testCategory.isDebugEnabled());
    QVERIFY(testCategory.isWarningEnabled());
    QVERIFY(testCategory.isCriticalEnabled());

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY2(object != nullptr, component.errorString().toUtf8());

    QVERIFY(messageHandler.messages().contains("qt.test: console.info"));
    QVERIFY(messageHandler.messages().contains("qt.test: console.warn"));
    QVERIFY(messageHandler.messages().contains("qt.test: console.error"));
    QVERIFY(!messageHandler.messages().contains("qt.test.warning: console.debug"));
    QVERIFY(!messageHandler.messages().contains("qt.test.warning: console.log"));
    QVERIFY(!messageHandler.messages().contains("qt.test.warning: console.info"));
    QVERIFY(messageHandler.messages().contains("qt.test.warning: console.warn"));
    QVERIFY(messageHandler.messages().contains("qt.test.warning: console.error"));

    QString emptyCategory = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(56).arg(5) +
                            "QML LoggingCategory: Declaring the name of the LoggingCategory is mandatory and cannot be changed later !";
    QVERIFY(messageHandler.messages().contains(emptyCategory));

    QString changedCategory = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(45).arg(5) +
                            "QML LoggingCategory: The name of a LoggingCategory cannot be changed after the Item is created";
    QVERIFY(messageHandler.messages().contains(changedCategory));

    QString changedDefaultLogLevel = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(45).arg(5) +
                            "QML LoggingCategory: The defaultLogLevel of a LoggingCategory cannot be changed after the Item is created";
    QVERIFY(messageHandler.messages().contains(changedDefaultLogLevel));

    QString useEmptyCategory = "default: " + QString::fromLatin1("%1:%2: ").arg(testUrl.toString()).arg(75) +
                            "Error: A QmlLoggingCatgory was provided without a valid name";
    QVERIFY(messageHandler.messages().contains(useEmptyCategory));

    delete object;
}

void tst_qqmlconsole::tracing()
{
    QUrl testUrl = testFileUrl("tracing.qml");

    QString traceText =
            QString::fromLatin1("tracing (%1:%2)\n").arg(testUrl.toString()).arg(37) +
            QString::fromLatin1("onCompleted (%1:%2)").arg(testUrl.toString()).arg(41);

    QTest::ignoreMessage(QtDebugMsg, qPrintable(traceText));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlconsole::profiling()
{
    QUrl testUrl = testFileUrl("profiling.qml");

    // profiling()
    QTest::ignoreMessage(QtWarningMsg, "Cannot start profiling because debug service is disabled. Start with -qmljsdebugger=port:XXXXX.");
    QTest::ignoreMessage(QtWarningMsg, "Ignoring console.profileEnd(): the debug service is disabled.");

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlconsole::testAssert()
{
    QUrl testUrl = testFileUrl("assert.qml");

    // assert()
    QString assert1 = "This will fail\n" +
            QString::fromLatin1("onCompleted (%1:%2)").arg(testUrl.toString()).arg(41);

    QString assert2 = "This will fail too\n" +
            QString::fromLatin1("assertFail (%1:%2)\n").arg(testUrl.toString()).arg(34) +
            QString::fromLatin1("onCompleted (%1:%2)").arg(testUrl.toString()).arg(46);

    QTest::ignoreMessage(QtCriticalMsg, qPrintable(assert1));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(assert2));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlconsole::exception()
{
    QUrl testUrl = testFileUrl("exception.qml");

    // exception()
    QString exception1 = "Exception 1\n" +
            QString::fromLatin1("onCompleted (%1:%2)").arg(testUrl.toString()).arg(38);

    QString exception2 = "Exception 2\n" +
            QString::fromLatin1("exceptionFail (%1:%2)\n").arg(testUrl.toString()).arg(33) +
            QString::fromLatin1("onCompleted (%1:%2)").arg(testUrl.toString()).arg(43);

    QTest::ignoreMessage(QtCriticalMsg, qPrintable(exception1));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(exception2));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

QTEST_MAIN(tst_qqmlconsole)

#include "tst_qqmlconsole.moc"
