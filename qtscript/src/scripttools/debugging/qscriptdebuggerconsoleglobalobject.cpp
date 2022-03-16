/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSCriptTools module of the Qt Toolkit.
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

#include "qscriptdebuggerconsoleglobalobject_p.h"
#include "qscriptdebuggercommandschedulerinterface_p.h"
#include "qscriptdebuggercommandschedulerfrontend_p.h"
#include "qscriptmessagehandlerinterface_p.h"
#include "qscriptdebuggerconsole_p.h"
#include "qscriptdebuggerconsolecommandmanager_p.h"

#include <private/qobject_p.h>

#include <QtScript/qscriptengine.h>

QT_BEGIN_NAMESPACE

class QScriptDebuggerConsoleGlobalObjectPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QScriptDebuggerConsoleGlobalObject)
public:
    QScriptDebuggerConsoleGlobalObjectPrivate();
    ~QScriptDebuggerConsoleGlobalObjectPrivate();

    QScriptDebuggerCommandSchedulerInterface *scheduler;
    QScriptDebuggerResponseHandlerInterface *responseHandler;
    QScriptMessageHandlerInterface *messageHandler;
    QScriptDebuggerConsole *console;
};

QScriptDebuggerConsoleGlobalObjectPrivate::QScriptDebuggerConsoleGlobalObjectPrivate()
{
    scheduler = 0;
    responseHandler = 0;
    messageHandler = 0;
    console = 0;
}

QScriptDebuggerConsoleGlobalObjectPrivate::~QScriptDebuggerConsoleGlobalObjectPrivate()
{
}

QScriptDebuggerConsoleGlobalObject::QScriptDebuggerConsoleGlobalObject(QObject *parent)
    : QObject(*new QScriptDebuggerConsoleGlobalObjectPrivate, parent)
{
}

QScriptDebuggerConsoleGlobalObject::~QScriptDebuggerConsoleGlobalObject()
{
}

QScriptDebuggerCommandSchedulerInterface *QScriptDebuggerConsoleGlobalObject::scheduler() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->scheduler;
}

void QScriptDebuggerConsoleGlobalObject::setScheduler(QScriptDebuggerCommandSchedulerInterface *scheduler)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->scheduler = scheduler;
}

QScriptDebuggerResponseHandlerInterface *QScriptDebuggerConsoleGlobalObject::responseHandler() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->responseHandler;
}

void QScriptDebuggerConsoleGlobalObject::setResponseHandler(
    QScriptDebuggerResponseHandlerInterface *responseHandler)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->responseHandler = responseHandler;
}

QScriptMessageHandlerInterface *QScriptDebuggerConsoleGlobalObject::messageHandler() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->messageHandler;
}

void QScriptDebuggerConsoleGlobalObject::setMessageHandler(QScriptMessageHandlerInterface *messageHandler)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->messageHandler = messageHandler;
}

QScriptDebuggerConsole *QScriptDebuggerConsoleGlobalObject::console() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console;
}

void QScriptDebuggerConsoleGlobalObject::setConsole(QScriptDebuggerConsole *console)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->console = console;
}

// ### the scheduleXXX functions could take a callback function as argument (rather than using the
// global handleResponse() function)

int QScriptDebuggerConsoleGlobalObject::scheduleInterrupt()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleInterrupt();
}

int QScriptDebuggerConsoleGlobalObject::scheduleContinue()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleContinue();
}

int QScriptDebuggerConsoleGlobalObject::scheduleStepInto(int count)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleStepInto(count);
}

int QScriptDebuggerConsoleGlobalObject::scheduleStepOver(int count)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleStepOver(count);
}

int QScriptDebuggerConsoleGlobalObject::scheduleStepOut()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleStepOut();
}

int QScriptDebuggerConsoleGlobalObject::scheduleRunToLocation(const QString &fileName, int lineNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleRunToLocation(fileName, lineNumber);
}

int QScriptDebuggerConsoleGlobalObject::scheduleRunToLocation(qint64 scriptId, int lineNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleRunToLocation(scriptId, lineNumber);
}

int QScriptDebuggerConsoleGlobalObject::scheduleForceReturn(int contextIndex, const QScriptDebuggerValue &value)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleForceReturn(contextIndex, value);
}

int QScriptDebuggerConsoleGlobalObject::scheduleSetBreakpoint(const QScriptBreakpointData &data)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleSetBreakpoint(data);
}

int QScriptDebuggerConsoleGlobalObject::scheduleDeleteBreakpoint(int id)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleDeleteBreakpoint(id);
}

int QScriptDebuggerConsoleGlobalObject::scheduleDeleteAllBreakpoints()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleDeleteAllBreakpoints();
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetBreakpoints()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetBreakpoints();
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetBreakpointData(int id)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetBreakpointData(id);
}

int QScriptDebuggerConsoleGlobalObject::scheduleSetBreakpointData(int id, const QScriptBreakpointData &data)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleSetBreakpointData(id, data);
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetScripts()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetScripts();
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetScriptData(qint64 id)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetScriptData(id);
}

