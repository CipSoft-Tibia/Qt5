/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScript module of the Qt Toolkit.
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

#include "qscriptdeclarativeclass_p.h"
#include "qscriptdeclarativeobject_p.h"
#include "qscriptobject_p.h"
#include "qscriptstaticscopeobject_p.h"
#include <QtScript/qscriptstring.h>
#include <QtScript/qscriptengine.h>
#include <QtScript/qscriptengineagent.h>
#include <private/qscriptengine_p.h>
#include <private/qscriptvalue_p.h>
#include <private/qscriptqobject_p.h>
#include <private/qscriptactivationobject_p.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

/*!
\class QScriptDeclarativeClass::Value
\internal
\brief The QScriptDeclarativeClass::Value class acts as a container for JavaScript data types.

QScriptDeclarativeClass::Value class is similar to QScriptValue, but it is slightly faster.  
Unlike QScriptValue, however, Value instances cannot be stored as they may not survive garbage 
collection.  If you need to store a Value, convert it to a QScriptValue and store that.
*/

QScriptDeclarativeClass::Value::Value()
{
    new (this) JSC::JSValue(JSC::jsUndefined());
}

QScriptDeclarativeClass::Value::Value(const Value &other)
{
    new (this) JSC::JSValue((JSC::JSValue &)other);
}

static QScriptDeclarativeClass::Value jscToValue(const JSC::JSValue &val)
{
    return QScriptDeclarativeClass::Value((QScriptDeclarativeClass::Value &)val);
}

QScriptDeclarativeClass::Value::Value(QScriptContext *ctxt, int value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::frameForContext(ctxt), value);
}

QScriptDeclarativeClass::Value::Value(QScriptContext *ctxt, uint value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::frameForContext(ctxt), value);
}

QScriptDeclarativeClass::Value::Value(QScriptContext *, bool value)
{
    if (value)
        new (this) JSC::JSValue(JSC::JSValue::JSTrue);
    else
        new (this) JSC::JSValue(JSC::JSValue::JSFalse);
}

QScriptDeclarativeClass::Value::Value(QScriptContext *ctxt, double value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::frameForContext(ctxt), value);
}

QScriptDeclarativeClass::Value::Value(QScriptContext *ctxt, float value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::frameForContext(ctxt), value);
}

QScriptDeclarativeClass::Value::Value(QScriptContext *ctxt, const QString &value)
{
    new (this) JSC::JSValue(JSC::jsString(QScriptEnginePrivate::frameForContext(ctxt), value));
}

QScriptDeclarativeClass::Value::Value(QScriptContext *ctxt, const QScriptValue &value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::get(ctxt->engine())->scriptValueToJSCValue(value));
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, int value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::get(eng)->currentFrame, value);
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, uint value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::get(eng)->currentFrame, value);
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, bool value)
{
    if (value)
        new (this) JSC::JSValue(JSC::JSValue::JSTrue);
    else
        new (this) JSC::JSValue(JSC::JSValue::JSFalse);
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, double value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::get(eng)->currentFrame, value);
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, float value)
{
    new (this) JSC::JSValue(QScriptEnginePrivate::get(eng)->currentFrame, value);
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, const QString &value)
{
    new (this) JSC::JSValue(JSC::jsString(QScriptEnginePrivate::get(eng)->currentFrame, value));
}

QScriptDeclarativeClass::Value::Value(QScriptEngine *eng, const QScriptValue &value)
{
        new (this) JSC::JSValue(QScriptEnginePrivate::get(eng)->scriptValueToJSCValue(value));
}

QScriptDeclarativeClass::Value::~Value()
{
    ((JSC::JSValue *)(this))->~JSValue();
}

QScriptValue QScriptDeclarativeClass::Value::toScriptValue(QScriptEngine *engine) const
{
    return QScriptEnginePrivate::get(engine)->scriptValueFromJSCValue((JSC::JSValue &)*this);
}

