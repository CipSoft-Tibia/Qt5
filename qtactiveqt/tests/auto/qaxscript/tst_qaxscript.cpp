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
#include <QAxScriptManager>
#include <QAxScript>

class tst_QAxScript : public QObject
{
    Q_OBJECT

private slots:
    void scriptReturnValue();
};

void tst_QAxScript::scriptReturnValue()
{
    QAxScriptManager scriptManager;
    const char scriptCode[] =
        "function foo() {\n"
        "    return 'test';\n"
        "}\n"; // QTBUG-42289, fails when DISPATCH_PROPERTYGET is used.
    QAxScript *script = scriptManager.load(QLatin1String(scriptCode),
                                           QStringLiteral("Test"),
                                           QStringLiteral("JScript"));
    QVERIFY2(script, "Unable to load script (CoInitializeEx() called?)");
    const QVariant result = script->call("foo()");
    QCOMPARE(result, QVariant(QStringLiteral("test")));
}

QTEST_MAIN(tst_QAxScript)
#include "tst_qaxscript.moc"
