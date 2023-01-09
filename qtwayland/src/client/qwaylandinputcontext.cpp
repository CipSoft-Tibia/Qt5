/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandClient module of the Qt Toolkit.
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


#include "qwaylandinputcontext_p.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QTextCharFormat>
#include <QtGui/QWindow>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandinputmethodeventbuilder_p.h"
#include "qwaylandwindow_p.h"

#if QT_CONFIG(xkbcommon)
#include <locale.h>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcQpaInputMethods, "qt.qpa.input.methods")

namespace QtWaylandClient {

namespace {
const Qt::InputMethodQueries supportedQueries = Qt::ImEnabled |
                                                Qt::ImSurroundingText |
                                                Qt::ImCursorPosition |
                                                Qt::ImAnchorPosition |
                                                Qt::ImHints |
                                                Qt::ImCursorRectangle |
                                                Qt::ImPreferredLanguage;
}

QWaylandTextInput::QWaylandTextInput(QWaylandDisplay *display, struct ::zwp_text_input_v2 *text_input)
    : QtWayland::zwp_text_input_v2(text_input)
    , m_display(display)
{
}

QWaylandTextInput::~QWaylandTextInput()
{
    if (m_resetCallback)
        wl_callback_destroy(m_resetCallback);
}

void QWaylandTextInput::reset()
{
    m_builder.reset();
    m_preeditCommit = QString();
    updateState(Qt::ImQueryAll, update_state_reset);
}

void QWaylandTextInput::commit()
{
    if (QObject *o = QGuiApplication::focusObject()) {
        QInputMethodEvent event;
        event.setCommitString(m_preeditCommit);
        QCoreApplication::sendEvent(o, &event);
    }

    reset();
}

const wl_callback_listener QWaylandTextInput::callbackListener = {
    QWaylandTextInput::resetCallback
};

void QWaylandTextInput::resetCallback(void *data, wl_callback *, uint32_t)
{
    QWaylandTextInput *self = static_cast<QWaylandTextInput*>(data);

    if (self->m_resetCallback) {
        wl_callback_destroy(self->m_resetCallback);
        self->m_resetCallback = nullptr;
    }
}

void QWaylandTextInput::updateState(Qt::InputMethodQueries queries, uint32_t flags)
{
    if (!QGuiApplication::focusObject())
        return;

    if (!QGuiApplication::focusWindow() || !QGuiApplication::focusWindow()->handle())
        return;

    auto *window = static_cast<QWaylandWindow *>(QGuiApplication::focusWindow()->handle());
    auto *surface = window->wlSurface();
    if (!surface || (surface != m_surface))
        return;

    queries &= supportedQueries;

    // Surrounding text, cursor and anchor positions are transferred together
    if ((queries & Qt::ImSurroundingText) || (queries & Qt::ImCursorPosition) || (queries & Qt::ImAnchorPosition))
        queries |= Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition;

    QInputMethodQueryEvent event(queries);
    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);

    if ((queries & Qt::ImSurroundingText) || (queries & Qt::ImCursorPosition) || (queries & Qt::ImAnchorPosition)) {
        QString text = event.value(Qt::ImSurroundingText).toString();
        int cursor = event.value(Qt::ImCursorPosition).toInt();
        int anchor = event.value(Qt::ImAnchorPosition).toInt();

        // Make sure text is not too big
        if (text.toUtf8().size() > 2048) {
            int c = qAbs(cursor - anchor) <= 512 ? qMin(cursor, anchor) + qAbs(cursor - anchor) / 2: cursor;

            const int offset = c - qBound(0, c, 512 - qMin(text.size() - c, 256));
            text = text.mid(offset + c - 256, 512);
            cursor -= offset;
            anchor -= offset;
        }

        set_surrounding_text(text, QWaylandInputMethodEventBuilder::indexToWayland(text, cursor), QWaylandInputMethodEventBuilder::indexToWayland(text, anchor));
    }

    if (queries & Qt::ImHints) {
        QWaylandInputMethodContentType contentType = QWaylandInputMethodContentType::convert(static_cast<Qt::InputMethodHints>(event.value(Qt::ImHints).toInt()));
        set_content_type(contentType.hint, contentType.purpose);
    }

    if (queries & Qt::ImCursorRectangle) {
        const QRect &cRect = event.value(Qt::ImCursorRectangle).toRect();
        const QRect &windowRect = QGuiApplication::inputMethod()->inputItemTransform().mapRect(cRect);
        const QMargins margins = window->frameMargins();
        const QRect &surfaceRect = windowRect.translated(margins.left(), margins.top());
        set_cursor_rectangle(surfaceRect.x(), surfaceRect.y(), surfaceRect.width(), surfaceRect.height());
    }

