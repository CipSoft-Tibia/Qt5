/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"
#include "../shared/qtest_quickcontrols.h"

#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>
#include <QtQuickTemplates2/private/qquickstackview_p.h>

using namespace QQuickVisualTestUtil;

class tst_QQuickPopup : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void initTestCase();
    void visible_data();
    void visible();
    void state();
    void overlay_data();
    void overlay();
    void zOrder_data();
    void zOrder();
    void windowChange();
    void closePolicy_data();
    void closePolicy();
    void activeFocusOnClose1();
    void activeFocusOnClose2();
    void activeFocusOnClose3();
    void activeFocusOnClosingSeveralPopups();
    void hover_data();
    void hover();
    void wheel_data();
    void wheel();
    void parentDestroyed();
    void nested();
    void grabber();
    void cursorShape();
    void componentComplete();
    void closeOnEscapeWithNestedPopups();
    void enabled();
    void orientation_data();
    void orientation();
    void qquickview();
    void disabledPalette();
    void disabledParentPalette();
    void toolTipCrashOnClose();
    void setOverlayParentToNull();
    void centerInOverlayWithinStackViewItem();
};

void tst_QQuickPopup::initTestCase()
{
    QQmlDataTest::initTestCase();
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
}

void tst_QQuickPopup::visible_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("Window") << "window.qml";
    QTest::newRow("ApplicationWindow") << "applicationwindow.qml";
}

void tst_QQuickPopup::visible()
{
    QFETCH(QString, source);
    QQuickApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);
    QQuickItem *popupItem = popup->popupItem();

    popup->open();
    QVERIFY(popup->isVisible());

    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);
    QVERIFY(overlay->childItems().contains(popupItem));

    popup->close();
    QTRY_VERIFY(!popup->isVisible());
    QVERIFY(!overlay->childItems().contains(popupItem));

    popup->setVisible(true);
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->childItems().contains(popupItem));

    popup->setVisible(false);
    QTRY_VERIFY(!popup->isVisible());
    QVERIFY(!overlay->childItems().contains(popupItem));
}

void tst_QQuickPopup::state()
{
    QQuickApplicationHelper helper(this, "applicationwindow.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QCOMPARE(popup->isVisible(), false);

    QSignalSpy visibleChangedSpy(popup, SIGNAL(visibleChanged()));
    QSignalSpy aboutToShowSpy(popup, SIGNAL(aboutToShow()));
    QSignalSpy aboutToHideSpy(popup, SIGNAL(aboutToHide()));
    QSignalSpy openedSpy(popup, SIGNAL(opened()));
    QSignalSpy closedSpy(popup, SIGNAL(closed()));

    QVERIFY(visibleChangedSpy.isValid());
    QVERIFY(aboutToShowSpy.isValid());
    QVERIFY(aboutToHideSpy.isValid());
    QVERIFY(openedSpy.isValid());
    QVERIFY(closedSpy.isValid());

    popup->open();
    QCOMPARE(visibleChangedSpy.count(), 1);
    QCOMPARE(aboutToShowSpy.count(), 1);
    QCOMPARE(aboutToHideSpy.count(), 0);
    QTRY_COMPARE(openedSpy.count(), 1);
    QCOMPARE(closedSpy.count(), 0);

    popup->close();
    QTRY_COMPARE(visibleChangedSpy.count(), 2);
    QCOMPARE(aboutToShowSpy.count(), 1);
    QCOMPARE(aboutToHideSpy.count(), 1);
    QCOMPARE(openedSpy.count(), 1);
    QTRY_COMPARE(closedSpy.count(), 1);
}

void tst_QQuickPopup::overlay_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("modal");
    QTest::addColumn<bool>("dim");

    QTest::newRow("Window") << "window.qml" << false << false;
    QTest::newRow("Window,dim") << "window.qml" << false << true;
    QTest::newRow("Window,modal") << "window.qml" << true << false;
    QTest::newRow("Window,modal,dim") << "window.qml" << true << true;

    QTest::newRow("ApplicationWindow") << "applicationwindow.qml" << false << false;
    QTest::newRow("ApplicationWindow,dim") << "applicationwindow.qml" << false << true;
    QTest::newRow("ApplicationWindow,modal") << "applicationwindow.qml" << true << false;
    QTest::newRow("ApplicationWindow,modal,dim") << "applicationwindow.qml" << true << true;
}

