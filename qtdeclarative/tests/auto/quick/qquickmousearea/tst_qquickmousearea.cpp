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
#include <QtTest/QSignalSpy>
#include <QtQuick/private/qquickdrag_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquickflickable_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include <QtGui/qstylehints.h>
#include <QtGui/QCursor>
#include <QtGui/QScreen>
#include <qpa/qwindowsysteminterface.h>

class CircleMask : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)

public:
    virtual ~CircleMask() {}
    qreal radius() const { return m_radius; }
    void setRadius(qreal radius)
    {
        if (m_radius == radius)
            return;
        m_radius = radius;
        emit radiusChanged();
    }

    Q_INVOKABLE bool contains(const QPointF &point) const
    {
        QPointF center(m_radius, m_radius);
        QLineF line(center, point);
        return line.length() <= m_radius;
    }

signals:
    void radiusChanged();

private:
    qreal m_radius;
};

class EventSender : public QObject {
    Q_OBJECT

public:
    Q_INVOKABLE void sendMouseClick(QObject* obj ,qreal x , qreal y) {
        {
            QMouseEvent event(QEvent::MouseButtonPress, QPointF(x , y), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(obj, &event);
        }
        {
            QMouseEvent event(QEvent::MouseButtonRelease, QPointF(x , y), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            qApp->sendEvent(obj, &event);
        }
    }
};

class tst_QQuickMouseArea: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickMouseArea()
        : device(nullptr)
    {
        qmlRegisterType<CircleMask>("Test", 1, 0, "CircleMask");
        qmlRegisterType<EventSender>("Test", 1, 0, "EventSender");
    }

private slots:
    void initTestCase() override;
    void dragProperties();
    void resetDrag();
    void dragging_data() { acceptedButton_data(); }
    void dragging();
    void dragSmoothed();
    void dragThreshold_data();
    void dragThreshold();
    void invalidDrag_data() { rejectedButton_data(); }
    void invalidDrag();
    void cancelDragging();
    void availableDistanceLessThanDragThreshold();
    void setDragOnPressed();
    void updateMouseAreaPosOnClick();
    void updateMouseAreaPosOnResize();
    void noOnClickedWithPressAndHold();
    void onMousePressRejected();
    void pressedCanceledOnWindowDeactivate_data();
    void pressedCanceledOnWindowDeactivate();
    void doubleClick_data() { acceptedButton_data(); }
    void doubleClick();
    void clickTwice_data() { acceptedButton_data(); }
    void clickTwice();
    void invalidClick_data() { rejectedButton_data(); }
    void invalidClick();
    void pressedOrdering();
    void preventStealing();
    void clickThrough();
    void hoverPosition();
    void hoverPropagation();
    void hoverVisible();
    void hoverAfterPress();
    void subtreeHoverEnabled();
    void disableAfterPress();
    void onWheel();
    void transformedMouseArea_data();
    void transformedMouseArea();
    void pressedMultipleButtons_data();
    void pressedMultipleButtons();
    void changeAxis();
#if QT_CONFIG(cursor)
    void cursorShape();
#endif
    void moveAndReleaseWithoutPress();
    void nestedStopAtBounds();
    void nestedStopAtBounds_data();
    void nestedFlickableStopAtBounds();
    void containsPress_data();
    void containsPress();
    void ignoreBySource();
    void notPressedAfterStolenGrab();
    void pressAndHold_data();
    void pressAndHold();
    void pressOneAndTapAnother_data();
    void pressOneAndTapAnother();
    void mask();
    void nestedEventDelivery();
    void settingHiddenInPressUngrabs();

private:
    int startDragDistance() const {
        return QGuiApplication::styleHints()->startDragDistance();
    }
    void acceptedButton_data();
    void rejectedButton_data();
    QTouchDevice *device;
};

Q_DECLARE_METATYPE(Qt::MouseButton)
Q_DECLARE_METATYPE(Qt::MouseButtons)

void tst_QQuickMouseArea::initTestCase()
{
    QQmlDataTest::initTestCase();
    if (!device) {
        device = new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }
}

void tst_QQuickMouseArea::acceptedButton_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("left") << Qt::MouseButtons(Qt::LeftButton) << Qt::LeftButton;
    QTest::newRow("right") << Qt::MouseButtons(Qt::RightButton) << Qt::RightButton;
    QTest::newRow("middle") << Qt::MouseButtons(Qt::MiddleButton) << Qt::MiddleButton;

    QTest::newRow("left (left|right)") << Qt::MouseButtons(Qt::LeftButton | Qt::RightButton) << Qt::LeftButton;
    QTest::newRow("right (right|middle)") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::RightButton;
    QTest::newRow("middle (left|middle)") << Qt::MouseButtons(Qt::LeftButton | Qt::MiddleButton) << Qt::MiddleButton;
}

void tst_QQuickMouseArea::rejectedButton_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("middle (left|right)") << Qt::MouseButtons(Qt::LeftButton | Qt::RightButton) << Qt::MiddleButton;
    QTest::newRow("left (right|middle)") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::LeftButton;
    QTest::newRow("right (left|middle)") << Qt::MouseButtons(Qt::LeftButton | Qt::MiddleButton) << Qt::RightButton;
}

void tst_QQuickMouseArea::dragProperties()
{

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragproperties.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);

    // axis
    QCOMPARE(drag->axis(), QQuickDrag::XAndYAxis);
    QSignalSpy axisSpy(drag, SIGNAL(axisChanged()));
    drag->setAxis(QQuickDrag::XAxis);
    QCOMPARE(drag->axis(), QQuickDrag::XAxis);
    QCOMPARE(axisSpy.count(),1);
    drag->setAxis(QQuickDrag::XAxis);
    QCOMPARE(axisSpy.count(),1);

    // minimum and maximum properties
    QSignalSpy xminSpy(drag, SIGNAL(minimumXChanged()));
    QSignalSpy xmaxSpy(drag, SIGNAL(maximumXChanged()));
    QSignalSpy yminSpy(drag, SIGNAL(minimumYChanged()));
    QSignalSpy ymaxSpy(drag, SIGNAL(maximumYChanged()));

    QCOMPARE(drag->xmin(), 0.0);
    QCOMPARE(drag->xmax(), rootItem->width()-blackRect->width());
    QCOMPARE(drag->ymin(), 0.0);
    QCOMPARE(drag->ymax(), rootItem->height()-blackRect->height());

    drag->setXmin(10);
    drag->setXmax(10);
    drag->setYmin(10);
    drag->setYmax(10);

    QCOMPARE(drag->xmin(), 10.0);
    QCOMPARE(drag->xmax(), 10.0);
    QCOMPARE(drag->ymin(), 10.0);
    QCOMPARE(drag->ymax(), 10.0);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    drag->setXmin(10);
    drag->setXmax(10);
    drag->setYmin(10);
    drag->setYmax(10);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    // filterChildren
    QSignalSpy filterChildrenSpy(drag, SIGNAL(filterChildrenChanged()));

    drag->setFilterChildren(true);

    QVERIFY(drag->filterChildren());
    QCOMPARE(filterChildrenSpy.count(), 1);

    drag->setFilterChildren(true);
    QCOMPARE(filterChildrenSpy.count(), 1);

    // threshold
    QCOMPARE(int(drag->threshold()), qApp->styleHints()->startDragDistance());
    QSignalSpy thresholdSpy(drag, SIGNAL(thresholdChanged()));
    drag->setThreshold(0.0);
    QCOMPARE(drag->threshold(), 0.0);
    QCOMPARE(thresholdSpy.count(), 1);
    drag->setThreshold(99);
    QCOMPARE(thresholdSpy.count(), 2);
    drag->setThreshold(99);
    QCOMPARE(thresholdSpy.count(), 2);
    drag->resetThreshold();
    QCOMPARE(int(drag->threshold()), qApp->styleHints()->startDragDistance());
    QCOMPARE(thresholdSpy.count(), 3);
}

