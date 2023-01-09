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

#include "qqmlnativedebugservice.h"

#include <private/qqmldebugconnector_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4script_p.h>
#include <private/qv4string_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4identifier_p.h>
#include <private/qv4runtime_p.h>
#include <private/qversionedpacket_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qv4identifiertable_p.h>

#include <QtQml/qjsengine.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qvector.h>
#include <QtCore/qpointer.h>

//#define TRACE_PROTOCOL(s) qDebug() << s
#define TRACE_PROTOCOL(s)

QT_BEGIN_NAMESPACE

using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

class BreakPoint
{
public:
    BreakPoint() : id(-1), lineNumber(-1), enabled(false), ignoreCount(0), hitCount(0) {}
    bool isValid() const { return lineNumber >= 0 && !fileName.isEmpty(); }

    int id;
    int lineNumber;
    QString fileName;
    bool enabled;
    QString condition;
    int ignoreCount;

    int hitCount;
};

inline uint qHash(const BreakPoint &b, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(b.fileName, seed) ^ b.lineNumber;
}

inline bool operator==(const BreakPoint &a, const BreakPoint &b)
{
    return a.lineNumber == b.lineNumber && a.fileName == b.fileName
            && a.enabled == b.enabled && a.condition == b.condition
            && a.ignoreCount == b.ignoreCount;
}

static void setError(QJsonObject *response, const QString &msg)
{
    response->insert(QStringLiteral("type"), QStringLiteral("error"));
    response->insert(QStringLiteral("msg"), msg);
}

class NativeDebugger;

class Collector
{
public:
    Collector(QV4::ExecutionEngine *engine)
        : m_engine(engine), m_anonCount(0)
    {}

    void collect(QJsonArray *output, const QString &parentIName, const QString &name,
                 const QV4::Value &value);

    bool isExpanded(const QString &iname) const { return m_expanded.contains(iname); }

public:
    QV4::ExecutionEngine *m_engine;
    int m_anonCount;
    QStringList m_expanded;
};

// Encapsulate Breakpoint handling
// Could be made per-NativeDebugger (i.e. per execution engine, if needed)
class BreakPointHandler
{
public:
    BreakPointHandler() : m_haveBreakPoints(false), m_breakOnThrow(true), m_lastBreakpoint(1) {}

    void handleSetBreakpoint(QJsonObject *response, const QJsonObject &arguments);
    void handleRemoveBreakpoint(QJsonObject *response, const QJsonObject &arguments);

    void removeBreakPoint(int id);
    void enableBreakPoint(int id, bool onoff);

    void setBreakOnThrow(bool onoff);
    bool m_haveBreakPoints;
    bool m_breakOnThrow;
    int m_lastBreakpoint;
    QVector<BreakPoint> m_breakPoints;
};

void BreakPointHandler::handleSetBreakpoint(QJsonObject *response, const QJsonObject &arguments)
{
    TRACE_PROTOCOL("SET BREAKPOINT" << arguments);
    QString type = arguments.value(QLatin1String("type")).toString();

    QString fileName = arguments.value(QLatin1String("file")).toString();
    if (fileName.isEmpty()) {
        setError(response, QStringLiteral("breakpoint has no file name"));
        return;
    }

    int line = arguments.value(QLatin1String("line")).toInt(-1);
    if (line < 0) {
        setError(response, QStringLiteral("breakpoint has an invalid line number"));
        return;
    }

    BreakPoint bp;
    bp.id = m_lastBreakpoint++;
    bp.fileName = fileName.mid(fileName.lastIndexOf('/') + 1);
    bp.lineNumber = line;
    bp.enabled  = arguments.value(QLatin1String("enabled")).toBool(true);
    bp.condition = arguments.value(QLatin1String("condition")).toString();
    bp.ignoreCount = arguments.value(QLatin1String("ignorecount")).toInt();
    m_breakPoints.append(bp);

    m_haveBreakPoints = true;

    response->insert(QStringLiteral("type"), type);
    response->insert(QStringLiteral("breakpoint"), bp.id);
}

void BreakPointHandler::handleRemoveBreakpoint(QJsonObject *response, const QJsonObject &arguments)
{
    int id = arguments.value(QLatin1String("id")).toInt();
    removeBreakPoint(id);
    response->insert(QStringLiteral("id"), id);
}

class NativeDebugger : public QV4::Debugging::Debugger
{
public:
    NativeDebugger(QQmlNativeDebugServiceImpl *service, QV4::ExecutionEngine *engine);

