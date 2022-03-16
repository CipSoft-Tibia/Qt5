/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** us via http://www.qt.io/contact-us/.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "config.h"
#include "qscriptcontextinfo.h"

#include "qscriptcontext_p.h"
#include "qscriptengine.h"
#include "qscriptengine_p.h"
#include "../bridge/qscriptqobject_p.h"
#include <QtCore/qdatastream.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qshareddata.h>
#include "CodeBlock.h"
#include "JSFunction.h"
#if ENABLE(JIT)
#include "MacroAssemblerCodeRef.h"
#endif

QT_BEGIN_NAMESPACE

/*!
  \since 4.4
  \class QScriptContextInfo
  \inmodule QtScript
  \brief The QScriptContextInfo class provides additional information about a QScriptContext.

  \ingroup script


  QScriptContextInfo is typically used for debugging purposes. It can
  provide information about the code being executed, such as the type
  of the called function, and the original source code location of the
  current statement.

  If the called function is executing Qt Script code, you can obtain
  the script location with the functions fileName() and lineNumber().

  You can obtain the starting line number and ending line number of a
  Qt Script function definition with functionStartLineNumber() and
  functionEndLineNumber(), respectively.

  For Qt Script functions and Qt methods (e.g. slots), you can call
  functionParameterNames() to get the names of the formal parameters of the
  function.

  For Qt methods and Qt property accessors, you can obtain the index
  of the underlying QMetaMethod or QMetaProperty by calling
  functionMetaIndex().

  \sa QScriptContext, QScriptEngineAgent
*/

/*!
    \enum QScriptContextInfo::FunctionType

    This enum specifies the type of function being called.

    \value ScriptFunction The function is a Qt Script function, i.e. it was defined through a call to QScriptEngine::evaluate().
    \value QtFunction The function is a Qt function (a signal, slot or method).
    \value QtPropertyFunction The function is a Qt property getter or setter.
    \value NativeFunction The function is a built-in Qt Script function, or it was defined through a call to QScriptEngine::newFunction().
*/

class QScriptContextInfoPrivate : public QSharedData
{
    Q_DECLARE_PUBLIC(QScriptContextInfo)
public:
    QScriptContextInfoPrivate();
    QScriptContextInfoPrivate(const QScriptContext *context);
    ~QScriptContextInfoPrivate();

    qint64 scriptId;
    int lineNumber;
    int columnNumber;
    QString fileName;

    QString functionName;
    QScriptContextInfo::FunctionType functionType;

    int functionStartLineNumber;
    int functionEndLineNumber;
    int functionMetaIndex;

    QStringList parameterNames;

    QScriptContextInfo *q_ptr;
};

/*!
  \internal
*/
QScriptContextInfoPrivate::QScriptContextInfoPrivate()
{
    functionType = QScriptContextInfo::NativeFunction;
    functionMetaIndex = -1;
    functionStartLineNumber = -1;
    functionEndLineNumber = -1;
    scriptId = -1;
    lineNumber = -1;
    columnNumber = -1;
}