void tst_QQuickMouseArea::resetDrag()
{
    QQuickView window;
    QByteArray errorMessage;
    window.rootContext()->setContextProperty("haveTarget", QVariant(true));
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragreset.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    QVERIFY(drag->target() != nullptr);
    window.rootContext()->setContextProperty("haveTarget", QVariant(false));
    QCOMPARE(targetSpy.count(),1);
    QVERIFY(!drag->target());
}

void tst_QQuickMouseArea::dragging()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragging.qml"), true, &errorMessage), errorMessage.constData());

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(acceptedButtons);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, button, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.
    // The item is moved relative to the position of the mouse when the drag
    // was triggered, this prevents a sudden change in position when the drag
    // threshold is exceeded.

    int dragThreshold = QGuiApplication::styleHints()->startDragDistance();

    // move the minimum distance to activate drag
    p += QPoint(dragThreshold + 1, dragThreshold + 1);
    QTest::mouseMove(&window, p);
    QVERIFY(!drag->active());

    // from here on move the item
    p += QPoint(1, 1);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    // on macOS the cursor movement is going through a native event which
    // means that it can actually take some time to show
    QTRY_COMPARE(blackRect->x(), 50.0 + 1);
    QCOMPARE(blackRect->y(), 50.0 + 1);

    p += QPoint(10, 10);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    QTest::mouseRelease(&window, button, Qt::NoModifier, p);
    QTRY_VERIFY(!drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);
}

void tst_QQuickMouseArea::dragSmoothed()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragging.qml"), true, &errorMessage), errorMessage.constData());

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    drag->setThreshold(5);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QVERIFY(!drag->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 102), 50);
    QTest::mouseMove(&window, QPoint(100, 106), 50);
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QTRY_COMPARE(blackRect->x(), 50.0);
    QTRY_COMPARE(blackRect->y(), 66.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,122));

    // reset rect position
    blackRect->setX(50.0);
    blackRect->setY(50.0);

    // now try with smoothed disabled
    drag->setSmoothed(false);
    QVERIFY(!drag->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 102), 50);
    QTest::mouseMove(&window, QPoint(100, 106), 50);
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QTRY_COMPARE(blackRect->x(), 50.0);
    QTRY_COMPARE(blackRect->y(), 72.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 122));
}

void tst_QQuickMouseArea::dragThreshold_data()
{
    QTest::addColumn<bool>("preventStealing");
    QTest::newRow("without preventStealing") << false;
    QTest::newRow("with preventStealing") << true;
}

void tst_QQuickMouseArea::dragThreshold()
{
    QFETCH(bool, preventStealing);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragging.qml"), true, &errorMessage), errorMessage.constData());

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    mouseRegion->setPreventStealing(preventStealing);
    QQuickDrag *drag = mouseRegion->drag();

    drag->setThreshold(5);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QVERIFY(!drag->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);
    QTest::mouseMove(&window, QPoint(100, 102), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 100), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 104), 50);
    QTest::mouseMove(&window, QPoint(100, 105), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 106), 50);
    QTest::mouseMove(&window, QPoint(100, 108), 50);
    QVERIFY(drag->active());
    QTest::mouseMove(&window, QPoint(100, 116), 50);
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 50.0);
    QTRY_COMPARE(blackRect->y(), 66.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(122,122));
    QTRY_VERIFY(!drag->active());

    // Immediate drag threshold
    drag->setThreshold(0);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 123), 50);
    QVERIFY(drag->active());
    QTest::mouseMove(&window, QPoint(100, 124), 50);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 124));
    QTRY_VERIFY(!drag->active());
    drag->resetThreshold();
}
void tst_QQuickMouseArea::invalidDrag()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragging.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(acceptedButtons);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    QTest::mousePress(&window, button, Qt::NoModifier, QPoint(100,100));

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    QTest::mouseMove(&window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(&window, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseRelease(&window, button, Qt::NoModifier, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);
}

void tst_QQuickMouseArea::cancelDragging()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragging.qml"), true, &errorMessage), errorMessage.constData());

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    p += QPoint(startDragDistance() + 1, 0);
    QTest::mouseMove(&window, p);

    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    mouseRegion->QQuickItem::ungrabMouse();
    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    QTest::mouseMove(&window, QPoint(132,132), 50);
    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(122,122));
}

// QTBUG-58347
void tst_QQuickMouseArea::availableDistanceLessThanDragThreshold()
{
    QQuickView view;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(view, testFileUrl("availableDistanceLessThanDragThreshold.qml"), true, &errorMessage),
             errorMessage.constData());
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject());

    QQuickMouseArea *mouseArea = view.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea);

    QPoint position(100, 100);
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    position.setX(301);
    QTest::mouseMove(&view, position);
    position.setX(501);
    QTest::mouseMove(&view, position);
    QVERIFY(mouseArea->drag()->active());
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!mouseArea->drag()->active());
    QCOMPARE(mouseArea->x(), 200.0);
}

