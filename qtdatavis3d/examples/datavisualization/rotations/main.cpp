/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Data Visualization module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "scatterdatamodifier.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtGui/QScreen>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Q3DScatter *graph = new Q3DScatter();
    QWidget *container = QWidget::createWindowContainer(graph);

    if (!graph->hasContext()) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't initialize the OpenGL context.");
        msgBox.exec();
        return -1;
    }

    QSize screenSize = graph->screen()->size();
    container->setMinimumSize(QSize(screenSize.width() / 2, screenSize.height() / 1.5));
    container->setMaximumSize(screenSize);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setFocusPolicy(Qt::StrongFocus);

    QWidget *widget = new QWidget;
    QHBoxLayout *hLayout = new QHBoxLayout(widget);
    QVBoxLayout *vLayout = new QVBoxLayout();
    hLayout->addWidget(container, 1);
    hLayout->addLayout(vLayout);

    widget->setWindowTitle(QStringLiteral("Item rotations example - Magnetic field of the sun"));

    QPushButton *toggleRotationButton = new QPushButton(widget);
    toggleRotationButton->setText(QStringLiteral("Toggle animation"));
    QPushButton *toggleSunButton = new QPushButton(widget);
    toggleSunButton->setText(QStringLiteral("Toggle Sun"));

    QSlider *fieldLinesSlider = new QSlider(Qt::Horizontal, widget);
    fieldLinesSlider->setTickInterval(1);
    fieldLinesSlider->setMinimum(1);
    fieldLinesSlider->setValue(12);
    fieldLinesSlider->setMaximum(128);

    QSlider *arrowsSlider = new QSlider(Qt::Horizontal, widget);
    arrowsSlider->setTickInterval(1);
    arrowsSlider->setMinimum(8);
    arrowsSlider->setValue(16);
    arrowsSlider->setMaximum(32);

    vLayout->addWidget(toggleRotationButton);
    vLayout->addWidget(toggleSunButton);
    vLayout->addWidget(new QLabel(QStringLiteral("Field Lines (1 - 128):")));
    vLayout->addWidget(fieldLinesSlider);
    vLayout->addWidget(new QLabel(QStringLiteral("Arrows per line (8 - 32):")));
    vLayout->addWidget(arrowsSlider, 1, Qt::AlignTop);

    ScatterDataModifier *modifier = new ScatterDataModifier(graph);

    QObject::connect(toggleRotationButton, &QPushButton::clicked, modifier,
                     &ScatterDataModifier::toggleRotation);
    QObject::connect(toggleSunButton, &QPushButton::clicked, modifier,
                     &ScatterDataModifier::toggleSun);
    QObject::connect(fieldLinesSlider, &QSlider::valueChanged, modifier,
                     &ScatterDataModifier::setFieldLines);
    QObject::connect(arrowsSlider, &QSlider::valueChanged, modifier,
                     &ScatterDataModifier::setArrowsPerLine);

    widget->show();
    return app.exec();
}