void tst_QQuickPopup::overlay()
{
    QFETCH(QString, source);
    QFETCH(bool, modal);
    QFETCH(bool, dim);

    QQuickApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickOverlay *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);

    QSignalSpy overlayPressedSignal(overlay, SIGNAL(pressed()));
    QSignalSpy overlayReleasedSignal(overlay, SIGNAL(released()));
    QVERIFY(overlayPressedSignal.isValid());
    QVERIFY(overlayReleasedSignal.isValid());

    QVERIFY(!overlay->isVisible()); // no popups open

    QTest::mouseClick(window, Qt::LeftButton);
    QCOMPARE(overlayPressedSignal.count(), 0);
    QCOMPARE(overlayReleasedSignal.count(), 0);

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QQuickOverlayAttached *overlayAttached = qobject_cast<QQuickOverlayAttached *>(qmlAttachedPropertiesObject<QQuickOverlay>(popup));
    QVERIFY(overlayAttached);
    QCOMPARE(overlayAttached->overlay(), overlay);

    QSignalSpy overlayAttachedPressedSignal(overlayAttached, SIGNAL(pressed()));
    QSignalSpy overlayAttachedReleasedSignal(overlayAttached, SIGNAL(released()));
    QVERIFY(overlayAttachedPressedSignal.isValid());
    QVERIFY(overlayAttachedReleasedSignal.isValid());

    QQuickButton *button = window->property("button").value<QQuickButton*>();
    QVERIFY(button);

    int overlayPressCount = 0;
    int overlayReleaseCount = 0;

    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());
    QTRY_VERIFY(popup->isOpened());

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), ++overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount);
    QCOMPARE(overlayAttachedPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayAttachedReleasedSignal.count(), overlayReleaseCount);

    QTRY_VERIFY(!popup->isVisible());
    QVERIFY(!overlay->isVisible());

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount); // no modal-popups open
    QCOMPARE(overlayAttachedPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayAttachedReleasedSignal.count(), overlayReleaseCount);

    popup->setDim(dim);
    popup->setModal(modal);
    popup->setClosePolicy(QQuickPopup::CloseOnReleaseOutside);

    // mouse
    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());
    QTRY_VERIFY(popup->isOpened());

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), ++overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount);
    QCOMPARE(overlayAttachedPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayAttachedReleasedSignal.count(), overlayReleaseCount);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), ++overlayReleaseCount);
    QCOMPARE(overlayAttachedPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayAttachedReleasedSignal.count(), overlayReleaseCount);

    QTRY_VERIFY(!popup->isVisible());
    QVERIFY(!overlay->isVisible());

    // touch
    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());

    struct TouchDeviceDeleter
    {
        static inline void cleanup(QTouchDevice *device)
        {
            QWindowSystemInterface::unregisterTouchDevice(device);
            delete device;
        }
    };

    QScopedPointer<QTouchDevice, TouchDeviceDeleter> device(new QTouchDevice);
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device.data());

    QTest::touchEvent(window, device.data()).press(0, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), ++overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount);
    QCOMPARE(overlayAttachedPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayAttachedReleasedSignal.count(), overlayReleaseCount);

    QTest::touchEvent(window, device.data()).release(0, QPoint(1, 1));
    QCOMPARE(overlayPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), ++overlayReleaseCount);
    QCOMPARE(overlayAttachedPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayAttachedReleasedSignal.count(), overlayReleaseCount);

    QTRY_VERIFY(!popup->isVisible());
    QVERIFY(!overlay->isVisible());

    // multi-touch
    popup->open();
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());
    QVERIFY(!button->isPressed());

    QTest::touchEvent(window, device.data()).press(0, button->mapToScene(QPointF(1, 1)).toPoint());
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());
    QCOMPARE(button->isPressed(), !modal);
    QCOMPARE(overlayPressedSignal.count(), ++overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount);

    QTest::touchEvent(window, device.data()).stationary(0).press(1, button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QVERIFY(popup->isVisible());
    QVERIFY(overlay->isVisible());
    QCOMPARE(button->isPressed(), !modal);
    QCOMPARE(overlayPressedSignal.count(), ++overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount);

    QTest::touchEvent(window, device.data()).release(0, button->mapToScene(QPointF(1, 1)).toPoint()).stationary(1);
    QTRY_VERIFY(!popup->isVisible());
    QVERIFY(!overlay->isVisible());
    QVERIFY(!button->isPressed());
    QCOMPARE(overlayPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), ++overlayReleaseCount);

    QTest::touchEvent(window, device.data()).release(1, button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QVERIFY(!popup->isVisible());
    QVERIFY(!overlay->isVisible());
    QVERIFY(!button->isPressed());
    QCOMPARE(overlayPressedSignal.count(), overlayPressCount);
    QCOMPARE(overlayReleasedSignal.count(), overlayReleaseCount);
}

void tst_QQuickPopup::zOrder_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("Window") << "window.qml";
    QTest::newRow("ApplicationWindow") << "applicationwindow.qml";
}

void tst_QQuickPopup::zOrder()
{
    QFETCH(QString, source);
    QQuickApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);
    popup->setModal(true);

    QQuickPopup *popup2 = window->property("popup2").value<QQuickPopup*>();
    QVERIFY(popup2);
    popup2->setModal(true);

    // show popups in reverse order. popup2 has higher z-order so it appears
    // on top and must be closed first, even if the other popup was opened last
    popup2->open();
    popup->open();
    QVERIFY(popup2->isVisible());
    QVERIFY(popup->isVisible());

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QTRY_VERIFY(!popup2->isVisible());
    QVERIFY(popup->isVisible());

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QVERIFY(!popup2->isVisible());
    QTRY_VERIFY(!popup->isVisible());
}