void tst_QQuickMouseArea::setDragOnPressed()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("setDragOnPressed.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(window.rootObject());
    QVERIFY(mouseArea);

    // target
    QQuickItem *target = mouseArea->findChild<QQuickItem*>("target");
    QVERIFY(target);

    QPoint p = QPoint(100, 100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    QQuickDrag *drag = mouseArea->drag();
    QVERIFY(drag);
    QVERIFY(!drag->active());

    QCOMPARE(target->x(), 50.0);
    QCOMPARE(target->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    p += QPoint(startDragDistance() + 1, 0);
    QTest::mouseMove(&window, p);

    p += QPoint(11, 0);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(target->x(), 61.0);
    QCOMPARE(target->y(), 50.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_VERIFY(!drag->active());
    QCOMPARE(target->x(), 61.0);
    QCOMPARE(target->y(), 50.0);
}

void tst_QQuickMouseArea::updateMouseAreaPosOnClick()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("updateMousePosOnClick.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != nullptr);

    QQuickRectangle *rect = window.rootObject()->findChild<QQuickRectangle*>("ball");
    QVERIFY(rect != nullptr);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    QMouseEvent event(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
    QGuiApplication::sendEvent(&window, &event);

    QCOMPARE(mouseRegion->mouseX(), 100.0);
    QCOMPARE(mouseRegion->mouseY(), 100.0);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());
}

void tst_QQuickMouseArea::updateMouseAreaPosOnResize()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("updateMousePosOnResize.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != nullptr);

    QQuickRectangle *rect = window.rootObject()->findChild<QQuickRectangle*>("brother");
    QVERIFY(rect != nullptr);

    QCOMPARE(mouseRegion->mouseX(), 0.0);
    QCOMPARE(mouseRegion->mouseY(), 0.0);

    QMouseEvent event(QEvent::MouseButtonPress, rect->position().toPoint(), Qt::LeftButton, Qt::LeftButton, nullptr);
    QGuiApplication::sendEvent(&window, &event);

    QVERIFY(!mouseRegion->property("emitPositionChanged").toBool());
    QVERIFY(mouseRegion->property("mouseMatchesPos").toBool());

    QCOMPARE(mouseRegion->property("x1").toReal(), 0.0);
    QCOMPARE(mouseRegion->property("y1").toReal(), 0.0);

    QCOMPARE(mouseRegion->property("x2").toReal(), rect->x());
    QCOMPARE(mouseRegion->property("y2").toReal(), rect->y());

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());
}

void tst_QQuickMouseArea::noOnClickedWithPressAndHold()
{
    {
        // We handle onPressAndHold, therefore no onClicked
        QQuickView window;
        QByteArray errorMessage;
        QVERIFY2(QQuickTest::initView(window, testFileUrl("clickandhold.qml"), true, &errorMessage), errorMessage.constData());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.rootObject() != nullptr);
        QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea*>(window.rootObject()->children().first());
        QVERIFY(mouseArea);

        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
        QGuiApplication::sendEvent(&window, &pressEvent);

        QCOMPARE(mouseArea->pressedButtons(), Qt::LeftButton);
        QVERIFY(!window.rootObject()->property("clicked").toBool());
        QVERIFY(!window.rootObject()->property("held").toBool());

        // timeout is 800 (in qquickmousearea.cpp)
        QTest::qWait(1000);
        QCoreApplication::processEvents();

        QVERIFY(!window.rootObject()->property("clicked").toBool());
        QVERIFY(window.rootObject()->property("held").toBool());

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
        QGuiApplication::sendEvent(&window, &releaseEvent);

        QTRY_VERIFY(window.rootObject()->property("held").toBool());
        QVERIFY(!window.rootObject()->property("clicked").toBool());
    }

    {
        // We do not handle onPressAndHold, therefore we get onClicked
        QQuickView window;
        QByteArray errorMessage;
        QVERIFY2(QQuickTest::initView(window, testFileUrl("noclickandhold.qml"), true, &errorMessage), errorMessage.constData());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.rootObject() != nullptr);

        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
        QGuiApplication::sendEvent(&window, &pressEvent);

        QVERIFY(!window.rootObject()->property("clicked").toBool());

        QTest::qWait(1000);

        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
        QGuiApplication::sendEvent(&window, &releaseEvent);

        QVERIFY(window.rootObject()->property("clicked").toBool());
    }
}

void tst_QQuickMouseArea::onMousePressRejected()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("rejectEvent.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);
    QVERIFY(window.rootObject()->property("enabled").toBool());

    QVERIFY(!window.rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr1_released").toBool());
    QVERIFY(!window.rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!window.rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr2_released").toBool());
    QVERIFY(!window.rootObject()->property("mr2_canceled").toBool());

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);

    QVERIFY(window.rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr1_released").toBool());
    QVERIFY(!window.rootObject()->property("mr1_canceled").toBool());
    QVERIFY(window.rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr2_released").toBool());
    QVERIFY(!window.rootObject()->property("mr2_canceled").toBool());

    QTest::qWait(200);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QVERIFY(window.rootObject()->property("mr1_released").toBool());
    QVERIFY(!window.rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!window.rootObject()->property("mr2_released").toBool());
}

void tst_QQuickMouseArea::pressedCanceledOnWindowDeactivate_data()
{
    QTest::addColumn<bool>("doubleClick");
    QTest::newRow("simple click") << false;
    QTest::newRow("double click") << true;
}


void tst_QQuickMouseArea::pressedCanceledOnWindowDeactivate()
{
    QFETCH(bool, doubleClick);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("pressedCanceled.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);
    QVERIFY(!window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());

    int expectedRelease = 0;
    int expectedClicks = 0;
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);


    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);

    QGuiApplication::sendEvent(&window, &pressEvent);

    QTRY_VERIFY(window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    if (doubleClick) {
        QGuiApplication::sendEvent(&window, &releaseEvent);
        QTRY_VERIFY(!window.rootObject()->property("pressed").toBool());
        QVERIFY(!window.rootObject()->property("canceled").toBool());
        QCOMPARE(window.rootObject()->property("released").toInt(), ++expectedRelease);
        QCOMPARE(window.rootObject()->property("clicked").toInt(), ++expectedClicks);

        QGuiApplication::sendEvent(&window, &pressEvent);
        QMouseEvent pressEvent2(QEvent::MouseButtonDblClick, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
        QGuiApplication::sendEvent(&window, &pressEvent2);

        QTRY_VERIFY(window.rootObject()->property("pressed").toBool());
        QVERIFY(!window.rootObject()->property("canceled").toBool());
        QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
        QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);
        QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);
    }


    QWindow *secondWindow = qvariant_cast<QWindow*>(window.rootObject()->property("secondWindow"));
    secondWindow->setProperty("visible", true);
    QVERIFY(QTest::qWaitForWindowExposed(secondWindow));

    QTRY_VERIFY(!window.rootObject()->property("pressed").toBool());
    QVERIFY(window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    //press again
    QGuiApplication::sendEvent(&window, &pressEvent);
    QTRY_VERIFY(window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    //release
    QGuiApplication::sendEvent(&window, &releaseEvent);
    QTRY_VERIFY(!window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), ++expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), ++expectedClicks);
}

void tst_QQuickMouseArea::doubleClick()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("doubleclick.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    // The sequence for a double click is:
    // press, release, (click), press, double click, release
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("released").toInt(), 1);

    QGuiApplication::sendEvent(&window, &pressEvent);
    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 2);
}