    if (queries & Qt::ImPreferredLanguage) {
        const QString &language = event.value(Qt::ImPreferredLanguage).toString();
        set_preferred_language(language);
    }

    update_state(m_serial, flags);
    if (flags != update_state_change) {
        if (m_resetCallback)
            wl_callback_destroy(m_resetCallback);
        m_resetCallback = wl_display_sync(m_display->wl_display());
        wl_callback_add_listener(m_resetCallback, &QWaylandTextInput::callbackListener, this);
    }
}

void QWaylandTextInput::setCursorInsidePreedit(int)
{
    // Not supported yet
}

bool QWaylandTextInput::isInputPanelVisible() const
{
    return m_inputPanelVisible;
}

QRectF QWaylandTextInput::keyboardRect() const
{
    return m_keyboardRectangle;
}

QLocale QWaylandTextInput::locale() const
{
    return m_locale;
}

Qt::LayoutDirection QWaylandTextInput::inputDirection() const
{
    return m_inputDirection;
}

void QWaylandTextInput::zwp_text_input_v2_enter(uint32_t serial, ::wl_surface *surface)
{
    m_serial = serial;
    m_surface = surface;

    updateState(Qt::ImQueryAll, update_state_enter);
}

void QWaylandTextInput::zwp_text_input_v2_leave(uint32_t serial, ::wl_surface *surface)
{
    m_serial = serial;

    if (m_surface != surface) {
        qCDebug(qLcQpaInputMethods()) << Q_FUNC_INFO << "Got leave event for surface" << surface << "focused surface" << m_surface;
    }

    m_surface = nullptr;
}

void QWaylandTextInput::zwp_text_input_v2_modifiers_map(wl_array *map)
{
    const QList<QByteArray> modifiersMap = QByteArray::fromRawData(static_cast<const char*>(map->data), map->size).split('\0');

    m_modifiersMap.clear();

    for (const QByteArray &modifier : modifiersMap) {
        if (modifier == "Shift")
            m_modifiersMap.append(Qt::ShiftModifier);
        else if (modifier == "Control")
            m_modifiersMap.append(Qt::ControlModifier);
        else if (modifier == "Alt")
            m_modifiersMap.append(Qt::AltModifier);
        else if (modifier == "Mod1")
            m_modifiersMap.append(Qt::AltModifier);
        else if (modifier == "Mod4")
            m_modifiersMap.append(Qt::MetaModifier);
        else
            m_modifiersMap.append(Qt::NoModifier);
    }
}

void QWaylandTextInput::zwp_text_input_v2_input_panel_state(uint32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
    const bool inputPanelVisible = (visible == input_panel_visibility_visible);
    if (m_inputPanelVisible != inputPanelVisible) {
        m_inputPanelVisible = inputPanelVisible;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitInputPanelVisibleChanged();
    }
    const QRectF keyboardRectangle(x, y, width, height);
    if (m_keyboardRectangle != keyboardRectangle) {
        m_keyboardRectangle = keyboardRectangle;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitKeyboardRectChanged();
    }
}

void QWaylandTextInput::zwp_text_input_v2_preedit_string(const QString &text, const QString &commit)
{
    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard preedit_string: reset not confirmed";
        m_builder.reset();
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    QInputMethodEvent event = m_builder.buildPreedit(text);

    m_builder.reset();
    m_preeditCommit = commit;

    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
}

void QWaylandTextInput::zwp_text_input_v2_preedit_styling(uint32_t index, uint32_t length, uint32_t style)
{
    m_builder.addPreeditStyling(index, length, style);
}

void QWaylandTextInput::zwp_text_input_v2_preedit_cursor(int32_t index)
{
    m_builder.setPreeditCursor(index);
}

void QWaylandTextInput::zwp_text_input_v2_commit_string(const QString &text)
{
    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard commit_string: reset not confirmed";
        m_builder.reset();
        return;
    }

    if (!QGuiApplication::focusObject())
        return;

    QInputMethodEvent event = m_builder.buildCommit(text);

    m_builder.reset();

    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
}

void QWaylandTextInput::zwp_text_input_v2_cursor_position(int32_t index, int32_t anchor)
{
    m_builder.setCursorPosition(index, anchor);
}

void QWaylandTextInput::zwp_text_input_v2_delete_surrounding_text(uint32_t before_length, uint32_t after_length)
{
    m_builder.setDeleteSurroundingText(before_length, after_length);
}

