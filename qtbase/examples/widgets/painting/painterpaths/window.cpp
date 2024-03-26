// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "renderarea.h"
#include "window.h"

#include <QtWidgets>

#include <qmath.h>

//! [1]
Window::Window()
{
    QPainterPath rectPath;
    rectPath.moveTo(20.0, 30.0);
    rectPath.lineTo(80.0, 30.0);
    rectPath.lineTo(80.0, 70.0);
    rectPath.lineTo(20.0, 70.0);
    rectPath.closeSubpath();
//! [1]

//! [2]
    QPainterPath roundRectPath;
    roundRectPath.moveTo(80.0, 35.0);
    roundRectPath.arcTo(70.0, 30.0, 10.0, 10.0, 0.0, 90.0);
    roundRectPath.lineTo(25.0, 30.0);
    roundRectPath.arcTo(20.0, 30.0, 10.0, 10.0, 90.0, 90.0);
    roundRectPath.lineTo(20.0, 65.0);
    roundRectPath.arcTo(20.0, 60.0, 10.0, 10.0, 180.0, 90.0);
    roundRectPath.lineTo(75.0, 70.0);
    roundRectPath.arcTo(70.0, 60.0, 10.0, 10.0, 270.0, 90.0);
    roundRectPath.closeSubpath();
//! [2]

//! [3]
    QPainterPath ellipsePath;
    ellipsePath.moveTo(80.0, 50.0);
    ellipsePath.arcTo(20.0, 30.0, 60.0, 40.0, 0.0, 360.0);
//! [3]

//! [4]
    QPainterPath piePath;
    piePath.moveTo(50.0, 50.0);
    piePath.arcTo(20.0, 30.0, 60.0, 40.0, 60.0, 240.0);
    piePath.closeSubpath();
//! [4]

//! [5]
    QPainterPath polygonPath;
    polygonPath.moveTo(10.0, 80.0);
    polygonPath.lineTo(20.0, 10.0);
    polygonPath.lineTo(80.0, 30.0);
    polygonPath.lineTo(90.0, 70.0);
    polygonPath.closeSubpath();
//! [5]

//! [6]
    QPainterPath groupPath;
    groupPath.moveTo(60.0, 40.0);
    groupPath.arcTo(20.0, 20.0, 40.0, 40.0, 0.0, 360.0);
    groupPath.moveTo(40.0, 40.0);
    groupPath.lineTo(40.0, 80.0);
    groupPath.lineTo(80.0, 80.0);
    groupPath.lineTo(80.0, 40.0);
    groupPath.closeSubpath();
//! [6]

//! [7]
    QPainterPath textPath;
    QFont timesFont("Times", 50);
    timesFont.setStyleStrategy(QFont::ForceOutline);
    textPath.addText(10, 70, timesFont, tr("Qt"));
//! [7]

//! [8]
    QPainterPath bezierPath;
    bezierPath.moveTo(20, 30);
    bezierPath.cubicTo(80, 0, 50, 50, 80, 80);
//! [8]

//! [9]
    QPainterPath starPath;
    starPath.moveTo(90, 50);
    for (int i = 1; i < 5; ++i) {
        starPath.lineTo(50 + 40 * std::cos(0.8 * i * M_PI),
                        50 + 40 * std::sin(0.8 * i * M_PI));
    }
    starPath.closeSubpath();
//! [9]

//! [10]
    renderAreas.push_back(new RenderArea(rectPath));
    renderAreas.push_back(new RenderArea(roundRectPath));
    renderAreas.push_back(new RenderArea(ellipsePath));
    renderAreas.push_back(new RenderArea(piePath));
    renderAreas.push_back(new RenderArea(polygonPath));
    renderAreas.push_back(new RenderArea(groupPath));
    renderAreas.push_back(new RenderArea(textPath));
    renderAreas.push_back(new RenderArea(bezierPath));
    renderAreas.push_back(new RenderArea(starPath));
//! [10]

//! [11]
    fillRuleComboBox = new QComboBox;
    fillRuleComboBox->addItem(tr("Odd Even"), Qt::OddEvenFill);
    fillRuleComboBox->addItem(tr("Winding"), Qt::WindingFill);

    fillRuleLabel = new QLabel(tr("Fill &Rule:"));
    fillRuleLabel->setBuddy(fillRuleComboBox);
//! [11]

//! [12]
    fillColor1ComboBox = new QComboBox;
    populateWithColors(fillColor1ComboBox);
    fillColor1ComboBox->setCurrentIndex(fillColor1ComboBox->findText("mediumslateblue"));

    fillColor2ComboBox = new QComboBox;
    populateWithColors(fillColor2ComboBox);
    fillColor2ComboBox->setCurrentIndex(fillColor2ComboBox->findText("cornsilk"));

    fillGradientLabel = new QLabel(tr("&Fill Gradient:"));
    fillGradientLabel->setBuddy(fillColor1ComboBox);

    fillToLabel = new QLabel(tr("to"));
    fillToLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    penWidthSpinBox = new QSpinBox;
    penWidthSpinBox->setRange(0, 20);

    penWidthLabel = new QLabel(tr("&Pen Width:"));
    penWidthLabel->setBuddy(penWidthSpinBox);

    penColorComboBox = new QComboBox;
    populateWithColors(penColorComboBox);
    penColorComboBox->setCurrentIndex(penColorComboBox->findText("darkslateblue"));

    penColorLabel = new QLabel(tr("Pen &Color:"));
    penColorLabel->setBuddy(penColorComboBox);

    rotationAngleSpinBox = new QSpinBox;
    rotationAngleSpinBox->setRange(0, 359);
    rotationAngleSpinBox->setWrapping(true);
    rotationAngleSpinBox->setSuffix(QLatin1String("\xB0"));

    rotationAngleLabel = new QLabel(tr("&Rotation Angle:"));
    rotationAngleLabel->setBuddy(rotationAngleSpinBox);
//! [12]

//! [16]
    connect(fillRuleComboBox, &QComboBox::activated,
            this, &Window::fillRuleChanged);
    connect(fillColor1ComboBox, &QComboBox::activated,
            this, &Window::fillGradientChanged);
    connect(fillColor2ComboBox, &QComboBox::activated,
            this, &Window::fillGradientChanged);
    connect(penColorComboBox, &QComboBox::activated,
            this, &Window::penColorChanged);

    for (RenderArea *area : std::as_const(renderAreas)) {
        connect(penWidthSpinBox, &QSpinBox::valueChanged,
                area, &RenderArea::setPenWidth);
        connect(rotationAngleSpinBox, &QSpinBox::valueChanged,
                area, &RenderArea::setRotationAngle);
    }

//! [16] //! [17]
    QGridLayout *topLayout = new QGridLayout;

    int i = 0;
    for (RenderArea *area : std::as_const(renderAreas)) {
        topLayout->addWidget(area, i / 3, i % 3);
        ++i;
    }

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addLayout(topLayout, 0, 0, 1, 4);
    mainLayout->addWidget(fillRuleLabel, 1, 0);
    mainLayout->addWidget(fillRuleComboBox, 1, 1, 1, 3);
    mainLayout->addWidget(fillGradientLabel, 2, 0);
    mainLayout->addWidget(fillColor1ComboBox, 2, 1);
    mainLayout->addWidget(fillToLabel, 2, 2);
    mainLayout->addWidget(fillColor2ComboBox, 2, 3);
    mainLayout->addWidget(penWidthLabel, 3, 0);
    mainLayout->addWidget(penWidthSpinBox, 3, 1, 1, 3);
    mainLayout->addWidget(penColorLabel, 4, 0);
    mainLayout->addWidget(penColorComboBox, 4, 1, 1, 3);
    mainLayout->addWidget(rotationAngleLabel, 5, 0);
    mainLayout->addWidget(rotationAngleSpinBox, 5, 1, 1, 3);
    setLayout(mainLayout);
//! [17]

//! [18]
    fillRuleChanged();
    fillGradientChanged();
    penColorChanged();
    penWidthSpinBox->setValue(2);

    setWindowTitle(tr("Painter Paths"));
}
//! [18]

