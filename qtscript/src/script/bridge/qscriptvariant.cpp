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

#include "config.h"
#include "qscriptvariant_p.h"

#include "../api/qscriptengine.h"
#include "../api/qscriptengine_p.h"

#include "Error.h"
#include "PrototypeFunction.h"
#include "JSFunction.h"
#include "NativeFunctionWrapper.h"
#include "JSString.h"

namespace JSC
{
QT_USE_NAMESPACE
ASSERT_CLASS_FITS_IN_CELL(QScript::QVariantPrototype);
}

QT_BEGIN_NAMESPACE

namespace QScript
{

QVariantDelegate::QVariantDelegate(const QVariant &value)
    : m_value(value)
{
}

QVariantDelegate::~QVariantDelegate()
{
}

QVariant &QVariantDelegate::value()
{
    return m_value;
}

void QVariantDelegate::setValue(const QVariant &value)
{
    m_value = value;
}

QScriptObjectDelegate::Type QVariantDelegate::type() const
{
    return Variant;
}

static JSC::JSValue JSC_HOST_CALL variantProtoFuncValueOf(JSC::ExecState *exec, JSC::JSObject*,
                                                          JSC::JSValue thisValue, const JSC::ArgList&)
{
    QScriptEnginePrivate *engine = scriptEngineFromExec(exec);
    thisValue = engine->toUsableValue(thisValue);
    if (!thisValue.inherits(&QScriptObject::info))
        return throwError(exec, JSC::TypeError);
    QScriptObjectDelegate *delegate = static_cast<QScriptObject*>(JSC::asObject(thisValue))->delegate();
    if (!delegate || (delegate->type() != QScriptObjectDelegate::Variant))
        return throwError(exec, JSC::TypeError);
    const QVariant &v = static_cast<QVariantDelegate*>(delegate)->value();
    switch (v.type()) {
    case QVariant::Invalid:
        return JSC::jsUndefined();
    case QVariant::String:
        return JSC::jsString(exec, v.toString());

    case QVariant::Int:
        return JSC::jsNumber(exec, v.toInt());

    case QVariant::Bool:
        return JSC::jsBoolean(v.toBool());

    case QVariant::Double:
        return JSC::jsNumber(exec, v.toDouble());

//    case QVariant::Char:
//        return JSC::jsNumber(exec, v.toChar().unicode());

    case QVariant::UInt:
        return JSC::jsNumber(exec, v.toUInt());

    default:
        ;
    }
    return thisValue;
}

static JSC::JSValue JSC_HOST_CALL variantProtoFuncToString(JSC::ExecState *exec, JSC::JSObject *callee,
                                                           JSC::JSValue thisValue, const JSC::ArgList &args)
{
    QScriptEnginePrivate *engine = scriptEngineFromExec(exec);
    thisValue = engine->toUsableValue(thisValue);
    if (!thisValue.inherits(&QScriptObject::info))
        return throwError(exec, JSC::TypeError, "This object is not a QVariant");
    QScriptObjectDelegate *delegate = static_cast<QScriptObject*>(JSC::asObject(thisValue))->delegate();
    if (!delegate || (delegate->type() != QScriptObjectDelegate::Variant))
        return throwError(exec, JSC::TypeError, "This object is not a QVariant");
    const QVariant &v = static_cast<QVariantDelegate*>(delegate)->value();
    JSC::UString result;
    JSC::JSValue value = variantProtoFuncValueOf(exec, callee, thisValue, args);
    if (value.isObject()) {
        result = v.toString();
        if (result.isEmpty() && !v.canConvert(QVariant::String))
            result = QString::fromLatin1("QVariant(%0)").arg(QString::fromLatin1(v.typeName()));
    } else {
        result = value.toString(exec);
    }
    return JSC::jsString(exec, result);
}

bool QVariantDelegate::compareToObject(QScriptObject *, JSC::ExecState *exec, JSC::JSObject *o2)
{
    const QVariant &variant1 = value();
    return variant1 == QScriptEnginePrivate::toVariant(exec, o2);
}

QVariantPrototype::QVariantPrototype(JSC::ExecState* exec, WTF::PassRefPtr<JSC::Structure> structure,
                                     JSC::Structure* prototypeFunctionStructure)
    : QScriptObject(structure)
{
    setDelegate(new QVariantDelegate(QVariant()));

    putDirectFunction(exec, new (exec) JSC::NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().toString, variantProtoFuncToString), JSC::DontEnum);
    putDirectFunction(exec, new (exec) JSC::NativeFunctionWrapper(exec, prototypeFunctionStructure, 0, exec->propertyNames().valueOf, variantProtoFuncValueOf), JSC::DontEnum);
}


} // namespace QScript

QT_END_NAMESPACE
