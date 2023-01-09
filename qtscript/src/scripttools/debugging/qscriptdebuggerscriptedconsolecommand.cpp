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

#include "qscriptdebuggerscriptedconsolecommand_p.h"
#include "qscriptdebuggerconsolecommand_p_p.h"
#include "qscriptdebuggerconsolecommandjob_p.h"
#include "qscriptdebuggerconsolecommandjob_p_p.h"
#include "qscriptmessagehandlerinterface_p.h"
#include "qscriptdebuggerconsoleglobalobject_p.h"
#include "qscriptdebuggerresponse_p.h"
#include "qscriptdebuggercommandschedulerinterface_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtScript/qscriptengine.h>
#include <QtScript/qscriptvalue.h>
#include <QtScript/qscriptvalueiterator.h>
#include <QtScript/qscriptcontextinfo.h>
#include <QtCore/qdebug.h>

Q_DECLARE_METATYPE(QScriptDebuggerResponse)

QT_BEGIN_NAMESPACE

/*!
  \since 4.5
  \class QScriptDebuggerScriptedConsoleCommand
  \internal

  \brief The QScriptDebuggerScriptedConsoleCommand class encapsulates a command defined in a script.
*/

class QScriptDebuggerScriptedConsoleCommandPrivate
    : public QScriptDebuggerConsoleCommandPrivate
{
    Q_DECLARE_PUBLIC(QScriptDebuggerScriptedConsoleCommand)
public:
    QScriptDebuggerScriptedConsoleCommandPrivate();
    ~QScriptDebuggerScriptedConsoleCommandPrivate();

    QString name;
    QString group;
    QString shortDescription;
    QString longDescription;
    QStringList aliases;
    QStringList seeAlso;
    QStringList argumentTypes;
    QStringList subCommands;
    QScriptValue globalObject;
    QScriptValue execFunction;
    QScriptValue responseFunction;
};

QScriptDebuggerScriptedConsoleCommandPrivate::QScriptDebuggerScriptedConsoleCommandPrivate()
{
}

QScriptDebuggerScriptedConsoleCommandPrivate::~QScriptDebuggerScriptedConsoleCommandPrivate()
{
}

QScriptDebuggerScriptedConsoleCommand::QScriptDebuggerScriptedConsoleCommand(
    const QString &name, const QString &group,
    const QString &shortDescription, const QString &longDescription,
    const QStringList &aliases, const QStringList &seeAlso,
    const QStringList &argumentTypes, const QStringList &subCommands,
    const QScriptValue &globalObject,
    const QScriptValue &execFunction, const QScriptValue &responseFunction)
    : QScriptDebuggerConsoleCommand(*new QScriptDebuggerScriptedConsoleCommandPrivate)
{
    Q_D(QScriptDebuggerScriptedConsoleCommand);
    d->name = name;
    d->group = group;
    d->shortDescription = shortDescription;
    d->longDescription = longDescription;
    d->aliases = aliases;
    d->seeAlso = seeAlso;
    d->argumentTypes = argumentTypes;
    d->subCommands = subCommands;
    d->globalObject = globalObject;
    d->execFunction = execFunction;
    d->responseFunction = responseFunction;
}

QScriptDebuggerScriptedConsoleCommand::~QScriptDebuggerScriptedConsoleCommand()
{
}

class QScriptDebuggerScriptedConsoleCommandJobPrivate;
class QScriptDebuggerScriptedConsoleCommandJob
    : public QScriptDebuggerConsoleCommandJob,
      public QScriptDebuggerCommandSchedulerInterface
{
public:
    QScriptDebuggerScriptedConsoleCommandJob(
        QScriptDebuggerScriptedConsoleCommandPrivate *command,
        const QStringList &arguments,
        QScriptDebuggerConsole *console,
        QScriptMessageHandlerInterface *messageHandler,
        QScriptDebuggerCommandSchedulerInterface *commandScheduler);
    ~QScriptDebuggerScriptedConsoleCommandJob();

    int scheduleCommand(
        const QScriptDebuggerCommand &command,
        QScriptDebuggerResponseHandlerInterface *responseHandler);

    void start();
    void handleResponse(const QScriptDebuggerResponse &response,
                        int commandId);

private:
    Q_DECLARE_PRIVATE(QScriptDebuggerScriptedConsoleCommandJob)
    Q_DISABLE_COPY(QScriptDebuggerScriptedConsoleCommandJob)
};