//! [19]
void Window::fillRuleChanged()
{
    Qt::FillRule rule = (Qt::FillRule)currentItemData(fillRuleComboBox).toInt();

    for (RenderArea *area : std::as_const(renderAreas))
        area->setFillRule(rule);
}
//! [19]

//! [20]
void Window::fillGradientChanged()
{
    QColor color1 = qvariant_cast<QColor>(currentItemData(fillColor1ComboBox));
    QColor color2 = qvariant_cast<QColor>(currentItemData(fillColor2ComboBox));

    for (RenderArea *area : std::as_const(renderAreas))
        area->setFillGradient(color1, color2);
}
//! [20]

//! [21]
void Window::penColorChanged()
{
    QColor color = qvariant_cast<QColor>(currentItemData(penColorComboBox));

    for (RenderArea *area : std::as_const(renderAreas))
        area->setPenColor(color);
}
//! [21]

//! [22]
void Window::populateWithColors(QComboBox *comboBox)
{
    const QStringList colorNames = QColor::colorNames();
    for (const QString &name : colorNames)
        comboBox->addItem(name, QColor(name));
}
//! [22]

//! [23]
QVariant Window::currentItemData(QComboBox *comboBox)
{
    return comboBox->itemData(comboBox->currentIndex());
}
//! [23]
