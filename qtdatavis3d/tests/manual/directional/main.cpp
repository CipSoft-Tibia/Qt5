// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "scatterdatamodifier.h"

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QLabel>
#include <QScreen>
#include <QFontDatabase>

int main(int argc, char **argv)
{
    qputenv("QSG_RHI_BACKEND", "opengl");
    QApplication app(argc, argv);
    Q3DScatter *graph = new Q3DScatter();
    QWidget *container = QWidget::createWindowContainer(graph);

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

    widget->setWindowTitle(QStringLiteral("Directional scatter"));

    QComboBox *themeList = new QComboBox(widget);
    themeList->addItem(QStringLiteral("Qt"));
    themeList->addItem(QStringLiteral("Primary Colors"));
    themeList->addItem(QStringLiteral("Digia"));
    themeList->addItem(QStringLiteral("Stone Moss"));
    themeList->addItem(QStringLiteral("Army Blue"));
    themeList->addItem(QStringLiteral("Retro"));
    themeList->addItem(QStringLiteral("Ebony"));
    themeList->addItem(QStringLiteral("Isabelle"));
    themeList->setCurrentIndex(6);

    QPushButton *labelButton = new QPushButton(widget);
    labelButton->setText(QStringLiteral("Change label style"));

    QComboBox *itemStyleList = new QComboBox(widget);
    itemStyleList->addItem(QStringLiteral("Arrow"), int(QAbstract3DSeries::MeshArrow));
    itemStyleList->addItem(QStringLiteral("Cube"), int(QAbstract3DSeries::MeshCube));
    itemStyleList->addItem(QStringLiteral("Minimal"), int(QAbstract3DSeries::MeshMinimal));
    itemStyleList->setCurrentIndex(-1);

    QPushButton *cameraButton = new QPushButton(widget);
    cameraButton->setText(QStringLiteral("Change camera preset"));

    QPushButton *toggleRotationButton = new QPushButton(widget);
    toggleRotationButton->setText(QStringLiteral("Toggle animation"));

    QCheckBox *backgroundCheckBox = new QCheckBox(widget);
    backgroundCheckBox->setText(QStringLiteral("Show background"));
    backgroundCheckBox->setChecked(true);

    QCheckBox *optimizationCheckBox = new QCheckBox(widget);
    optimizationCheckBox->setText(QStringLiteral("Optimization static"));

    QCheckBox *gridCheckBox = new QCheckBox(widget);
    gridCheckBox->setText(QStringLiteral("Show grid"));
    gridCheckBox->setChecked(true);

    QComboBox *shadowQuality = new QComboBox(widget);
    shadowQuality->addItem(QStringLiteral("None"));
    shadowQuality->addItem(QStringLiteral("Low"));
    shadowQuality->addItem(QStringLiteral("Medium"));
    shadowQuality->addItem(QStringLiteral("High"));
    shadowQuality->addItem(QStringLiteral("Low Soft"));
    shadowQuality->addItem(QStringLiteral("Medium Soft"));
    shadowQuality->addItem(QStringLiteral("High Soft"));
    shadowQuality->setCurrentIndex(4);

    QFontComboBox *fontList = new QFontComboBox(widget);
    fontList->setCurrentFont(QFont("Arial"));

    vLayout->addWidget(labelButton, 0, Qt::AlignTop);
    vLayout->addWidget(cameraButton, 0, Qt::AlignTop);
    vLayout->addWidget(toggleRotationButton, 0, Qt::AlignTop);
    vLayout->addWidget(optimizationCheckBox);
    vLayout->addWidget(backgroundCheckBox);
    vLayout->addWidget(gridCheckBox);
    vLayout->addWidget(new QLabel(QStringLiteral("Change dot style")));
    vLayout->addWidget(itemStyleList);
    vLayout->addWidget(new QLabel(QStringLiteral("Change theme")));
    vLayout->addWidget(themeList);
    vLayout->addWidget(new QLabel(QStringLiteral("Adjust shadow quality")));
    vLayout->addWidget(shadowQuality);
    vLayout->addWidget(new QLabel(QStringLiteral("Change font")));
    vLayout->addWidget(fontList, 1, Qt::AlignTop);

    ScatterDataModifier *modifier = new ScatterDataModifier(graph);

    QObject::connect(cameraButton, &QPushButton::clicked, modifier,
                     &ScatterDataModifier::changePresetCamera);
    QObject::connect(toggleRotationButton, &QPushButton::clicked, modifier,
                     &ScatterDataModifier::toggleRotation);
    QObject::connect(labelButton, &QPushButton::clicked, modifier,
                     &ScatterDataModifier::changeLabelStyle);

    QObject::connect(backgroundCheckBox, &QCheckBox::stateChanged, modifier,
                     &ScatterDataModifier::setBackgroundEnabled);
    QObject::connect(gridCheckBox, &QCheckBox::stateChanged, modifier,
                     &ScatterDataModifier::setGridEnabled);

    QObject::connect(modifier, &ScatterDataModifier::backgroundEnabledChanged,
                     backgroundCheckBox, &QCheckBox::setChecked);
    QObject::connect(optimizationCheckBox, &QCheckBox::stateChanged,
                     modifier, &ScatterDataModifier::enableOptimization);
    QObject::connect(modifier, &ScatterDataModifier::gridEnabledChanged,
                     gridCheckBox, &QCheckBox::setChecked);
    QObject::connect(itemStyleList, SIGNAL(currentIndexChanged(int)), modifier,
                     SLOT(changeStyle(int)));

    QObject::connect(themeList, SIGNAL(currentIndexChanged(int)), modifier,
                     SLOT(changeTheme(int)));

    QObject::connect(shadowQuality, SIGNAL(currentIndexChanged(int)), modifier,
                     SLOT(changeShadowQuality(int)));

    QObject::connect(modifier, &ScatterDataModifier::shadowQualityChanged, shadowQuality,
                     &QComboBox::setCurrentIndex);
    QObject::connect(graph, &Q3DScatter::shadowQualityChanged, modifier,
                     &ScatterDataModifier::shadowQualityUpdatedByVisual);

    QObject::connect(fontList, &QFontComboBox::currentFontChanged, modifier,
                     &ScatterDataModifier::changeFont);

    QObject::connect(modifier, &ScatterDataModifier::fontChanged, fontList,
                     &QFontComboBox::setCurrentFont);

    itemStyleList->setCurrentIndex(0);
    optimizationCheckBox->setChecked(true);

    widget->show();
    return app.exec();
}