class QScriptDebuggerScriptedConsoleCommandJobPrivate
    : public QScriptDebuggerConsoleCommandJobPrivate
{
public:
    QScriptDebuggerScriptedConsoleCommandJobPrivate() : command(0), commandCount(0) {}
    ~QScriptDebuggerScriptedConsoleCommandJobPrivate() {}

    QScriptDebuggerScriptedConsoleCommandPrivate *command;
    QStringList arguments;
    int commandCount;
};

QScriptDebuggerScriptedConsoleCommandJob::QScriptDebuggerScriptedConsoleCommandJob(
    QScriptDebuggerScriptedConsoleCommandPrivate *command,
    const QStringList &arguments,
    QScriptDebuggerConsole *console,
    QScriptMessageHandlerInterface *messageHandler,
    QScriptDebuggerCommandSchedulerInterface *commandScheduler)
    : QScriptDebuggerConsoleCommandJob(*new QScriptDebuggerScriptedConsoleCommandJobPrivate,
                                       console, messageHandler, commandScheduler)
{
    Q_D(QScriptDebuggerScriptedConsoleCommandJob);
    d->command = command;
    d->arguments = arguments;
}

QScriptDebuggerScriptedConsoleCommandJob::~QScriptDebuggerScriptedConsoleCommandJob()
{
}

int QScriptDebuggerScriptedConsoleCommandJob::scheduleCommand(
    const QScriptDebuggerCommand &command,
    QScriptDebuggerResponseHandlerInterface *responseHandler)
{
    Q_D(QScriptDebuggerScriptedConsoleCommandJob);
    ++d->commandCount;
    return commandScheduler()->scheduleCommand(command, responseHandler);
}

void QScriptDebuggerScriptedConsoleCommandJob::start()
{
    Q_D(QScriptDebuggerScriptedConsoleCommandJob);
    QScriptEngine *engine = d->command->globalObject.engine();
    engine->setGlobalObject(d->command->globalObject);
    QScriptValueList args;
    for (int i = 0; i < d->arguments.size(); ++i)
        args.append(QScriptValue(engine, d->arguments.at(i)));
    QScriptDebuggerConsoleGlobalObject *global;
    global = qobject_cast<QScriptDebuggerConsoleGlobalObject*>(engine->globalObject().toQObject());
    Q_ASSERT(global != 0);
    global->setScheduler(this);
    global->setResponseHandler(this);
    global->setMessageHandler(d->messageHandler);
    global->setConsole(d->console);
    d->commandCount = 0;
    QScriptValue ret = d->command->execFunction.call(QScriptValue(), args);
    global->setScheduler(0);
    global->setResponseHandler(0);
    global->setMessageHandler(0);
    global->setConsole(0);
    if (ret.isError()) {
        qWarning("*** internal error: %s", qPrintable(ret.toString()));
    }
    if (d->commandCount == 0)
        finish();
}

void QScriptDebuggerScriptedConsoleCommandJob::handleResponse(
    const QScriptDebuggerResponse &response,
    int commandId)
{
    Q_D(QScriptDebuggerScriptedConsoleCommandJob);
    // ### generalize
    QScriptEngine *engine = d->command->globalObject.engine();
    engine->setGlobalObject(d->command->globalObject);
    QScriptValueList args;
    args.append(engine->toScriptValue(response));
    args.append(QScriptValue(engine, commandId));
    QScriptDebuggerConsoleGlobalObject *global;
    global = qobject_cast<QScriptDebuggerConsoleGlobalObject*>(d->command->globalObject.toQObject());
    Q_ASSERT(global != 0);
    global->setScheduler(this);
    global->setResponseHandler(this);
    global->setMessageHandler(d->messageHandler);
    global->setConsole(d->console);
    d->commandCount = 0;
    QScriptValue ret = d->command->responseFunction.call(QScriptValue(), args);
    global->setScheduler(0);
    global->setResponseHandler(0);
    global->setMessageHandler(0);
    global->setConsole(0);
    if (ret.isError()) {
        qWarning("*** internal error: %s", qPrintable(ret.toString()));
    }
    if (d->commandCount == 0)
        finish();
}

/*!
  \internal
*/
QString QScriptDebuggerScriptedConsoleCommand::name() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->name;
}

/*!
  \internal
*/
QString QScriptDebuggerScriptedConsoleCommand::group() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->group;
}

/*!
  \internal
*/
QString QScriptDebuggerScriptedConsoleCommand::shortDescription() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->shortDescription;
}

/*!
  \internal
*/
QString QScriptDebuggerScriptedConsoleCommand::longDescription() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->longDescription;
}

/*!
  \internal
*/
QStringList QScriptDebuggerScriptedConsoleCommand::aliases() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->aliases;
}

/*!
  \internal
*/
QStringList QScriptDebuggerScriptedConsoleCommand::seeAlso() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->seeAlso;
}

