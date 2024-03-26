// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4debuggeragent.h"
#include "qv4debugservice.h"
#include "qv4datacollector.h"

#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

QT_BEGIN_NAMESPACE

QV4Debugger *QV4DebuggerAgent::pausedDebugger() const
{
    for (QV4Debugger *debugger : m_debuggers) {
        if (debugger->state() == QV4Debugger::Paused)
            return debugger;
    }
    return nullptr;
}

bool QV4DebuggerAgent::isRunning() const
{
    // "running" means none of the engines are paused.
    return pausedDebugger() == nullptr;
}

void QV4DebuggerAgent::debuggerPaused(QV4Debugger *debugger, QV4Debugger::PauseReason reason)
{
    Q_UNUSED(reason);

    debugger->collector()->clear();

    QJsonObject event, body, script;
    event.insert(QStringLiteral("type"), QStringLiteral("event"));

    switch (reason) {
    case QV4Debugger::Step:
    case QV4Debugger::PauseRequest:
    case QV4Debugger::BreakPointHit: {
        event.insert(QStringLiteral("event"), QStringLiteral("break"));
        QV4::CppStackFrame *frame = debugger->engine()->currentStackFrame;
        if (!frame)
            break;

        body.insert(QStringLiteral("invocationText"), frame->function());
        body.insert(QStringLiteral("sourceLine"), qAbs(frame->lineNumber()) - 1);
//        if (frame->column > 0)
//            body.insert(QStringLiteral("sourceColumn"), frame->column);
        QJsonArray breakPoints;
        foreach (int breakPointId, breakPointIds(frame->source(), frame->lineNumber()))
            breakPoints.push_back(breakPointId);
        body.insert(QStringLiteral("breakpoints"), breakPoints);
        script.insert(QStringLiteral("name"), frame->source());
    } break;
    case QV4Debugger::Throwing:
        // TODO: complete this!
        event.insert(QStringLiteral("event"), QStringLiteral("exception"));
        break;
    }

    if (!script.isEmpty())
        body.insert(QStringLiteral("script"), script);
    if (!body.isEmpty())
        event.insert(QStringLiteral("body"), body);
    m_debugService->send(event);
}

void QV4DebuggerAgent::addDebugger(QV4Debugger *debugger)
{
    Q_ASSERT(!m_debuggers.contains(debugger));
    m_debuggers << debugger;

    debugger->setBreakOnThrow(m_breakOnThrow);

    for (const BreakPoint &breakPoint : std::as_const(m_breakPoints))
        if (breakPoint.enabled)
            debugger->addBreakPoint(breakPoint.fileName, breakPoint.lineNr, breakPoint.condition);

    connect(debugger, &QObject::destroyed, this, &QV4DebuggerAgent::handleDebuggerDeleted);
    connect(debugger, &QV4Debugger::debuggerPaused, this, &QV4DebuggerAgent::debuggerPaused,
            Qt::QueuedConnection);
}

void QV4DebuggerAgent::removeDebugger(QV4Debugger *debugger)
{
    m_debuggers.removeAll(debugger);
    disconnect(debugger, &QObject::destroyed, this, &QV4DebuggerAgent::handleDebuggerDeleted);
    disconnect(debugger, &QV4Debugger::debuggerPaused, this, &QV4DebuggerAgent::debuggerPaused);
}

const QList<QV4Debugger *> &QV4DebuggerAgent::debuggers()
{
    return m_debuggers;
}

void QV4DebuggerAgent::handleDebuggerDeleted(QObject *debugger)
{
    m_debuggers.removeAll(static_cast<QV4Debugger *>(debugger));
}

void QV4DebuggerAgent::pause(QV4Debugger *debugger) const
{
    debugger->pause();
}

void QV4DebuggerAgent::pauseAll() const
{
    for (QV4Debugger *debugger : m_debuggers)
        pause(debugger);
}

void QV4DebuggerAgent::resumeAll() const
{
    for (QV4Debugger *debugger : m_debuggers)
        if (debugger->state() == QV4Debugger::Paused)
            debugger->resume(QV4Debugger::FullThrottle);
}

int QV4DebuggerAgent::addBreakPoint(const QString &fileName, int lineNumber, bool enabled, const QString &condition)
{
    if (enabled) {
        for (QV4Debugger *debugger : std::as_const(m_debuggers))
            debugger->addBreakPoint(fileName, lineNumber, condition);
    }

    const int id = ++m_lastBreakPointId;
    m_breakPoints.insert(id, BreakPoint(fileName, lineNumber, enabled, condition));
    return id;
}

void QV4DebuggerAgent::removeBreakPoint(int id)
{
    BreakPoint breakPoint = m_breakPoints.value(id);
    if (!breakPoint.isValid())
        return;

    m_breakPoints.remove(id);

    if (breakPoint.enabled)
        for (QV4Debugger *debugger : std::as_const(m_debuggers))
            debugger->removeBreakPoint(breakPoint.fileName, breakPoint.lineNr);
}

void QV4DebuggerAgent::removeAllBreakPoints()
{
    for (auto it = m_breakPoints.keyBegin(), end = m_breakPoints.keyEnd(); it != end; ++it)
        removeBreakPoint(*it);
}

void QV4DebuggerAgent::enableBreakPoint(int id, bool onoff)
{
    BreakPoint &breakPoint = m_breakPoints[id];
    if (!breakPoint.isValid() || breakPoint.enabled == onoff)
        return;
    breakPoint.enabled = onoff;

    for (QV4Debugger *debugger : std::as_const(m_debuggers)) {
        if (onoff)
            debugger->addBreakPoint(breakPoint.fileName, breakPoint.lineNr, breakPoint.condition);
        else
            debugger->removeBreakPoint(breakPoint.fileName, breakPoint.lineNr);
    }
}

QList<int> QV4DebuggerAgent::breakPointIds(const QString &fileName, int lineNumber) const
{
    QList<int> ids;

    for (QHash<int, BreakPoint>::const_iterator i = m_breakPoints.begin(), ei = m_breakPoints.end(); i != ei; ++i)
        if (i->lineNr == lineNumber && fileName.endsWith(i->fileName))
            ids.push_back(i.key());

    return ids;
}

void QV4DebuggerAgent::setBreakOnThrow(bool onoff)
{
    if (onoff != m_breakOnThrow) {
        m_breakOnThrow = onoff;
        for (QV4Debugger *debugger : std::as_const(m_debuggers))
            debugger->setBreakOnThrow(onoff);
    }
}

void QV4DebuggerAgent::clearAllPauseRequests()
{
    for (QV4Debugger *debugger : std::as_const(m_debuggers))
        debugger->clearPauseRequest();
}

QT_END_NAMESPACE

#include "moc_qv4debuggeragent.cpp"
