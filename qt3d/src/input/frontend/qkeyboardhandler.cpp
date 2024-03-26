// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qkeyboardhandler.h"
#include "qkeyboardhandler_p.h"

#include <Qt3DInput/qkeyboarddevice.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DInput {

namespace {


// SigMap and the sigMap table are taken from QQ2 QQuickKeysAttached
struct SigMap {
    int key;
    const char *sig;
};

const SigMap sigMap[] = {
    { Qt::Key_Left, "leftPressed" },
    { Qt::Key_Right, "rightPressed" },
    { Qt::Key_Up, "upPressed" },
    { Qt::Key_Down, "downPressed" },
    { Qt::Key_Tab, "tabPressed" },
    { Qt::Key_Backtab, "backtabPressed" },
    { Qt::Key_Asterisk, "asteriskPressed" },
    { Qt::Key_NumberSign, "numberSignPressed" },
    { Qt::Key_Escape, "escapePressed" },
    { Qt::Key_Return, "returnPressed" },
    { Qt::Key_Enter, "enterPressed" },
    { Qt::Key_Delete, "deletePressed" },
    { Qt::Key_Space, "spacePressed" },
    { Qt::Key_Back, "backPressed" },
    { Qt::Key_Cancel, "cancelPressed" },
    { Qt::Key_Select, "selectPressed" },
    { Qt::Key_Yes, "yesPressed" },
    { Qt::Key_No, "noPressed" },
    { Qt::Key_Context1, "context1Pressed" },
    { Qt::Key_Context2, "context2Pressed" },
    { Qt::Key_Context3, "context3Pressed" },
    { Qt::Key_Context4, "context4Pressed" },
    { Qt::Key_Call, "callPressed" },
    { Qt::Key_Hangup, "hangupPressed" },
    { Qt::Key_Flip, "flipPressed" },
    { Qt::Key_Menu, "menuPressed" },
    { Qt::Key_VolumeUp, "volumeUpPressed" },
    { Qt::Key_VolumeDown, "volumeDownPressed" },
    { 0, 0 }
};

const QByteArray keyToSignal(int key)
{
    QByteArray keySignal;
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        keySignal = "digit0Pressed";
        keySignal[5] = '0' + (key - Qt::Key_0);
    } else {
        int i = 0;
        while (sigMap[i].key && sigMap[i].key != key)
            ++i;
        keySignal = sigMap[i].sig;
    }
    return keySignal;
}

} // anonymous

QKeyboardHandlerPrivate::QKeyboardHandlerPrivate()
    : QComponentPrivate()
    , m_keyboardDevice(nullptr)
    , m_focus(false)
{
    m_shareable = false;
}

QKeyboardHandlerPrivate::~QKeyboardHandlerPrivate()
{
}

void QKeyboardHandlerPrivate::keyEvent(QKeyEvent *event)
{
    Q_Q(QKeyboardHandler);
    if (event->type() == QEvent::KeyPress) {
        emit q->pressed(event);

        QByteArray keySignal = keyToSignal(event->key());
        if (!keySignal.isEmpty()) {
            keySignal += "(Qt3DInput::QKeyEvent*)";
            // TO DO: Finding if the signal is connected to anything before doing the invocation
            // could be an improvement
            // That's what QQ2 does but since it accesses QML private classes to do so, that may not be
            // applicable in our case
            int idx = QKeyboardHandler::staticMetaObject.indexOfSignal(keySignal);
            q->metaObject()->method(idx).invoke(q, Qt::DirectConnection, Q_ARG(QKeyEvent*, event));
        }
    } else if (event->type() == QEvent::KeyRelease) {
        emit q->released(event);
    }
}


/*!
    \class Qt3DInput::QKeyboardHandler
    \inmodule Qt3DInput
    \brief Provides keyboard event notification.
    \since 5.5
*/

/*!
    \qmltype KeyboardHandler
    \inqmlmodule Qt3D.Input
    \instantiates Qt3DInput::QKeyboardHandler
    \inherits Component3D
    \brief QML frontend for QKeyboardHandler C++ class.
    \since 5.5
*/

/*!
    Constructs a new QKeyboardHandler instance with parent \a parent.
 */
QKeyboardHandler::QKeyboardHandler(QNode *parent)
    : QComponent(*new QKeyboardHandlerPrivate, parent)
{
}

/*! \internal */
QKeyboardHandler::~QKeyboardHandler()
{
}

/*!
    \qmlproperty KeyboardDevice Qt3D.Input::KeyboardHandler::sourceDevice
*/

/*!
    \property Qt3DInput::QKeyboardHandler::sourceDevice

    Holds the keyboard device of the QKeyboardHandler. Without a valid device,
    the QKeyboardHandler won't receive any event.
 */
void QKeyboardHandler::setSourceDevice(QKeyboardDevice *keyboardDevice)
{
    Q_D(QKeyboardHandler);
    if (d->m_keyboardDevice != keyboardDevice) {

        if (d->m_keyboardDevice)
            d->unregisterDestructionHelper(d->m_keyboardDevice);

        if (keyboardDevice && !keyboardDevice->parent())
            keyboardDevice->setParent(this);

        d->m_keyboardDevice = keyboardDevice;

        // Ensures proper bookkeeping
        if (d->m_keyboardDevice)
            d->registerDestructionHelper(keyboardDevice, &QKeyboardHandler::setSourceDevice, d->m_keyboardDevice);

        emit sourceDeviceChanged(keyboardDevice);
    }
}

/*!
    Returns the current keyboard device.
 */
