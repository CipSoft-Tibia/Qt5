/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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


#include "qv4errorobject_p.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include "qv4string_p.h"
#include <private/qv4mm_p.h>
#include <private/qv4codegen_p.h>

#ifndef Q_OS_WIN
#  include <time.h>
#  ifndef Q_OS_VXWORKS
#    include <sys/time.h>
#  else
#    include "qplatformdefs.h"
#  endif
#else
#  include <windows.h>
#endif

using namespace QV4;

void Heap::ErrorObject::init()
{
    Object::init();
    stackTrace = nullptr;

    Scope scope(internalClass->engine);
    Scoped<QV4::ErrorObject> e(scope, this);

    if (internalClass == scope.engine->internalClasses(EngineBase::Class_ErrorProto))
        return;

    setProperty(scope.engine, QV4::ErrorObject::Index_Stack, scope.engine->getStackFunction()->d());
    setProperty(scope.engine, QV4::ErrorObject::Index_StackSetter, Value::undefinedValue());
    setProperty(scope.engine, QV4::ErrorObject::Index_FileName, Value::undefinedValue());
    setProperty(scope.engine, QV4::ErrorObject::Index_LineNumber, Value::undefinedValue());
}

void Heap::ErrorObject::init(const Value &message, ErrorType t)
{
    Object::init();
    errorType = t;

    Scope scope(internalClass->engine);
    Scoped<QV4::ErrorObject> e(scope, this);

    setProperty(scope.engine, QV4::ErrorObject::Index_Stack, scope.engine->getStackFunction()->d());
    setProperty(scope.engine, QV4::ErrorObject::Index_StackSetter, Value::undefinedValue());

    e->d()->stackTrace = new StackTrace(scope.engine->stackTrace());
    if (!e->d()->stackTrace->isEmpty()) {
        setProperty(scope.engine, QV4::ErrorObject::Index_FileName, scope.engine->newString(e->d()->stackTrace->at(0).source));
        setProperty(scope.engine, QV4::ErrorObject::Index_LineNumber, Value::fromInt32(e->d()->stackTrace->at(0).line));
    }

    if (!message.isUndefined())
        setProperty(scope.engine, QV4::ErrorObject::Index_Message, message);
}

void Heap::ErrorObject::init(const Value &message, const QString &fileName, int line, int column, ErrorObject::ErrorType t)
{
    Q_UNUSED(fileName); // ####
    Object::init();
    errorType = t;

    Scope scope(internalClass->engine);
    Scoped<QV4::ErrorObject> e(scope, this);

    setProperty(scope.engine, QV4::ErrorObject::Index_Stack, scope.engine->getStackFunction()->d());
    setProperty(scope.engine, QV4::ErrorObject::Index_StackSetter, Value::undefinedValue());

    e->d()->stackTrace = new StackTrace(scope.engine->stackTrace());
    StackFrame frame;
    frame.source = fileName;
    frame.line = line;
    frame.column = column;
    e->d()->stackTrace->prepend(frame);

    Q_ASSERT(!e->d()->stackTrace->isEmpty());
    setProperty(scope.engine, QV4::ErrorObject::Index_FileName, scope.engine->newString(e->d()->stackTrace->at(0).source));
    setProperty(scope.engine, QV4::ErrorObject::Index_LineNumber, Value::fromInt32(e->d()->stackTrace->at(0).line));

    if (!message.isUndefined())
        setProperty(scope.engine, QV4::ErrorObject::Index_Message, message);
}

const char *ErrorObject::className(Heap::ErrorObject::ErrorType t)
{
    switch (t) {
    case Heap::ErrorObject::Error:
        return "Error";
    case Heap::ErrorObject::EvalError:
        return "EvalError";
    case Heap::ErrorObject::RangeError:
        return "RangeError";
    case Heap::ErrorObject::ReferenceError:
        return "ReferenceError";
    case Heap::ErrorObject::SyntaxError:
        return "SyntaxError";
    case Heap::ErrorObject::TypeError:
        return "TypeError";
    case Heap::ErrorObject::URIError:
        return "URIError";
    }
    Q_UNREACHABLE();
}

ReturnedValue ErrorObject::method_get_stack(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const ErrorObject *This = thisObject->as<ErrorObject>();
    if (!This)
        return v4->throwTypeError();
    if (!This->d()->stack) {
        QString trace;
        for (int i = 0; i < This->d()->stackTrace->count(); ++i) {
            if (i > 0)
                trace += QLatin1Char('\n');
            const StackFrame &frame = This->d()->stackTrace->at(i);
            trace += frame.function + QLatin1Char('@') + frame.source;
            if (frame.line >= 0)
                trace += QLatin1Char(':') + QString::number(frame.line);
        }
        This->d()->stack.set(v4, v4->newString(trace));
    }
    return This->d()->stack->asReturnedValue();
}

