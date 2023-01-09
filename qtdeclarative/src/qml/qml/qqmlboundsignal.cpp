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

#include "qqmlboundsignal_p.h"

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include "qqmlengine_p.h"
#include "qqmlexpression_p.h"
#include "qqmlcontext_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlglobal_p.h"
#include <private/qqmlprofiler_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include "qqmlinfo.h"

#include <private/qjsvalue_p.h>
#include <private/qv4value_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4qmlcontext_p.h>

#include <QtCore/qdebug.h>

#include <qtqml_tracepoints_p.h>

QT_BEGIN_NAMESPACE

QQmlBoundSignalExpression::QQmlBoundSignalExpression(QObject *target, int index,
                                                     QQmlContextData *ctxt, QObject *scope, const QString &expression,
                                                     const QString &fileName, quint16 line, quint16 column,
                                                     const QString &handlerName,
                                                     const QString &parameterString)
    : QQmlJavaScriptExpression(),
      m_index(index),
      m_target(target)
{
    init(ctxt, scope);

    QV4::ExecutionEngine *v4 = engine()->handle();

    QString function;

    // Add some leading whitespace to account for the binding's column offset.
    // It's 2 off because a, we start counting at 1 and b, the '(' below is not counted.
    function += QString(qMax(column, (quint16)2) - 2, QChar(QChar::Space))
              + QLatin1String("(function ") + handlerName + QLatin1Char('(');

    if (parameterString.isEmpty()) {
        QString error;
        //TODO: look at using the property cache here (as in the compiler)
        //      for further optimization
        QMetaMethod signal = QMetaObjectPrivate::signal(m_target->metaObject(), m_index);
        function += QQmlPropertyCache::signalParameterStringForJS(v4, signal.parameterNames(), &error);

        if (!error.isEmpty()) {
            qmlWarning(scopeObject()) << error;
            return;
        }
    } else
        function += parameterString;

    function += QLatin1String(") { ") + expression + QLatin1String(" })");
    QV4::Scope valueScope(v4);
    QV4::ScopedFunctionObject f(valueScope, evalFunction(context(), scopeObject(), function, fileName, line));
    QV4::ScopedContext context(valueScope, f->scope());
    setupFunction(context, f->function());
}

QQmlBoundSignalExpression::QQmlBoundSignalExpression(QObject *target, int index, QQmlContextData *ctxt, QObject *scopeObject,
                                                     QV4::Function *function, QV4::ExecutionContext *scope)
    : QQmlJavaScriptExpression(),
      m_index(index),
      m_target(target)
{
    // It's important to call init first, because m_index gets remapped in case of cloned signals.
    init(ctxt, scopeObject);

    QV4::ExecutionEngine *engine = ctxt->engine->handle();

    // If the function is marked as having a nested function, then the user wrote:
    //   onSomeSignal: function() { /*....*/ }
    // So take that nested function:
    if (auto closure = function->nestedFunction()) {
        function = closure;
    } else {
        QList<QByteArray> signalParameters = QMetaObjectPrivate::signal(m_target->metaObject(), m_index).parameterNames();
        if (!signalParameters.isEmpty()) {
            QString error;
            QQmlPropertyCache::signalParameterStringForJS(engine, signalParameters, &error);
            if (!error.isEmpty()) {
                qmlWarning(scopeObject) << error;
                return;
            }
            function->updateInternalClass(engine, signalParameters);
        }
    }

    QV4::Scope valueScope(engine);
    QV4::Scoped<QV4::QmlContext> qmlContext(valueScope, scope);
    if (!qmlContext)
        qmlContext = QV4::QmlContext::create(engine->rootContext(), ctxt, scopeObject);
    setupFunction(qmlContext, function);
}

void QQmlBoundSignalExpression::init(QQmlContextData *ctxt, QObject *scope)
{
    setNotifyOnValueChanged(false);
    setContext(ctxt);
    setScopeObject(scope);

    Q_ASSERT(m_target && m_index > -1);
    m_index = QQmlPropertyCache::originalClone(m_target, m_index);
}

QQmlBoundSignalExpression::~QQmlBoundSignalExpression()
{
}

QString QQmlBoundSignalExpression::expressionIdentifier() const
{
    QQmlSourceLocation loc = sourceLocation();
    return loc.sourceFile + QLatin1Char(':') + QString::number(loc.line);
}

