/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "environment.h"
#include "qcontext2dcanvas.h"
#include "context2d.h"
#include <QScriptValueIterator>
#include <QDateTime>

struct FakeDomEvent
{
    enum KeyCodes  {
        DOM_VK_UNDEFINED            = 0x0,
        DOM_VK_RIGHT_ALT            = 0x12,
        DOM_VK_LEFT_ALT             = 0x12,
        DOM_VK_LEFT_CONTROL         = 0x11,
        DOM_VK_RIGHT_CONTROL        = 0x11,
        DOM_VK_LEFT_SHIFT           = 0x10,
        DOM_VK_RIGHT_SHIFT          = 0x10,
        DOM_VK_META                 = 0x9D,
        DOM_VK_BACK_SPACE           = 0x08,
        DOM_VK_CAPS_LOCK            = 0x14,
        DOM_VK_DELETE               = 0x7F,
        DOM_VK_END                  = 0x23,
        DOM_VK_ENTER                = 0x0D,
        DOM_VK_ESCAPE               = 0x1B,
        DOM_VK_HOME                 = 0x24,
        DOM_VK_NUM_LOCK             = 0x90,
        DOM_VK_PAUSE                = 0x13,
        DOM_VK_PRINTSCREEN          = 0x9A,
        DOM_VK_SCROLL_LOCK          = 0x91,
        DOM_VK_SPACE                = 0x20,
        DOM_VK_TAB                  = 0x09,
        DOM_VK_LEFT                 = 0x25,
        DOM_VK_RIGHT                = 0x27,
        DOM_VK_UP                   = 0x26,
        DOM_VK_DOWN                 = 0x28,
        DOM_VK_PAGE_DOWN            = 0x22,
        DOM_VK_PAGE_UP              = 0x21,
        DOM_VK_F1                   = 0x70,
        DOM_VK_F2                   = 0x71,
        DOM_VK_F3                   = 0x72,
        DOM_VK_F4                   = 0x73,
        DOM_VK_F5                   = 0x74,
        DOM_VK_F6                   = 0x75,
        DOM_VK_F7                   = 0x76,
        DOM_VK_F8                   = 0x77,
        DOM_VK_F9                   = 0x78,
        DOM_VK_F10                  = 0x79,
        DOM_VK_F11                  = 0x7A,
        DOM_VK_F12                  = 0x7B,
        DOM_VK_F13                  = 0xF000,
        DOM_VK_F14                  = 0xF001,
        DOM_VK_F15                  = 0xF002,
        DOM_VK_F16                  = 0xF003,
        DOM_VK_F17                  = 0xF004,
        DOM_VK_F18                  = 0xF005,
        DOM_VK_F19                  = 0xF006,
        DOM_VK_F20                  = 0xF007,
        DOM_VK_F21                  = 0xF008,
        DOM_VK_F22                  = 0xF009,
        DOM_VK_F23                  = 0xF00A,
        DOM_VK_F24                  = 0xF00B
    };

    static int qtToDomKey(int keyCode);
};

