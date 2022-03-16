/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include "objects.h"
#include <QLayout>
#include <QPainter>

/* Implementation of QParentWidget */
//! [0]
QParentWidget::QParentWidget(QWidget *parent)
: QWidget(parent),
  m_vbox(new QVBoxLayout(this))
{
}

//! [0] //! [1]
void QParentWidget::createSubWidget(const QString &name)
{
    QSubWidget *sw = new QSubWidget(this, name);
    m_vbox->addWidget(sw);
    sw->setLabel(name);
    sw->show();
}

//! [1] //! [2]
QSubWidget *QParentWidget::subWidget(const QString &name)
{
    return findChild<QSubWidget *>(name);
}

//! [2]
QSize QParentWidget::sizeHint() const
{
    return QWidget::sizeHint().expandedTo(QSize(100, 100));
}

/* Implementation of QSubWidget */
//! [3]
QSubWidget::QSubWidget(QWidget *parent, const QString &name)
: QWidget(parent)
{
    setObjectName(name);
}

void QSubWidget::setLabel(const QString &text)
{
    m_label = text;
    setObjectName(text);
    update();
}

QString QSubWidget::label() const
{
    return m_label;
}

QSize QSubWidget::sizeHint() const
{
    QFontMetrics fm(font());
    return QSize(fm.width(m_label), fm.height());
}

void QSubWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(palette().text().color());
    painter.drawText(rect(), Qt::AlignCenter, m_label);
//! [3] //! [4]
}
//! [4]
