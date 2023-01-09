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

#include "qscriptdebuggerresponse_p.h"
#include "qscriptdebuggervalue_p.h"

#include <QtScript/qscriptcontextinfo.h>
#include <QtCore/qdatastream.h>

Q_DECLARE_METATYPE(QScriptBreakpointData)
Q_DECLARE_METATYPE(QScriptBreakpointMap)
Q_DECLARE_METATYPE(QScriptScriptData)
Q_DECLARE_METATYPE(QScriptScriptMap)
Q_DECLARE_METATYPE(QScriptDebuggerValue)
Q_DECLARE_METATYPE(QScriptDebuggerValueList)
Q_DECLARE_METATYPE(QScriptDebuggerValueProperty)
Q_DECLARE_METATYPE(QScriptDebuggerValuePropertyList)
Q_DECLARE_METATYPE(QScriptContextInfo)

QT_BEGIN_NAMESPACE

/*!
  \since 4.5
  \class QScriptDebuggerResponse
  \internal

  \brief The QScriptDebuggerResponse class represents a front-end's response to a QScriptDebuggerCommand.

  A response contains an error code and result.
*/

class QScriptDebuggerResponsePrivate
{
public:
    QScriptDebuggerResponsePrivate();
    ~QScriptDebuggerResponsePrivate();

    QScriptDebuggerResponse::Error error;
    QVariant result;
    bool async;
};

QScriptDebuggerResponsePrivate::QScriptDebuggerResponsePrivate()
{
    error = QScriptDebuggerResponse::NoError;
    async = false;
}

QScriptDebuggerResponsePrivate::~QScriptDebuggerResponsePrivate()
{
}

QScriptDebuggerResponse::QScriptDebuggerResponse()
    : d_ptr(new QScriptDebuggerResponsePrivate)
{
}

QScriptDebuggerResponse::QScriptDebuggerResponse(const QScriptDebuggerResponse &other)
    : d_ptr(new QScriptDebuggerResponsePrivate)
{
    *d_ptr = *other.d_ptr;
}

QScriptDebuggerResponse::~QScriptDebuggerResponse()
{
}

QScriptDebuggerResponse &QScriptDebuggerResponse::operator=(const QScriptDebuggerResponse &other)
{
    *d_ptr = *other.d_ptr;
    return *this;
}

/*!
  Returns the error code of this response.
*/
QScriptDebuggerResponse::Error QScriptDebuggerResponse::error() const
{
    Q_D(const QScriptDebuggerResponse);
    return d->error;
}

/*!
  Sets the \a error code of this response.
*/
void QScriptDebuggerResponse::setError(Error error)
{
    Q_D(QScriptDebuggerResponse);
    d->error = error;
}

/*!
  Returns the result of this response. This function is provided for
  convenience.
*/
QVariant QScriptDebuggerResponse::result() const
{
    Q_D(const QScriptDebuggerResponse);
    return d->result;
}

/*!
  Sets the Result attribute of this response to the given \a
  value. This function is provided for convenience.
*/
void QScriptDebuggerResponse::setResult(const QVariant &value)
{
    Q_D(QScriptDebuggerResponse);
    d->result = value;
}

void QScriptDebuggerResponse::setResult(int value)
{
    Q_D(QScriptDebuggerResponse);
    d->result = value;
}

void QScriptDebuggerResponse::setResult(const QString &value)
{
    Q_D(QScriptDebuggerResponse);
    d->result = value;
}

void QScriptDebuggerResponse::setResult(const QScriptBreakpointData &data)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(data);
}

void QScriptDebuggerResponse::setResult(const QScriptBreakpointMap &breakpoints)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(breakpoints);
}

void QScriptDebuggerResponse::setResult(const QScriptScriptMap &scripts)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(scripts);
}

void QScriptDebuggerResponse::setResult(const QScriptScriptData &data)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(data);
}

void QScriptDebuggerResponse::setResult(const QScriptDebuggerValue &value)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(value);
}