/*!
  \internal
*/
QStringList QScriptDebuggerScriptedConsoleCommand::argumentTypes() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->argumentTypes;
}

/*!
  \internal
*/
QStringList QScriptDebuggerScriptedConsoleCommand::subCommands() const
{
    Q_D(const QScriptDebuggerScriptedConsoleCommand);
    return d->subCommands;
}

/*!
  \internal
*/
QScriptDebuggerConsoleCommandJob *QScriptDebuggerScriptedConsoleCommand::createJob(
    const QStringList &arguments,
    QScriptDebuggerConsole *console,
    QScriptMessageHandlerInterface *messageHandler,
    QScriptDebuggerCommandSchedulerInterface *commandScheduler)
{
    Q_D(QScriptDebuggerScriptedConsoleCommand);
    return new QScriptDebuggerScriptedConsoleCommandJob(
        d, arguments, console, messageHandler, commandScheduler);
}

/*!
  Parses a command defined by the given \a program.
  Returns an object that encapsulates the command, or 0 if parsing failed.
*/
QScriptDebuggerScriptedConsoleCommand *QScriptDebuggerScriptedConsoleCommand::parse(
    const QString &program, const QString &fileName,
    QScriptEngine *engine, QScriptMessageHandlerInterface *messageHandler)
{
    // create a custom global object
    QScriptDebuggerConsoleGlobalObject *cppGlobal = new QScriptDebuggerConsoleGlobalObject();
    QScriptValue global = engine->newQObject(cppGlobal,
                                             QScriptEngine::ScriptOwnership,
                                             QScriptEngine::ExcludeSuperClassContents);
    {
        QScriptValueIterator it(engine->globalObject());
        while (it.hasNext()) {
            it.next();
            global.setProperty(it.scriptName(), it.value(), it.flags());
        }
    }
    engine->setGlobalObject(global);

    cppGlobal->setMessageHandler(messageHandler);
    QScriptValue ret = engine->evaluate(program, fileName);
    cppGlobal->setMessageHandler(0);
    if (engine->hasUncaughtException()) {
        messageHandler->message(QtCriticalMsg, ret.toString(), fileName,
                                engine->uncaughtExceptionLineNumber());
        return 0;
    }

    QScriptValue name = global.property(QLatin1String("name"));
    if (!name.isString()) {
        messageHandler->message(QtCriticalMsg, QLatin1String("command definition lacks a name"), fileName);
        return 0;
    }
    QString nameStr = name.toString();

    QScriptValue group = global.property(QLatin1String("group"));
    if (!group.isString()) {
        messageHandler->message(QtCriticalMsg, QString::fromLatin1("definition of command \"%0\" lacks a group name")
                                .arg(nameStr), fileName);
        return 0;
    }
    QString groupStr = group.toString();

    QScriptValue shortDesc = global.property(QLatin1String("shortDescription"));
    if (!shortDesc.isString()) {
        messageHandler->message(QtCriticalMsg, QString::fromLatin1("definition of command \"%0\" lacks shortDescription")
                                .arg(nameStr), fileName);
        return 0;
    }
    QString shortDescStr = shortDesc.toString();

    QScriptValue longDesc = global.property(QLatin1String("longDescription"));
    if (!longDesc.isString()) {
        messageHandler->message(QtCriticalMsg, QString::fromLatin1("definition of command \"%0\" lacks longDescription")
                                .arg(nameStr), fileName);
        return 0;
    }
    QString longDescStr = longDesc.toString();

    QStringList aliases;
    qScriptValueToSequence(global.property(QLatin1String("aliases")), aliases);

    QStringList seeAlso;
    qScriptValueToSequence(global.property(QLatin1String("seeAlso")), seeAlso);

    QStringList argTypes;
    qScriptValueToSequence(global.property(QLatin1String("argumentTypes")), argTypes);

    QStringList subCommands;
    qScriptValueToSequence(global.property(QLatin1String("subCommands")), subCommands);

    QScriptValue execFunction = global.property(QLatin1String("execute"));
    if (!execFunction.isFunction()) {
        messageHandler->message(QtCriticalMsg, QString::fromLatin1("definition of command \"%0\" lacks execute() function")
                                .arg(nameStr), fileName);
        return 0;
    }

    QScriptValue responseFunction = global.property(QLatin1String("handleResponse"));

    QScriptDebuggerScriptedConsoleCommand *result = new QScriptDebuggerScriptedConsoleCommand(
        nameStr, groupStr,
        shortDescStr, longDescStr,
        aliases, seeAlso,
        argTypes, subCommands,
        global, execFunction, responseFunction);
    return result;
}

QT_END_NAMESPACE
