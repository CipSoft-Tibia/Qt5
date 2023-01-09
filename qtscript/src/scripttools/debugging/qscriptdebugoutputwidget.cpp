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

#include "qscriptdebugoutputwidget_p.h"
#include "qscriptdebugoutputwidgetinterface_p_p.h"

#include <QtCore/qdebug.h>
#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qplaintextedit.h>
#include <QtWidgets/qscrollbar.h>

QT_BEGIN_NAMESPACE

namespace {

class QScriptDebugOutputWidgetOutputEdit : public QPlainTextEdit
{
public:
    QScriptDebugOutputWidgetOutputEdit(QWidget *parent = 0)
        : QPlainTextEdit(parent)
    {
        setReadOnly(true);
//        setFocusPolicy(Qt::NoFocus);
        setMaximumBlockCount(2500);
    }

    void scrollToBottom()
    {
        QScrollBar *bar = verticalScrollBar();
        bar->setValue(bar->maximum());
    }

    int charactersPerLine() const
    {
        QFontMetrics fm(font());
        return width() / fm.maxWidth();
    }
};

} // namespace

class QScriptDebugOutputWidgetPrivate
    : public QScriptDebugOutputWidgetInterfacePrivate
{
    Q_DECLARE_PUBLIC(QScriptDebugOutputWidget)
public:
    QScriptDebugOutputWidgetPrivate();
    ~QScriptDebugOutputWidgetPrivate();

    QScriptDebugOutputWidgetOutputEdit *outputEdit;
};

QScriptDebugOutputWidgetPrivate::QScriptDebugOutputWidgetPrivate()
{
}

QScriptDebugOutputWidgetPrivate::~QScriptDebugOutputWidgetPrivate()
{
}

QScriptDebugOutputWidget::QScriptDebugOutputWidget(QWidget *parent)
    : QScriptDebugOutputWidgetInterface(*new QScriptDebugOutputWidgetPrivate, parent, {})
{
    Q_D(QScriptDebugOutputWidget);
    d->outputEdit = new QScriptDebugOutputWidgetOutputEdit();
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);
    vbox->addWidget(d->outputEdit);

#ifndef QT_NO_STYLE_STYLESHEET
    QString sheet = QString::fromLatin1("font-size: 14px; font-family: \"Monospace\";");
    setStyleSheet(sheet);
#endif
}

QScriptDebugOutputWidget::~QScriptDebugOutputWidget()
{
}

void QScriptDebugOutputWidget::message(
    QtMsgType type, const QString &text, const QString &fileName,
    int lineNumber, int columnNumber, const QVariant &/*data*/)
{
    // ### unify with QScriptDebuggerConsoleWidget::message()
    Q_D(QScriptDebugOutputWidget);
    QString msg;
    if (!fileName.isEmpty() || (lineNumber != -1)) {
        if (!fileName.isEmpty())
            msg.append(fileName);
        else
            msg.append(QLatin1String("<noname>"));
        if (lineNumber != -1) {
            msg.append(QLatin1Char(':'));
            msg.append(QString::number(lineNumber));
            if (columnNumber != -1) {
                msg.append(QLatin1Char(':'));
                msg.append(QString::number(columnNumber));
            }
        }
        msg.append(QLatin1String(": "));
    }
    msg.append(text);
    QTextCharFormat oldFmt = d->outputEdit->currentCharFormat();
    QTextCharFormat fmt(oldFmt);
    if (type == QtCriticalMsg) {
        fmt.setForeground(Qt::red);
        d->outputEdit->setCurrentCharFormat(fmt);
    }
    d->outputEdit->appendPlainText(msg);
    d->outputEdit->setCurrentCharFormat(oldFmt);
    d->outputEdit->scrollToBottom();
}

void QScriptDebugOutputWidget::clear()
{
    Q_D(QScriptDebugOutputWidget);
    d->outputEdit->clear();
}

QT_END_NAMESPACE
