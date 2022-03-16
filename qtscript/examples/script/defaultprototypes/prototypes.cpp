/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "prototypes.h"
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QListWidget>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptEngine>
#include <QDebug>

Q_DECLARE_METATYPE(QListWidgetItem*)
Q_DECLARE_METATYPE(QListWidget*)

//! [0]
ListWidgetItemPrototype::ListWidgetItemPrototype(QObject *parent)
    : QObject(parent)
{
}

QString ListWidgetItemPrototype::text() const
{
    QListWidgetItem *item = qscriptvalue_cast<QListWidgetItem*>(thisObject());
    if (item)
        return item->text();
    return QString();
}

void ListWidgetItemPrototype::setText(const QString &text)
{
    QListWidgetItem *item = qscriptvalue_cast<QListWidgetItem*>(thisObject());
    if (item)
        item->setText(text);
}

QString ListWidgetItemPrototype::toString() const
{
    return QString("ListWidgetItem(text = %0)").arg(text());
}
//! [0]



//! [1]
ListWidgetPrototype::ListWidgetPrototype(QObject *parent)
    : QObject(parent)
{
}

void ListWidgetPrototype::addItem(const QString &text)
{
    QListWidget *widget = qscriptvalue_cast<QListWidget*>(thisObject());
    if (widget)
        widget->addItem(text);
}

void ListWidgetPrototype::addItems(const QStringList &texts)
{
    QListWidget *widget = qscriptvalue_cast<QListWidget*>(thisObject());
    if (widget)
        widget->addItems(texts);
}

void ListWidgetPrototype::setBackgroundColor(const QString &colorName)
{
    QListWidget *widget = qscriptvalue_cast<QListWidget*>(thisObject());
    if (widget) {
#ifdef Q_WS_MAEMO_5
        QString style = QString("QListWidget::item {background-color: %1;}").arg(colorName);
        style += "QListWidget::item {selection-color: black;}";
        widget->setStyleSheet(style);
#else
        QPalette palette = widget->palette();
        QColor color(colorName);
        palette.setBrush(QPalette::Base, color);
        widget->setPalette(palette);
#endif
    }
}
//! [1]
