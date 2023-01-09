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

#include "globjwin.h"
#include "glbox.h"
#include <QPushButton>
#include <QSlider>
#include <QLayout>
#include <QFrame>
#include <QMenuBar>
#include <QMenu>
#include <QApplication>


GLObjectWindow::GLObjectWindow(QWidget *parent)
    : QWidget(parent)
{
    // Create a menu
    QMenu *file = new QMenu(this);
    file->addAction(tr("Exit"), qApp, &QApplication::quit);

    // Create a menu bar
    QMenuBar *m = new QMenuBar(this);
    m->addMenu(file)->setText(tr("&File"));

    // Create a nice frame to put around the OpenGL widget
    QFrame *f = new QFrame(this);
    f->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    f->setLineWidth(2);

    // Create our OpenGL widget
    GLBox *c = new GLBox(f, "glbox");

    // Create the three sliders; one for each rotation axis
    QSlider *x = new QSlider(Qt::Vertical, this);
    x->setMaximum(360);
    x->setPageStep(60);
    x->setTickPosition(QSlider::TicksLeft);
    connect(x, &QSlider::valueChanged, c, &GLBox::setXRotation);

    QSlider *y = new QSlider(Qt::Vertical, this);
    y->setMaximum(360);
    y->setPageStep(60);
    y->setTickPosition(QSlider::TicksLeft);
    connect(y, &QSlider::valueChanged, c, &GLBox::setYRotation);

    QSlider *z = new QSlider(Qt::Vertical, this);
    z->setMaximum(360);
    z->setPageStep(60);
    z->setTickPosition(QSlider::TicksLeft);
    connect(z, &QSlider::valueChanged, c, &GLBox::setZRotation);

    // Now that we have all the widgets, put them into a nice layout

    // Top level layout, puts the sliders to the left of the frame/GL widget
    QHBoxLayout *hlayout = new QHBoxLayout(this);

    // Put the sliders on top of each other
    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addWidget(x);
    vlayout->addWidget(y);
    vlayout->addWidget(z);

    // Put the GL widget inside the frame
    QHBoxLayout *flayout = new QHBoxLayout(f);
    flayout->setMargin(0);
    flayout->addWidget(c, 1);

    hlayout->setMenuBar(m);
    hlayout->addLayout(vlayout);
    hlayout->addWidget(f, 1);
}