void QWaylandTextInput::zwp_text_input_v2_keysym(uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers)
{
#if QT_CONFIG(xkbcommon)
    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard keysym: reset not confirmed";
        return;
    }

    if (!QGuiApplication::focusWindow())
        return;

    Qt::KeyboardModifiers qtModifiers = modifiersToQtModifiers(modifiers);

    QEvent::Type type = state == WL_KEYBOARD_KEY_STATE_PRESSED ? QEvent::KeyPress : QEvent::KeyRelease;
    QString text = QXkbCommon::lookupStringNoKeysymTransformations(sym);
    int qtkey = QXkbCommon::keysymToQtKey(sym, qtModifiers);

    QWindowSystemInterface::handleKeyEvent(QGuiApplication::focusWindow(),
                                           time, type, qtkey, qtModifiers, text);
#else
    Q_UNUSED(time);
    Q_UNUSED(sym);
    Q_UNUSED(state);
    Q_UNUSED(modifiers);
#endif
}

void QWaylandTextInput::zwp_text_input_v2_language(const QString &language)
{
    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard language: reset not confirmed";
        return;
    }

    const QLocale locale(language);
    if (m_locale != locale) {
        m_locale = locale;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitLocaleChanged();
    }
}

void QWaylandTextInput::zwp_text_input_v2_text_direction(uint32_t direction)
{
    if (m_resetCallback) {
        qCDebug(qLcQpaInputMethods()) << "discard text_direction: reset not confirmed";
        return;
    }

    const Qt::LayoutDirection inputDirection = (direction == text_direction_auto) ? Qt::LayoutDirectionAuto :
                                               (direction == text_direction_ltr) ? Qt::LeftToRight :
                                               (direction == text_direction_rtl) ? Qt::RightToLeft : Qt::LayoutDirectionAuto;
    if (m_inputDirection != inputDirection) {
        m_inputDirection = inputDirection;
        QGuiApplicationPrivate::platformIntegration()->inputContext()->emitInputDirectionChanged(m_inputDirection);
    }
}

void QWaylandTextInput::zwp_text_input_v2_input_method_changed(uint32_t serial, uint32_t flags)
{
    Q_UNUSED(flags);

    m_serial = serial;
    updateState(Qt::ImQueryAll, update_state_full);
}

Qt::KeyboardModifiers QWaylandTextInput::modifiersToQtModifiers(uint32_t modifiers)
{
    Qt::KeyboardModifiers ret = Qt::NoModifier;
    for (int i = 0; i < m_modifiersMap.size(); ++i) {
        if (modifiers & (1 << i)) {
            ret |= m_modifiersMap[i];
        }
    }
    return ret;
}

QWaylandInputContext::QWaylandInputContext(QWaylandDisplay *display)
    : mDisplay(display)
{
}

QWaylandInputContext::~QWaylandInputContext()
{
}

bool QWaylandInputContext::isValid() const
{
    return mDisplay->textInputManager() != nullptr;
}

void QWaylandInputContext::reset()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;
#if QT_CONFIG(xkbcommon)
    if (m_composeState)
        xkb_compose_state_reset(m_composeState);
#endif

    QPlatformInputContext::reset();

    if (!textInput())
        return;

    textInput()->reset();
}

void QWaylandInputContext::commit()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return;

    textInput()->commit();
}

static ::wl_surface *surfaceForWindow(QWindow *window)
{
    if (!window || !window->handle())
        return nullptr;

    auto *waylandWindow = static_cast<QWaylandWindow *>(window->handle());
    return waylandWindow->wlSurface();
}

void QWaylandInputContext::update(Qt::InputMethodQueries queries)
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO << queries;

    if (!QGuiApplication::focusObject() || !textInput())
        return;

    auto *currentSurface = surfaceForWindow(mCurrentWindow);

    if (currentSurface && !inputMethodAccepted()) {
        textInput()->disable(currentSurface);
        mCurrentWindow.clear();
    } else if (!currentSurface && inputMethodAccepted()) {
        QWindow *window = QGuiApplication::focusWindow();
        if (auto *focusSurface = surfaceForWindow(window)) {
            textInput()->enable(focusSurface);
            mCurrentWindow = window;
        }
    }

    textInput()->updateState(queries, QtWayland::zwp_text_input_v2::update_state_change);
}

void QWaylandInputContext::invokeAction(QInputMethod::Action action, int cursorPostion)
{
    if (!textInput())
        return;

    if (action == QInputMethod::Click)
        textInput()->setCursorInsidePreedit(cursorPostion);
}

void QWaylandInputContext::showInputPanel()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return;

    textInput()->show_input_panel();
}

void QWaylandInputContext::hideInputPanel()
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return;

    textInput()->hide_input_panel();
}