void QQmlBoundSignalExpression::expressionChanged()
{
    // bound signals do not notify on change.
}

QString QQmlBoundSignalExpression::expression() const
{
    if (expressionFunctionValid())
        return QStringLiteral("function() { [native code] }");
    return QString();
}

// Parts of this function mirror code in QQmlExpressionPrivate::value() and v8value().
// Changes made here may need to be made there and vice versa.
void QQmlBoundSignalExpression::evaluate(void **a)
{
    Q_ASSERT (context() && engine());

    if (!expressionFunctionValid())
        return;

    QQmlEngine *qmlengine = engine();
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(qmlengine);
    QV4::ExecutionEngine *v4 = qmlengine->handle();
    QV4::Scope scope(v4);

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

    QQmlMetaObject::ArgTypeStorage storage;
    //TODO: lookup via signal index rather than method index as an optimization
    int methodIndex = QMetaObjectPrivate::signal(m_target->metaObject(), m_index).methodIndex();
    int *argsTypes = QQmlMetaObject(m_target).methodParameterTypes(methodIndex, &storage, nullptr);
    int argCount = argsTypes ? *argsTypes : 0;

    QV4::JSCallData jsCall(scope, argCount);
    for (int ii = 0; ii < argCount; ++ii) {
        int type = argsTypes[ii + 1];
        //### ideally we would use metaTypeToJS, however it currently gives different results
        //    for several cases (such as QVariant type and QObject-derived types)
        //args[ii] = engine->metaTypeToJS(type, a[ii + 1]);
        if (type == qMetaTypeId<QJSValue>()) {
            if (QV4::Value *v4Value = QJSValuePrivate::valueForData(reinterpret_cast<QJSValue *>(a[ii + 1]), &jsCall->args[ii]))
                jsCall->args[ii] = *v4Value;
            else
                jsCall->args[ii] = QV4::Encode::undefined();
        } else if (type == QMetaType::QVariant) {
            jsCall->args[ii] = scope.engine->fromVariant(*((QVariant *)a[ii + 1]));
        } else if (type == QMetaType::Int) {
            //### optimization. Can go away if we switch to metaTypeToJS, or be expanded otherwise
            jsCall->args[ii] = QV4::Value::fromInt32(*reinterpret_cast<const int*>(a[ii + 1]));
        } else if (ep->isQObject(type)) {
            if (!*reinterpret_cast<void* const *>(a[ii + 1]))
                jsCall->args[ii] = QV4::Value::nullValue();
            else
                jsCall->args[ii] = QV4::QObjectWrapper::wrap(v4, *reinterpret_cast<QObject* const *>(a[ii + 1]));
        } else {
            jsCall->args[ii] = scope.engine->fromVariant(QVariant(type, a[ii + 1]));
        }
    }

    QQmlJavaScriptExpression::evaluate(jsCall.callData(), nullptr);

    ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
}

void QQmlBoundSignalExpression::evaluate(const QList<QVariant> &args)
{
    Q_ASSERT (context() && engine());

    if (!expressionFunctionValid())
        return;

    QQmlEngine *qmlengine = engine();
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(qmlengine);
    QV4::Scope scope(qmlengine->handle());

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

    QV4::JSCallData jsCall(scope, args.count());
    for (int ii = 0; ii < args.count(); ++ii) {
        jsCall->args[ii] = scope.engine->fromVariant(args[ii]);
    }

    QQmlJavaScriptExpression::evaluate(jsCall.callData(), nullptr);

    ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
}

////////////////////////////////////////////////////////////////////////


/*! \internal
    \a signal MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QQmlBoundSignal::QQmlBoundSignal(QObject *target, int signal, QObject *owner,
                                 QQmlEngine *engine)
    : QQmlNotifierEndpoint(QQmlNotifierEndpoint::QQmlBoundSignal),
      m_prevSignal(nullptr), m_nextSignal(nullptr),
      m_enabled(true), m_expression(nullptr)
{
    addToObject(owner);

    /*
        If this is a cloned method, connect to the 'original'. For example,
        for the signal 'void aSignal(int parameter = 0)', if the method
        index refers to 'aSignal()', get the index of 'aSignal(int)'.
        This ensures that 'parameter' will be available from QML.
    */
    signal = QQmlPropertyCache::originalClone(target, signal);
    QQmlNotifierEndpoint::connect(target, signal, engine);
}