QScriptDeclarativeClass::PersistentIdentifier::PersistentIdentifier()
    : identifier(0), engine(0)
{
    new (&d) JSC::Identifier();
}

QScriptDeclarativeClass::PersistentIdentifier::~PersistentIdentifier()
{
    if (engine) {
        QScript::APIShim shim(engine);
        ((JSC::Identifier &)d).JSC::Identifier::~Identifier();
    } else {
        ((JSC::Identifier &)d).JSC::Identifier::~Identifier();
    }
}

QScriptDeclarativeClass::PersistentIdentifier::PersistentIdentifier(const PersistentIdentifier &other)
{
    identifier = other.identifier;
    engine = other.engine;
    new (&d) JSC::Identifier((JSC::Identifier &)(other.d));
}

QScriptDeclarativeClass::PersistentIdentifier &
QScriptDeclarativeClass::PersistentIdentifier::operator=(const PersistentIdentifier &other)
{
    identifier = other.identifier;
    engine = other.engine;
    ((JSC::Identifier &)d) = (JSC::Identifier &)(other.d);
    return *this;
}

QString QScriptDeclarativeClass::PersistentIdentifier::toString() const
{
    return ((JSC::Identifier &)d).ustring();
}

QScriptDeclarativeClass::QScriptDeclarativeClass(QScriptEngine *engine)
: d_ptr(new QScriptDeclarativeClassPrivate)
{
    Q_ASSERT(sizeof(void*) == sizeof(JSC::Identifier));
    d_ptr->q_ptr = this;
    d_ptr->engine = engine;
}

QScriptValue QScriptDeclarativeClass::newObject(QScriptEngine *engine, 
                                                QScriptDeclarativeClass *scriptClass, 
                                                Object *object)
{
    Q_ASSERT(engine);
    Q_ASSERT(scriptClass);

    QScriptEnginePrivate *p = static_cast<QScriptEnginePrivate *>(QObjectPrivate::get(engine));
    QScript::APIShim shim(p);

    JSC::ExecState* exec = p->currentFrame;
    QScriptObject *result = new (exec) QScriptObject(p->scriptObjectStructure);
    result->setDelegate(new QScript::DeclarativeObjectDelegate(scriptClass, object));
    return p->scriptValueFromJSCValue(result);
}

QScriptDeclarativeClass::Value 
QScriptDeclarativeClass::newObjectValue(QScriptEngine *engine, 
                                        QScriptDeclarativeClass *scriptClass, 
                                        Object *object)
{
    Q_ASSERT(engine);
    Q_ASSERT(scriptClass);

    QScriptEnginePrivate *p = static_cast<QScriptEnginePrivate *>(QObjectPrivate::get(engine)); 
    QScript::APIShim shim(p);

    JSC::ExecState* exec = p->currentFrame;
    QScriptObject *result = new (exec) QScriptObject(p->scriptObjectStructure);
    result->setDelegate(new QScript::DeclarativeObjectDelegate(scriptClass, object));
    return jscToValue(JSC::JSValue(result));
}

QScriptDeclarativeClass *QScriptDeclarativeClass::scriptClass(const QScriptValue &v)
{
    QScriptValuePrivate *d = QScriptValuePrivate::get(v);
    if (!d || !d->isJSC())
        return 0;
    return QScriptEnginePrivate::declarativeClass(d->jscValue);
}

QScriptDeclarativeClass::Object *QScriptDeclarativeClass::object(const QScriptValue &v)
{
    QScriptValuePrivate *d = QScriptValuePrivate::get(v);
    if (!d || !d->isJSC())
        return 0;
    return QScriptEnginePrivate::declarativeObject(d->jscValue);
}