    void signalEmitted(const QString &signal);

    QV4::ExecutionEngine *engine() const { return m_engine; }

    bool pauseAtNextOpportunity() const override {
        return m_pauseRequested
                || m_service->m_breakHandler->m_haveBreakPoints
                || m_stepping >= StepOver;
    }

    void maybeBreakAtInstruction() override;
    void enteringFunction() override;
    void leavingFunction(const QV4::ReturnedValue &retVal) override;
    void aboutToThrow() override;

    void handleCommand(QJsonObject *response, const QString &cmd, const QJsonObject &arguments);

private:
    void handleBacktrace(QJsonObject *response, const QJsonObject &arguments);
    void handleVariables(QJsonObject *response, const QJsonObject &arguments);
    void handleExpressions(QJsonObject *response, const QJsonObject &arguments);

    void handleDebuggerDeleted(QObject *debugger);

    QV4::ReturnedValue evaluateExpression(const QString &expression);
    bool checkCondition(const QString &expression);

    QStringList breakOnSignals;

    enum Speed {
        NotStepping = 0,
        StepOut,
        StepOver,
        StepIn,
    };

    void pauseAndWait();
    void pause();
    void handleContinue(QJsonObject *reponse, Speed speed);

    QV4::Function *getFunction() const;

    bool reallyHitTheBreakPoint(const QV4::Function *function, int lineNumber);

    QV4::ExecutionEngine *m_engine;
    QQmlNativeDebugServiceImpl *m_service;
    QV4::CppStackFrame *m_currentFrame = nullptr;
    Speed m_stepping;
    bool m_pauseRequested;
    bool m_runningJob;

    QV4::PersistentValue m_returnedValue;
};

bool NativeDebugger::checkCondition(const QString &expression)
{
    QV4::Scope scope(m_engine);
    QV4::ScopedValue r(scope, evaluateExpression(expression));
    return r->booleanValue();
}

QV4::ReturnedValue NativeDebugger::evaluateExpression(const QString &expression)
{
    QV4::Scope scope(m_engine);
    m_runningJob = true;

    QV4::ExecutionContext *ctx = m_engine->currentStackFrame ? m_engine->currentContext()
                                                             : m_engine->scriptContext();

    QV4::Script script(ctx, QV4::Compiler::ContextType::Eval, expression);
    if (const QV4::Function *function = m_engine->currentStackFrame
            ? m_engine->currentStackFrame->v4Function : m_engine->globalCode)
        script.strictMode = function->isStrict();
    // In order for property lookups in QML to work, we need to disable fast v4 lookups.
    // That is a side-effect of inheritContext.
    script.inheritContext = true;
    script.parse();
    if (!m_engine->hasException) {
        if (m_engine->currentStackFrame) {
            QV4::ScopedValue thisObject(scope, m_engine->currentStackFrame->thisObject());
            script.run(thisObject);
        } else {
            script.run();
        }
    }

    m_runningJob = false;
    return QV4::Encode::undefined();
}

NativeDebugger::NativeDebugger(QQmlNativeDebugServiceImpl *service, QV4::ExecutionEngine *engine)
    : m_returnedValue(engine, QV4::Value::undefinedValue())
{
    m_stepping = NotStepping;
    m_pauseRequested = false;
    m_runningJob = false;
    m_service = service;
    m_engine = engine;
    TRACE_PROTOCOL("Creating native debugger");
}

void NativeDebugger::signalEmitted(const QString &signal)
{
    //This function is only called by QQmlBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.

    //Parse just the name and remove the class info
    //Normalize to Lower case.
    QString signalName = signal.left(signal.indexOf(QLatin1Char('('))).toLower();

    for (const QString &signal : qAsConst(breakOnSignals)) {
        if (signal == signalName) {
            // TODO: pause debugger
            break;
        }
    }
}

void NativeDebugger::handleCommand(QJsonObject *response, const QString &cmd,
                                   const QJsonObject &arguments)
{
    if (cmd == QLatin1String("backtrace"))
        handleBacktrace(response, arguments);
    else if (cmd == QLatin1String("variables"))
        handleVariables(response, arguments);
    else if (cmd == QLatin1String("expressions"))
        handleExpressions(response, arguments);
    else if (cmd == QLatin1String("stepin"))
        handleContinue(response, StepIn);
    else if (cmd == QLatin1String("stepout"))
        handleContinue(response, StepOut);
    else if (cmd == QLatin1String("stepover"))
        handleContinue(response, StepOver);
    else if (cmd == QLatin1String("continue"))
        handleContinue(response, NotStepping);
}

