// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <QString>
#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>
#include <QScopedPointer>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlpromise : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qqmlpromise() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void promise_all_empty_input();
    void promise_all_noniterable_input();
//    void promise_all_invoke_then_method();
    void promise_all_resolve();
    void promise_all_reject_reject_is_last();
    void promise_all_reject_reject_is_mid();
    void promise_get_length();
    void promise_executor_function_extensible();
    void promise_executor_reject();
    void promise_executor_resolve();
    void promise_executor_throw_exception();
    void promise_async_resolve_with_value();
    void promise_async_reject_with_value();
    void promise_resolve_with_value();
    void promise_resolve_function_length();
    void promise_resolve_is_a_function();
    void promise_resolve_with_array();
    void promise_resolve_with_empty();
    void promise_resolve_with_promise();
    void promise_race_empty_input();
    void promise_race_resolve_1st_in_executor_function();
    void promise_race_resolve_1st();
    void promise_race_resolve_2nd();
    void promise_reject_with_value();
    void promise_reject_catch();
    void then_resolve_chaining();
    void then_reject_chaining();
    void then_fulfilled_non_callable();
    void then_reject_non_callable();
    void then_resolve_multiple_then();
    void promiseChain();
    void promiseHandlerThrows();

private:
    void execute_test(QString testName);
};

void tst_qqmlpromise::promise_all_empty_input()
{
    execute_test("promise-all-empty-input.qml");
}

void tst_qqmlpromise::promise_all_noniterable_input()
{
    execute_test("promise-all-noniterable-input.qml");
}

// TODO: Fix the test
//void tst_qqmlpromise::promise_all_invoke_then_method()
//{
//    execute_test("promise-all-invoke-then-method.qml");
//}

void tst_qqmlpromise::promise_all_resolve()
{
    execute_test("promise-all-resolve.qml");
}

void tst_qqmlpromise::promise_all_reject_reject_is_last()
{
    execute_test("promise-all-reject-reject-is-last.qml");
}

void tst_qqmlpromise::promise_all_reject_reject_is_mid()
{
    execute_test("promise-all-reject-reject-is-mid.qml");
}

void tst_qqmlpromise::promise_get_length()
{
    execute_test("promise-get-length.qml");
}

void tst_qqmlpromise::promise_executor_resolve()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("promise-executor-resolve.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    component.completeCreate();

    QTRY_COMPARE(object->property("wasExecutorCalled").toBool(), true);
    QTRY_COMPARE(object->property("wasPromiseResolved").toBool(), true);
    // TODO: now "object" type is returned. fix
    // QCOMPARE(object->property("wasPromiseTypeReturnedByThen").toBool(), true);
    QTRY_COMPARE(object->property("wasResolutionForwardedCorrectly").toBool(), true);
    QTRY_COMPARE(object->property("wasNewPromiseObjectCreatedByThen").toBool(), true);
}

void tst_qqmlpromise::promise_executor_throw_exception()
{
    execute_test("promise-executor-throw-exception.qml");
}

void tst_qqmlpromise::promise_async_resolve_with_value()
{
    execute_test("promise-async-resolve-with-value.qml");
}

void tst_qqmlpromise::promise_async_reject_with_value()
{
    execute_test("promise-async-reject-with-value.qml");
}

void tst_qqmlpromise::promise_resolve_with_value()
{
    execute_test("promise-resolve-with-value.qml");
}

void tst_qqmlpromise::promise_resolve_function_length()
{
    execute_test("promise-resolve-function-length.qml");
}

void tst_qqmlpromise::promise_resolve_is_a_function()
{
    execute_test("promise-resolve-is-a-function.qml");
}

void tst_qqmlpromise::promise_resolve_with_array()
{
    execute_test("promise-resolve-with-array.qml");
}

void tst_qqmlpromise::promise_resolve_with_empty()
{
    execute_test("promise-resolve-with-empty.qml");
}

void tst_qqmlpromise::promise_resolve_with_promise()
{
    execute_test("promise-resolve-with-promise.qml");
}

void tst_qqmlpromise::promise_race_empty_input()
{
    execute_test("promise-race-empty-input.qml");
}

void tst_qqmlpromise::promise_race_resolve_1st_in_executor_function()
{
    execute_test("promise-race-resolve-1st-in-executor-function.qml");
}

void tst_qqmlpromise::promise_race_resolve_1st()
{
    execute_test("promise-race-resolve-1st.qml");
}

void tst_qqmlpromise::promise_race_resolve_2nd()
{
    execute_test("promise-race-resolve-2nd.qml");
}

void tst_qqmlpromise::promise_reject_with_value()
{
    execute_test("promise-reject-with-value.qml");
}

void tst_qqmlpromise::promise_reject_catch()
{
    execute_test("promise-reject-catch.qml");
}

void tst_qqmlpromise::promise_executor_function_extensible()
{
    execute_test("promise-executor-function-extensible.qml");
}

void tst_qqmlpromise::promise_executor_reject()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("promise-executor-reject.qml"));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    component.completeCreate();

    QTRY_COMPARE(object->property("wasExecutorCalled").toBool(), true);
    QTRY_COMPARE(object->property("wasPromiseRejected").toBool(), true);
    // TODO: now "object" type is returned. fix
    // QCOMPARE(object->property("wasPromiseTypeReturnedByThen").toBool(), true);
    QTRY_COMPARE(object->property("wasResolutionForwardedCorrectly").toBool(), true);
}

void tst_qqmlpromise::then_resolve_chaining()
{
    execute_test("then-resolve-chaining.qml");
}

void tst_qqmlpromise::then_reject_chaining()
{
    execute_test("then-reject-chaining.qml");
}

void tst_qqmlpromise::then_fulfilled_non_callable()
{
    execute_test("then-fulfilled-non-callable.qml");
}

void tst_qqmlpromise::then_reject_non_callable()
{
    execute_test("then-reject-non-callable.qml");
}

void tst_qqmlpromise::then_resolve_multiple_then()
{
    execute_test("then-resolve-multiple-then.qml");
}

void tst_qqmlpromise::execute_test(QString testName)
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl(testName));
    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QVERIFY(!object.isNull());
    component.completeCreate();

    QTRY_COMPARE(object->property("wasTestSuccessful").toBool(), true);
}

void tst_qqmlpromise::promiseChain()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("promisechain.qml"));
    QVERIFY(component.isReady());
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "2");
    QTest::ignoreMessage(QtDebugMsg, "3");
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    QTRY_VERIFY(root->property("x") == 42);

}

void tst_qqmlpromise::promiseHandlerThrows()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("promisehandlerthrows.qml"));
    QVERIFY(component.isReady());
    QTest::ignoreMessage(QtDebugMsg, "Rethrowing err");
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    QTRY_VERIFY(root->property("errorMessage") == QLatin1String("Some error"));
}


QTEST_MAIN(tst_qqmlpromise)

#include "tst_qqmlpromise.moc"
