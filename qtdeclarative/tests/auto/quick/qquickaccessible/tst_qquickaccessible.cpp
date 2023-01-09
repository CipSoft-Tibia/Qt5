/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#include <QtTest/qtestaccessible.h>

#include <QtGui/qaccessible.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformaccessibility.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquicktext_p.h>

#include "../../shared/util.h"
#include "../shared/visualtestutil.h"

#define EXPECT(cond) \
    do { \
        if (!errorAt && !(cond)) { \
            errorAt = __LINE__; \
            qWarning("level: %d, middle: %d, role: %d (%s)", treelevel, middle, iface->role(), #cond); \
        } \
    } while (0)


//TESTED_FILES=

class tst_QQuickAccessible : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickAccessible();
    virtual ~tst_QQuickAccessible();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void commonTests_data();
    void commonTests();

    void quickAttachedProperties();
    void basicPropertiesTest();
    void hitTest();
    void checkableTest();
    void ignoredTest();
};

tst_QQuickAccessible::tst_QQuickAccessible()
{

}

tst_QQuickAccessible::~tst_QQuickAccessible()
{

}

void tst_QQuickAccessible::initTestCase()
{
    QQmlDataTest::initTestCase();
    QTestAccessibility::initialize();
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    if (!pfIntegration->accessibility())
        QSKIP("This platform does not support accessibility");
    pfIntegration->accessibility()->setActive(true);
}

void tst_QQuickAccessible::cleanupTestCase()
{
    QTestAccessibility::cleanup();
}

void tst_QQuickAccessible::init()
{
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::cleanup()
{
    const EventList list = QTestAccessibility::events();
    if (!list.isEmpty()) {
        qWarning("%d accessibility event(s) were not handled in testfunction '%s':", list.count(),
                 QString(QTest::currentTestFunction()).toLatin1().constData());
        for (int i = 0; i < list.count(); ++i)
            qWarning(" %d: Object: %p Event: '%s' Child: %d", i + 1, list.at(i)->object(),
                     qAccessibleEventString(list.at(i)->type()), list.at(i)->child());
    }
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::commonTests_data()
{
    QTest::addColumn<QString>("accessibleRoleFileName");

    QTest::newRow("Text") << "text.qml";
    QTest::newRow("PushButton") << "pushbutton.qml";
}

void tst_QQuickAccessible::commonTests()
{
    QFETCH(QString, accessibleRoleFileName);

    qDebug() << "testing" << accessibleRoleFileName;

    QQuickView *view = new QQuickView();
//    view->setFixedSize(240,320);
    view->setSource(testFileUrl(accessibleRoleFileName));
    view->show();
//    view->setFocus();
    QVERIFY(view->rootObject() != nullptr);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(view);
    QVERIFY(iface);

    delete view;
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::quickAttachedProperties()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != nullptr);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object);
        QCOMPARE(attachedObject, static_cast<QObject*>(nullptr));
        delete object;
    }

    // Attaching to non-item
    {
        QObject parent;
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML QtObject: Accessible must be attached to an Item");
        QQuickAccessibleAttached *attachedObj = new QQuickAccessibleAttached(&parent);

        QCOMPARE(attachedObj->ignored(), false);
        attachedObj->setIgnored(true);
        QCOMPARE(attachedObj->ignored(), false);
        attachedObj->setIgnored(false);
        QCOMPARE(attachedObj->ignored(), false);
    }

    // Attached property
    {
        QQuickItem parent;
        QQuickAccessibleAttached *attachedObj = new QQuickAccessibleAttached(&parent);

        attachedObj->name();

        QVariant pp = attachedObj->property("name");
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "Accessible.role: Accessible.Button\n"
                                "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != nullptr);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object));
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), true);
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), true);
            QCOMPARE(attachedObject->wasNameExplicitlySet(), false);
        }
        delete object;
    }

    // Attached property
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "Accessible.role: Accessible.Button\n"
                                "Accessible.name: \"Donald\"\n"
                                "Accessible.description: \"Duck\"\n"
                                "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != nullptr);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object));
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Donald"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Duck"));
            QCOMPARE(attachedObject->wasNameExplicitlySet(), true);
        }
        delete object;
    }

    // Check overriding of attached role for Text
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nText {\n"
                          "Accessible.role: Accessible.Button\n"
                          "Accessible.name: \"TextButton\"\n"
                          "Accessible.description: \"Text Button\"\n"
                          "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != nullptr);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object);
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("TextButton"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Text Button"));
        }
        delete object;
    }
    // Check overriding of attached role for Text
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nListView {\n"
                          "id: list\n"
                          "model: 5\n"
                          "delegate: Text {\n"
                          "objectName: \"acc_text\"\n"
                          "Accessible.role: Accessible.Button\n"
                          "Accessible.name: \"TextButton\"\n"
                          "Accessible.description: \"Text Button\"\n"
                          "}\n"
                          "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != nullptr);

        QQuickListView *listview = qobject_cast<QQuickListView *>(object);
        QVERIFY(listview != nullptr);
        QQuickItem *contentItem = listview->contentItem();
        QQuickText *childItem = QQuickVisualTestUtil::findItem<QQuickText>(contentItem, "acc_text");
        QVERIFY(childItem != nullptr);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(childItem);
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("TextButton"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Text Button"));
        }
        delete object;
    }
    // Check that a name can be implicitly set.
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick 2.0
            Text {
                Accessible.role: Accessible.Button
                Accessible.description: "Text Button"
            })", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object.data()));
        QVERIFY(attachedObject);
        QVERIFY(!attachedObject->wasNameExplicitlySet());

        attachedObject->setNameImplicitly(QLatin1String("Implicit"));
        QCOMPARE(attachedObject->name(), QLatin1String("Implicit"));
        QVERIFY(!attachedObject->wasNameExplicitlySet());

        attachedObject->setName(QLatin1String("Explicit"));
        QCOMPARE(attachedObject->name(), QLatin1String("Explicit"));
        QVERIFY(attachedObject->wasNameExplicitlySet());
    }
    QTestAccessibility::clearEvents();
}