static QString encodeFrame(QV4::CppStackFrame *f)
{
    QQmlDebugPacket ds;
    ds << quintptr(f);
    return QString::fromLatin1(ds.data().toHex());
}

static void decodeFrame(const QString &f, QV4::CppStackFrame **frame)
{
    quintptr rawFrame;
    QQmlDebugPacket ds(QByteArray::fromHex(f.toLatin1()));
    ds >> rawFrame;
    *frame = reinterpret_cast<QV4::CppStackFrame *>(rawFrame);
}

void NativeDebugger::handleBacktrace(QJsonObject *response, const QJsonObject &arguments)
{
    int limit = arguments.value(QLatin1String("limit")).toInt(0);

    QJsonArray frameArray;
    QV4::CppStackFrame *f= m_engine->currentStackFrame;
    for (int i  = 0; i < limit && f; ++i) {
        QV4::Function *function = f->v4Function;

        QJsonObject frame;
        frame.insert(QStringLiteral("language"), QStringLiteral("js"));
        frame.insert(QStringLiteral("context"), encodeFrame(f));

        if (QV4::Heap::String *functionName = function->name())
            frame.insert(QStringLiteral("function"), functionName->toQString());
        frame.insert(QStringLiteral("file"), function->sourceFile());

        int line = f->lineNumber();
        frame.insert(QStringLiteral("line"), (line < 0 ? -line : line));

        frameArray.push_back(frame);

        f = f->parent;
    }

    response->insert(QStringLiteral("frames"), frameArray);
}

void Collector::collect(QJsonArray *out, const QString &parentIName, const QString &name,
                        const QV4::Value &value)
{
    QJsonObject dict;
    QV4::Scope scope(m_engine);

    QString nonEmptyName = name.isEmpty() ? QString::fromLatin1("@%1").arg(m_anonCount++) : name;
    QString iname = parentIName + QLatin1Char('.') + nonEmptyName;
    dict.insert(QStringLiteral("iname"), iname);
    dict.insert(QStringLiteral("name"), nonEmptyName);

    QV4::ScopedValue typeString(scope, QV4::Runtime::TypeofValue::call(m_engine, value));
    dict.insert(QStringLiteral("type"), typeString->toQStringNoThrow());

    switch (value.type()) {
    case QV4::Value::Empty_Type:
        dict.insert(QStringLiteral("valueencoded"), QStringLiteral("empty"));
        dict.insert(QStringLiteral("haschild"), false);
        break;
    case QV4::Value::Undefined_Type:
        dict.insert(QStringLiteral("valueencoded"), QStringLiteral("undefined"));
        dict.insert(QStringLiteral("haschild"), false);
        break;
    case QV4::Value::Null_Type:
        dict.insert(QStringLiteral("type"), QStringLiteral("object"));
        dict.insert(QStringLiteral("valueencoded"), QStringLiteral("null"));
        dict.insert(QStringLiteral("haschild"), false);
        break;
    case QV4::Value::Boolean_Type:
        dict.insert(QStringLiteral("value"), value.booleanValue());
        dict.insert(QStringLiteral("haschild"), false);
        break;
    case QV4::Value::Managed_Type:
        if (const QV4::String *string = value.as<QV4::String>()) {
            dict.insert(QStringLiteral("value"), string->toQStringNoThrow());
            dict.insert(QStringLiteral("haschild"), false);
            dict.insert(QStringLiteral("valueencoded"), QStringLiteral("utf16"));
            dict.insert(QStringLiteral("quoted"), true);
        } else if (const QV4::ArrayObject *array = value.as<QV4::ArrayObject>()) {
            // The size of an array is number of its numerical properties.
            // We don't consider free form object properties here.
            const uint n = array->getLength();
            dict.insert(QStringLiteral("value"), qint64(n));
            dict.insert(QStringLiteral("valueencoded"), QStringLiteral("itemcount"));
            dict.insert(QStringLiteral("haschild"), qint64(n));
            if (isExpanded(iname)) {
                QJsonArray children;
                for (uint i = 0; i < n; ++i) {
                    QV4::ReturnedValue v = array->get(i);
                    QV4::ScopedValue sval(scope, v);
                    collect(&children, iname, QString::number(i), *sval);
                }
                dict.insert(QStringLiteral("children"), children);
            }
        } else if (const QV4::Object *object = value.as<QV4::Object>()) {
            QJsonArray children;
            bool expanded = isExpanded(iname);
            qint64 numProperties = 0;
            QV4::ObjectIterator it(scope, object, QV4::ObjectIterator::EnumerableOnly);
            QV4::ScopedProperty p(scope);
            QV4::ScopedPropertyKey name(scope);
            while (true) {
                QV4::PropertyAttributes attrs;
                name = it.next(p, &attrs);
                if (!name->isValid())
                    break;
                if (name->isStringOrSymbol()) {
                    ++numProperties;
                    if (expanded) {
                        QV4::Value v = p.property->value;
                        collect(&children, iname, name->toQString(), v);
                    }
                }
            }
            dict.insert(QStringLiteral("value"), numProperties);
            dict.insert(QStringLiteral("valueencoded"), QStringLiteral("itemcount"));
            dict.insert(QStringLiteral("haschild"), numProperties > 0);
            if (expanded)
                dict.insert(QStringLiteral("children"), children);
        }
        break;
    case QV4::Value::Integer_Type:
        dict.insert(QStringLiteral("value"), value.integerValue());
        dict.insert(QStringLiteral("haschild"), false);
        break;
    default: // double
        dict.insert(QStringLiteral("value"), value.doubleValue());
        dict.insert(QStringLiteral("haschild"), false);
    }

    out->append(dict);
}