QScriptValue QScriptDeclarativeClass::function(const QScriptValue &v, const Identifier &name)
{
    QScriptValuePrivate *d = QScriptValuePrivate::get(v);

    if (!d->isObject())
        return QScriptValue();

    QScript::APIShim shim(d->engine);
    JSC::ExecState *exec = d->engine->currentFrame;
    JSC::JSObject *object = d->jscValue.getObject();
    JSC::PropertySlot slot(const_cast<JSC::JSObject*>(object));
    JSC::JSValue result;

    JSC::Identifier id(exec, (JSC::UString::Rep *)name);

    if (const_cast<JSC::JSObject*>(object)->getOwnPropertySlot(exec, id, slot)) {
        result = slot.getValue(exec, id);
        if (QScript::isFunction(result))
            return d->engine->scriptValueFromJSCValue(result);
    }

    return QScriptValue();
}

QScriptValue QScriptDeclarativeClass::property(const QScriptValue &v, const Identifier &name)
{
    QScriptValuePrivate *d = QScriptValuePrivate::get(v);

    if (!d->isObject())
        return QScriptValue();

    QScript::APIShim shim(d->engine);
    JSC::ExecState *exec = d->engine->currentFrame;
    JSC::JSObject *object = d->jscValue.getObject();
    JSC::PropertySlot slot(const_cast<JSC::JSObject*>(object));
    JSC::JSValue result;

    JSC::Identifier id(exec, (JSC::UString::Rep *)name);

    if (const_cast<JSC::JSObject*>(object)->getOwnPropertySlot(exec, id, slot)) {
        result = slot.getValue(exec, id);
        return d->engine->scriptValueFromJSCValue(result);
    }

    return QScriptValue();
}

QScriptDeclarativeClass::Value
QScriptDeclarativeClass::functionValue(const QScriptValue &v, const Identifier &name)
{
    QScriptValuePrivate *d = QScriptValuePrivate::get(v);

    if (!d->isObject())
        return Value();

    QScript::APIShim shim(d->engine);
    JSC::ExecState *exec = d->engine->currentFrame;
    JSC::JSObject *object = d->jscValue.getObject();
    JSC::PropertySlot slot(const_cast<JSC::JSObject*>(object));
    JSC::JSValue result;

    JSC::Identifier id(exec, (JSC::UString::Rep *)name);

    if (const_cast<JSC::JSObject*>(object)->getOwnPropertySlot(exec, id, slot)) {
        result = slot.getValue(exec, id);
        if (QScript::isFunction(result))
            return jscToValue(result);
    }

    return Value();
}

QScriptDeclarativeClass::Value
QScriptDeclarativeClass::propertyValue(const QScriptValue &v, const Identifier &name)
{
    QScriptValuePrivate *d = QScriptValuePrivate::get(v);

    if (!d->isObject())
        return Value();

    QScript::APIShim shim(d->engine);
    JSC::ExecState *exec = d->engine->currentFrame;
    JSC::JSObject *object = d->jscValue.getObject();
    JSC::PropertySlot slot(const_cast<JSC::JSObject*>(object));
    JSC::JSValue result;

    JSC::Identifier id(exec, (JSC::UString::Rep *)name);

    if (const_cast<JSC::JSObject*>(object)->getOwnPropertySlot(exec, id, slot)) {
        result = slot.getValue(exec, id);
        return jscToValue(result);
    }

    return Value();
}