void tst_QQuickAccessible::basicPropertiesTest()
{
    QAccessibleInterface *app = QAccessible::queryAccessibleInterface(qApp);
    QCOMPARE(app->childCount(), 0);

    QQuickView *window = new QQuickView();
    window->setSource(testFileUrl("text.qml"));
    window->show();
    QCOMPARE(app->childCount(), 1);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window);
    QVERIFY(iface);
    QCOMPARE(iface->childCount(), 1);

    QAccessibleInterface *item = iface->child(0);
    QVERIFY(item);
    QCOMPARE(item->childCount(), 5);
    QCOMPARE(item->rect().size(), QSize(400, 400));
    QCOMPARE(item->role(), QAccessible::Client);
    QCOMPARE(iface->indexOfChild(item), 0);

    QAccessibleInterface *text = item->child(0);
    QVERIFY(text);
    QCOMPARE(text->childCount(), 0);

    QCOMPARE(text->text(QAccessible::Name), QLatin1String("Hello Accessibility"));
    QCOMPARE(text->rect().size(), QSize(200, 50));
    QCOMPARE(text->rect().x(), item->rect().x() + 100);
    QCOMPARE(text->rect().y(), item->rect().y() + 20);
    QCOMPARE(text->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text), 0);

    QAccessibleInterface *text2 = item->child(1);
    QVERIFY(text2);
    QCOMPARE(text2->childCount(), 0);

    QCOMPARE(text2->text(QAccessible::Name), QLatin1String("The Hello 2 accessible text"));
    QCOMPARE(text2->rect().size(), QSize(100, 40));
    QCOMPARE(text2->rect().x(), item->rect().x() + 100);
    QCOMPARE(text2->rect().y(), item->rect().y() + 40);
    QCOMPARE(text2->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text2), 1);
    QCOMPARE(text2->state().editable, 0u);
    QCOMPARE(text2->state().readOnly, 1);

    QCOMPARE(iface->indexOfChild(text2), -1);
    QCOMPARE(text2->indexOfChild(item), -1);

    // TextInput
    QAccessibleInterface *textInput = item->child(2);
    QVERIFY(textInput);
    QCOMPARE(textInput->childCount(), 0);
    QCOMPARE(textInput->role(), QAccessible::EditableText);
    QCOMPARE(textInput->state().editable, 1);
    QCOMPARE(textInput->state().readOnly, 0);
    QCOMPARE(textInput->state().multiLine, 0);
    QCOMPARE(textInput->state().focusable, 1);
    QCOMPARE(textInput->text(QAccessible::Value), "A text input");
    auto textInterface = textInput->textInterface();
    QVERIFY(textInterface);
    auto editableTextInterface = textInput->editableTextInterface();
    QEXPECT_FAIL("", "EditableTextInterface is not implemented", Continue);
    QVERIFY(editableTextInterface);
    auto newText = QString("a new text");
    textInput->setText(QAccessible::Value, newText);
    QCOMPARE(textInput->text(QAccessible::Value), newText);

    // TextEdit
    QAccessibleInterface *textEdit = item->child(3);
    QVERIFY(textEdit);
    QCOMPARE(textEdit->childCount(), 0);
    QCOMPARE(textEdit->role(), QAccessible::EditableText);
    QCOMPARE(textEdit->state().editable, 1);
    QCOMPARE(textEdit->state().readOnly, 0);
    QCOMPARE(textEdit->state().focusable, 1);
    QCOMPARE(textEdit->text(QAccessible::Value), "A multi-line text edit\nTesting Accessibility.");
    auto textEditTextInterface = textEdit->textInterface();
    QVERIFY(textEditTextInterface);
    auto textEditEditableTextInterface = textEdit->editableTextInterface();
    QEXPECT_FAIL("", "EditableTextInterface is not implemented", Continue);
    QVERIFY(textEditEditableTextInterface);
    textEdit->setText(QAccessible::Value, newText);
    QCOMPARE(textEdit->text(QAccessible::Value), newText);
    QEXPECT_FAIL("", "multi line is not implemented", Continue);
    QCOMPARE(textInput->state().multiLine, 1);

    // Text "Hello 3"
    QAccessibleInterface *text3 = item->child(4);
    QVERIFY(text3);
    QCOMPARE(text3->childCount(), 0);
    QCOMPARE(text3->text(QAccessible::Name), QLatin1String("Hello 3"));
    QCOMPARE(text3->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text3), 4);
    QCOMPARE(text3->state().editable, 0);
    QCOMPARE(text3->state().readOnly, 0);
    // test implicit state values due to role change
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(text3->object());
    attached->setRole(QAccessible::StaticText);
    QCOMPARE(text3->role(), QAccessible::StaticText);
    QCOMPARE(text3->state().readOnly, 1);

    // see if implicit changes back
    attached->setRole(QAccessible::EditableText);
    QEXPECT_FAIL("", "EditableText does not implicitly set readOnly to false", Continue);
    QCOMPARE(text3->state().readOnly, 0);
    // explicitly set state
    attached->set_readOnly(false);
    attached->setRole(QAccessible::StaticText);
    QCOMPARE(text3->state().readOnly, 0);

    delete window;
    QTestAccessibility::clearEvents();
}