void tst_QQuickPopup::windowChange()
{
    QQuickPopup popup;
    QSignalSpy spy(&popup, SIGNAL(windowChanged(QQuickWindow*)));
    QVERIFY(spy.isValid());

    QQuickItem item;
    popup.setParentItem(&item);
    QVERIFY(!popup.window());
    QCOMPARE(spy.count(), 0);

    QQuickWindow window;
    item.setParentItem(window.contentItem());
    QCOMPARE(popup.window(), &window);
    QCOMPARE(spy.count(), 1);

    item.setParentItem(nullptr);
    QVERIFY(!popup.window());
    QCOMPARE(spy.count(), 2);

    popup.setParentItem(window.contentItem());
    QCOMPARE(popup.window(), &window);
    QCOMPARE(spy.count(), 3);

    popup.resetParentItem();
    QVERIFY(!popup.window());
    QCOMPARE(spy.count(), 4);

    popup.setParent(&window);
    popup.resetParentItem();
    QCOMPARE(popup.window(), &window);
    QCOMPARE(spy.count(), 5);

    popup.setParent(this);
    popup.resetParentItem();
    QVERIFY(!popup.window());
    QCOMPARE(spy.count(), 6);

    item.setParentItem(window.contentItem());
    popup.setParent(&item);
    popup.resetParentItem();
    QCOMPARE(popup.window(), &window);
    QCOMPARE(spy.count(), 7);

    popup.setParent(nullptr);
}

Q_DECLARE_METATYPE(QQuickPopup::ClosePolicy)

void tst_QQuickPopup::closePolicy_data()
{
    qRegisterMetaType<QQuickPopup::ClosePolicy>();

    QTest::addColumn<QString>("source");
    QTest::addColumn<QQuickPopup::ClosePolicy>("closePolicy");

    QTest::newRow("Window:NoAutoClose") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::NoAutoClose);
    QTest::newRow("Window:CloseOnPressOutside") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutside);
    QTest::newRow("Window:CloseOnPressOutsideParent") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutsideParent);
    QTest::newRow("Window:CloseOnPressOutside|Parent") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutside | QQuickPopup::CloseOnPressOutsideParent);
    QTest::newRow("Window:CloseOnReleaseOutside") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnReleaseOutside);
    QTest::newRow("Window:CloseOnReleaseOutside|Parent") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnReleaseOutside | QQuickPopup::CloseOnReleaseOutsideParent);
    QTest::newRow("Window:CloseOnEscape") << "window.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnEscape);

    QTest::newRow("ApplicationWindow:NoAutoClose") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::NoAutoClose);
    QTest::newRow("ApplicationWindow:CloseOnPressOutside") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutside);
    QTest::newRow("ApplicationWindow:CloseOnPressOutsideParent") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutsideParent);
    QTest::newRow("ApplicationWindow:CloseOnPressOutside|Parent") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnPressOutside | QQuickPopup::CloseOnPressOutsideParent);
    QTest::newRow("ApplicationWindow:CloseOnReleaseOutside") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnReleaseOutside);
    QTest::newRow("ApplicationWindow:CloseOnReleaseOutside|Parent") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnReleaseOutside | QQuickPopup::CloseOnReleaseOutsideParent);
    QTest::newRow("ApplicationWindow:CloseOnEscape") << "applicationwindow.qml"<< static_cast<QQuickPopup::ClosePolicy>(QQuickPopup::CloseOnEscape);
}

void tst_QQuickPopup::closePolicy()
{
    QFETCH(QString, source);
    QFETCH(QQuickPopup::ClosePolicy, closePolicy);

    QQuickApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QQuickButton *button = window->property("button").value<QQuickButton*>();
    QVERIFY(button);

    popup->setModal(true);
    popup->setFocus(true);
    popup->setClosePolicy(closePolicy);

    popup->open();
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(popup->isOpened());

    // press outside popup and its parent
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1), 50);
    if (closePolicy.testFlag(QQuickPopup::CloseOnPressOutside) || closePolicy.testFlag(QQuickPopup::CloseOnPressOutsideParent))
        QTRY_VERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(popup->isOpened());

    // release outside popup and its parent
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    if (closePolicy.testFlag(QQuickPopup::CloseOnReleaseOutside))
        QTRY_VERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(popup->isOpened());

    // press outside popup but inside its parent
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x() + 1, button->y() + 1));
    if (closePolicy.testFlag(QQuickPopup::CloseOnPressOutside) && !closePolicy.testFlag(QQuickPopup::CloseOnPressOutsideParent))
        QTRY_VERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(popup->isOpened());

    // release outside popup but inside its parent
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x() + 1, button->y() + 1));
    if (closePolicy.testFlag(QQuickPopup::CloseOnReleaseOutside) && !closePolicy.testFlag(QQuickPopup::CloseOnReleaseOutsideParent))
        QTRY_VERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());

    popup->open();
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(popup->isOpened());

    // press inside and release outside
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(button->x() + popup->x() + 1,
                                                                     button->y() + popup->y() + 1));
    QVERIFY(popup->isVisible());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QVERIFY(popup->isVisible());

    // escape
    QTest::keyClick(window, Qt::Key_Escape);
    if (closePolicy.testFlag(QQuickPopup::CloseOnEscape))
        QTRY_VERIFY(!popup->isVisible());
    else
        QVERIFY(popup->isVisible());
}