/*
Returns the scope chain entry at \a index.  If index is less than 0, returns
entries starting at the end.  For example, scopeChainValue(context, -1) will return
the value last in the scope chain.
*/
QScriptValue QScriptDeclarativeClass::scopeChainValue(QScriptContext *context, int index)
{
    context->activationObject(); //ensure the creation of the normal scope for native context
    const JSC::CallFrame *frame = QScriptEnginePrivate::frameForContext(context);
    QScriptEnginePrivate *engine = QScript::scriptEngineFromExec(frame);
    QScript::APIShim shim(engine);

    JSC::ScopeChainNode *node = frame->scopeChain();
    JSC::ScopeChainIterator it(node);

    if (index < 0) {
        int count = 0;
        for (it = node->begin(); it != node->end(); ++it) 
            ++count;

        index = qAbs(index);
        if (index > count)
            return QScriptValue();
        else
            index = count - index;
    }

    for (it = node->begin(); it != node->end(); ++it) {

        if (index == 0) {

            JSC::JSObject *object = *it;
            if (!object) return QScriptValue();

            if (object->inherits(&QScript::QScriptActivationObject::info)
                    && (static_cast<QScript::QScriptActivationObject*>(object)->delegate() != 0)) {
                // Return the object that property access is being delegated to
                object = static_cast<QScript::QScriptActivationObject*>(object)->delegate();
            }
            return engine->scriptValueFromJSCValue(object);

        } else {
            --index;
        }

    }

    return QScriptValue();
}

/*!
  Enters a new execution context and returns the associated
  QScriptContext object.

  Once you are done with the context, you should call popContext() to
  restore the old context.

  By default, the `this' object of the new context is the Global Object.
  The context's \l{QScriptContext::callee()}{callee}() will be invalid.

  The context's scope chain initially contains only the Global Object
  and the QScriptContext's activation object.

  This function behaves exactly like QScriptEngine::popContext();
  it exists because QScriptEngine::popContext() used to have a bug
  that caused the scope chain of the new context to be incorrect.
*/
QScriptContext * QScriptDeclarativeClass::pushCleanContext(QScriptEngine *engine)
{
    if (!engine)
        return 0;

    return engine->pushContext();
}

QScriptDeclarativeClass::~QScriptDeclarativeClass()
{
}

QScriptEngine *QScriptDeclarativeClass::engine() const
{
    return d_ptr->engine;
}

bool QScriptDeclarativeClass::supportsCall() const
{
    return d_ptr->supportsCall;
}

void QScriptDeclarativeClass::setSupportsCall(bool c)
{
    d_ptr->supportsCall = c;
}

QScriptDeclarativeClass::PersistentIdentifier 
QScriptDeclarativeClass::createPersistentIdentifier(const QString &str)
{
    QScriptEnginePrivate *p = 
        static_cast<QScriptEnginePrivate *>(QObjectPrivate::get(d_ptr->engine)); 
    QScript::APIShim shim(p);
    JSC::ExecState* exec = p->currentFrame;

    PersistentIdentifier rv(p);
    new (&rv.d) JSC::Identifier(exec, (UChar *)str.constData(), str.size());
    rv.identifier = (void *)((JSC::Identifier &)rv.d).ustring().rep();
    return rv;
}

QScriptDeclarativeClass::PersistentIdentifier 
QScriptDeclarativeClass::createPersistentIdentifier(const Identifier &id)
{
    QScriptEnginePrivate *p = 
        static_cast<QScriptEnginePrivate *>(QObjectPrivate::get(d_ptr->engine)); 
    QScript::APIShim shim(p);
    JSC::ExecState* exec = p->currentFrame;

    PersistentIdentifier rv(p);
    new (&rv.d) JSC::Identifier(exec, (JSC::UString::Rep *)id);
    rv.identifier = (void *)((JSC::Identifier &)rv.d).ustring().rep();
    return rv;
}

QString QScriptDeclarativeClass::toString(const Identifier &identifier)
{
    JSC::UString::Rep *r = (JSC::UString::Rep *)identifier;
    return QString((QChar *)r->data(), r->size());
}

bool QScriptDeclarativeClass::startsWithUpper(const Identifier &identifier)
{
    JSC::UString::Rep *r = (JSC::UString::Rep *)identifier;
    if (r->size() < 1)
        return false;
    return QChar::category((ushort)(r->data()[0])) == QChar::Letter_Uppercase;
}

quint32 QScriptDeclarativeClass::toArrayIndex(const Identifier &identifier, bool *ok)
{
    JSC::UString::Rep *r = (JSC::UString::Rep *)identifier;
    JSC::UString s(r);
    return s.toArrayIndex(ok);
}