QQmlBoundSignal::~QQmlBoundSignal()
{
    removeFromObject();
}

void QQmlBoundSignal::addToObject(QObject *obj)
{
    Q_ASSERT(!m_prevSignal);
    Q_ASSERT(obj);

    QQmlData *data = QQmlData::get(obj, true);

    m_nextSignal = data->signalHandlers;
    if (m_nextSignal) m_nextSignal->m_prevSignal = &m_nextSignal;
    m_prevSignal = &data->signalHandlers;
    data->signalHandlers = this;
}

void QQmlBoundSignal::removeFromObject()
{
    if (m_prevSignal) {
        *m_prevSignal = m_nextSignal;
        if (m_nextSignal) m_nextSignal->m_prevSignal = m_prevSignal;
        m_prevSignal = nullptr;
        m_nextSignal = nullptr;
    }
}


/*!
    Returns the signal expression.
*/
QQmlBoundSignalExpression *QQmlBoundSignal::expression() const
{
    return m_expression;
}

/*!
    Sets the signal expression to \a e.

    The QQmlBoundSignal instance takes ownership of \a e (and does not add a reference).
*/
void QQmlBoundSignal::takeExpression(QQmlBoundSignalExpression *e)
{
    m_expression.take(e);
    if (m_expression)
        m_expression->setNotifyOnValueChanged(false);
}

/*!
    This property holds whether the item will emit signals.

    The QQmlBoundSignal callback will only emit a signal if this property is set to true.

    By default, this property is true.
 */
void QQmlBoundSignal::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
}

void QQmlBoundSignal_callback(QQmlNotifierEndpoint *e, void **a)
{
    QQmlBoundSignal *s = static_cast<QQmlBoundSignal*>(e);

    if (!s->m_expression || !s->m_enabled)
        return;

    QV4DebugService *service = QQmlDebugConnector::service<QV4DebugService>();
    if (service)
        service->signalEmitted(QString::fromUtf8(QMetaObjectPrivate::signal(
                                                     s->m_expression->target()->metaObject(),
                                                     s->signalIndex()).methodSignature()));

    QQmlEngine *engine;
    if (s->m_expression && (engine = s->m_expression->engine())) {
        Q_TRACE_SCOPE(QQmlHandlingSignal, engine,
                      s->m_expression->function() ? s->m_expression->function()->name()->toQString() : QString(),
                      s->m_expression->sourceLocation().sourceFile, s->m_expression->sourceLocation().line,
                      s->m_expression->sourceLocation().column);
        QQmlHandlingSignalProfiler prof(QQmlEnginePrivate::get(engine)->profiler, s->m_expression);
        s->m_expression->evaluate(a);
        if (s->m_expression && s->m_expression->hasError()) {
            QQmlEnginePrivate::warning(engine, s->m_expression->error(engine));
        }
    }
}

////////////////////////////////////////////////////////////////////////

QQmlBoundSignalExpressionPointer::QQmlBoundSignalExpressionPointer(QQmlBoundSignalExpression *o)
: o(o)
{
    if (o) o->addref();
}

QQmlBoundSignalExpressionPointer::QQmlBoundSignalExpressionPointer(const QQmlBoundSignalExpressionPointer &other)
: o(other.o)
{
    if (o) o->addref();
}

QQmlBoundSignalExpressionPointer::~QQmlBoundSignalExpressionPointer()
{
    if (o) o->release();
}

QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::operator=(const QQmlBoundSignalExpressionPointer &other)
{
    if (other.o) other.o->addref();
    if (o) o->release();
    o = other.o;
    return *this;
}

QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::operator=(QQmlBoundSignalExpression *other)
{
    if (other) other->addref();
    if (o) o->release();
    o = other;
    return *this;
}

/*!
Takes ownership of \a other.  take() does *not* add a reference, as it assumes ownership
of the callers reference of other.
*/
QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::take(QQmlBoundSignalExpression *other)
{
    if (o) o->release();
    o = other;
    return *this;
}

QT_END_NAMESPACE