DEFINE_OBJECT_VTABLE(ErrorObject);

void Heap::SyntaxErrorObject::init(const Value &msg)
{
    Heap::ErrorObject::init(msg, SyntaxError);
}

void Heap::SyntaxErrorObject::init(const Value &msg, const QString &fileName, int lineNumber, int columnNumber)
{
    Heap::ErrorObject::init(msg, fileName, lineNumber, columnNumber, SyntaxError);
}

void Heap::EvalErrorObject::init(const Value &message)
{
    Heap::ErrorObject::init(message, EvalError);
}

void Heap::RangeErrorObject::init(const Value &message)
{
    Heap::ErrorObject::init(message, RangeError);
}

void Heap::ReferenceErrorObject::init(const Value &message)
{
    Heap::ErrorObject::init(message, ReferenceError);
}

void Heap::ReferenceErrorObject::init(const Value &msg, const QString &fileName, int lineNumber, int columnNumber)
{
    Heap::ErrorObject::init(msg, fileName, lineNumber, columnNumber, ReferenceError);
}

void Heap::TypeErrorObject::init(const Value &message)
{
    Heap::ErrorObject::init(message, TypeError);
}

void Heap::URIErrorObject::init(const Value &message)
{
    Heap::ErrorObject::init(message, URIError);
}

DEFINE_OBJECT_VTABLE(ErrorCtor);
DEFINE_OBJECT_VTABLE(EvalErrorCtor);
DEFINE_OBJECT_VTABLE(RangeErrorCtor);
DEFINE_OBJECT_VTABLE(ReferenceErrorCtor);
DEFINE_OBJECT_VTABLE(SyntaxErrorCtor);
DEFINE_OBJECT_VTABLE(TypeErrorCtor);
DEFINE_OBJECT_VTABLE(URIErrorCtor);

void Heap::ErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("Error"));
}

void Heap::ErrorCtor::init(QV4::ExecutionContext *scope, const QString &name)
{
    Heap::FunctionObject::init(scope, name);
}

ReturnedValue ErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<ErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

ReturnedValue ErrorCtor::virtualCall(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    return f->callAsConstructor(argv, argc);
}

void Heap::EvalErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("EvalError"));
}

ReturnedValue EvalErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<EvalErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

void Heap::RangeErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("RangeError"));
}

ReturnedValue RangeErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<RangeErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

void Heap::ReferenceErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("ReferenceError"));
}

ReturnedValue ReferenceErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<ReferenceErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

void Heap::SyntaxErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("SyntaxError"));
}

ReturnedValue SyntaxErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<SyntaxErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

void Heap::TypeErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("TypeError"));
}

ReturnedValue TypeErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<TypeErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

void Heap::URIErrorCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("URIError"));
}

ReturnedValue URIErrorCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Value v = argc ? *argv : Value::undefinedValue();
    return ErrorObject::create<URIErrorObject>(f->engine(), v, newTarget)->asReturnedValue();
}

void ErrorPrototype::init(ExecutionEngine *engine, Object *ctor, Object *obj, Heap::ErrorObject::ErrorType t)
{
    Scope scope(engine);
    ScopedString s(scope);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = obj));
    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(1));
    obj->setProperty(Index_Constructor, ctor->d());
    obj->setProperty(Index_Message, engine->id_empty()->d());
    obj->setProperty(Index_Name, engine->newString(QString::fromLatin1(ErrorObject::className(t))));
    obj->defineDefaultProperty(engine->id_toString(), method_toString, 0);
}

ReturnedValue ErrorPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const Object *o = thisObject->as<Object>();
    if (!o)
        return v4->throwTypeError();

    Scope scope(v4);
    ScopedValue name(scope, o->get(scope.engine->id_name()));
    QString qname;
    if (name->isUndefined())
        qname = QStringLiteral("Error");
    else
        qname = name->toQString();

    ScopedString s(scope, scope.engine->newString(QStringLiteral("message")));
    ScopedValue message(scope, o->get(s));
    QString qmessage;
    if (!message->isUndefined())
        qmessage = message->toQString();

    QString str;
    if (qname.isEmpty()) {
        str = qmessage;
    } else if (qmessage.isEmpty()) {
        str = qname;
    } else {
        str = qname + QLatin1String(": ") + qmessage;
    }

    return scope.engine->newString(str)->asReturnedValue();
}