/*!
  \internal
*/
QScriptContextInfoPrivate::QScriptContextInfoPrivate(const QScriptContext *context)
{
    Q_ASSERT(context);
    functionType = QScriptContextInfo::NativeFunction;
    functionMetaIndex = -1;
    functionStartLineNumber = -1;
    functionEndLineNumber = -1;
    scriptId = -1;
    lineNumber = -1;
    columnNumber = -1;

    JSC::CallFrame *frame = const_cast<JSC::CallFrame *>(QScriptEnginePrivate::frameForContext(context));

    // Get the line number:

    //We need to know the context directly up in the backtrace, in order to get the line number, and adjust the global context
    JSC::CallFrame *rewindContext = QScriptEnginePrivate::get(context->engine())->currentFrame;
    if (QScriptEnginePrivate::contextForFrame(rewindContext) == context) {  //top context
        frame = rewindContext; //for retreiving the global context's "fake" frame
        // An agent might have provided the line number.
        lineNumber = QScript::scriptEngineFromExec(frame)->agentLineNumber;
        if (lineNumber == -1)
            lineNumber = QScript::scriptEngineFromExec(frame)->uncaughtExceptionLineNumber;
    } else {
        // rewind the stack from the top in order to find the frame from the caller where the returnPC is stored
        while (rewindContext && QScriptEnginePrivate::contextForFrame(rewindContext->callerFrame()->removeHostCallFrameFlag()) != context)
            rewindContext = rewindContext->callerFrame()->removeHostCallFrameFlag();
        if (rewindContext) {
            frame = rewindContext->callerFrame()->removeHostCallFrameFlag(); //for retreiving the global context's "fake" frame

            JSC::Instruction *returnPC = rewindContext->returnPC();
            JSC::CodeBlock *codeBlock = frame->codeBlock();
            if (returnPC && codeBlock && QScriptEnginePrivate::hasValidCodeBlockRegister(frame)) {
#if ENABLE(JIT)
                JSC::JITCode code = codeBlock->getJITCode();
                uintptr_t jitOffset = reinterpret_cast<uintptr_t>(JSC::ReturnAddressPtr(returnPC).value()) - reinterpret_cast<uintptr_t>(code.addressForCall().executableAddress());
                // We can only use the JIT code offset if it's smaller than the JIT size;
                // otherwise calling getBytecodeIndex() is meaningless.
                if (jitOffset < code.size()) {
                    unsigned bytecodeOffset = codeBlock->getBytecodeIndex(frame, JSC::ReturnAddressPtr(returnPC));
#else
                unsigned bytecodeOffset = returnPC - codeBlock->instructions().begin();
#endif
                bytecodeOffset--; //because returnPC is on the next instruction. We want the current one
                lineNumber = codeBlock->lineNumberForBytecodeOffset(const_cast<JSC::ExecState *>(frame), bytecodeOffset);
#if ENABLE(JIT)
                }
#endif
            }
        }
    }

    // Get the filename and the scriptId:
    JSC::CodeBlock *codeBlock = frame->codeBlock();
    if (codeBlock && QScriptEnginePrivate::hasValidCodeBlockRegister(frame)) {
           JSC::SourceProvider *source = codeBlock->source();
           scriptId = source->asID();
           fileName = source->url();
    }

    // Get the others information:
    JSC::JSObject *callee = frame->callee();
    if (callee && callee->inherits(&JSC::InternalFunction::info))
        functionName = JSC::asInternalFunction(callee)->name(frame);
    if (callee && callee->inherits(&JSC::JSFunction::info)
        && !JSC::asFunction(callee)->isHostFunction()) {
        functionType = QScriptContextInfo::ScriptFunction;
        JSC::FunctionExecutable *body = JSC::asFunction(callee)->jsExecutable();
        functionStartLineNumber = body->lineNo();
        functionEndLineNumber = body->lastLine();
        for (size_t i = 0; i < body->parameterCount(); ++i)
            parameterNames.append(body->parameterName(i));
        // ### get the function name from the AST
    } else if (callee && callee->inherits(&QScript::QtFunction::info)) {
        functionType = QScriptContextInfo::QtFunction;
        functionMetaIndex = static_cast<QScript::QtFunction*>(callee)->specificIndex(context);
        const QMetaObject *meta = static_cast<QScript::QtFunction*>(callee)->metaObject();
        if (meta != 0) {
            QMetaMethod method = meta->method(functionMetaIndex);
            QList<QByteArray> formals = method.parameterNames();
            for (int i = 0; i < formals.count(); ++i)
                parameterNames.append(QLatin1String(formals.at(i)));
        }
    }
    else if (callee && callee->inherits(&QScript::QtPropertyFunction::info)) {
        functionType = QScriptContextInfo::QtPropertyFunction;
        functionMetaIndex = static_cast<QScript::QtPropertyFunction*>(callee)->propertyIndex();
    }
}