int FakeDomEvent::qtToDomKey(int keyCode)
{
    switch (keyCode) {
    case Qt::Key_Backspace:
        return  DOM_VK_BACK_SPACE;
    case Qt::Key_Enter:
        return  DOM_VK_ENTER;
    case Qt::Key_Return:
        return  DOM_VK_ENTER;
    case Qt::Key_NumLock:
        return  DOM_VK_NUM_LOCK;
    case Qt::Key_Alt:
        return  DOM_VK_RIGHT_ALT;
    case Qt::Key_Control:
        return  DOM_VK_LEFT_CONTROL;
    case Qt::Key_Shift:
        return  DOM_VK_LEFT_SHIFT;
    case Qt::Key_Meta:
        return  DOM_VK_META;
    case Qt::Key_CapsLock:
        return  DOM_VK_CAPS_LOCK;
    case Qt::Key_Delete:
        return  DOM_VK_DELETE;
    case Qt::Key_End:
        return  DOM_VK_END;
    case Qt::Key_Escape:
        return  DOM_VK_ESCAPE;
    case Qt::Key_Home:
        return  DOM_VK_HOME;
    case Qt::Key_Pause:
        return  DOM_VK_PAUSE;
    case Qt::Key_Print:
        return  DOM_VK_PRINTSCREEN;
    case Qt::Key_ScrollLock:
        return  DOM_VK_SCROLL_LOCK;
    case Qt::Key_Left:
        return  DOM_VK_LEFT;
    case Qt::Key_Right:
        return  DOM_VK_RIGHT;
    case Qt::Key_Up:
        return  DOM_VK_UP;
    case Qt::Key_Down:
        return  DOM_VK_DOWN;
    case Qt::Key_PageDown:
        return  DOM_VK_PAGE_DOWN;
    case Qt::Key_PageUp:
        return  DOM_VK_PAGE_UP;
    case Qt::Key_F1:
        return  DOM_VK_F1;
    case Qt::Key_F2:
        return  DOM_VK_F2;
    case Qt::Key_F3:
        return  DOM_VK_F3;
    case Qt::Key_F4:
        return  DOM_VK_F4;
    case Qt::Key_F5:
        return  DOM_VK_F5;
    case Qt::Key_F6:
        return  DOM_VK_F6;
    case Qt::Key_F7:
        return  DOM_VK_F7;
    case Qt::Key_F8:
        return  DOM_VK_F8;
    case Qt::Key_F9:
        return  DOM_VK_F9;
    case Qt::Key_F10:
        return  DOM_VK_F10;
    case Qt::Key_F11:
        return  DOM_VK_F11;
    case Qt::Key_F12:
        return  DOM_VK_F12;
    case Qt::Key_F13:
        return  DOM_VK_F13;
    case Qt::Key_F14:
        return  DOM_VK_F14;
    case Qt::Key_F15:
        return  DOM_VK_F15;
    case Qt::Key_F16:
        return  DOM_VK_F16;
    case Qt::Key_F17:
        return  DOM_VK_F17;
    case Qt::Key_F18:
        return  DOM_VK_F18;
    case Qt::Key_F19:
        return  DOM_VK_F19;
    case Qt::Key_F20:
        return  DOM_VK_F20;
    case Qt::Key_F21:
        return  DOM_VK_F21;
    case Qt::Key_F22:
        return  DOM_VK_F22;
    case Qt::Key_F23:
        return  DOM_VK_F23;
    case Qt::Key_F24:
        return  DOM_VK_F24;
    }
    return keyCode;
}

//! [0]
Environment::Environment(QObject *parent)
    : QObject(parent)
{
    m_engine = new QScriptEngine(this);

    m_document = m_engine->newQObject(
        new Document(this), QScriptEngine::QtOwnership,
        QScriptEngine::ExcludeSuperClassContents);

    CanvasGradientPrototype::setup(m_engine);

    m_originalGlobalObject = m_engine->globalObject();
    reset();
}
//! [0]

Environment::~Environment()
{
}

QScriptEngine *Environment::engine() const
{
    return m_engine;
}

QScriptValue Environment::document() const
{
    return m_document;
}

int Environment::setTimeout(const QScriptValue &expression, int delay)
{
    if (expression.isString() || expression.isFunction()) {
        int timerId = startTimer(delay);
        m_timeoutHash.insert(timerId, expression);
        return timerId;
    }
    return -1;
}

void Environment::clearTimeout(int timerId)
{
    killTimer(timerId);
    m_timeoutHash.remove(timerId);
}

//! [1]
int Environment::setInterval(const QScriptValue &expression, int delay)
{
    if (expression.isString() || expression.isFunction()) {
        int timerId = startTimer(delay);
        m_intervalHash.insert(timerId, expression);
        return timerId;
    }
    return -1;
}

void Environment::clearInterval(int timerId)
{
    killTimer(timerId);
    m_intervalHash.remove(timerId);
}

void Environment::timerEvent(QTimerEvent *event)
{
    int id = event->timerId();
    QScriptValue expression = m_intervalHash.value(id);
    if (!expression.isValid()) {
        expression = m_timeoutHash.value(id);
        if (expression.isValid())
            killTimer(id);
    }
    if (expression.isString()) {
        evaluate(expression.toString());
    } else if (expression.isFunction()) {
        expression.call();
    }
    maybeEmitScriptError();
}
//! [1]

//! [5]
void Environment::addCanvas(QContext2DCanvas *canvas)
{
    m_canvases.append(canvas);
}

QContext2DCanvas *Environment::canvasByName(const QString &name) const
{
    for (int i = 0; i < m_canvases.size(); ++i) {
        QContext2DCanvas *canvas = m_canvases.at(i);
        if (canvas->objectName() == name)
            return canvas;
    }
    return 0;
}
//! [5]

QList<QContext2DCanvas*> Environment::canvases() const
{
    return m_canvases;
}

