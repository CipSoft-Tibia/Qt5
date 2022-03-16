/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  location.h
*/

#ifndef LOCATION_H
#define LOCATION_H

#include <qstack.h>
#include <qcoreapplication.h>

QT_BEGIN_NAMESPACE

class Config;
class QRegExp;

class Location
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Location)

public:
    Location();
    Location(const QString& filePath);
    Location(const Location& other);
    ~Location() { delete stk; }

    Location& operator=(const Location& other);

    void start();
    void advance(QChar ch);
    void advanceLines(int n) { stkTop->lineNo += n; stkTop->columnNo = 1; }

    void push(const QString& filePath);
    void pop();
    void setEtc(bool etc) { etcetera = etc; }
    void setLineNo(int no) { stkTop->lineNo = no; }
    void setColumnNo(int no) { stkTop->columnNo = no; }

    bool isEmpty() const { return stkDepth == 0; }
    int depth() const { return stkDepth; }
    const QString& filePath() const { return stkTop->filePath; }
    QString fileName() const;
    QString fileSuffix() const;
    int lineNo() const { return stkTop->lineNo; }
    int columnNo() const { return stkTop->columnNo; }
    bool etc() const { return etcetera; }
    void warning(const QString& message,
                 const QString& details = QString()) const;
    void error(const QString& message,
               const QString& details = QString()) const;
    void fatal(const QString& message,
               const QString& details = QString()) const;
    void report(const QString& message,
                const QString& details = QString()) const;

    static const Location null;

    static void initialize(const Config& config);
    static void terminate();
    static void information(const QString& message);
    static void internalError(const QString& hint);
    static void logToStdErr(const QString& message);
    static void startLoggingProgress() { logProgress_ = true; }
    static void stopLoggingProgress() { logProgress_ = false; }
    static QString canonicalRelativePath(const QString &path);
    static int exitCode();

private:
    enum MessageType { Warning, Error, Report };

    struct StackEntry
    {
        QString filePath;
        int lineNo;
        int columnNo;
    };
    friend class QTypeInfo<StackEntry>;

    void emitMessage(MessageType type,
                     const QString& message,
                     const QString& details) const;
    QString toString() const;
    QString top() const;

private:
    StackEntry stkBottom;
    QStack<StackEntry> *stk;
    StackEntry *stkTop;
    int stkDepth;
    bool etcetera;

    static int tabSize;
    static int warningCount;
    static int warningLimit;
    static QString programName;
    static QString project;
    static QRegExp *spuriousRegExp;
    static bool logProgress_;
};
Q_DECLARE_TYPEINFO(Location::StackEntry, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Location, Q_COMPLEX_TYPE); // stkTop = &stkBottom

QT_END_NAMESPACE

#endif
