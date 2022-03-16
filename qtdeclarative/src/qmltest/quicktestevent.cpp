/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quicktestevent_p.h"
#include <QtTest/qtestkeyboard.h>
#include <QtQml/qqml.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

namespace QTest {
    extern int Q_TESTLIB_EXPORT defaultMouseDelay();
}

QuickTestEvent::QuickTestEvent(QObject *parent)
    : QObject(parent)
{
}

QuickTestEvent::~QuickTestEvent()
{
}

int QuickTestEvent::defaultMouseDelay() const
{
    return QTest::defaultMouseDelay();
}

bool QuickTestEvent::keyPress(int key, int modifiers, int delay)
{
    QWindow *window = activeWindow();
    if (!window)
        return false;
    QTest::keyPress(window, Qt::Key(key), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyRelease(int key, int modifiers, int delay)
{
    QWindow *window = activeWindow();
    if (!window)
        return false;
    QTest::keyRelease(window, Qt::Key(key), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyClick(int key, int modifiers, int delay)
{
    QWindow *window = activeWindow();
    if (!window)
        return false;
    QTest::keyClick(window, Qt::Key(key), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyPressChar(const QString &character, int modifiers, int delay)
{
    QTEST_ASSERT(character.length() == 1);
    QWindow *window = activeWindow();
    if (!window)
        return false;
    QTest::keyPress(window, character[0].toLatin1(), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyReleaseChar(const QString &character, int modifiers, int delay)
{
    QTEST_ASSERT(character.length() == 1);
    QWindow *window = activeWindow();
    if (!window)
        return false;
    QTest::keyRelease(window, character[0].toLatin1(), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

bool QuickTestEvent::keyClickChar(const QString &character, int modifiers, int delay)
{
    QTEST_ASSERT(character.length() == 1);
    QWindow *window = activeWindow();
    if (!window)
        return false;
    QTest::keyClick(window, character[0].toLatin1(), Qt::KeyboardModifiers(modifiers), delay);
    return true;
}

#if QT_CONFIG(shortcut)
// valueToKeySequence() is copied from qquickshortcut.cpp
static QKeySequence valueToKeySequence(const QVariant &value)
{
    if (value.type() == QVariant::Int)
        return QKeySequence(static_cast<QKeySequence::StandardKey>(value.toInt()));
    return QKeySequence::fromString(value.toString());
}
#endif

bool QuickTestEvent::keySequence(const QVariant &keySequence)
{
    QWindow *window = activeWindow();
    if (!window)
        return false;
#if QT_CONFIG(shortcut)
    QTest::keySequence(window, valueToKeySequence(keySequence));
#endif
    return true;
}

namespace QtQuickTest
{
    enum MouseAction { MousePress, MouseRelease, MouseClick, MouseDoubleClick, MouseMove, MouseDoubleClickSequence };

    int lastMouseTimestamp = 0;

    // TODO should be Qt::MouseButtons buttons in case multiple buttons are pressed
    static void mouseEvent(MouseAction action, QWindow *window,
                           QObject *item, Qt::MouseButton button,
                           Qt::KeyboardModifiers stateKey, const QPointF &_pos, int delay=-1)
    {
        QTEST_ASSERT(window);
        QTEST_ASSERT(item);

        if (delay == -1 || delay < QTest::defaultMouseDelay())
            delay = QTest::defaultMouseDelay();
        if (delay > 0) {
            QTest::qWait(delay);
            lastMouseTimestamp += delay;
        }

        if (action == MouseClick) {
            mouseEvent(MousePress, window, item, button, stateKey, _pos);
            mouseEvent(MouseRelease, window, item, button, stateKey, _pos);
            return;
        }

        if (action == MouseDoubleClickSequence) {
            mouseEvent(MousePress, window, item, button, stateKey, _pos);
            mouseEvent(MouseRelease, window, item, button, stateKey, _pos);
            mouseEvent(MousePress, window, item, button, stateKey, _pos);
            mouseEvent(MouseDoubleClick, window, item, button, stateKey, _pos);
            mouseEvent(MouseRelease, window, item, button, stateKey, _pos);
            return;
        }

        QPoint pos = _pos.toPoint();
        QQuickItem *sgitem = qobject_cast<QQuickItem *>(item);
        if (sgitem)
            pos = sgitem->mapToScene(_pos).toPoint();
        QTEST_ASSERT(button == Qt::NoButton || button & Qt::MouseButtonMask);
        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);

        QMouseEvent me(QEvent::User, QPoint(), Qt::LeftButton, button, stateKey);
        switch (action)
        {
            case MousePress:
                me = QMouseEvent(QEvent::MouseButtonPress, pos, window->mapToGlobal(pos), button, button, stateKey);
                me.setTimestamp(++lastMouseTimestamp);
                break;
            case MouseRelease:
                me = QMouseEvent(QEvent::MouseButtonRelease, pos, window->mapToGlobal(pos), button, nullptr, stateKey);
                me.setTimestamp(++lastMouseTimestamp);
                lastMouseTimestamp += 500; // avoid double clicks being generated
                break;
            case MouseDoubleClick:
                me = QMouseEvent(QEvent::MouseButtonDblClick, pos, window->mapToGlobal(pos), button, button, stateKey);
                me.setTimestamp(++lastMouseTimestamp);
                break;
            case MouseMove:
                // with move event the button is NoButton, but 'buttons' holds the currently pressed buttons
                me = QMouseEvent(QEvent::MouseMove, pos, window->mapToGlobal(pos), Qt::NoButton, button, stateKey);
                me.setTimestamp(++lastMouseTimestamp);
                break;
            default:
                QTEST_ASSERT(false);
        }
        QSpontaneKeyEvent::setSpontaneous(&me);
        if (!qApp->notify(window, &me)) {
            static const char *mouseActionNames[] =
                { "MousePress", "MouseRelease", "MouseClick", "MouseDoubleClick", "MouseMove", "MouseDoubleClickSequence" };
            QString warning = QString::fromLatin1("Mouse event \"%1\" not accepted by receiving window");
            QWARN(warning.arg(QString::fromLatin1(mouseActionNames[static_cast<int>(action)])).toLatin1().data());
        }
    }

#if QT_CONFIG(wheelevent)
    static void mouseWheel(QWindow* window, QObject* item, Qt::MouseButtons buttons,
                                Qt::KeyboardModifiers stateKey,
                                QPointF _pos, int xDelta, int yDelta, int delay = -1)
    {
        QTEST_ASSERT(window);
        QTEST_ASSERT(item);
        if (delay == -1 || delay < QTest::defaultMouseDelay())
            delay = QTest::defaultMouseDelay();
        if (delay > 0)
            QTest::qWait(delay);

        QPoint pos;
        QQuickItem *sgitem = qobject_cast<QQuickItem *>(item);
        if (sgitem)
            pos = sgitem->mapToScene(_pos).toPoint();

        QTEST_ASSERT(buttons == Qt::NoButton || buttons & Qt::MouseButtonMask);
        QTEST_ASSERT(stateKey == 0 || stateKey & Qt::KeyboardModifierMask);

        stateKey &= static_cast<unsigned int>(Qt::KeyboardModifierMask);
        QWheelEvent we(pos, window->mapToGlobal(pos), QPoint(0, 0), QPoint(xDelta, yDelta), 0, Qt::Vertical, buttons, stateKey);

        QSpontaneKeyEvent::setSpontaneous(&we); // hmmmm
        if (!qApp->notify(window, &we))
            QTest::qWarn("Wheel event not accepted by receiving window");
    }
#endif
};

bool QuickTestEvent::mousePress
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    m_pressedButtons.setFlag(Qt::MouseButton(button), true);
    QtQuickTest::mouseEvent(QtQuickTest::MousePress, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

#if QT_CONFIG(wheelevent)
bool QuickTestEvent::mouseWheel(
    QObject *item, qreal x, qreal y, int buttons,
    int modifiers, int xDelta, int yDelta, int delay)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    QtQuickTest::mouseWheel(view, item, Qt::MouseButtons(buttons),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), xDelta, yDelta, delay);
    return true;
}
#endif

bool QuickTestEvent::mouseRelease
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    m_pressedButtons.setFlag(Qt::MouseButton(button), false);
    QtQuickTest::mouseEvent(QtQuickTest::MouseRelease, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseClick
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseClick, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseDoubleClick
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseDoubleClick, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseDoubleClickSequence
    (QObject *item, qreal x, qreal y, int button,
     int modifiers, int delay)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    QtQuickTest::mouseEvent(QtQuickTest::MouseDoubleClickSequence, view, item,
                            Qt::MouseButton(button),
                            Qt::KeyboardModifiers(modifiers),
                            QPointF(x, y), delay);
    return true;
}

bool QuickTestEvent::mouseMove
    (QObject *item, qreal x, qreal y, int delay, int buttons)
{
    QWindow *view = eventWindow(item);
    if (!view)
        return false;
    const Qt::MouseButtons effectiveButtons = buttons ? Qt::MouseButtons(buttons) : m_pressedButtons;
    QtQuickTest::mouseEvent(QtQuickTest::MouseMove, view, item,
                            Qt::MouseButton(int(effectiveButtons)), Qt::NoModifier,
                            QPointF(x, y), delay);
    return true;
}

QWindow *QuickTestEvent::eventWindow(QObject *item)
{
    QWindow * window = qobject_cast<QWindow *>(item);
    if (window)
        return window;

    QQuickItem *quickItem = qobject_cast<QQuickItem *>(item);
    if (quickItem)
        return quickItem->window();

    QQuickItem *testParentitem = qobject_cast<QQuickItem *>(parent());
    if (testParentitem)
        return testParentitem->window();
    return nullptr;
}

QWindow *QuickTestEvent::activeWindow()
{
    if (QWindow *window = QGuiApplication::focusWindow())
        return window;
    return eventWindow();
}

QQuickTouchEventSequence::QQuickTouchEventSequence(QuickTestEvent *testEvent, QObject *item)
        : QObject(testEvent)
        , m_sequence(QTest::touchEvent(testEvent->eventWindow(item), testEvent->touchDevice()))
        , m_testEvent(testEvent)
{
}

QObject *QQuickTouchEventSequence::press(int touchId, QObject *item, qreal x, qreal y)
{
    QWindow *view = m_testEvent->eventWindow(item);
    if (view) {
        QPointF pos(x, y);
        QQuickItem *quickItem = qobject_cast<QQuickItem *>(item);
        if (quickItem) {
            pos = quickItem->mapToScene(pos);
        }
        m_sequence.press(touchId, pos.toPoint(), view);
    }
    return this;
}

QObject *QQuickTouchEventSequence::move(int touchId, QObject *item, qreal x, qreal y)
{
    QWindow *view = m_testEvent->eventWindow(item);
    if (view) {
        QPointF pos(x, y);
        QQuickItem *quickItem = qobject_cast<QQuickItem *>(item);
        if (quickItem) {
            pos = quickItem->mapToScene(pos);
        }
        m_sequence.move(touchId, pos.toPoint(), view);
    }
    return this;
}

QObject *QQuickTouchEventSequence::release(int touchId, QObject *item, qreal x, qreal y)
{
    QWindow *view = m_testEvent->eventWindow(item);
    if (view) {
        QPointF pos(x, y);
        QQuickItem *quickItem = qobject_cast<QQuickItem *>(item);
        if (quickItem) {
            pos = quickItem->mapToScene(pos);
        }
        m_sequence.release(touchId, pos.toPoint(), view);
    }
    return this;
}

QObject *QQuickTouchEventSequence::stationary(int touchId)
{
    m_sequence.stationary(touchId);
    return this;
}

QObject *QQuickTouchEventSequence::commit()
{
    m_sequence.commit();
    return this;
}

/*!
    Return a simulated touchscreen, creating one if necessary

    \internal
*/

QTouchDevice *QuickTestEvent::touchDevice()
{
    static QTouchDevice *device(nullptr);

    if (!device) {
        device = new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }
    return device;
}

/*!
    Creates a new QQuickTouchEventSequence.

    If valid, \a item determines the QWindow that touch events are sent to.
    Test code should use touchEvent() from the QML TestCase type.

    \internal
*/
QQuickTouchEventSequence *QuickTestEvent::touchEvent(QObject *item)
{
    return new QQuickTouchEventSequence(this, item);
}

QT_END_NAMESPACE