QKeyboardDevice *QKeyboardHandler::sourceDevice() const
{
    Q_D(const QKeyboardHandler);
    return d->m_keyboardDevice;
}

/*!
    \qmlproperty bool Qt3D.Input::KeyboardHandler::focus
*/

/*!
    \property Qt3DInput::QKeyboardHandler::focus

    Holds \c true if the QKeyboardHandlers has focus.
 */
bool QKeyboardHandler::focus() const
{
    Q_D(const QKeyboardHandler);
    return d->m_focus;
}

/*!
    Sets the focus to \a focus. If focus is not currently set to \c true,
    this component will receive keyboard focus.
 */
void QKeyboardHandler::setFocus(bool focus)
{
    Q_D(QKeyboardHandler);
    if (d->m_focus != focus) {
        d->m_focus = focus;
        emit focusChanged(focus);
    }
}

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit0Pressed(KeyEvent event)

    This signal is emitted when the 0 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit1Pressed(KeyEvent event)

    This signal is emitted when the 1 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit2Pressed(KeyEvent event)

    This signal is emitted when the 2 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit3Pressed(KeyEvent event)

    This signal is emitted when the 3 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit4Pressed(KeyEvent event)

    This signal is emitted when the 4 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit5Pressed(KeyEvent event)

    This signal is emitted when the 5 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit6Pressed(KeyEvent event)

    This signal is emitted when the 6 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit7Pressed(KeyEvent event)

    This signal is emitted when the 7 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit8Pressed(KeyEvent event)

    This signal is emitted when the 8 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::digit9Pressed(KeyEvent event)

    This signal is emitted when the 9 key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::leftPressed(KeyEvent event)

    This signal is emitted when the left key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::rightPressed(KeyEvent event)

    This signal is emitted when the right key is pressed with the event details being contained within \a event
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::upPressed(KeyEvent event)

    This signal is emitted when the up key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::downPressed(KeyEvent event)

    This signal is emitted when the down key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::tabPressed(KeyEvent event)

    This signal is emitted when the tab key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::backtabPressed(KeyEvent event)

    This signal is emitted when the backtab key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::asteriskPressed(KeyEvent event)

    This signal is emitted when the * key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::numberSignPressed(KeyEvent event)

    This signal is emitted when the number sign key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::escapePressed(KeyEvent event)

    This signal is emitted when the escape key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::returnPressed(KeyEvent event)

    This signal is emitted when the return key is pressed with the event details being contained within \a event.

*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::enterPressed(KeyEvent event)

    This signal is emitted when the enter key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::deletePressed(KeyEvent event)

    This signal is emitted when the delete key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::spacePressed(KeyEvent event)

    This signal is emitted when the space key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::backPressed(KeyEvent event)

    This signal is emitted when the back key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::cancelPressed(KeyEvent event)

    This signal is emitted when the cancel key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::selectPressed(KeyEvent event)

    This signal is emitted when the select key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::yesPressed(KeyEvent event)

    This signal is emitted when the yes key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::noPressed(KeyEvent event)

    This signal is emitted when the yes key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::context1Pressed(KeyEvent event)

    This signal is emitted when the context 1  key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::context2Pressed(KeyEvent event)

    This signal is emitted when the context 2  key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::context3Pressed(KeyEvent event)

    This signal is emitted when the context 2  key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::context4Pressed(KeyEvent event)

    This signal is emitted when the context 4  key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::callPressed(KeyEvent event)

    This signal is emitted when the call  key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::hangupPressed(KeyEvent event)

    This signal is emitted when the hangup  key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::flipPressed(KeyEvent event)

    This signal is emitted when the flip key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::menuPressed(KeyEvent event)

    This signal is emitted when the menu key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::volumeUpPressed(KeyEvent event)

    This signal is emitted when the volume up key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::volumeDownPressed(KeyEvent event)

    This signal is emitted when the volume down key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::pressed(KeyEvent event)

    This signal is emitted when a key is pressed with the event details being contained within \a event.
*/

/*!
    \qmlsignal Qt3D.Input::KeyboardHandler::released(KeyEvent event)

    This signal is emitted when a key is released with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit0Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 0 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit1Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 1 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit2Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 2 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit3Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 3 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit4Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 4 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit5Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 5 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit6Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 6 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit7Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 7 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit8Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 8 key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::digit9Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the 9 key is pressed with the event details being contained within \a event
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::leftPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the left key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::rightPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the right key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::upPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the up key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::downPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the down key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::tabPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the tab key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::backtabPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the backtab key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::asteriskPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the * key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::numberSignPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the number sign key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::escapePressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the escape key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::returnPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the return key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::enterPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the enter key is pressed with the event details being contained within \a event.

*/

/*!
    \fn Qt3DInput::QKeyboardHandler::deletePressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the delete key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::spacePressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the space key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::backPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the back key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::cancelPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the cancel key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::selectPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the select key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::yesPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the yes key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::noPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the yes key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::context1Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the context 1  key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::context2Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the context 2  key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::context3Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the context 2  key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::context4Pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the context 4  key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::callPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the call  key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::hangupPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the hangup  key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::flipPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the flip key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::menuPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the menu key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::volumeUpPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the volume up key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::volumeDownPressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when the volume down key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::pressed(Qt3DInput::QKeyEvent *event)

    This signal is emitted when a key is pressed with the event details being contained within \a event.
*/

/*!
    \fn Qt3DInput::QKeyboardHandler::released(Qt3DInput::QKeyEvent *event)

    This signal is emitted when a key is released with the event details being contained within \a event.
*/
} // namespace Qt3DInput

QT_END_NAMESPACE

#include "moc_qkeyboardhandler.cpp"