void Environment::reset()
{
    if (m_engine->isEvaluating())
        m_engine->abortEvaluation();

    {
        QHash<int, QScriptValue>::const_iterator it;
        for (it = m_intervalHash.constBegin(); it != m_intervalHash.constEnd(); ++it)
            killTimer(it.key());
        m_intervalHash.clear();
        for (it = m_timeoutHash.constBegin(); it != m_timeoutHash.constEnd(); ++it)
            killTimer(it.key());
        m_timeoutHash.clear();
    }

    for (int i = 0; i < m_canvases.size(); ++i)
        m_canvases.at(i)->reset();

    QScriptValue self = m_engine->newQObject(
        this, QScriptEngine::QtOwnership,
        QScriptEngine::ExcludeSuperClassContents);

    {
        QScriptValueIterator it(m_originalGlobalObject);
        while (it.hasNext()) {
            it.next();
            self.setProperty(it.scriptName(), it.value(), it.flags());
        }
    }

    self.setProperty("self", self);
    self.setProperty("window", self);

    QScriptValue navigator = m_engine->newObject();
    navigator.setProperty("appCodeName", "context2d");
    navigator.setProperty("appMinorVersion", 1);
    navigator.setProperty("appVersion", 1);
    navigator.setProperty("browserLanguage", "en_US");
    navigator.setProperty("cookieEnabled", false);
    navigator.setProperty("cpuClass", "i686");
    navigator.setProperty("onLine", false);
    navigator.setProperty("platform", "bogus OS");
    navigator.setProperty("systemLanguage", "en_US");
    navigator.setProperty("userAgent", "Context2D/1.1");
    navigator.setProperty("userLanguage", "en_US");
    self.setProperty("navigator", navigator);

    m_engine->setGlobalObject(self);

    m_engine->collectGarbage();
}

QScriptValue Environment::evaluate(const QString &code, const QString &fileName)
{
    return m_engine->evaluate(code, fileName);
}

bool Environment::hasIntervalTimers() const
{
    return !m_intervalHash.isEmpty();
}

// This is used by the Context2D Qt Script benchmark.
void Environment::triggerTimers()
{
    for (int x = 0; x < 2; ++x) {
        QList<int> timerIds = x ? m_intervalHash.keys() : m_timeoutHash.keys();
        for (int i = 0; i < timerIds.size(); ++i) {
            QTimerEvent fakeEvent(timerIds.at(i));
            timerEvent(&fakeEvent);
        }
    }
}

//! [2]
QScriptValue Environment::toWrapper(QObject *object)
{
    return m_engine->newQObject(object, QScriptEngine::QtOwnership,
                                QScriptEngine::PreferExistingWrapperObject
                                | QScriptEngine::ExcludeSuperClassContents);
}
//! [2]

