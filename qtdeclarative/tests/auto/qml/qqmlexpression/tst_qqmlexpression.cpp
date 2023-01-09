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
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlscriptstring.h>
#include "../../shared/util.h"

class tst_qqmlexpression : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlexpression() {}

private slots:
    void scriptString();
    void syntaxError();
    void exception();
    void expressionFromDataComponent();
};

class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlScriptString scriptString READ scriptString WRITE setScriptString)
    Q_PROPERTY(QQmlScriptString scriptStringError READ scriptStringError WRITE setScriptStringError)
public:
    TestObject(QObject *parent = nullptr) : QObject(parent) {}

    QQmlScriptString scriptString() const { return m_scriptString; }
    void setScriptString(QQmlScriptString scriptString) { m_scriptString = scriptString; }

    QQmlScriptString scriptStringError() const { return m_scriptStringError; }
    void setScriptStringError(QQmlScriptString scriptString) { m_scriptStringError = scriptString; }

private:
    QQmlScriptString m_scriptString;
    QQmlScriptString m_scriptStringError;
};

QML_DECLARE_TYPE(TestObject)

void tst_qqmlexpression::scriptString()
{
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("scriptString.qml"));
    TestObject *testObj = qobject_cast<TestObject*>(c.create());
    QVERIFY(testObj != nullptr);

    QQmlScriptString script = testObj->scriptString();
    QVERIFY(!script.isEmpty());

    QQmlExpression expression(script);
    QVariant value = expression.evaluate();
    QCOMPARE(value.toInt(), 15);

    QQmlScriptString scriptError = testObj->scriptStringError();
    QVERIFY(!scriptError.isEmpty());

    //verify that the expression has the correct error location information
    QQmlExpression expressionError(scriptError);
    QVariant valueError = expressionError.evaluate();
    QVERIFY(!valueError.isValid());
    QVERIFY(expressionError.hasError());
    QQmlError error = expressionError.error();
    QCOMPARE(error.url(), c.url());
    QCOMPARE(error.line(), 8);
}

// QTBUG-21310 - crash test
void tst_qqmlexpression::syntaxError()
{
    QQmlEngine engine;
    QQmlExpression expression(engine.rootContext(), nullptr, "asd asd");
    bool isUndefined = false;
    QVariant v = expression.evaluate(&isUndefined);
    QCOMPARE(v, QVariant());
    QVERIFY(expression.hasError());
    QCOMPARE(expression.error().description(), "SyntaxError: Expected token `;'");
    QVERIFY(isUndefined);
}

void tst_qqmlexpression::exception()
{
    QQmlEngine engine;
    QQmlExpression expression(engine.rootContext(), nullptr, "abc=123");
    QVariant v = expression.evaluate();
    QCOMPARE(v, QVariant());
    QVERIFY(expression.hasError());
}

void tst_qqmlexpression::expressionFromDataComponent()
{
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine);

    const QString fn(QLatin1String("expressionFromDataComponent.qml"));
    QUrl url = testFileUrl(fn);
    QString path = testFile(fn);

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        c.setData(f.readAll(), url);
    }

    QScopedPointer<TestObject> object;
    object.reset(qobject_cast<TestObject*>(c.create()));
    Q_ASSERT(!object.isNull());

    QQmlExpression expression(object->scriptString());
    QVariant result = expression.evaluate();
    QCOMPARE(result.type(), QVariant::String);
    QCOMPARE(result.toString(), QStringLiteral("success"));
}

QTEST_MAIN(tst_qqmlexpression)

#include "tst_qqmlexpression.moc"
