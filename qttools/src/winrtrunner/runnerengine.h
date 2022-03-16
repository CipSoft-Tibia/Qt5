/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef RUNNERENGINE_H
#define RUNNERENGINE_H

#include <QtCore/QString>

QT_USE_NAMESPACE

class RunnerEngine
{
public:
    virtual ~RunnerEngine() { }
    virtual bool install(bool removeFirst = false) = 0;
    virtual bool remove() = 0;
    virtual bool start() = 0;
    virtual bool enableDebugging(const QString &debugger, const QString &debuggerArguments) = 0;
    virtual bool disableDebugging() = 0;
    virtual bool setLoopbackExemptClientEnabled(bool enabled) = 0;
    virtual bool setLoopbackExemptServerEnabled(bool enabled) = 0;
    virtual bool setLoggingRules(const QByteArray &rules) = 0;
    virtual bool suspend() = 0;
    virtual bool waitForFinished(int secs) = 0;
    virtual bool stop() = 0;
    virtual qint64 pid() const = 0;
    virtual int exitCode() const = 0;
    virtual QString executable() const = 0;
    virtual QString devicePath(const QString &relativePath) const = 0;
    virtual bool sendFile(const QString &localFile, const QString &deviceFile) = 0;
    virtual bool receiveFile(const QString &deviceFile, const QString &localFile) = 0;
};

#endif // RUNNERENGINE_H