QAccessibleInterface *topLevelChildAt(QAccessibleInterface *iface, int x, int y)
{
    QAccessibleInterface *child = iface->childAt(x, y);
    if (!child)
        return nullptr;

    QAccessibleInterface *childOfChild;
    while ( ( childOfChild = child->childAt(x, y)) ) {
        child = childOfChild;
    }
    return child;
}

void tst_QQuickAccessible::hitTest()
{
    QQuickView *window = new QQuickView;
    window->setSource(testFileUrl("hittest.qml"));
    window->show();

    QAccessibleInterface *windowIface = QAccessible::queryAccessibleInterface(window);
    QVERIFY(windowIface);
    QAccessibleInterface *rootItem = windowIface->child(0);
    QRect rootRect = rootItem->rect();

    // check the root item from app
    QAccessibleInterface *appIface = QAccessible::queryAccessibleInterface(qApp);
    QVERIFY(appIface);
    QAccessibleInterface *itemHit = appIface->childAt(rootRect.x() + 200, rootRect.y() + 50);
    QVERIFY(itemHit);
    QCOMPARE(itemHit->rect(), rootRect);

    QAccessibleInterface *rootItemIface;
    for (int c = 0; c < rootItem->childCount(); ++c) {
        QAccessibleInterface *iface = rootItem->child(c);
        QString name = iface->text(QAccessible::Name);
        if (name == QLatin1String("rect1")) {
            // hit rect1
            QAccessibleInterface *rect1 = iface;
            QRect rect1Rect = rect1->rect();
            QAccessibleInterface *rootItemIface = rootItem->childAt(rect1Rect.x() + 10, rect1Rect.y() + 10);
            QVERIFY(rootItemIface);
            QCOMPARE(rect1Rect, rootItemIface->rect());
            QCOMPARE(rootItemIface->text(QAccessible::Name), QLatin1String("rect1"));

            // should also work from top level (app)
            QAccessibleInterface *app(QAccessible::queryAccessibleInterface(qApp));
            QAccessibleInterface *itemHit2(topLevelChildAt(app, rect1Rect.x() + 10, rect1Rect.y() + 10));
            QVERIFY(itemHit2);
            QCOMPARE(itemHit2->rect(), rect1Rect);
            QCOMPARE(itemHit2->text(QAccessible::Name), QLatin1String("rect1"));
        } else if (name == QLatin1String("rect2")) {
            QAccessibleInterface *rect2 = iface;
            // FIXME: This is seems broken on OS X
            // QCOMPARE(rect2->rect().translated(rootItem->rect().x(), rootItem->rect().y()), QRect(0, 50, 100, 100));
            QAccessibleInterface *rect20 = rect2->child(0);
            QVERIFY(rect20);
            QCOMPARE(rect20->text(QAccessible::Name), QLatin1String("rect20"));
            QPoint p = rect20->rect().bottomRight() + QPoint(20, 20);
            QAccessibleInterface *rect201 = rect20->childAt(p.x(), p.y());
            QVERIFY(rect201);
            QCOMPARE(rect201->text(QAccessible::Name), QLatin1String("rect201"));
            rootItemIface = topLevelChildAt(windowIface, p.x(), p.y());
            QVERIFY(rootItemIface);
            QCOMPARE(rootItemIface->text(QAccessible::Name), QLatin1String("rect201"));

        }
    }

    delete window;
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::checkableTest()
{
    QScopedPointer<QQuickView> window(new QQuickView());
    window->setSource(testFileUrl("checkbuttons.qml"));
    window->show();

    QQuickItem *contentItem = window->contentItem();
    QVERIFY(contentItem);
    QQuickItem *rootItem = contentItem->childItems().first();
    QVERIFY(rootItem);

    // the window becomes active
    QAccessible::State activatedChange;
    activatedChange.active = true;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window.data());
    QVERIFY(iface);
    QAccessibleInterface *root = iface->child(0);

    QAccessibleInterface *button1 = root->child(0);
    QCOMPARE(button1->role(), QAccessible::Button);
    QVERIFY(!(button1->state().checked));
    QVERIFY(!(button1->state().checkable));

    QVERIFY(button1->state().focusable);
    QVERIFY(!button1->state().focused);

    QTestAccessibility::clearEvents();

    // set properties
    QQuickItem *button1item = qobject_cast<QQuickItem*>(rootItem->childItems().at(0));
    QVERIFY(button1item);
    QCOMPARE(button1item->objectName(), QLatin1String("button1"));
    button1item->forceActiveFocus();
    QVERIFY(button1->state().focusable);
    QVERIFY(button1->state().focused);

    QAccessibleEvent focusEvent(button1item, QAccessible::Focus);
    QVERIFY_EVENT(&focusEvent);

    QAccessibleInterface *button2 = root->child(1);
    QVERIFY(!(button2->state().checked));
    QVERIFY(button2->state().checkable);
    QQuickItem *button2item = qobject_cast<QQuickItem*>(rootItem->childItems().at(1));
    QVERIFY(button2item);
    QCOMPARE(button2item->objectName(), QLatin1String("button2"));

    QAccessibleInterface *button3 = root->child(2);
    QVERIFY(button3->state().checked);
    QVERIFY(button3->state().checkable);

    QAccessibleInterface *checkBox1 = root->child(3);
    QCOMPARE(checkBox1->role(), QAccessible::CheckBox);
    QVERIFY(checkBox1->state().checked);
    QVERIFY(checkBox1->state().checkable);
    QQuickItem *checkbox1item = qobject_cast<QQuickItem*>(rootItem->childItems().at(3));
    QVERIFY(checkbox1item);
    QCOMPARE(checkbox1item->objectName(), QLatin1String("checkbox1"));

    checkbox1item->setProperty("checked", false);
    QVERIFY(!checkBox1->state().checked);
    QAccessible::State checkState;
    checkState.checked = true;
    QAccessibleStateChangeEvent checkChanged(checkbox1item, checkState);
    QVERIFY_EVENT(&checkChanged);

    checkbox1item->setProperty("checked", true);
    QVERIFY(checkBox1->state().checked);
    QVERIFY_EVENT(&checkChanged);

    QAccessibleInterface *checkBox2 = root->child(4);
    QVERIFY(!(checkBox2->state().checked));
    QVERIFY(checkBox2->state().checkable);

    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::ignoredTest()
{
    QScopedPointer<QQuickView> window(new QQuickView());
    window->setSource(testFileUrl("ignored.qml"));
    window->show();

    QQuickItem *contentItem = window->contentItem();
    QVERIFY(contentItem);
    QQuickItem *rootItem = contentItem->childItems().first();
    QVERIFY(rootItem);

    // the window becomes active
    QAccessible::State activatedChange;
    activatedChange.active = true;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window.data());
    QVERIFY(iface);
    QAccessibleInterface *rectangleA = iface->child(0);

    QCOMPARE(rectangleA->role(), QAccessible::StaticText);
    QCOMPARE(rectangleA->text(QAccessible::Name), QLatin1String("A"));
    static const char *expected = "BEFIHD";
    // check if node "C" and "G" is skipped and that the order is as expected.
    for (int i = 0; i < rectangleA->childCount(); ++i) {
        QAccessibleInterface *child = rectangleA->child(i);
        QCOMPARE(child->text(QAccessible::Name), QString(QLatin1Char(expected[i])));
    }
    QTestAccessibility::clearEvents();
}

QTEST_MAIN(tst_QQuickAccessible)

#include "tst_qquickaccessible.moc"