void tst_QQuickPopup::activeFocusOnClose1()
{
    // Test that a popup that never sets focus: true (e.g. ToolTip) doesn't affect
    // the active focus item when it closes.
    QQuickApplicationHelper helper(this, QStringLiteral("activeFocusOnClose1.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *focusedPopup = helper.appWindow->property("focusedPopup").value<QQuickPopup*>();
    QVERIFY(focusedPopup);

    QQuickPopup *nonFocusedPopup = helper.appWindow->property("nonFocusedPopup").value<QQuickPopup*>();
    QVERIFY(nonFocusedPopup);

    focusedPopup->open();
    QVERIFY(focusedPopup->isVisible());
    QTRY_VERIFY(focusedPopup->isOpened());
    QVERIFY(focusedPopup->hasActiveFocus());

    nonFocusedPopup->open();
    QVERIFY(nonFocusedPopup->isVisible());
    QTRY_VERIFY(nonFocusedPopup->isOpened());
    QVERIFY(focusedPopup->hasActiveFocus());

    nonFocusedPopup->close();
    QTRY_VERIFY(!nonFocusedPopup->isVisible());
    QVERIFY(focusedPopup->hasActiveFocus());

    // QTBUG-66113: force active focus on a popup that did not request focus
    nonFocusedPopup->open();
    nonFocusedPopup->forceActiveFocus();
    QVERIFY(nonFocusedPopup->isVisible());
    QTRY_VERIFY(nonFocusedPopup->isOpened());
    QVERIFY(nonFocusedPopup->hasActiveFocus());

    nonFocusedPopup->close();
    QTRY_VERIFY(!nonFocusedPopup->isVisible());
    QVERIFY(focusedPopup->hasActiveFocus());
}

void tst_QQuickPopup::activeFocusOnClose2()
{
    // Test that a popup that sets focus: true but relinquishes focus (e.g. by
    // calling forceActiveFocus() on another item) before it closes doesn't
    // affect the active focus item when it closes.
    QQuickApplicationHelper helper(this, QStringLiteral("activeFocusOnClose2.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup1 = helper.appWindow->property("popup1").value<QQuickPopup*>();
    QVERIFY(popup1);

    QQuickPopup *popup2 = helper.appWindow->property("popup2").value<QQuickPopup*>();
    QVERIFY(popup2);

    QQuickButton *closePopup2Button = helper.appWindow->property("closePopup2Button").value<QQuickButton*>();
    QVERIFY(closePopup2Button);

    popup1->open();
    QVERIFY(popup1->isVisible());
    QTRY_VERIFY(popup1->isOpened());
    QVERIFY(popup1->hasActiveFocus());

    popup2->open();
    QVERIFY(popup2->isVisible());
    QTRY_VERIFY(popup2->isOpened());
    QVERIFY(popup2->hasActiveFocus());

    // Causes popup1.contentItem.forceActiveFocus() to be called, then closes popup2.
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        closePopup2Button->mapToScene(QPointF(closePopup2Button->width() / 2, closePopup2Button->height() / 2)).toPoint());
    QTRY_VERIFY(!popup2->isVisible());
    QVERIFY(popup1->hasActiveFocus());
}

void tst_QQuickPopup::activeFocusOnClose3()
{
    // Test that a closing popup that had focus doesn't steal focus from
    // another popup that the focus was transferred to.
    QQuickApplicationHelper helper(this, QStringLiteral("activeFocusOnClose3.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup1 = helper.appWindow->property("popup1").value<QQuickPopup*>();
    QVERIFY(popup1);

    QQuickPopup *popup2 = helper.appWindow->property("popup2").value<QQuickPopup*>();
    QVERIFY(popup2);

    popup1->open();
    QVERIFY(popup1->isVisible());
    QTRY_VERIFY(popup1->hasActiveFocus());

    popup2->open();
    popup1->close();

    QSignalSpy closedSpy(popup1, SIGNAL(closed()));
    QVERIFY(closedSpy.isValid());
    QVERIFY(closedSpy.wait());

    QVERIFY(!popup1->isVisible());
    QVERIFY(popup2->isVisible());
    QTRY_VERIFY(popup2->hasActiveFocus());
}

void tst_QQuickPopup::activeFocusOnClosingSeveralPopups()
{
    // Test that active focus isn't lost when multiple popup closing simultaneously
    QQuickApplicationHelper helper(this, QStringLiteral("activeFocusOnClosingSeveralPopups.qml"));
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickItem *button = window->property("button").value<QQuickItem *>();
    QVERIFY(button);

    QQuickPopup *popup1 = window->property("popup1").value<QQuickPopup *>();
    QVERIFY(popup1);

    QQuickPopup *popup2 = window->property("popup2").value<QQuickPopup *>();
    QVERIFY(popup2);

    QCOMPARE(button->hasActiveFocus(), true);
    popup1->open();
    QTRY_VERIFY(popup1->isOpened());
    QVERIFY(popup1->hasActiveFocus());
    popup2->open();
    QTRY_VERIFY(popup2->isOpened());
    QVERIFY(popup2->hasActiveFocus());
    QTRY_COMPARE(button->hasActiveFocus(), false);
    // close the unfocused popup first
    popup1->close();
    popup2->close();
    QTRY_VERIFY(!popup1->isVisible());
    QTRY_VERIFY(!popup2->isVisible());
    QTRY_COMPARE(button->hasActiveFocus(), true);

    popup1->open();
    QTRY_VERIFY(popup1->isOpened());
    QVERIFY(popup1->hasActiveFocus());
    popup2->open();
    QTRY_VERIFY(popup2->isOpened());
    QVERIFY(popup2->hasActiveFocus());
    QTRY_COMPARE(button->hasActiveFocus(), false);
    // close the focused popup first
    popup2->close();
    popup1->close();
    QTRY_VERIFY(!popup1->isVisible());
    QTRY_VERIFY(!popup2->isVisible());
    QTRY_COMPARE(button->hasActiveFocus(), true);
}

void tst_QQuickPopup::hover_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("modal");

    QTest::newRow("Window:modal") << "window-hover.qml" << true;
    QTest::newRow("Window:modeless") << "window-hover.qml" << false;
    QTest::newRow("ApplicationWindow:modal") << "applicationwindow-hover.qml" << true;
    QTest::newRow("ApplicationWindow:modeless") << "applicationwindow-hover.qml" << false;
}

void tst_QQuickPopup::hover()
{
    QFETCH(QString, source);
    QFETCH(bool, modal);

    QQuickApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);
    popup->setModal(modal);

    QQuickButton *parentButton = window->property("parentButton").value<QQuickButton*>();
    QVERIFY(parentButton);
    parentButton->setHoverEnabled(true);

    QQuickButton *childButton = window->property("childButton").value<QQuickButton*>();
    QVERIFY(childButton);
    childButton->setHoverEnabled(true);

    QSignalSpy openedSpy(popup, SIGNAL(opened()));
    QVERIFY(openedSpy.isValid());
    popup->open();
    QVERIFY(openedSpy.count() == 1 || openedSpy.wait());

    // hover the parent button outside the popup
    QTest::mouseMove(window, QPoint(window->width() - 1, window->height() - 1));
    QCOMPARE(parentButton->isHovered(), !modal);
    QVERIFY(!childButton->isHovered());

    // hover the popup background
    QTest::mouseMove(window, QPoint(1, 1));
    QVERIFY(!parentButton->isHovered());
    QVERIFY(!childButton->isHovered());

    // hover the child button in a popup
    QTest::mouseMove(window, QPoint(popup->x() + popup->width() / 2, popup->y() + popup->height() / 2));
    QVERIFY(!parentButton->isHovered());
    QVERIFY(childButton->isHovered());

    QSignalSpy closedSpy(popup, SIGNAL(closed()));
    QVERIFY(closedSpy.isValid());
    popup->close();
    QVERIFY(closedSpy.count() == 1 || closedSpy.wait());

    // hover the parent button after closing the popup
    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() / 2));
    QVERIFY(parentButton->isHovered());
}

void tst_QQuickPopup::wheel_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("modal");

    QTest::newRow("Window:modal") << "window-wheel.qml" << true;
    QTest::newRow("Window:modeless") << "window-wheel.qml" << false;
    QTest::newRow("ApplicationWindow:modal") << "applicationwindow-wheel.qml" << true;
    QTest::newRow("ApplicationWindow:modeless") << "applicationwindow-wheel.qml" << false;
}

