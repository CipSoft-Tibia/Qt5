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

#include "qscriptdebuggerstandardwidgetfactory_p.h"
#include "qscriptdebuggerconsolewidget_p.h"
#include "qscriptdebuggerstackwidget_p.h"
#include "qscriptdebuggerscriptswidget_p.h"
#include "qscriptdebuggerlocalswidget_p.h"
#include "qscriptdebuggercodewidget_p.h"
#include "qscriptdebuggercodefinderwidget_p.h"
#include "qscriptbreakpointswidget_p.h"
#include "qscriptdebugoutputwidget_p.h"
#include "qscripterrorlogwidget_p.h"

QT_BEGIN_NAMESPACE

QScriptDebuggerStandardWidgetFactory::QScriptDebuggerStandardWidgetFactory(QObject *parent)
    : QObject(parent)
{
}

QScriptDebuggerStandardWidgetFactory::~QScriptDebuggerStandardWidgetFactory()
{
}

QScriptDebugOutputWidgetInterface *QScriptDebuggerStandardWidgetFactory::createDebugOutputWidget()
{
    return new QScriptDebugOutputWidget();
}

QScriptDebuggerConsoleWidgetInterface *QScriptDebuggerStandardWidgetFactory::createConsoleWidget()
{
    return new QScriptDebuggerConsoleWidget();
}

QScriptErrorLogWidgetInterface *QScriptDebuggerStandardWidgetFactory::createErrorLogWidget()
{
    return new QScriptErrorLogWidget();
}

QScriptDebuggerCodeFinderWidgetInterface *QScriptDebuggerStandardWidgetFactory::createCodeFinderWidget()
{
    return new QScriptDebuggerCodeFinderWidget();
}

QScriptDebuggerStackWidgetInterface *QScriptDebuggerStandardWidgetFactory::createStackWidget()
{
    return new QScriptDebuggerStackWidget();
}

QScriptDebuggerScriptsWidgetInterface *QScriptDebuggerStandardWidgetFactory::createScriptsWidget()
{
    return new QScriptDebuggerScriptsWidget();
}

QScriptDebuggerLocalsWidgetInterface *QScriptDebuggerStandardWidgetFactory::createLocalsWidget()
{
    return new QScriptDebuggerLocalsWidget();
}

QScriptDebuggerCodeWidgetInterface *QScriptDebuggerStandardWidgetFactory::createCodeWidget()
{
    return new QScriptDebuggerCodeWidget();
}

QScriptBreakpointsWidgetInterface *QScriptDebuggerStandardWidgetFactory::createBreakpointsWidget()
{
    return new QScriptBreakpointsWidget();
}

QT_END_NAMESPACE
