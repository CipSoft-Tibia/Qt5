/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
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
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlFileSelector>
#include <QQmlApplicationEngine>
#include <QFileSelector>
#include <QQmlContext>
#include <QLoggingCategory>
#include <qqmlinfo.h>
#include "../../shared/util.h"

class tst_qqmlfileselector : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlfileselector() {}

private slots:
    void basicTest();
    void basicTestCached();
    void applicationEngineTest();

};

void tst_qqmlfileselector::basicTest()
{
    QQmlEngine engine;
    QQmlFileSelector selector(&engine);
    selector.setExtraSelectors(QStringList() << "basic");

    QQmlComponent component(&engine, testFileUrl("basicTest.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));

    delete object;
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type == QtDebugMsg
            && QByteArray(context.category) == QByteArray("qt.qml.diskcache")
            && message.contains("QML source file has moved to a different location.")) {
        QFAIL(message.toUtf8());
    }
}

void tst_qqmlfileselector::basicTestCached()
{
    basicTest(); // Seed the cache, in case basicTestCached() is run on its own
    QtMessageHandler defaultHandler = qInstallMessageHandler(&messageHandler);
    QLoggingCategory::setFilterRules("qt.qml.diskcache.debug=true");
    basicTest(); // Run again and check that the file is in the cache now
    QLoggingCategory::setFilterRules(QString());
    qInstallMessageHandler(defaultHandler);
}

void tst_qqmlfileselector::applicationEngineTest()
{
    QQmlApplicationEngine engine;
    QQmlFileSelector* selector = QQmlFileSelector::get(&engine);
    QVERIFY(selector != nullptr);
    selector->setExtraSelectors(QStringList() << "basic");

    QQmlComponent component(&engine, testFileUrl("basicTest.qml"));
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("value").toString(), QString("selected"));

    delete object;
}

QTEST_MAIN(tst_qqmlfileselector)

#include "tst_qqmlfileselector.moc"