// QTBUG-14832
void tst_QQuickMouseArea::clickTwice()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("clicktwice.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);

    QGuiApplication::sendEvent(&window, &pressEvent);
    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 2);
    QCOMPARE(window.rootObject()->property("released").toInt(), 2);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 2);
}

void tst_QQuickMouseArea::invalidClick()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("doubleclick.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    // The sequence for a double click is:
    // press, release, (click), press, double click, release
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("released").toInt(), 0);

    QGuiApplication::sendEvent(&window, &pressEvent);
    pressEvent = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100), button, button, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("clicked").toInt(), 0);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 0);
    QCOMPARE(window.rootObject()->property("released").toInt(), 0);
}

void tst_QQuickMouseArea::pressedOrdering()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("pressedOrdering.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("base"));

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
    QGuiApplication::sendEvent(&window, &pressEvent);

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("pressed"));

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100), Qt::LeftButton, Qt::LeftButton, nullptr);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("toggled"));

    QGuiApplication::sendEvent(&window, &pressEvent);

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("pressed"));
}

void tst_QQuickMouseArea::preventStealing()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("preventstealing.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mousearea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));

    QPoint p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    p += QPoint(-startDragDistance() * 2, -startDragDistance() * 2);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);

    // We should have received all four move events
    QTRY_COMPARE(mousePositionSpy.count(), 4);
    mousePositionSpy.clear();
    QVERIFY(mouseArea->pressed());

    // Flickable content should not have moved.
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);

    // Now allow stealing and confirm Flickable does its thing.
    window.rootObject()->setProperty("stealing", false);

    p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    p += QPoint(-startDragDistance() * 2, -startDragDistance() * 2);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);

    // We should only have received the first move event
    QTRY_COMPARE(mousePositionSpy.count(), 1);
    // Our press should be taken away
    QVERIFY(!mouseArea->pressed());

    // Flickable swallows the first move, then moves 2*10 px
    QTRY_COMPARE(flickable->contentX(), 20.);
    QCOMPARE(flickable->contentY(), 20.);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
}

void tst_QQuickMouseArea::clickThrough()
{
    //With no handlers defined click, doubleClick and PressAndHold should propagate to those with handlers
    QScopedPointer<QQuickView> window(new QQuickView);
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(*window.data(), testFileUrl("clickThrough.qml"), true, &errorMessage), errorMessage.constData());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QVERIFY(window->rootObject() != nullptr);

    // to avoid generating a double click.
    const int doubleClickInterval = qApp->styleHints()->mouseDoubleClickInterval() + 10;

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QTRY_COMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 1);

    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 0);
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::qWait(1000);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QTRY_COMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 1);
    QTRY_COMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    QTest::mouseDClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 2);
    QTRY_COMPARE(window->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    window.reset(new QQuickView);

    //With handlers defined click, doubleClick and PressAndHold should propagate only when explicitly ignored
    QVERIFY2(QQuickTest::initView(*window.data(), testFileUrl("clickThrough2.qml"), true, &errorMessage), errorMessage.constData());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QVERIFY(window->rootObject() != nullptr);

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 0);

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::qWait(1000);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 0);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 0);

    QTest::mouseDClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 0);
    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 0);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 0);

    window->rootObject()->setProperty("letThrough", QVariant(true));

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 1);

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::qWait(1000);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QCOMPARE(window->rootObject()->property("clicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    QTest::mouseDClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 2);
    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    window->rootObject()->setProperty("noPropagation", QVariant(true));

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::qWait(1000);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QTest::mouseDClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(100);

    QCOMPARE(window->rootObject()->property("presses").toInt(), 0);
    QTRY_COMPARE(window->rootObject()->property("clicks").toInt(), 2);
    QCOMPARE(window->rootObject()->property("doubleClicks").toInt(), 1);
    QCOMPARE(window->rootObject()->property("pressAndHolds").toInt(), 1);

    window.reset(new QQuickView);

    //QTBUG-34368 - Shouldn't propagate to disabled mouse areas
    QVERIFY2(QQuickTest::initView(*window.data(), testFileUrl("qtbug34368.qml"), true, &errorMessage), errorMessage.constData());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QVERIFY(window->rootObject() != nullptr);

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QCOMPARE(window->rootObject()->property("clicksEnabled").toInt(), 1);
    QCOMPARE(window->rootObject()->property("clicksDisabled").toInt(), 1); //Not disabled yet

    window->rootObject()->setProperty("disableLower", QVariant(true));

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QCOMPARE(window->rootObject()->property("clicksEnabled").toInt(), 2);
    QCOMPARE(window->rootObject()->property("clicksDisabled").toInt(), 1); //disabled, shouldn't increment

    window.reset(new QQuickView);

    //QTBUG-49100
    QVERIFY2(QQuickTest::initView(*window.data(), testFileUrl("qtbug49100.qml"), true, &errorMessage), errorMessage.constData());
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QVERIFY(window->rootObject() != nullptr);

    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QVERIFY(window->rootObject() != nullptr);
}

void tst_QQuickMouseArea::hoverPosition()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("hoverPosition.qml"), true, &errorMessage), errorMessage.constData());
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QCOMPARE(root->property("mouseX").toReal(), qreal(0));
    QCOMPARE(root->property("mouseY").toReal(), qreal(0));

    QTest::mouseMove(&window,QPoint(10,32));


    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));
}

void tst_QQuickMouseArea::hoverPropagation()
{
    //QTBUG-18175, to behave like GV did.
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("hoverPropagation.qml"), true, &errorMessage), errorMessage.constData());
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), false);

    QMouseEvent moveEvent(QEvent::MouseMove, QPoint(32, 32), Qt::NoButton, Qt::NoButton, nullptr);
    QGuiApplication::sendEvent(&window, &moveEvent);

    QCOMPARE(root->property("point1").toBool(), true);
    QCOMPARE(root->property("point2").toBool(), false);

    QMouseEvent moveEvent2(QEvent::MouseMove, QPoint(232, 32), Qt::NoButton, Qt::NoButton, nullptr);
    QGuiApplication::sendEvent(&window, &moveEvent2);
    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), true);
}

void tst_QQuickMouseArea::hoverVisible()
{
    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimimal platforms");

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("hoverVisible.qml"), true, &errorMessage), errorMessage.constData());
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QQuickMouseArea *mouseTracker = window.rootObject()->findChild<QQuickMouseArea*>("mousetracker");
    QVERIFY(mouseTracker != nullptr);

    QSignalSpy enteredSpy(mouseTracker, SIGNAL(entered()));

    // Note: We need to use a position that is different from the position in the last event
    // generated in the previous test case. Otherwise it is not interpreted as a move.
    QTest::mouseMove(&window,QPoint(11,33));

    QCOMPARE(mouseTracker->hovered(), false);
    QCOMPARE(enteredSpy.count(), 0);

    mouseTracker->setVisible(true);

    QCOMPARE(mouseTracker->hovered(), true);
    QCOMPARE(enteredSpy.count(), 1);

    QCOMPARE(QPointF(mouseTracker->mouseX(), mouseTracker->mouseY()), QPointF(11,33));
}