bool QWaylandInputContext::isInputPanelVisible() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return QPlatformInputContext::isInputPanelVisible();

    return textInput()->isInputPanelVisible();
}

QRectF QWaylandInputContext::keyboardRect() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return QPlatformInputContext::keyboardRect();

    return textInput()->keyboardRect();
}

QLocale QWaylandInputContext::locale() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return QPlatformInputContext::locale();

    return textInput()->locale();
}

Qt::LayoutDirection QWaylandInputContext::inputDirection() const
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;

    if (!textInput())
        return QPlatformInputContext::inputDirection();

    return textInput()->inputDirection();
}

void QWaylandInputContext::setFocusObject(QObject *object)
{
    qCDebug(qLcQpaInputMethods) << Q_FUNC_INFO;
#if QT_CONFIG(xkbcommon)
    m_focusObject = object;
#else
    Q_UNUSED(object);
#endif

    if (!textInput())
        return;

    QWindow *window = QGuiApplication::focusWindow();

    if (mCurrentWindow && mCurrentWindow->handle()) {
        if (mCurrentWindow.data() != window || !inputMethodAccepted()) {
            auto *surface = static_cast<QWaylandWindow *>(mCurrentWindow->handle())->wlSurface();
            if (surface)
                textInput()->disable(surface);
            mCurrentWindow.clear();
        }
    }

    if (window && window->handle() && inputMethodAccepted()) {
        if (mCurrentWindow.data() != window) {
            auto *surface = static_cast<QWaylandWindow *>(window->handle())->wlSurface();
            if (surface) {
                textInput()->enable(surface);
                mCurrentWindow = window;
            }
        }
        textInput()->updateState(Qt::ImQueryAll, QtWayland::zwp_text_input_v2::update_state_enter);
    }
}

QWaylandTextInput *QWaylandInputContext::textInput() const
{
    return mDisplay->defaultInputDevice()->textInput();
}

#if QT_CONFIG(xkbcommon)

void QWaylandInputContext::ensureInitialized()
{
    if (m_initialized)
        return;

    if (!m_XkbContext) {
        qCWarning(qLcQpaInputMethods) << "error: xkb context has not been set on" << metaObject()->className();
        return;
    }

    m_initialized = true;
    const char *locale = setlocale(LC_CTYPE, "");
    if (!locale)
        locale = setlocale(LC_CTYPE, nullptr);
    qCDebug(qLcQpaInputMethods) << "detected locale (LC_CTYPE):" << locale;

    m_composeTable = xkb_compose_table_new_from_locale(m_XkbContext, locale, XKB_COMPOSE_COMPILE_NO_FLAGS);
    if (m_composeTable)
        m_composeState = xkb_compose_state_new(m_composeTable, XKB_COMPOSE_STATE_NO_FLAGS);

    if (!m_composeTable) {
        qCWarning(qLcQpaInputMethods, "failed to create compose table");
        return;
    }
    if (!m_composeState) {
        qCWarning(qLcQpaInputMethods, "failed to create compose state");
        return;
    }
}

bool QWaylandInputContext::filterEvent(const QEvent *event)
{
    auto keyEvent = static_cast<const QKeyEvent *>(event);
    if (keyEvent->type() != QEvent::KeyPress)
        return false;

    if (!inputMethodAccepted())
        return false;

    // lazy initialization - we don't want to do this on an app startup
    ensureInitialized();

    if (!m_composeTable || !m_composeState)
        return false;

    xkb_compose_state_feed(m_composeState, keyEvent->nativeVirtualKey());

    switch (xkb_compose_state_get_status(m_composeState)) {
    case XKB_COMPOSE_COMPOSING:
        return true;
    case XKB_COMPOSE_CANCELLED:
        reset();
        return false;
    case XKB_COMPOSE_COMPOSED:
    {
        const int size = xkb_compose_state_get_utf8(m_composeState, nullptr, 0);
        QVarLengthArray<char, 32> buffer(size + 1);
        xkb_compose_state_get_utf8(m_composeState, buffer.data(), buffer.size());
        QString composedText = QString::fromUtf8(buffer.constData());

        QInputMethodEvent event;
        event.setCommitString(composedText);

        if (!m_focusObject && qApp)
            m_focusObject = qApp->focusObject();

        if (m_focusObject)
            QCoreApplication::sendEvent(m_focusObject, &event);
        else
            qCWarning(qLcQpaInputMethods, "no focus object");

        reset();
        return true;
    }
    case XKB_COMPOSE_NOTHING:
        return false;
    default:
        Q_UNREACHABLE();
        return false;
    }
}

#endif

}

QT_END_NAMESPACE