void NativeDebugger::handleVariables(QJsonObject *response, const QJsonObject &arguments)
{
    TRACE_PROTOCOL("Build variables");
    QV4::CppStackFrame *frame = nullptr;
    decodeFrame(arguments.value(QLatin1String("context")).toString(), &frame);
    if (!frame) {
        setError(response, QStringLiteral("No stack frame passed"));
        return;
    }
    TRACE_PROTOCOL("Context: " << frame);

    QV4::ExecutionEngine *engine = frame->v4Function->internalClass->engine;
    if (!engine) {
        setError(response, QStringLiteral("No execution engine passed"));
        return;
    }
    TRACE_PROTOCOL("Engine: " << engine);

    Collector collector(engine);
    const QJsonArray expanded = arguments.value(QLatin1String("expanded")).toArray();
    for (const QJsonValue &ex : expanded)
        collector.m_expanded.append(ex.toString());
    TRACE_PROTOCOL("Expanded: " << collector.m_expanded);

    QJsonArray output;
    QV4::Scope scope(engine);

    QV4::ScopedValue thisObject(scope, frame->thisObject());
    collector.collect(&output, QString(), QStringLiteral("this"), thisObject);
    QV4::Scoped<QV4::CallContext> callContext(scope, frame->callContext());
    if (callContext) {
        QV4::Heap::InternalClass *ic = callContext->internalClass();
        QV4::ScopedValue v(scope);
        for (uint i = 0; i < ic->size; ++i) {
            QString name = ic->keyAt(i);
            v = callContext->d()->locals[i];
            collector.collect(&output, QString(), name, v);
        }
    }

    response->insert(QStringLiteral("variables"), output);
}

void NativeDebugger::handleExpressions(QJsonObject *response, const QJsonObject &arguments)
{
    TRACE_PROTOCOL("Evaluate expressions");
    QV4::CppStackFrame *frame = nullptr;
    decodeFrame(arguments.value(QLatin1String("context")).toString(), &frame);
    if (!frame) {
        setError(response, QStringLiteral("No stack frame passed"));
        return;
    }
    TRACE_PROTOCOL("Context: " << executionContext);

    QV4::ExecutionEngine *engine = frame->v4Function->internalClass->engine;
    if (!engine) {
        setError(response, QStringLiteral("No execution engine passed"));
        return;
    }
    TRACE_PROTOCOL("Engines: " << engine << m_engine);

    Collector collector(engine);
    const QJsonArray expanded = arguments.value(QLatin1String("expanded")).toArray();
    for (const QJsonValue &ex : expanded)
        collector.m_expanded.append(ex.toString());
    TRACE_PROTOCOL("Expanded: " << collector.m_expanded);

    QJsonArray output;
    QV4::Scope scope(engine);

    const QJsonArray expressions = arguments.value(QLatin1String("expressions")).toArray();
    for (const QJsonValue &expr : expressions) {
        QString expression = expr.toObject().value(QLatin1String("expression")).toString();
        QString name = expr.toObject().value(QLatin1String("name")).toString();
        TRACE_PROTOCOL("Evaluate expression: " << expression);
        m_runningJob = true;

        QV4::ScopedValue result(scope, evaluateExpression(expression));

        m_runningJob = false;
        if (result->isUndefined()) {
            QJsonObject dict;
            dict.insert(QStringLiteral("name"), name);
            dict.insert(QStringLiteral("valueencoded"), QStringLiteral("undefined"));
            output.append(dict);
        } else if (result.ptr && result.ptr->rawValue()) {
            collector.collect(&output, QString(), name, *result);
        } else {
            QJsonObject dict;
            dict.insert(QStringLiteral("name"), name);
            dict.insert(QStringLiteral("valueencoded"), QStringLiteral("notaccessible"));
            output.append(dict);
        }
        TRACE_PROTOCOL("EXCEPTION: " << engine->hasException);
        engine->hasException = false;
    }

    response->insert(QStringLiteral("expressions"), output);
}

