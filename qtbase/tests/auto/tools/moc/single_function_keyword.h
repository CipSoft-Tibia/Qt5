// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SINGLE_FUNCTION_KEYWORD_H
#define SINGLE_FUNCTION_KEYWORD_H
#include <QObject>

class SingleFunctionKeywordBeforeReturnType : public QObject
{
    Q_OBJECT
public:
    inline SingleFunctionKeywordBeforeReturnType() {}

    Q_SIGNAL void mySignal();

    Q_SLOT void mySlot() { mySignal(); }
};

class SingleFunctionKeywordBeforeInline : public QObject
{
    Q_OBJECT
public:
    inline SingleFunctionKeywordBeforeInline() {}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wundefined-inline")
    Q_SIGNAL inline void mySignal();
QT_WARNING_POP

    Q_SLOT inline void mySlot() { emit mySignal(); }
};

class SingleFunctionKeywordAfterInline : public QObject
{
    Q_OBJECT
public:
    inline SingleFunctionKeywordAfterInline() {}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wundefined-inline")
    inline Q_SIGNAL void mySignal();
QT_WARNING_POP

    inline Q_SLOT void mySlot() { emit mySignal(); }
};

#endif // SINGLE_FUNCTION_KEYWORD_H
