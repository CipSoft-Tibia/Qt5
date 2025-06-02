// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    auto quickWidget = new QQuickWidget;
    Q3DScatterWidgetItem *graph = new Q3DScatterWidgetItem();
    graph->setWidget(quickWidget);

    QSize screenSize = graph->widget()->screen()->size();
    graph->widget()->setMinimumSize(QSize(screenSize.width() / 2, screenSize.height() / 1.5));
    graph->widget()->setMaximumSize(screenSize);
    graph->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    graph->widget()->setFocusPolicy(Qt::StrongFocus);
    graph->widget()->setResizeMode(QQuickWidget::SizeRootObjectToView);

    QWidget *widget = new QWidget;
    QHBoxLayout *hLayout = new QHBoxLayout(widget);
    QVBoxLayout *vLayout = new QVBoxLayout();
    hLayout->addWidget(graph->widget(), 1);
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
    int retVal = app.exec();
    delete modifier;
    delete quickWidget;
    return retVal;
}