//! [3]
void Environment::handleEvent(QContext2DCanvas *canvas, QMouseEvent *e)
{
    QString type;
    switch (e->type()) {
    case QEvent::MouseButtonPress:
        type = "mousedown"; break;
    case QEvent::MouseButtonRelease:
        type = "mouseup"; break;
    case QEvent::MouseMove:
        type = "mousemove"; break;
    default: break;
    }
    if (type.isEmpty())
        return;

    QScriptValue handlerObject;
    QScriptValue handler = eventHandler(canvas, type, &handlerObject);
    if (!handler.isFunction())
        return;

    QScriptValue scriptEvent = newFakeDomEvent(type, toWrapper(canvas));
    // MouseEvent
    scriptEvent.setProperty("screenX", e->globalX(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("screenY", e->globalY(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("clientX", e->x(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("clientY", e->y(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("layerX", e->x(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("layerY", e->y(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("pageX", e->x(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("pageY", e->y(), QScriptValue::ReadOnly);
    scriptEvent.setProperty("altKey", (e->modifiers() & Qt::AltModifier) != 0,
                            QScriptValue::ReadOnly);
    scriptEvent.setProperty("ctrlKey", (e->modifiers() & Qt::ControlModifier) != 0,
                            QScriptValue::ReadOnly);
    scriptEvent.setProperty("metaKey", (e->modifiers() & Qt::MetaModifier) != 0,
                            QScriptValue::ReadOnly);
    scriptEvent.setProperty("shiftKey", (e->modifiers() & Qt::ShiftModifier) != 0,
                            QScriptValue::ReadOnly);
    int button = 0;
    if (e->button() == Qt::RightButton)
        button = 2;
    else if (e->button() == Qt::MidButton)
        button = 1;
    scriptEvent.setProperty("button", button);
    scriptEvent.setProperty("relatedTarget", m_engine->nullValue(),
                            QScriptValue::ReadOnly);
    handler.call(handlerObject, QScriptValueList() << scriptEvent);
    maybeEmitScriptError();
}
//! [3]

void Environment::handleEvent(QContext2DCanvas *canvas, QKeyEvent *e)
{
    QString type;
    switch (e->type()) {
    case QEvent::KeyPress:
        type = "keydown"; break;
    case QEvent::KeyRelease:
        type = "keyup"; break;
    default: break;
    }
    if (type.isEmpty())
        return;

    QScriptValue handlerObject;
    QScriptValue handler = eventHandler(canvas, type, &handlerObject);
    if (!handler.isFunction())
        return;

    QScriptValue scriptEvent = newFakeDomEvent(type, toWrapper(canvas));
    // KeyEvent
    scriptEvent.setProperty("isChar", !e->text().isEmpty());
    scriptEvent.setProperty("charCode", e->text());
    scriptEvent.setProperty("keyCode", FakeDomEvent::qtToDomKey(e->key()));
    scriptEvent.setProperty("which", e->key());

    handler.call(handlerObject, QScriptValueList() << scriptEvent);
    maybeEmitScriptError();
}

QScriptValue Environment::eventHandler(QContext2DCanvas *canvas, const QString &type,
                                       QScriptValue *who)
{
    QString handlerName = "on" + type;
    QScriptValue obj = toWrapper(canvas);
    QScriptValue handler = obj.property(handlerName);
    if (!handler.isValid()) {
        obj = m_document;
        handler = obj.property(handlerName);
    }
    if (who && handler.isFunction())
        *who = obj;
    return handler;
}

//! [4]
QScriptValue Environment::newFakeDomEvent(const QString &type, const QScriptValue &target)
{
    QScriptValue e = m_engine->newObject();
    // Event
    e.setProperty("type", type, QScriptValue::ReadOnly);
    e.setProperty("bubbles", true, QScriptValue::ReadOnly);
    e.setProperty("cancelable", false, QScriptValue::ReadOnly);
    e.setProperty("target", target, QScriptValue::ReadOnly);
    e.setProperty("currentTarget", target, QScriptValue::ReadOnly);
    e.setProperty("eventPhase", 3); // bubbling
    e.setProperty("timeStamp", QDateTime::currentDateTime().toTime_t());
    // UIEvent
    e.setProperty("detail", 0, QScriptValue::ReadOnly);
    e.setProperty("view", m_engine->globalObject(), QScriptValue::ReadOnly);
    return e;
}
//! [4]

void Environment::maybeEmitScriptError()
{
    if (m_engine->hasUncaughtException())
        emit scriptError(m_engine->uncaughtException());
}


Document::Document(Environment *env)
    : QObject(env)
{
}

Document::~Document()
{
}

QScriptValue Document::getElementById(const QString &id) const
{
    Environment *env = qobject_cast<Environment*>(parent());
    QContext2DCanvas *canvas = env->canvasByName(id);
    if (!canvas)
        return QScriptValue();
    return env->toWrapper(canvas);
}

QScriptValue Document::getElementsByTagName(const QString &name) const
{
    if (name != "canvas")
        return QScriptValue();
    Environment *env = qobject_cast<Environment*>(parent());
    QList<QContext2DCanvas*> list = env->canvases();
    QScriptValue result = env->engine()->newArray(list.size());
    for (int i = 0; i < list.size(); ++i)
        result.setProperty(i, env->toWrapper(list.at(i)));
    return result;
}

void Document::addEventListener(const QString &type, const QScriptValue &listener,
                                bool useCapture)
{
    Q_UNUSED(useCapture);
    if (listener.isFunction()) {
        Environment *env = qobject_cast<Environment*>(parent());
        QScriptValue self = env->toWrapper(this);
        self.setProperty("on" + type, listener);
    }
}


QColor colorFromString(const QString &name);

CanvasGradientPrototype::CanvasGradientPrototype(QObject *parent)
    : QObject(parent)
{
}

void CanvasGradientPrototype::addColorStop(qreal offset, const QString &color)
{
    CanvasGradient *self = qscriptvalue_cast<CanvasGradient*>(thisObject());
    if (!self || (self->value.type() == QGradient::NoGradient))
        return;
    self->value.setColorAt(offset, colorFromString(color));
}

void CanvasGradientPrototype::setup(QScriptEngine *engine)
{
    CanvasGradientPrototype *proto = new CanvasGradientPrototype();
    engine->setDefaultPrototype(qMetaTypeId<CanvasGradient>(),
        engine->newQObject(proto, QScriptEngine::ScriptOwnership,
                           QScriptEngine::ExcludeSuperClassContents));
}