/*!
  \internal
*/
QScriptContextInfoPrivate::~QScriptContextInfoPrivate()
{
}

/*!
  Constructs a new QScriptContextInfo from the given \a context.

  The relevant information is extracted from the \a context at
  construction time; i.e. if you continue script execution in the \a
  context, the new state of the context will not be reflected in a
  previously created QScriptContextInfo.
*/
QScriptContextInfo::QScriptContextInfo(const QScriptContext *context)
    : d_ptr(0)
{
    if (context) {
        d_ptr = new QScriptContextInfoPrivate(context);
        d_ptr->q_ptr = this;
    }
}

/*!
  Constructs a new QScriptContextInfo from the \a other info.
*/
QScriptContextInfo::QScriptContextInfo(const QScriptContextInfo &other)
    : d_ptr(other.d_ptr)
{
}

/*!
  Constructs a null QScriptContextInfo.

  \sa isNull()
*/
QScriptContextInfo::QScriptContextInfo()
    : d_ptr(0)
{
}

/*!
  Destroys the QScriptContextInfo.
*/
QScriptContextInfo::~QScriptContextInfo()
{
}

/*!
  Assigns the \a other info to this QScriptContextInfo,
  and returns a reference to this QScriptContextInfo.
*/
QScriptContextInfo &QScriptContextInfo::operator=(const QScriptContextInfo &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

/*!
  Returns the ID of the script where the code being executed was
  defined, or -1 if the ID is not available (i.e. a native function is
  being executed).

  \sa QScriptEngineAgent::scriptLoad()
*/
qint64 QScriptContextInfo::scriptId() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return -1;
    return d->scriptId;
}

/*!
  Returns the name of the file where the code being executed was
  defined, if available; otherwise returns an empty string.

  For Qt Script code, this function returns the fileName argument
  that was passed to QScriptEngine::evaluate().

  \sa lineNumber(), functionName()
*/
QString QScriptContextInfo::fileName() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return QString();
    return d->fileName;
}

/*!
  Returns the line number corresponding to the statement being
  executed, or -1 if the line number is not available.

  The line number is only available if Qt Script code is being
  executed.

  \sa columnNumber(), fileName()
*/
int QScriptContextInfo::lineNumber() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return -1;
    return d->lineNumber;
}

/*!
  \obsolete
*/
int QScriptContextInfo::columnNumber() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return -1;
    return d->columnNumber;
}

/*!
  Returns the name of the called function, or an empty string if
  the name is not available.

  For script functions of type QtPropertyFunction, this function
  always returns the name of the property; you can use
  QScriptContext::argumentCount() to differentiate between reads and
  writes.

  \sa fileName(), functionType()
*/
QString QScriptContextInfo::functionName() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return QString();
    return d->functionName;
}

/*!
  Returns the type of the called function.

  \sa functionName(), QScriptContext::callee()
*/
QScriptContextInfo::FunctionType QScriptContextInfo::functionType() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return NativeFunction;
    return d->functionType;
}

/*!
  Returns the line number where the definition of the called function
  starts, or -1 if the line number is not available.

  The starting line number is only available if the functionType() is
  ScriptFunction.

  \sa functionEndLineNumber(), fileName()
*/
int QScriptContextInfo::functionStartLineNumber() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return -1;
    return d->functionStartLineNumber;
}

/*!
  Returns the line number where the definition of the called function
  ends, or -1 if the line number is not available.

  The ending line number is only available if the functionType() is
  ScriptFunction.

  \sa functionStartLineNumber()
*/
int QScriptContextInfo::functionEndLineNumber() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return -1;
    return d->functionEndLineNumber;
}

/*!
  Returns the names of the formal parameters of the called function,
  or an empty QStringList if the parameter names are not available.

  \sa QScriptContext::argument()
*/
QStringList QScriptContextInfo::functionParameterNames() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return QStringList();
    return d->parameterNames;
}