static bool sendWheelEvent(QQuickItem *item, const QPoint &localPos, int degrees)
{
    QQuickWindow *window = item->window();
    QWheelEvent wheelEvent(localPos, item->window()->mapToGlobal(localPos), QPoint(0, 0), QPoint(0, 8 * degrees), 0, Qt::Vertical, Qt::NoButton, 0);
    QSpontaneKeyEvent::setSpontaneous(&wheelEvent);
    return qGuiApp->notify(window, &wheelEvent);
}

void tst_QQuickPopup::wheel()
{
    QFETCH(QString, source);
    QFETCH(bool, modal);

    QQuickApplicationHelper helper(this, source);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickSlider *contentSlider = window->property("contentSlider").value<QQuickSlider*>();
    QVERIFY(contentSlider);

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup && popup->contentItem());
    popup->setModal(modal);

    QQuickSlider *popupSlider = window->property("popupSlider").value<QQuickSlider*>();
    QVERIFY(popupSlider);

    {
        // wheel over the content
        qreal oldContentValue = contentSlider->value();
        qreal oldPopupValue = popupSlider->value();

        QVERIFY(sendWheelEvent(contentSlider, QPoint(contentSlider->width() / 2, contentSlider->height() / 2), 15));

        QVERIFY(!qFuzzyCompare(contentSlider->value(), oldContentValue)); // must have moved
        QVERIFY(qFuzzyCompare(popupSlider->value(), oldPopupValue)); // must not have moved
    }

    QSignalSpy openedSpy(popup, SIGNAL(opened()));
    QVERIFY(openedSpy.isValid());
    popup->open();
    QVERIFY(openedSpy.count() == 1 || openedSpy.wait());

    {
        // wheel over the popup content
        qreal oldContentValue = contentSlider->value();
        qreal oldPopupValue = popupSlider->value();

        QVERIFY(sendWheelEvent(popupSlider, QPoint(popupSlider->width() / 2, popupSlider->height() / 2), 15));

        QVERIFY(qFuzzyCompare(contentSlider->value(), oldContentValue)); // must not have moved
        QVERIFY(!qFuzzyCompare(popupSlider->value(), oldPopupValue)); // must have moved
    }

    {
        // wheel over the overlay
        qreal oldContentValue = contentSlider->value();
        qreal oldPopupValue = popupSlider->value();

        QVERIFY(sendWheelEvent(QQuickOverlay::overlay(window), QPoint(0, 0), 15));

        if (modal) {
            // the content below a modal overlay must not move
            QVERIFY(qFuzzyCompare(contentSlider->value(), oldContentValue));
        } else {
            // the content below a modeless overlay must move
            QVERIFY(!qFuzzyCompare(contentSlider->value(), oldContentValue));
        }
        QVERIFY(qFuzzyCompare(popupSlider->value(), oldPopupValue)); // must not have moved
    }
}