int QScriptDebuggerConsoleGlobalObject::scheduleScriptsCheckpoint()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleScriptsCheckpoint();
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetScriptsDelta()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetScriptsDelta();
}

int QScriptDebuggerConsoleGlobalObject::scheduleResolveScript(const QString &fileName)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleResolveScript(fileName);
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetBacktrace()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetBacktrace();
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetThisObject(int contextIndex)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetThisObject(contextIndex);
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetActivationObject(int contextIndex)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetActivationObject(contextIndex);
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetContextCount()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetContextCount();
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetContextInfo(int contextIndex)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetContextInfo(contextIndex);
}

int QScriptDebuggerConsoleGlobalObject::scheduleNewScriptValueIterator(const QScriptDebuggerValue &object)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleNewScriptValueIterator(object);
}

int QScriptDebuggerConsoleGlobalObject::scheduleGetPropertiesByIterator(int id, int count)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleGetPropertiesByIterator(id, count);
}

int QScriptDebuggerConsoleGlobalObject::scheduleDeleteScriptValueIterator(int id)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleDeleteScriptValueIterator(id);
}

int QScriptDebuggerConsoleGlobalObject::scheduleEvaluate(int contextIndex, const QString &program,
                                                         const QString &fileName,
                                                         int lineNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleEvaluate(contextIndex, program, fileName, lineNumber);
}

int QScriptDebuggerConsoleGlobalObject::scheduleScriptValueToString(const QScriptDebuggerValue &value)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleScriptValueToString(value);
}

int QScriptDebuggerConsoleGlobalObject::scheduleClearExceptions()
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    QScriptDebuggerCommandSchedulerFrontend frontend(d->scheduler, d->responseHandler);
    return frontend.scheduleClearExceptions();
}

int QScriptDebuggerConsoleGlobalObject::scheduleCommand(const QScriptDebuggerCommand &command)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    return d->scheduler->scheduleCommand(command, d->responseHandler);
}

void QScriptDebuggerConsoleGlobalObject::warning(const QString &text,
                                                 const QString &fileName,
                                                 int lineNumber, int columnNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    Q_ASSERT(d->messageHandler != 0);
    d->messageHandler->message(QtWarningMsg, text, fileName, lineNumber, columnNumber);
}

void QScriptDebuggerConsoleGlobalObject::message(const QString &text,
                                                 const QString &fileName,
                                                 int lineNumber, int columnNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    Q_ASSERT(d->messageHandler != 0);
    d->messageHandler->message(QtDebugMsg, text, fileName, lineNumber, columnNumber);
}

void QScriptDebuggerConsoleGlobalObject::error(const QString &text,
                                               const QString &fileName,
                                               int lineNumber, int columnNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    Q_ASSERT(d->messageHandler != 0);
    d->messageHandler->message(QtCriticalMsg, text, fileName, lineNumber, columnNumber);
}

int QScriptDebuggerConsoleGlobalObject::getCurrentFrameIndex() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->currentFrameIndex();
}

void QScriptDebuggerConsoleGlobalObject::setCurrentFrameIndex(int index)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->console->setCurrentFrameIndex(index);
}

int QScriptDebuggerConsoleGlobalObject::getCurrentLineNumber() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->currentLineNumber();
}

void QScriptDebuggerConsoleGlobalObject::setCurrentLineNumber(int lineNumber)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->console->setCurrentLineNumber(lineNumber);
}

qint64 QScriptDebuggerConsoleGlobalObject::getCurrentScriptId() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->currentScriptId();
}

void QScriptDebuggerConsoleGlobalObject::setCurrentScriptId(qint64 id)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->console->setCurrentScriptId(id);
}

qint64 QScriptDebuggerConsoleGlobalObject::getSessionId() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->sessionId();
}

QScriptDebuggerConsoleCommandGroupMap QScriptDebuggerConsoleGlobalObject::getCommandGroups() const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->commandManager()->commandGroups();
}

QScriptDebuggerConsoleCommand *QScriptDebuggerConsoleGlobalObject::findCommand(const QString &name) const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->commandManager()->findCommand(name);
}

QScriptDebuggerConsoleCommandList QScriptDebuggerConsoleGlobalObject::getCommandsInGroup(const QString &name) const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->commandManager()->commandsInGroup(name);
}

QStringList QScriptDebuggerConsoleGlobalObject::getCommandCompletions(const QString &prefix) const
{
    Q_D(const QScriptDebuggerConsoleGlobalObject);
    return d->console->commandManager()->completions(prefix);
}

bool QScriptDebuggerConsoleGlobalObject::checkSyntax(const QString &program)
{
    return (QScriptEngine::checkSyntax(program).state() == QScriptSyntaxCheckResult::Valid);
}

void QScriptDebuggerConsoleGlobalObject::setEvaluateAction(int action)
{
    Q_D(QScriptDebuggerConsoleGlobalObject);
    d->console->setEvaluateAction(action);
}

QT_END_NAMESPACE