void BreakPointHandler::removeBreakPoint(int id)
{
    for (int i = 0; i != m_breakPoints.size(); ++i) {
        if (m_breakPoints.at(i).id == id) {
            m_breakPoints.remove(i);
            m_haveBreakPoints = !m_breakPoints.isEmpty();
            return;
        }
    }
}

void BreakPointHandler::enableBreakPoint(int id, bool enabled)
{
    m_breakPoints[id].enabled = enabled;
}

void NativeDebugger::pause()
{
    m_pauseRequested = true;
}

void NativeDebugger::handleContinue(QJsonObject *response, Speed speed)
{
    Q_UNUSED(response);

    if (!m_returnedValue.isUndefined())
        m_returnedValue.set(m_engine, QV4::Encode::undefined());

    m_currentFrame = m_engine->currentStackFrame;
    m_stepping = speed;
}

void NativeDebugger::maybeBreakAtInstruction()
{
    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    if (m_stepping == StepOver) {
        if (m_currentFrame == m_engine->currentStackFrame)
            pauseAndWait();
        return;
    }

    if (m_stepping == StepIn) {
        pauseAndWait();
        return;
    }

    if (m_pauseRequested) { // Serve debugging requests from the agent
        m_pauseRequested = false;
        pauseAndWait();
        return;
    }

    if (m_service->m_breakHandler->m_haveBreakPoints) {
        if (QV4::Function *function = getFunction()) {
            // lineNumber will be negative for Ret instructions, so those won't match
            const int lineNumber = m_engine->currentStackFrame->lineNumber();
            if (reallyHitTheBreakPoint(function, lineNumber))
                pauseAndWait();
        }
    }
}

void NativeDebugger::enteringFunction()
{
    if (m_runningJob)
        return;

    if (m_stepping == StepIn) {
        m_currentFrame = m_engine->currentStackFrame;
    }
}

void NativeDebugger::leavingFunction(const QV4::ReturnedValue &retVal)
{
    if (m_runningJob)
        return;

    if (m_stepping != NotStepping && m_currentFrame == m_engine->currentStackFrame) {
        m_currentFrame = m_currentFrame->parent;
        m_stepping = StepOver;
        m_returnedValue.set(m_engine, retVal);
    }
}

void NativeDebugger::aboutToThrow()
{
    if (!m_service->m_breakHandler->m_breakOnThrow)
        return;

    if (m_runningJob) // do not re-enter when we're doing a job for the debugger.
        return;

    QJsonObject event;
    // TODO: complete this!
    event.insert(QStringLiteral("event"), QStringLiteral("exception"));
    m_service->emitAsynchronousMessageToClient(event);
}

QV4::Function *NativeDebugger::getFunction() const
{
    if (m_engine->currentStackFrame)
        return m_engine->currentStackFrame->v4Function;
    else
        return m_engine->globalCode;
}

void NativeDebugger::pauseAndWait()
{
    QJsonObject event;

    event.insert(QStringLiteral("event"), QStringLiteral("break"));
    event.insert(QStringLiteral("language"), QStringLiteral("js"));
    if (QV4::CppStackFrame *frame = m_engine->currentStackFrame) {
        QV4::Function *function = frame->v4Function;
        event.insert(QStringLiteral("file"), function->sourceFile());
        int line = frame->lineNumber();
        event.insert(QStringLiteral("line"), (line < 0 ? -line : line));
    }

    m_service->emitAsynchronousMessageToClient(event);
}