void tst_QQuickPopup::parentDestroyed()
{
    QQuickPopup popup;
    popup.setParentItem(new QQuickItem);
    delete popup.parentItem();
    QVERIFY(!popup.parentItem());
}

void tst_QQuickPopup::nested()
{
    QQuickApplicationHelper helper(this, QStringLiteral("nested.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickPopup *modalPopup = window->property("modalPopup").value<QQuickPopup *>();
    QVERIFY(modalPopup);

    QQuickPopup *modelessPopup = window->property("modelessPopup").value<QQuickPopup *>();
    QVERIFY(modelessPopup);

    modalPopup->open();
    QCOMPARE(modalPopup->isVisible(), true);

    modelessPopup->open();
    QCOMPARE(modelessPopup->isVisible(), true);

    // click outside the modeless popup on the top, but inside the modal popup below
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 150));

    QTRY_COMPARE(modelessPopup->isVisible(), false);
    QCOMPARE(modalPopup->isVisible(), true);
}

// QTBUG-56697
void tst_QQuickPopup::grabber()
{
    QQuickApplicationHelper helper(this, QStringLiteral("grabber.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickPopup *menu = window->property("menu").value<QQuickPopup *>();
    QVERIFY(menu);

    QQuickPopup *popup = window->property("popup").value<QQuickPopup *>();
    QVERIFY(popup);

    QQuickPopup *combo = window->property("combo").value<QQuickPopup *>();
    QVERIFY(combo);

    menu->open();
    QTRY_COMPARE(menu->isOpened(), true);
    QCOMPARE(popup->isVisible(), false);
    QCOMPARE(combo->isVisible(), false);

    // click a menu item to open the popup
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(menu->width() / 2, menu->height() / 2));
    QTRY_COMPARE(menu->isVisible(), false);
    QTRY_COMPARE(popup->isOpened(), true);
    QCOMPARE(combo->isVisible(), false);

    combo->open();
    QCOMPARE(menu->isVisible(), false);
    QCOMPARE(popup->isVisible(), true);
    QTRY_COMPARE(combo->isOpened(), true);

    // click outside to close both the combo popup and the parent popup
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 1, window->height() - 1));
    QCOMPARE(menu->isVisible(), false);
    QTRY_COMPARE(popup->isVisible(), false);
    QTRY_COMPARE(combo->isVisible(), false);

    menu->open();
    QTRY_COMPARE(menu->isOpened(), true);
    QCOMPARE(popup->isVisible(), false);
    QCOMPARE(combo->isVisible(), false);

    // click outside the menu to close it (QTBUG-56697)
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - 1, window->height() - 1));
    QTRY_COMPARE(menu->isVisible(), false);
    QCOMPARE(popup->isVisible(), false);
    QCOMPARE(combo->isVisible(), false);
}