/*!
  Returns the meta index of the called function, or -1 if the meta
  index is not available.

  The meta index is only available if the functionType() is QtFunction
  or QtPropertyFunction. For QtFunction, the meta index can be passed
  to QMetaObject::method() to obtain the corresponding method
  definition; for QtPropertyFunction, the meta index can be passed to
  QMetaObject::property() to obtain the corresponding property
  definition.

  \sa QScriptContext::thisObject()
*/
int QScriptContextInfo::functionMetaIndex() const
{
    Q_D(const QScriptContextInfo);
    if (!d)
        return -1;
    return d->functionMetaIndex;
}

/*!
  Returns true if this QScriptContextInfo is null, i.e. does not
  contain any information.
*/
bool QScriptContextInfo::isNull() const
{
    Q_D(const QScriptContextInfo);
    return (d == 0);
}

/*!
  Returns true if this QScriptContextInfo is equal to the \a other
  info, otherwise returns false.
*/
bool QScriptContextInfo::operator==(const QScriptContextInfo &other) const
{
    Q_D(const QScriptContextInfo);
    const QScriptContextInfoPrivate *od = other.d_func();
    if (d == od)
        return true;
    if (!d || !od)
        return false;
    return ((d->scriptId == od->scriptId)
            && (d->lineNumber == od->lineNumber)
            && (d->columnNumber == od->columnNumber)
            && (d->fileName == od->fileName)
            && (d->functionName == od->functionName)
            && (d->functionType == od->functionType)
            && (d->functionStartLineNumber == od->functionStartLineNumber)
            && (d->functionEndLineNumber == od->functionEndLineNumber)
            && (d->functionMetaIndex == od->functionMetaIndex)
            && (d->parameterNames == od->parameterNames));
}

/*!
  Returns true if this QScriptContextInfo is not equal to the \a other
  info, otherwise returns false.
*/
bool QScriptContextInfo::operator!=(const QScriptContextInfo &other) const
{
    return !(*this == other);
}

#ifndef QT_NO_DATASTREAM
/*!
  \fn QDataStream &operator<<(QDataStream &stream, const QScriptContextInfo &info)
  \since 4.4
  \relates QScriptContextInfo

  Writes the given \a info to the specified \a stream.
*/
QDataStream &operator<<(QDataStream &out, const QScriptContextInfo &info)
{
    out << info.scriptId();
    out << (qint32)info.lineNumber();
    out << (qint32)info.columnNumber();

    out << (quint32)info.functionType();
    out << (qint32)info.functionStartLineNumber();
    out << (qint32)info.functionEndLineNumber();
    out << (qint32)info.functionMetaIndex();

    out << info.fileName();
    out << info.functionName();
    out << info.functionParameterNames();

    return out;
}

/*!
  \fn QDataStream &operator>>(QDataStream &stream, QScriptContextInfo &info)
  \since 4.4
  \relates QScriptContextInfo

  Reads a QScriptContextInfo from the specified \a stream into the
  given \a info.
*/
Q_SCRIPT_EXPORT QDataStream &operator>>(QDataStream &in, QScriptContextInfo &info)
{
    if (!info.d_ptr) {
        info.d_ptr = new QScriptContextInfoPrivate();
    }

    in >> info.d_ptr->scriptId;

    qint32 line;
    in >> line;
    info.d_ptr->lineNumber = line;

    qint32 column;
    in >> column;
    info.d_ptr->columnNumber = column;

    quint32 ftype;
    in >> ftype;
    info.d_ptr->functionType = QScriptContextInfo::FunctionType(ftype);

    qint32 startLine;
    in >> startLine;
    info.d_ptr->functionStartLineNumber = startLine;

    qint32 endLine;
    in >> endLine;
    info.d_ptr->functionEndLineNumber = endLine;

    qint32 metaIndex;
    in >> metaIndex;
    info.d_ptr->functionMetaIndex = metaIndex;

    in >> info.d_ptr->fileName;
    in >> info.d_ptr->functionName;
    in >> info.d_ptr->parameterNames;

    return in;
}
#endif

QT_END_NAMESPACE