void tst_QQuickMouseArea::hoverAfterPress()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("hoverAfterPress.qml"), true, &errorMessage), errorMessage.constData());
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);
    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
}

void tst_QQuickMouseArea::subtreeHoverEnabled()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("qtbug54019.qml"), true, &errorMessage), errorMessage.constData());
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>();
    QQuickItemPrivate *rootPrivate = QQuickItemPrivate::get(root);
    QVERIFY(mouseArea != nullptr);
    QTest::mouseMove(&window, QPoint(10, 160));
    QCOMPARE(mouseArea->hovered(), false);
    QVERIFY(rootPrivate->subtreeHoverEnabled);
    QTest::mouseMove(&window, QPoint(10, 10));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(&window, QPoint(160, 10));
    QCOMPARE(mouseArea->hovered(), false);
}

void tst_QQuickMouseArea::disableAfterPress()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragging.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseArea->drag();
    QVERIFY(mouseArea != nullptr);
    QVERIFY(drag != nullptr);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));
    QSignalSpy mousePressSpy(mouseArea, SIGNAL(pressed(QQuickMouseEvent*)));
    QSignalSpy mouseReleaseSpy(mouseArea, SIGNAL(released(QQuickMouseEvent*)));

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());
    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_COMPARE(mousePressSpy.count(), 1);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    p += QPoint(startDragDistance() + 1, 0);
    QTest::mouseMove(&window, p);
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_COMPARE(mousePositionSpy.count(), 2);

    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    mouseArea->setEnabled(false);

    // move should still be acted upon
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_COMPARE(mousePositionSpy.count(), 4);

    QVERIFY(drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 83.0);

    QVERIFY(mouseArea->pressed());
    QVERIFY(mouseArea->hovered());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);

    QTRY_COMPARE(mouseReleaseSpy.count(), 1);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 83.0);

    QVERIFY(!mouseArea->pressed());
    QVERIFY(!mouseArea->hovered()); // since hover is not enabled

    // Next press will be ignored
    blackRect->setX(50);
    blackRect->setY(50);

    mousePressSpy.clear();
    mousePositionSpy.clear();
    mouseReleaseSpy.clear();

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(50);
    QCOMPARE(mousePressSpy.count(), 0);

    QTest::mouseMove(&window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(&window, QPoint(122,122));
    QTest::qWait(50);

    QCOMPARE(mousePositionSpy.count(), 0);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(122,122));
    QTest::qWait(50);

    QCOMPARE(mouseReleaseSpy.count(), 0);
}

void tst_QQuickMouseArea::onWheel()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("wheel.qml"), true, &errorMessage), errorMessage.constData());
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QWheelEvent wheelEvent(QPoint(10, 32), QPoint(10, 32), QPoint(60, 20), QPoint(0, 120),
                           0, Qt::Vertical,Qt::NoButton, Qt::ControlModifier);
    QGuiApplication::sendEvent(&window, &wheelEvent);

    QCOMPARE(root->property("angleDeltaY").toInt(), 120);
    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));
    QCOMPARE(root->property("controlPressed").toBool(), true);
}

void tst_QQuickMouseArea::transformedMouseArea_data()
{
    QTest::addColumn<bool>("insideTarget");
    QTest::addColumn<QList<QPoint> >("points");

    QList<QPoint> pointsInside;
    pointsInside << QPoint(200, 140)
                 << QPoint(140, 200)
                 << QPoint(200, 200)
                 << QPoint(260, 200)
                 << QPoint(200, 260);
    QTest::newRow("checking points inside") << true << pointsInside;

    QList<QPoint> pointsOutside;
    pointsOutside << QPoint(140, 140)
                  << QPoint(260, 140)
                  << QPoint(120, 200)
                  << QPoint(280, 200)
                  << QPoint(140, 260)
                  << QPoint(260, 260);
    QTest::newRow("checking points outside") << false << pointsOutside;
}

