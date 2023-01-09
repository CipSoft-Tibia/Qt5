/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tictactoe.h"
#include "tictactoeplugin.h"
#include "tictactoetaskmenu.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QExtensionManager>
#include <QtPlugin>

//! [0]
TicTacToePlugin::TicTacToePlugin(QObject *parent)
    : QObject(parent)
{
}

QString TicTacToePlugin::name() const
{
    return QStringLiteral("TicTacToe");
}

QString TicTacToePlugin::group() const
{
    return QStringLiteral("Display Widgets [Examples]");
}

QString TicTacToePlugin::toolTip() const
{
    return QString();
}

QString TicTacToePlugin::whatsThis() const
{
    return QString();
}

QString TicTacToePlugin::includeFile() const
{
    return QStringLiteral("tictactoe.h");
}

QIcon TicTacToePlugin::icon() const
{
    return QIcon();
}

bool TicTacToePlugin::isContainer() const
{
    return false;
}

QWidget *TicTacToePlugin::createWidget(QWidget *parent)
{
    TicTacToe *ticTacToe = new TicTacToe(parent);
    ticTacToe->setState(QStringLiteral("-X-XO----"));
    return ticTacToe;
}

bool TicTacToePlugin::isInitialized() const
{
    return initialized;
}

//! [0] //! [1]
void TicTacToePlugin::initialize(QDesignerFormEditorInterface *formEditor)
{
//! [1] //! [2]
    if (initialized)
        return;

    QExtensionManager *manager = formEditor->extensionManager();
    Q_ASSERT(manager != 0);
//! [2]

//! [3]
    manager->registerExtensions(new TicTacToeTaskMenuFactory(manager),
                                Q_TYPEID(QDesignerTaskMenuExtension));

    initialized = true;
}

QString TicTacToePlugin::domXml() const
{
    return QLatin1String("\
<ui language=\"c++\">\
    <widget class=\"TicTacToe\" name=\"ticTacToe\"/>\
    <customwidgets>\
        <customwidget>\
            <class>TicTacToe</class>\
            <propertyspecifications>\
            <tooltip name=\"state\">Tic Tac Toe state</tooltip>\
            <stringpropertyspecification name=\"state\" notr=\"true\" type=\"singleline\"/>\
            </propertyspecifications>\
        </customwidget>\
    </customwidgets>\
</ui>");
}

//! [3]