QScriptClass::QueryFlags 
QScriptDeclarativeClass::queryProperty(Object *object, const Identifier &name, 
                                       QScriptClass::QueryFlags flags)
{
    Q_UNUSED(object);
    Q_UNUSED(name);
    Q_UNUSED(flags);
    return {};
}

QScriptDeclarativeClass::Value
QScriptDeclarativeClass::property(Object *object, const Identifier &name)
{
    Q_UNUSED(object);
    Q_UNUSED(name);
    return Value();
}

void QScriptDeclarativeClass::setProperty(Object *object, const Identifier &name, 
                                          const QScriptValue &value)
{
    Q_UNUSED(object);
    Q_UNUSED(name);
    Q_UNUSED(value);
}

QScriptValue::PropertyFlags 
QScriptDeclarativeClass::propertyFlags(Object *object, const Identifier &name)
{
    Q_UNUSED(object);
    Q_UNUSED(name);
    return {};
}

QScriptDeclarativeClass::Value QScriptDeclarativeClass::call(Object *object, 
                                                             QScriptContext *ctxt)
{
    Q_UNUSED(object);
    Q_UNUSED(ctxt);
    return Value();
}

bool QScriptDeclarativeClass::compare(Object *o, Object *o2)
{
    return o == o2;
}

QStringList QScriptDeclarativeClass::propertyNames(Object *object)
{
    Q_UNUSED(object);
    return QStringList();
}

bool QScriptDeclarativeClass::isQObject() const
{
    return false;
}

QObject *QScriptDeclarativeClass::toQObject(Object *, bool *ok)
{
    if (ok) *ok = false;
    return 0;
}

QVariant QScriptDeclarativeClass::toVariant(Object *, bool *ok)
{
    if (ok) *ok = false;
    return QVariant();
}

QScriptContext *QScriptDeclarativeClass::context() const
{
    return d_ptr->context;
}

/*!
  Creates a scope object with a fixed set of undeletable properties.
*/
QScriptValue QScriptDeclarativeClass::newStaticScopeObject(
    QScriptEngine *engine, int propertyCount, const QString *names,
    const QScriptValue *values, const QScriptValue::PropertyFlags *flags)
{
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(engine);
    QScript::APIShim shim(eng_p);
    JSC::ExecState *exec = eng_p->currentFrame;
    QScriptStaticScopeObject::PropertyInfo *props = new QScriptStaticScopeObject::PropertyInfo[propertyCount];
    for (int i = 0; i < propertyCount; ++i) {
        unsigned attribs = QScriptEnginePrivate::propertyFlagsToJSCAttributes(flags[i]);
        Q_ASSERT_X(attribs & JSC::DontDelete, Q_FUNC_INFO, "All properties must be undeletable");
        JSC::Identifier id = JSC::Identifier(exec, names[i]);
        JSC::JSValue jsval = eng_p->scriptValueToJSCValue(values[i]);
        props[i] = QScriptStaticScopeObject::PropertyInfo(id, jsval, attribs);
    }
    QScriptValue result = eng_p->scriptValueFromJSCValue(new (exec)QScriptStaticScopeObject(eng_p->staticScopeObjectStructure,
                                                                                            propertyCount, props));
    delete[] props;
    return result;
}

/*!
  Creates a static scope object that's initially empty, but to which new
  properties can be added.
*/
QScriptValue QScriptDeclarativeClass::newStaticScopeObject(QScriptEngine *engine)
{
    QScriptEnginePrivate *eng_p = QScriptEnginePrivate::get(engine);
    QScript::APIShim shim(eng_p);
    return eng_p->scriptValueFromJSCValue(new (eng_p->currentFrame)QScriptStaticScopeObject(eng_p->staticScopeObjectStructure));
}

QT_END_NAMESPACE