void tst_QQuickMouseArea::transformedMouseArea()
{
    QFETCH(bool, insideTarget);
    QFETCH(QList<QPoint>, points);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("transformedMouseArea.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea != nullptr);

    foreach (const QPoint &point, points) {
        // check hover
        QTest::mouseMove(&window, point);
        QTRY_COMPARE(mouseArea->property("containsMouse").toBool(), insideTarget);

        // check mouse press
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(mouseArea->property("pressed").toBool(), insideTarget);

        // check mouse release
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(mouseArea->property("pressed").toBool(), false);
    }
}

struct MouseEvent {
    QEvent::Type type;
    Qt::MouseButton button;
};
Q_DECLARE_METATYPE(MouseEvent)

void tst_QQuickMouseArea::pressedMultipleButtons_data()
{
    QTest::addColumn<Qt::MouseButtons>("accepted");
    QTest::addColumn<QList<MouseEvent> >("mouseEvents");
    QTest::addColumn<QList<bool> >("pressed");
    QTest::addColumn<QList<Qt::MouseButtons> >("pressedButtons");
    QTest::addColumn<int>("changeCount");

    Qt::MouseButtons accepted;
    QList<MouseEvent> mouseEvents;
    QList<bool> pressed;
    QList<Qt::MouseButtons> pressedButtons;
    int changeCount;

    MouseEvent leftPress = { QEvent::MouseButtonPress, Qt::LeftButton };
    MouseEvent leftRelease = { QEvent::MouseButtonRelease, Qt::LeftButton };
    MouseEvent rightPress = { QEvent::MouseButtonPress, Qt::RightButton };
    MouseEvent rightRelease = { QEvent::MouseButtonRelease, Qt::RightButton };

    auto addRowWithFormattedTitleAndReset = [&]() {
        QByteArray title;
        title.append("Accept:");
        if (accepted & Qt::LeftButton)
            title.append(" LeftButton");
        if (accepted & Qt::RightButton)
            title.append(" RightButton");
        title.append(" | Events:");
        for (MouseEvent event : mouseEvents) {
            title.append(event.type == QEvent::MouseButtonPress ? " Press" : " Release");
            title.append(event.button == Qt::LeftButton ? " Left," : " Right,");
        }
        title.chop(1); // remove last comma
        QTest::newRow(title) << accepted << mouseEvents << pressed << pressedButtons << changeCount;

        mouseEvents.clear();
        pressed.clear();
        pressedButtons.clear();
    };

    accepted = Qt::LeftButton;
    changeCount = 2;
    mouseEvents << leftPress << rightPress << rightRelease << leftRelease;
    pressed << true << true << true << false;
    pressedButtons << Qt::LeftButton << Qt::LeftButton << Qt::LeftButton << Qt::NoButton;
    addRowWithFormattedTitleAndReset();

    accepted = Qt::LeftButton;
    changeCount = 2;
    mouseEvents << leftPress << rightPress << leftRelease << rightRelease;
    pressed << true << true << false << false;
    pressedButtons << Qt::LeftButton << Qt::LeftButton << Qt::NoButton << Qt::NoButton;
    addRowWithFormattedTitleAndReset();

    accepted = Qt::LeftButton | Qt::RightButton;
    changeCount = 4;
    mouseEvents << leftPress << rightPress << rightRelease << leftRelease;
    pressed << true << true << true << false;
    pressedButtons << Qt::LeftButton << (Qt::LeftButton | Qt::RightButton) << Qt::LeftButton
                   << Qt::NoButton;
    addRowWithFormattedTitleAndReset();

    accepted = Qt::RightButton;
    changeCount = 2;
    mouseEvents << rightPress << leftPress << rightRelease << leftRelease;
    pressed << true << true << false << false;
    pressedButtons << Qt::RightButton << Qt::RightButton << Qt::NoButton << Qt::NoButton;
    addRowWithFormattedTitleAndReset();
}

void tst_QQuickMouseArea::pressedMultipleButtons()
{
    QFETCH(Qt::MouseButtons, accepted);
    QFETCH(QList<MouseEvent>, mouseEvents);
    QFETCH(QList<bool>, pressed);
    QFETCH(QList<Qt::MouseButtons>, pressedButtons);
    QFETCH(int, changeCount);

    QQuickView view;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(view, testFileUrl("simple.qml"), true, &errorMessage), errorMessage.constData());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject() != nullptr);

    QQuickMouseArea *mouseArea = view.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy pressedSpy(mouseArea, SIGNAL(pressedChanged()));
    QSignalSpy pressedButtonsSpy(mouseArea, SIGNAL(pressedButtonsChanged()));
    mouseArea->setAcceptedMouseButtons(accepted);

    QPoint point(10, 10);
    for (int i = 0; i < mouseEvents.count(); ++i) {
        const MouseEvent mouseEvent = mouseEvents.at(i);
        if (mouseEvent.type == QEvent::MouseButtonPress)
            QTest::mousePress(&view, mouseEvent.button, Qt::NoModifier, point);
        else
            QTest::mouseRelease(&view, mouseEvent.button, Qt::NoModifier, point);
        QCOMPARE(mouseArea->pressed(), pressed.at(i));
        QCOMPARE(mouseArea->pressedButtons(), pressedButtons.at(i));
    }

    QCOMPARE(pressedSpy.count(), 2);
    QCOMPARE(pressedButtonsSpy.count(), changeCount);
}

void tst_QQuickMouseArea::changeAxis()
{
    QQuickView view;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(view, testFileUrl("changeAxis.qml"), true, &errorMessage), errorMessage.constData());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.rootObject() != nullptr);

    QQuickMouseArea *mouseRegion = view.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);

    // target
    QQuickItem *blackRect = view.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    // Start a diagonal drag
    QPoint p = QPoint(100, 100);
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    p += QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::mouseMove(&view, p);
    p += QPoint(11, 11);
    QTest::mouseMove(&view, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);
    QCOMPARE(drag->axis(), QQuickDrag::XAndYAxis);

    /* When blackRect.x becomes bigger than 75, the drag axis is changed to
     * Drag.YAxis by the QML code. Verify that this happens, and that the drag
     * movement is effectively constrained to the Y axis. */
    p += QPoint(22, 22);
    QTest::mouseMove(&view, p);

    QTRY_COMPARE(blackRect->x(), 83.0);
    QTRY_COMPARE(blackRect->y(), 83.0);
    QTRY_COMPARE(drag->axis(), QQuickDrag::YAxis);

    p += QPoint(11, 11);
    QTest::mouseMove(&view, p);

    QTRY_COMPARE(blackRect->y(), 94.0);
    QCOMPARE(blackRect->x(), 83.0);

    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, p);

    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 94.0);
}

#if QT_CONFIG(cursor)
void tst_QQuickMouseArea::cursorShape()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n MouseArea {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(object.data());
    QVERIFY(mouseArea);

    QSignalSpy spy(mouseArea, SIGNAL(cursorShapeChanged()));

    QCOMPARE(mouseArea->cursorShape(), Qt::ArrowCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::ArrowCursor);

    mouseArea->setCursorShape(Qt::IBeamCursor);
    QCOMPARE(mouseArea->cursorShape(), Qt::IBeamCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(spy.count(), 1);

    mouseArea->setCursorShape(Qt::IBeamCursor);
    QCOMPARE(spy.count(), 1);

    mouseArea->setCursorShape(Qt::WaitCursor);
    QCOMPARE(mouseArea->cursorShape(), Qt::WaitCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::WaitCursor);
    QCOMPARE(spy.count(), 2);
}
#endif

void tst_QQuickMouseArea::moveAndReleaseWithoutPress()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("moveAndReleaseWithoutPress.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QObject *root = window.rootObject();
    QVERIFY(root);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    // the press was not accepted, make sure there is no move or release event
    QTest::mouseMove(&window, QPoint(110,110), 50);

    // use qwait here because we want to make sure an event does NOT happen
    // the test fails if the default state changes, while it shouldn't
    QTest::qWait(100);
    QCOMPARE(root->property("hadMove").toBool(), false);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(110,110));
    QTest::qWait(100);
    QCOMPARE(root->property("hadRelease").toBool(), false);
}

void tst_QQuickMouseArea::nestedStopAtBounds_data()
{
    QTest::addColumn<bool>("transpose");
    QTest::addColumn<bool>("invert");

    QTest::newRow("left") << false << false;
    QTest::newRow("right") << false << true;
    QTest::newRow("top") << true << false;
    QTest::newRow("bottom") << true << true;
}