void QScriptDebuggerResponse::setResult(const QScriptDebuggerValueList &values)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(values);
}

void QScriptDebuggerResponse::setResult(const QScriptDebuggerValuePropertyList &props)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(props);
}

void QScriptDebuggerResponse::setResult(const QScriptContextInfo &info)
{
    Q_D(QScriptDebuggerResponse);
    d->result = QVariant::fromValue(info);
}

int QScriptDebuggerResponse::resultAsInt() const
{
    Q_D(const QScriptDebuggerResponse);
    return d->result.toInt();
}

qint64 QScriptDebuggerResponse::resultAsLongLong() const
{
    Q_D(const QScriptDebuggerResponse);
    return d->result.toLongLong();
}

QString QScriptDebuggerResponse::resultAsString() const
{
    Q_D(const QScriptDebuggerResponse);
    return d->result.toString();
}

QScriptBreakpointData QScriptDebuggerResponse::resultAsBreakpointData() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptBreakpointData>(d->result);
}

QScriptBreakpointMap QScriptDebuggerResponse::resultAsBreakpoints() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptBreakpointMap>(d->result);
}

QScriptScriptMap QScriptDebuggerResponse::resultAsScripts() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptScriptMap>(d->result);
}

QScriptScriptData QScriptDebuggerResponse::resultAsScriptData() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptScriptData>(d->result);
}

QScriptDebuggerValue QScriptDebuggerResponse::resultAsScriptValue() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptDebuggerValue>(d->result);
}

QScriptDebuggerValueList QScriptDebuggerResponse::resultAsScriptValueList() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptDebuggerValueList>(d->result);
}

QScriptDebuggerValuePropertyList QScriptDebuggerResponse::resultAsScriptValuePropertyList() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptDebuggerValuePropertyList>(d->result);
}

QScriptContextInfo QScriptDebuggerResponse::resultAsContextInfo() const
{
    Q_D(const QScriptDebuggerResponse);
    return qvariant_cast<QScriptContextInfo>(d->result);
}

bool QScriptDebuggerResponse::async() const
{
    Q_D(const QScriptDebuggerResponse);
    return d->async;
}

void QScriptDebuggerResponse::setAsync(bool async)
{
    Q_D(QScriptDebuggerResponse);
    d->async = async;
}

/*!
  Returns true if this QScriptDebuggerResponse is equal to the \a other
  response, otherwise returns false.
*/
bool QScriptDebuggerResponse::operator==(const QScriptDebuggerResponse &other) const
{
    Q_D(const QScriptDebuggerResponse);
    const QScriptDebuggerResponsePrivate *od = other.d_func();
    if (d == od)
        return true;
    if (!d || !od)
        return false;
    return ((d->error == od->error)
            && (d->result == od->result)
            && (d->async == od->async));
}

/*!
  Returns true if this QScriptDebuggerResponse is not equal to the \a
  other response, otherwise returns false.
*/
bool QScriptDebuggerResponse::operator!=(const QScriptDebuggerResponse &other) const
{
    return !(*this == other);
}

/*!
  \relates QScriptDebuggerResponse

  Writes the given \a response to the specified \a stream.
*/
QDataStream &operator<<(QDataStream &out, const QScriptDebuggerResponse &response)
{
    const QScriptDebuggerResponsePrivate *d = response.d_ptr.data();
    out << (quint32)d->error;
    out << d->result;
    out << d->async;
    return out;
}

/*!
  \relates QScriptDebuggerResponse

  Reads a QScriptDebuggerResponse from the specified \a stream into the
  given \a response.
*/
QDataStream &operator>>(QDataStream &in, QScriptDebuggerResponse &response)
{
    QScriptDebuggerResponsePrivate *d = response.d_ptr.data();

    quint32 error;
    in >> error;
    d->error = QScriptDebuggerResponse::Error(error);
    in >> d->result;
    in >> d->async;

    return in;
}

QT_END_NAMESPACE