void tst_QQuickPopup::cursorShape()
{
    // Ensure that the mouse cursor has the correct shape when over a popup
    // which is itself over an item with a different shape.
    QQuickApplicationHelper helper(this, QStringLiteral("cursor.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickPopup *popup = helper.appWindow->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    popup->open();
    QVERIFY(popup->isVisible());
    QTRY_VERIFY(popup->isOpened());

    QQuickItem *textField = helper.appWindow->property("textField").value<QQuickItem*>();
    QVERIFY(textField);

    // Move the mouse over the text field.
    const QPoint textFieldPos(popup->x() - 10, textField->height() / 2);
    QTest::mouseMove(window, textFieldPos);
    QCOMPARE(window->cursor().shape(), textField->cursor().shape());

    // Move the mouse over the popup where it overlaps with the text field.
    const QPoint textFieldOverlapPos(popup->x() + 10, textField->height() / 2);
    QTest::mouseMove(window, textFieldOverlapPos);
    QCOMPARE(window->cursor().shape(), popup->popupItem()->cursor().shape());

    popup->close();
    QTRY_VERIFY(!popup->isVisible());
}

class FriendlyPopup : public QQuickPopup
{
    friend class tst_QQuickPopup;
};

void tst_QQuickPopup::componentComplete()
{
    FriendlyPopup cppPopup;
    QVERIFY(cppPopup.isComponentComplete());

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick.Controls 2.2; Popup { }", QUrl());

    FriendlyPopup *qmlPopup = static_cast<FriendlyPopup *>(component.beginCreate(engine.rootContext()));
    QVERIFY(qmlPopup);
    QVERIFY(!qmlPopup->isComponentComplete());

    component.completeCreate();
    QVERIFY(qmlPopup->isComponentComplete());
}

void tst_QQuickPopup::closeOnEscapeWithNestedPopups()
{
    // Tests the scenario in the Gallery example, where there are nested popups that should
    // close in the correct order when the Escape key is pressed.
    QQuickApplicationHelper helper(this, QStringLiteral("closeOnEscapeWithNestedPopups.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // The stack view should have two items, and it should pop the second when escape is pressed
    // and it has focus.
    QQuickStackView *stackView = window->findChild<QQuickStackView*>("stackView");
    QVERIFY(stackView);
    QCOMPARE(stackView->depth(), 2);

    QQuickItem *optionsToolButton = window->findChild<QQuickItem*>("optionsToolButton");
    QVERIFY(optionsToolButton);

    // Click on the options tool button. The settings menu should pop up.
    const QPoint optionsToolButtonCenter = optionsToolButton->mapToScene(
        QPointF(optionsToolButton->width() / 2, optionsToolButton->height() / 2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, optionsToolButtonCenter);

    QQuickPopup *optionsMenu = window->findChild<QQuickPopup*>("optionsMenu");
    QVERIFY(optionsMenu);
    QTRY_VERIFY(optionsMenu->isVisible());

    QQuickItem *settingsMenuItem = window->findChild<QQuickItem*>("settingsMenuItem");
    QVERIFY(settingsMenuItem);

    // Click on the settings menu item. The settings dialog should pop up.
    const QPoint settingsMenuItemCenter = settingsMenuItem->mapToScene(
        QPointF(settingsMenuItem->width() / 2, settingsMenuItem->height() / 2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, settingsMenuItemCenter);

    QQuickPopup *settingsDialog = window->contentItem()->findChild<QQuickPopup*>("settingsDialog");
    QVERIFY(settingsDialog);
    QTRY_VERIFY(settingsDialog->isVisible());

    QQuickComboBox *comboBox = window->contentItem()->findChild<QQuickComboBox*>("comboBox");
    QVERIFY(comboBox);

    // Click on the combo box button. The combo box popup should pop up.
    const QPoint comboBoxCenter = comboBox->mapToScene(
        QPointF(comboBox->width() / 2, comboBox->height() / 2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, comboBoxCenter);
    QTRY_VERIFY(comboBox->popup()->isVisible());

    // Close the combo box popup with the escape key. The settings dialog should still be visible.
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!comboBox->popup()->isVisible());
    QVERIFY(settingsDialog->isVisible());

    // Close the settings dialog with the escape key.
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!settingsDialog->isVisible());

    // The stack view should still have two items.
    QCOMPARE(stackView->depth(), 2);

    // Remove one by pressing the Escape key (the Shortcut should be activated).
    QTest::keyClick(window, Qt::Key_Escape);
    QCOMPARE(stackView->depth(), 1);
}

void tst_QQuickPopup::enabled()
{
    QQuickPopup popup;
    QVERIFY(popup.isEnabled());
    QVERIFY(popup.popupItem()->isEnabled());

    QSignalSpy enabledSpy(&popup, &QQuickPopup::enabledChanged);
    QVERIFY(enabledSpy.isValid());

    popup.setEnabled(false);
    QVERIFY(!popup.isEnabled());
    QVERIFY(!popup.popupItem()->isEnabled());
    QCOMPARE(enabledSpy.count(), 1);

    popup.popupItem()->setEnabled(true);
    QVERIFY(popup.isEnabled());
    QVERIFY(popup.popupItem()->isEnabled());
    QCOMPARE(enabledSpy.count(), 2);
}

void tst_QQuickPopup::orientation_data()
{
    QTest::addColumn<Qt::ScreenOrientation>("orientation");
    QTest::addColumn<QPointF>("position");

    QTest::newRow("Portrait") << Qt::PortraitOrientation << QPointF(330, 165);
    QTest::newRow("Landscape") << Qt::LandscapeOrientation << QPointF(165, 270);
    QTest::newRow("InvertedPortrait") << Qt::InvertedPortraitOrientation << QPointF(270, 135);
    QTest::newRow("InvertedLandscape") << Qt::InvertedLandscapeOrientation << QPointF(135, 330);
}

void tst_QQuickPopup::orientation()
{
    QFETCH(Qt::ScreenOrientation, orientation);
    QFETCH(QPointF, position);

    QQuickApplicationHelper helper(this, "orientation.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->reportContentOrientationChange(orientation);
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);
    popup->open();

    QCOMPARE(popup->popupItem()->position(), position);
}

void tst_QQuickPopup::qquickview()
{
    QQuickView view;
    view.setObjectName("QQuickView");
    view.resize(400, 400);
    view.setSource(testFileUrl("dialog.qml"));
    QVERIFY(view.status() != QQuickView::Error);
    view.contentItem()->setObjectName("QQuickViewContentItem");
    view.show();

    QQuickDialog *dialog = view.rootObject()->property("dialog").value<QQuickDialog*>();
    QVERIFY(dialog);
    QTRY_COMPARE(dialog->property("opened").toBool(), true);

    dialog->close();
    QTRY_COMPARE(dialog->property("visible").toBool(), false);

    // QTBUG-72746: shouldn't crash on application exit after closing a Dialog when using QQuickView.
}

// TODO: also test it out without setting enabled directly on menu, but on a parent

// QTBUG-73447
void tst_QQuickPopup::disabledPalette()
{
    QQuickApplicationHelper helper(this, "disabledPalette.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QSignalSpy popupEnabledSpy(popup, SIGNAL(enabledChanged()));
    QVERIFY(popupEnabledSpy.isValid());
    QSignalSpy popupPaletteSpy(popup, SIGNAL(paletteChanged()));
    QVERIFY(popupPaletteSpy.isValid());

    QSignalSpy popupItemEnabledSpy(popup->popupItem(), SIGNAL(enabledChanged()));
    QVERIFY(popupItemEnabledSpy.isValid());
    QSignalSpy popupItemPaletteSpy(popup->popupItem(), SIGNAL(paletteChanged()));
    QVERIFY(popupItemPaletteSpy.isValid());

    QPalette palette = popup->palette();
    palette.setColor(QPalette::Active, QPalette::Base, Qt::green);
    palette.setColor(QPalette::Disabled, QPalette::Base, Qt::red);
    popup->setPalette(palette);
    QCOMPARE(popupPaletteSpy.count(), 1);
    QCOMPARE(popupItemPaletteSpy.count(), 1);
    QCOMPARE(popup->background()->property("color").value<QColor>(), Qt::green);

    popup->setEnabled(false);
    QCOMPARE(popupEnabledSpy.count(), 1);
    QCOMPARE(popupItemEnabledSpy.count(), 1);
    QCOMPARE(popupPaletteSpy.count(), 2);
    QCOMPARE(popupItemPaletteSpy.count(), 2);
    QCOMPARE(popup->background()->property("color").value<QColor>(), Qt::red);
}

void tst_QQuickPopup::disabledParentPalette()
{
    QQuickApplicationHelper helper(this, "disabledPalette.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);

    QSignalSpy popupEnabledSpy(popup, SIGNAL(enabledChanged()));
    QVERIFY(popupEnabledSpy.isValid());
    QSignalSpy popupPaletteSpy(popup, SIGNAL(paletteChanged()));
    QVERIFY(popupPaletteSpy.isValid());

    QSignalSpy popupItemEnabledSpy(popup->popupItem(), SIGNAL(enabledChanged()));
    QVERIFY(popupItemEnabledSpy.isValid());
    QSignalSpy popupItemPaletteSpy(popup->popupItem(), SIGNAL(paletteChanged()));
    QVERIFY(popupItemPaletteSpy.isValid());

    QPalette palette = popup->palette();
    palette.setColor(QPalette::Active, QPalette::Base, Qt::green);
    palette.setColor(QPalette::Disabled, QPalette::Base, Qt::red);
    popup->setPalette(palette);
    QCOMPARE(popupPaletteSpy.count(), 1);
    QCOMPARE(popupItemPaletteSpy.count(), 1);
    QCOMPARE(popup->background()->property("color").value<QColor>(), Qt::green);

    // Disable the overlay (which is QQuickPopupItem's parent) to ensure that
    // the palette is changed when the popup is indirectly disabled.
    popup->open();
    QTRY_VERIFY(popup->isOpened());
    QVERIFY(QMetaObject::invokeMethod(window, "disableOverlay"));
    QVERIFY(!popup->isEnabled());
    QVERIFY(!popup->popupItem()->isEnabled());
    QCOMPARE(popup->background()->property("color").value<QColor>(), Qt::red);
    QCOMPARE(popupEnabledSpy.count(), 1);
    QCOMPARE(popupItemEnabledSpy.count(), 1);
    QCOMPARE(popupPaletteSpy.count(), 2);
    QCOMPARE(popupItemPaletteSpy.count(), 2);

    popup->close();
    QTRY_VERIFY(!popup->isVisible());
}

// QTBUG-73243
void tst_QQuickPopup::toolTipCrashOnClose()
{
    QQuickApplicationHelper helper(this, "toolTipCrashOnClose.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    // The warning only occurs with debug builds for some reason.
    // In any case, the warning is irrelevant, but using ShaderEffectSource is important, so we ignore it.
#ifdef QT_DEBUG
    QTest::ignoreMessage(QtWarningMsg, "ShaderEffectSource: 'recursive' must be set to true when rendering recursively.");
#endif
    QVERIFY(QTest::qWaitForWindowActive(window));

    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() / 2));
    QTRY_VERIFY(window->property("toolTipOpened").toBool());

    QVERIFY(window->close());
    // Shouldn't crash.
}

void tst_QQuickPopup::setOverlayParentToNull()
{
    QQuickApplicationHelper helper(this, "toolTipCrashOnClose.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
#ifdef QT_DEBUG
    QTest::ignoreMessage(QtWarningMsg, "ShaderEffectSource: 'recursive' must be set to true when rendering recursively.");
#endif
    QVERIFY(QTest::qWaitForWindowActive(window));

    QVERIFY(QMetaObject::invokeMethod(window, "nullifyOverlayParent"));

    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() / 2));
    QTRY_VERIFY(window->property("toolTipOpened").toBool());

    QVERIFY(window->close());
    // While nullifying the overlay parent doesn't make much sense, it shouldn't crash.
}

void tst_QQuickPopup::centerInOverlayWithinStackViewItem()
{
    QQuickApplicationHelper helper(this, "centerInOverlayWithinStackViewItem.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickPopup *popup = window->property("popup").value<QQuickPopup*>();
    QVERIFY(popup);
    QTRY_COMPARE(popup->isVisible(), true);

    // Shouldn't crash on exit.
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickPopup)

#include "tst_qquickpopup.moc"