void tst_QQuickMouseArea::nestedStopAtBounds()
{
    QFETCH(bool, transpose);
    QFETCH(bool, invert);

    QQuickView view;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(view, testFileUrl("nestedStopAtBounds.qml"), true, &errorMessage), errorMessage.constData());
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject());

    QQuickMouseArea *outer =  view.rootObject()->findChild<QQuickMouseArea*>("outer");
    QVERIFY(outer);

    QQuickMouseArea *inner = outer->findChild<QQuickMouseArea*>("inner");
    QVERIFY(inner);
    inner->drag()->setAxis(transpose ? QQuickDrag::YAxis : QQuickDrag::XAxis);
    inner->setX(invert ? 100 : 0);
    inner->setY(invert ? 100 : 0);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 200);
    int &axis = transpose ? position.ry() : position.rx();

    // drag toward the aligned boundary.  Outer mouse area dragged.
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    axis += invert ? threshold * 2 : -threshold * 2;
    QTest::mouseMove(&view, position);
    axis += invert ? threshold : -threshold;
    QTest::mouseMove(&view, position);
    QCOMPARE(outer->drag()->active(), true);
    QCOMPARE(inner->drag()->active(), false);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!outer->drag()->active());

    axis = 200;
    outer->setX(50);
    outer->setY(50);

    // drag away from the aligned boundary.  Inner mouse area dragged.
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    axis += invert ? -threshold * 2 : threshold * 2;
    QTest::mouseMove(&view, position);
    axis += invert ? -threshold : threshold;
    QTest::mouseMove(&view, position);
    QTRY_COMPARE(outer->drag()->active(), false);
    QTRY_COMPARE(inner->drag()->active(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);
}

void tst_QQuickMouseArea::nestedFlickableStopAtBounds()
{
    QQuickView view;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(view, testFileUrl("nestedFlickableStopAtBounds.qml"), false, &errorMessage), errorMessage.constData());
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject());

    QQuickMouseArea *mouseArea =  view.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea);

    QQuickFlickable *flickable = mouseArea->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 280);
    int &pos = position.ry();

    // Drag up - should move the Flickable to end
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos -= threshold * 2;
    QTest::mouseMove(&view, position);
    pos -= threshold * 2;
    QTest::mouseMove(&view, position);
    QTest::qWait(10);
    pos -= 150;
    QTest::mouseMove(&view, position);
    QVERIFY(flickable->isDragging());
    QVERIFY(!mouseArea->drag()->active());
    QCOMPARE(flickable->isAtYEnd(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);

    QTRY_VERIFY(!flickable->isMoving());

    pos = 280;

    // Drag up again - should activate MouseArea drag
    QVERIFY(!mouseArea->drag()->active());
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos -= threshold * 2;
    QTest::mouseMove(&view, position);
    pos -= threshold * 2;
    QTest::mouseMove(&view, position);
    QTest::qWait(10);
    pos -= 20;
    QTest::mouseMove(&view, position);
    QVERIFY(mouseArea->drag()->active());
    QCOMPARE(flickable->isAtYEnd(), true);
    QVERIFY(!flickable->isDragging());
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);

    // Drag to the top and verify that the MouseArea doesn't steal the grab when we drag back (QTBUG-56036)
    pos = 50;

    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos += threshold;
    QTest::mouseMove(&view, position);
    pos += threshold;
    QTest::mouseMove(&view, position);
    QTest::qWait(10);
    pos += 150;
    QTest::mouseMove(&view, position);
    QVERIFY(flickable->isDragging());
    QVERIFY(!mouseArea->drag()->active());
    QCOMPARE(flickable->isAtYBeginning(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);

    QTRY_VERIFY(!flickable->isMoving());

    pos = 280;

    // Drag up again - should not activate MouseArea drag
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos -= threshold;
    QTest::mouseMove(&view, position);
    pos -= threshold;
    QTest::mouseMove(&view, position);
    QTest::qWait(10);
    pos -= 100;
    QTest::mouseMove(&view, position);
    QVERIFY(flickable->isDragging());
    QVERIFY(!mouseArea->drag()->active());
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);
}

void tst_QQuickMouseArea::containsPress_data()
{
    QTest::addColumn<bool>("hoverEnabled");

    QTest::newRow("hover enabled") << true;
    QTest::newRow("hover disaabled") << false;
}

void tst_QQuickMouseArea::containsPress()
{
    QFETCH(bool, hoverEnabled);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("containsPress.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy containsPressSpy(mouseArea, SIGNAL(containsPressChanged()));

    mouseArea->setHoverEnabled(hoverEnabled);

    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QCOMPARE(mouseArea->pressed(), false);
    QCOMPARE(mouseArea->containsPress(), false);

    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), hoverEnabled);
    QCOMPARE(mouseArea->pressed(), false);
    QCOMPARE(mouseArea->containsPress(), false);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTRY_COMPARE(mouseArea->pressed(), true);
    QCOMPARE(mouseArea->containsPress(), true);
    QCOMPARE(containsPressSpy.count(), 1);

    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QCOMPARE(mouseArea->pressed(), true);
    QCOMPARE(mouseArea->containsPress(), false);
    QCOMPARE(containsPressSpy.count(), 2);

    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QCOMPARE(mouseArea->pressed(), true);
    QCOMPARE(mouseArea->containsPress(), true);
    QCOMPARE(containsPressSpy.count(), 3);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), hoverEnabled);
    QCOMPARE(mouseArea->pressed(), false);
    QCOMPARE(mouseArea->containsPress(), false);
    QCOMPARE(containsPressSpy.count(), 4);
}

void tst_QQuickMouseArea::ignoreBySource()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("ignoreBySource.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(window.rootObject());

    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>("mousearea");
    QVERIFY(mouseArea);

    QQuickFlickable *flickable = root->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    // MouseArea should grab the press because it's interested in non-synthesized mouse events
    QPoint p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(window.mouseGrabberItem(), mouseArea);
    // That was a real mouse event
    QCOMPARE(root->property("lastEventSource").toInt(), int(Qt::MouseEventNotSynthesized));

    // Flickable content should not move
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(window.mouseGrabberItem(), nullptr);

    // Now try touch events and confirm that MouseArea ignores them, while Flickable does its thing
    p = QPoint(80, 80);
    QTest::touchEvent(&window, device).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.mouseGrabberItem(), flickable);

    // That was a fake mouse event
    QCOMPARE(root->property("lastEventSource").toInt(), int(Qt::MouseEventSynthesizedByQt));
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);

    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.mouseGrabberItem(), flickable);
    QTest::touchEvent(&window, device).release(0, p, &window);
    QQuickTouchUtils::flush(&window);

    // Flickable content should have moved
    QTRY_VERIFY(flickable->contentX() > 1);
    QVERIFY(flickable->contentY() > 1);


    // Now tell the MouseArea to accept only synthesized events, and repeat the tests
    root->setProperty("allowedSource", Qt::MouseEventSynthesizedByQt);
    flickable->setContentX(0);
    flickable->setContentY(0);


    // MouseArea should ignore the press because it's interested in synthesized mouse events
    p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QVERIFY(window.mouseGrabberItem() != mouseArea);
    // That was a real mouse event
    QVERIFY(root->property("lastEventSource").toInt() == Qt::MouseEventNotSynthesized);

    // Flickable content should move
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(flickable->contentX() > 1);
    QVERIFY(flickable->contentY() > 1);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(47, 47));
    flickable->setContentX(0);
    flickable->setContentY(0);

    // Now try touch events and confirm that MouseArea gets them, while Flickable doesn't
    p = QPoint(80, 80);
    QTest::touchEvent(&window, device).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.mouseGrabberItem(), mouseArea);
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.mouseGrabberItem(), mouseArea);
    QTest::touchEvent(&window, device).release(0, QPoint(47,47), &window);
    QQuickTouchUtils::flush(&window);

    // Flickable content should not have moved
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);
}

