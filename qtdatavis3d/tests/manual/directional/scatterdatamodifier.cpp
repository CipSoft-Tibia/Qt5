// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "scatterdatamodifier.h"
#include <QtDataVisualization/qscatterdataproxy.h>
#include <QtDataVisualization/qvalue3daxis.h>
#include <QtDataVisualization/q3dscene.h>
#include <QtDataVisualization/q3dcamera.h>
#include <QtDataVisualization/qscatter3dseries.h>
#include <QtDataVisualization/q3dtheme.h>
#include <qmath.h>
#include <QComboBox>

const int numberOfCols = 8;
const int numberOfRows = 8;
const float limit = 8.0f;
#define HEDGEHOG

ScatterDataModifier::ScatterDataModifier(Q3DScatter *scatter)
    : m_graph(scatter),
      m_fontSize(40.0f),
      m_style(QAbstract3DSeries::MeshUserDefined),
      m_smooth(true)
{
    m_graph->activeTheme()->setType(Q3DTheme::ThemeEbony);
    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(m_fontSize);
    m_graph->activeTheme()->setFont(font);
    m_graph->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftLow);
    m_graph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetFront);

    m_graph->setAxisX(new QValue3DAxis);
    m_graph->setAxisY(new QValue3DAxis);
    m_graph->setAxisZ(new QValue3DAxis);

    QScatterDataProxy *proxy = new QScatterDataProxy;
    QScatter3DSeries *series = new QScatter3DSeries(proxy);
    series->setItemLabelFormat("@xTitle: @xLabel @yTitle: @yLabel @zTitle: @zLabel");
    m_graph->addSeries(series);

    QObject::connect(&m_rotationTimer, &QTimer::timeout, this,
                     &ScatterDataModifier::triggerRotation);

    addData();
}

ScatterDataModifier::~ScatterDataModifier()
{
    delete m_graph;
}

void ScatterDataModifier::addData()
{
    // Configure the axes according to the data
    m_graph->axisX()->setTitle("X");
    m_graph->axisY()->setTitle("Y");
    m_graph->axisZ()->setTitle("Z");
    m_graph->axisX()->setRange(-limit, limit);
    m_graph->axisY()->setRange(-1.0f, 1.0f);
    m_graph->axisZ()->setRange(-limit, limit);

    QScatterDataArray *dataArray = new QScatterDataArray;
    dataArray->resize(numberOfCols * numberOfRows);
    QScatterDataItem *ptrToDataArray = &dataArray->first();

    float angleStep = 360.0f / float(numberOfCols);
    float latAngleStep = 100.0f / float(numberOfRows);

    for (float i = 0; i < numberOfRows; i++) {
        float latAngle = float(i) * latAngleStep + 40.0f;
        float radius = qSin(qDegreesToRadians(latAngle)) * limit;
        float y = qCos(qDegreesToRadians(latAngle)) * 1.0f;
#ifdef HEDGEHOG
        float angleZ = qRadiansToDegrees(qAtan((y * limit / 2.0f) / radius));
        QQuaternion rotationZ = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 0.0f, 1.0f), angleZ - 90.0f);
#endif
        for (float j = 0; j < numberOfCols; j++) {
            float angle = float(j) * angleStep;
            float x = qCos(qDegreesToRadians(angle)) * radius;
            float z = qSin(qDegreesToRadians(angle)) * radius;

            float angleY = qRadiansToDegrees(qAtan(z / x));
            if (x < 0)
                angleY = 180.0f + angleY;
            if (x > 0 && z < 0)
                angleY = 360.0f + angleY;
#ifdef HEDGEHOG
            QQuaternion rotationY = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), angleY);
            QQuaternion rotation = rotationY * rotationZ;
#else
            QQuaternion rotation = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), angleY) *
                QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), -90.0f);
#endif

            ptrToDataArray->setPosition(QVector3D(x, y, z));
            ptrToDataArray->setRotation(rotation);
            ptrToDataArray++;
        }
    }

    m_graph->seriesList().at(0)->dataProxy()->resetArray(dataArray);
}

void ScatterDataModifier::enableOptimization(int enabled)
{
    if (enabled)
        m_graph->setOptimizationHints(QAbstract3DGraph::OptimizationStatic);
    else
        m_graph->setOptimizationHints(QAbstract3DGraph::OptimizationDefault);
}

void ScatterDataModifier::changeStyle(int style)
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(sender());
    if (comboBox) {
        m_style = QAbstract3DSeries::Mesh(comboBox->itemData(style).toInt());
        if (m_graph->seriesList().size())
            m_graph->seriesList().at(0)->setMesh(m_style);
    }
}

void ScatterDataModifier::changeTheme(int theme)
{
    Q3DTheme *currentTheme = m_graph->activeTheme();
    currentTheme->setType(Q3DTheme::Theme(theme));
    emit backgroundEnabledChanged(currentTheme->isBackgroundEnabled());
    emit gridEnabledChanged(currentTheme->isGridEnabled());
    emit fontChanged(currentTheme->font());
}

void ScatterDataModifier::changePresetCamera()
{
    static int preset = Q3DCamera::CameraPresetFrontLow;

    m_graph->scene()->activeCamera()->setCameraPreset((Q3DCamera::CameraPreset)preset);

    if (++preset > Q3DCamera::CameraPresetDirectlyBelow)
        preset = Q3DCamera::CameraPresetFrontLow;
}

void ScatterDataModifier::changeLabelStyle()
{
    m_graph->activeTheme()->setLabelBackgroundEnabled(!m_graph->activeTheme()->isLabelBackgroundEnabled());
}

void ScatterDataModifier::changeFont(const QFont &font)
{
    QFont newFont = font;
    newFont.setPointSizeF(m_fontSize);
    m_graph->activeTheme()->setFont(newFont);
}

void ScatterDataModifier::shadowQualityUpdatedByVisual(QAbstract3DGraph::ShadowQuality sq)
{
    int quality = int(sq);
    emit shadowQualityChanged(quality); // connected to a checkbox in main.cpp
}

void ScatterDataModifier::triggerRotation()
{
    if (m_graph->seriesList().size()) {
        int selectedIndex = m_graph->seriesList().at(0)->selectedItem();
        if (selectedIndex != QScatter3DSeries::invalidSelectionIndex()) {
            static float itemAngle = 0.0f;
            QScatterDataItem item(*(m_graph->seriesList().at(0)->dataProxy()->itemAt(selectedIndex)));
            QQuaternion itemRotation = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, itemAngle++);
            item.setRotation(itemRotation);
            m_graph->seriesList().at(0)->dataProxy()->setItem(selectedIndex, item);
        } else {
            static float seriesAngle = 0.0f;
            QQuaternion rotation = QQuaternion::fromAxisAndAngle(1.0f, 1.0f, 1.0f, seriesAngle++);
            m_graph->seriesList().at(0)->setMeshRotation(rotation);
        }
    }
}

void ScatterDataModifier::changeShadowQuality(int quality)
{
    QAbstract3DGraph::ShadowQuality sq = QAbstract3DGraph::ShadowQuality(quality);
    m_graph->setShadowQuality(sq);
}

void ScatterDataModifier::setBackgroundEnabled(int enabled)
{
    m_graph->activeTheme()->setBackgroundEnabled((bool)enabled);
}

void ScatterDataModifier::setGridEnabled(int enabled)
{
    m_graph->activeTheme()->setGridEnabled((bool)enabled);
}

void ScatterDataModifier::toggleRotation()
{
    if (m_rotationTimer.isActive())
        m_rotationTimer.stop();
    else
        m_rotationTimer.start(20);
}