bool NativeDebugger::reallyHitTheBreakPoint(const QV4::Function *function, int lineNumber)
{
    for (int i = 0, n = m_service->m_breakHandler->m_breakPoints.size(); i != n; ++i) {
        const BreakPoint &bp = m_service->m_breakHandler->m_breakPoints.at(i);
        if (bp.lineNumber == lineNumber) {
            const QString base = QUrl(function->sourceFile()).fileName();
            if (bp.fileName.endsWith(base)) {
                if (bp.condition.isEmpty() || checkCondition(bp.condition)) {
                    BreakPoint &mbp = m_service->m_breakHandler->m_breakPoints[i];
                    ++mbp.hitCount;
                    if (mbp.hitCount > mbp.ignoreCount)
                        return true;
                }
            }
        }
    }
    return false;
}

QQmlNativeDebugServiceImpl::QQmlNativeDebugServiceImpl(QObject *parent)
    : QQmlNativeDebugService(1.0, parent)
{
    m_breakHandler = new BreakPointHandler;
}

QQmlNativeDebugServiceImpl::~QQmlNativeDebugServiceImpl()
{
    delete m_breakHandler;
}

void QQmlNativeDebugServiceImpl::engineAboutToBeAdded(QJSEngine *engine)
{
    TRACE_PROTOCOL("Adding engine" << engine);
    if (engine) {
        QV4::ExecutionEngine *ee = engine->handle();
        TRACE_PROTOCOL("Adding execution engine" << ee);
        if (ee) {
            NativeDebugger *debugger = new NativeDebugger(this, ee);
            if (state() == Enabled)
                ee->setDebugger(debugger);
            m_debuggers.append(QPointer<NativeDebugger>(debugger));
        }
    }
    QQmlDebugService::engineAboutToBeAdded(engine);
}

void QQmlNativeDebugServiceImpl::engineAboutToBeRemoved(QJSEngine *engine)
{
    TRACE_PROTOCOL("Removing engine" << engine);
    if (engine) {
        QV4::ExecutionEngine *executionEngine = engine->handle();
        const auto debuggersCopy = m_debuggers;
        for (NativeDebugger *debugger : debuggersCopy) {
            if (debugger->engine() == executionEngine)
                m_debuggers.removeAll(debugger);
        }
    }
    QQmlDebugService::engineAboutToBeRemoved(engine);
}

void QQmlNativeDebugServiceImpl::stateAboutToBeChanged(QQmlDebugService::State state)
{
    if (state == Enabled) {
        for (NativeDebugger *debugger : qAsConst(m_debuggers)) {
            QV4::ExecutionEngine *engine = debugger->engine();
            if (!engine->debugger())
                engine->setDebugger(debugger);
        }
    }
    QQmlDebugService::stateAboutToBeChanged(state);
}

void QQmlNativeDebugServiceImpl::messageReceived(const QByteArray &message)
{
    TRACE_PROTOCOL("Native message received: " << message);
    QJsonObject request = QJsonDocument::fromJson(message).object();
    QJsonObject response;
    QJsonObject arguments = request.value(QLatin1String("arguments")).toObject();
    QString cmd = request.value(QLatin1String("command")).toString();

    if (cmd == QLatin1String("setbreakpoint")) {
        m_breakHandler->handleSetBreakpoint(&response, arguments);
    } else if (cmd == QLatin1String("removebreakpoint")) {
        m_breakHandler->handleRemoveBreakpoint(&response, arguments);
    } else if (cmd == QLatin1String("echo")) {
        response.insert(QStringLiteral("result"), arguments);
    } else {
        for (NativeDebugger *debugger : qAsConst(m_debuggers))
            if (debugger)
                debugger->handleCommand(&response, cmd, arguments);
    }
    QJsonDocument doc;
    doc.setObject(response);
    QByteArray ba = doc.toJson(QJsonDocument::Compact);
    TRACE_PROTOCOL("Sending synchronous response:" << ba.constData() << endl);
    emit messageToClient(s_key, ba);
}

void QQmlNativeDebugServiceImpl::emitAsynchronousMessageToClient(const QJsonObject &message)
{
    QJsonDocument doc;
    doc.setObject(message);
    QByteArray ba = doc.toJson(QJsonDocument::Compact);
    TRACE_PROTOCOL("Sending asynchronous message:" << ba.constData() << endl);
    emit messageToClient(s_key, ba);
}

QT_END_NAMESPACE