void tst_QQuickMouseArea::notPressedAfterStolenGrab()
{
    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickMouseArea *ma = new QQuickMouseArea(window.contentItem());
    ma->setSize(window.size());
    QObject::connect(ma,
                     static_cast<void (QQuickMouseArea::*)(QQuickMouseEvent*)>(&QQuickMouseArea::pressed),
                     [&]() { window.contentItem()->grabMouse(); });

    QTest::mouseClick(&window, Qt::LeftButton);
    QVERIFY(!ma->pressed());
}

void tst_QQuickMouseArea::pressAndHold_data()
{
    QTest::addColumn<int>("pressAndHoldInterval");
    QTest::addColumn<int>("waitTime");

    QTest::newRow("default") << -1 << QGuiApplication::styleHints()->mousePressAndHoldInterval();
    QTest::newRow("short") << 500 << 500;
    QTest::newRow("long") << 1000 << 1000;
}

void tst_QQuickMouseArea::pressAndHold()
{
    QFETCH(int, pressAndHoldInterval);
    QFETCH(int, waitTime);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("pressAndHold.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy pressAndHoldSpy(mouseArea, &QQuickMouseArea::pressAndHold);

    if (pressAndHoldInterval > -1)
        mouseArea->setPressAndHoldInterval(pressAndHoldInterval);
    else
        mouseArea->resetPressAndHoldInterval();

    QElapsedTimer t;
    t.start();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QVERIFY(pressAndHoldSpy.wait());
    // should be off by no more than 20% of waitTime
    QVERIFY(qAbs(t.elapsed() - waitTime) < (waitTime * 0.2));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
}

void tst_QQuickMouseArea::pressOneAndTapAnother_data()
{
    QTest::addColumn<bool>("pressMouseFirst");
    QTest::addColumn<bool>("releaseMouseFirst");

    QTest::newRow("press mouse, tap touch, release mouse") << true << false; // QTBUG-64249 as written
    QTest::newRow("press touch, press mouse, release touch, release mouse") << false << false;
    QTest::newRow("press mouse, press touch, release mouse, release touch") << true << true;
    // TODO fix in a separate patch after the 5.9->5.10 merge
    // QTest::newRow("press touch, click mouse, release touch") << false << true;
}

void tst_QQuickMouseArea::pressOneAndTapAnother()
{
    QFETCH(bool, pressMouseFirst);
    QFETCH(bool, releaseMouseFirst);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("twoMouseAreas.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);
    QQuickMouseArea *bottomMA = window.rootObject()->findChild<QQuickMouseArea*>("bottom");
    QVERIFY(bottomMA);
    QQuickMouseArea *topMA = window.rootObject()->findChild<QQuickMouseArea*>("top");
    QVERIFY(topMA);

    QPoint upper(32, 32);
    QPoint lower(32, window.height() - 32);

    // press them both
    if (pressMouseFirst) {
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->pressed(), true);

        QTest::touchEvent(&window, device).press(0, lower, &window);
        QQuickTouchUtils::flush(&window);
        QTRY_COMPARE(bottomMA->pressed(), true);
    } else {
        QTest::touchEvent(&window, device).press(0, lower, &window);
        QQuickTouchUtils::flush(&window);
        QTRY_COMPARE(bottomMA->pressed(), true);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->pressed(), true);
    }

    // release them both and make sure neither one gets stuck
    if (releaseMouseFirst) {
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->pressed(), false);

        QTest::touchEvent(&window, device).release(0, upper, &window);
        QQuickTouchUtils::flush(&window);
        QTRY_COMPARE(topMA->pressed(), false);
    } else {
        QTest::touchEvent(&window, device).release(0, upper, &window);
        QQuickTouchUtils::flush(&window);

        QTRY_COMPARE(topMA->pressed(), false);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->pressed(), false);
    }
}

void tst_QQuickMouseArea::mask()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("mask.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickItem *root = window.rootObject();
    QVERIFY(root != nullptr);

    // click inside the mask, and verify it registers
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);

    // click outside the mask (but inside the MouseArea), and verify it doesn't register
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);
}

void tst_QQuickMouseArea::nestedEventDelivery() // QTBUG-70898
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nestedSendEvent.qml"));
    QScopedPointer<QQuickWindow> window(qmlobject_cast<QQuickWindow *>(c.create()));
    QVERIFY(window.data());

    // Click each MouseArea and verify that it doesn't crash
    QByteArray message = "event went missing during delivery! (nested sendEvent() is not allowed)";
    QTest::ignoreMessage(QtWarningMsg, message);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QTest::ignoreMessage(QtWarningMsg, message); // twice though, actually
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(50,150));
}

void tst_QQuickMouseArea::settingHiddenInPressUngrabs()
{
    // When an item sets itself hidden, while handling pressed, it doesn't receive the grab.
    // But that in turn means it doesn't see any release events, so we need to make sure it
    // receives an ungrab event.

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("settingHiddenInPressUngrabs.qml"));
    QScopedPointer<QQuickWindow> window(qmlobject_cast<QQuickWindow *>(c.create()));
    QVERIFY(window.data());
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickMouseArea *catArea = window->findChild<QQuickMouseArea*>("cat");
    QVERIFY(catArea != nullptr);
    auto pointOnCatArea = catArea->mapToScene(QPointF(5.0, 5.0)).toPoint();
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, pointOnCatArea);

    QCoreApplication::processEvents();
    // The click hides the cat area
    QTRY_VERIFY(!catArea->isVisible());
    // The cat area is not stuck in pressed state.
    QVERIFY(!catArea->pressed());

    QQuickMouseArea *mouseArea = window->findChild<QQuickMouseArea*>("mouse");
    QVERIFY(mouseArea != nullptr);
    auto pointOnMouseArea = mouseArea->mapToScene(QPointF(5.0, 5.0)).toPoint();
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, pointOnMouseArea);

    QCoreApplication::processEvents();
    // The click disables the mouse area
    QTRY_VERIFY(!mouseArea->isEnabled());
    // The mouse area is not stuck in pressed state.
    QVERIFY(!mouseArea->pressed());
}

QTEST_MAIN(tst_QQuickMouseArea)

#include "tst_qquickmousearea.moc"
