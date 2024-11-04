// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef AXISGRID_H
#define AXISGRID_H

//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtQuick/private/qquickshadereffect_p.h>

QT_BEGIN_NAMESPACE

class AxisGrid : public QQuickShaderEffect
{
    Q_OBJECT
    Q_PROPERTY(QVector3D iResolution READ iResolution NOTIFY iResolutionChanged FINAL)
    Q_PROPERTY(qreal smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged FINAL)
    Q_PROPERTY(int origo READ origo WRITE setOrigo NOTIFY origoChanged FINAL)
    Q_PROPERTY(QVector4D barsVisibility READ barsVisibility WRITE setBarsVisibility NOTIFY barsVisibilityChanged FINAL)
    Q_PROPERTY(qreal gridWidth READ gridWidth WRITE setGridWidth NOTIFY gridWidthChanged FINAL)
    Q_PROPERTY(qreal gridHeight READ gridHeight WRITE setGridHeight NOTIFY gridHeightChanged FINAL)
    Q_PROPERTY(QPointF gridMovement READ gridMovement WRITE setGridMovement NOTIFY gridMovementChanged FINAL)
    Q_PROPERTY(QColor minorColor READ minorColor WRITE setMinorColor NOTIFY minorColorChanged FINAL)
    Q_PROPERTY(QColor majorColor READ majorColor WRITE setMajorColor NOTIFY majorColorChanged FINAL)
    Q_PROPERTY(qreal minorBarWidth READ minorBarWidth WRITE setMinorBarWidth NOTIFY minorBarWidthChanged FINAL)
    Q_PROPERTY(qreal majorBarWidth READ majorBarWidth WRITE setMajorBarWidth NOTIFY majorBarWidthChanged FINAL)
    Q_PROPERTY(qreal verticalMinorTickScale READ verticalMinorTickScale WRITE setVerticalMinorTickScale NOTIFY verticalMinorTickScaleChanged FINAL)
    Q_PROPERTY(qreal horizontalMinorTickScale READ horizontalMinorTickScale WRITE setHorizontalMinorTickScale NOTIFY horizontalMinorTickScaleChanged FINAL)
    QML_ELEMENT
public:
    AxisGrid(QQuickItem *parent = nullptr);

    void setupShaders();

    QVector3D iResolution() const;

    qreal smoothing() const;
    void setSmoothing(qreal newSmoothing);

    int origo() const;
    void setOrigo(int newOrigo);

    QVector4D barsVisibility() const;
    void setBarsVisibility(const QVector4D &newBarsVisibility);

    qreal gridWidth() const;
    void setGridWidth(qreal newGridWidth);

    qreal gridHeight() const;
    void setGridHeight(qreal newGridHeight);

    QPointF gridMovement() const;
    void setGridMovement(QPointF newGridMovement);

    QColor minorColor() const;
    void setMinorColor(const QColor &newMinorColor);

    QColor majorColor() const;
    void setMajorColor(const QColor &newMajorColor);

    qreal minorBarWidth() const;
    void setMinorBarWidth(qreal newMinorBarWidth);

    qreal majorBarWidth() const;
    void setMajorBarWidth(qreal newMajorBarWidth);

    qreal verticalMinorTickScale() const;
    void setVerticalMinorTickScale(qreal newVerticalMinorTickScale);

    qreal horizontalMinorTickScale() const;
    void setHorizontalMinorTickScale(qreal newHorizontalMinorTickScale);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

Q_SIGNALS:
    void iResolutionChanged();
    void smoothingChanged();
    void origoChanged();
    void barsVisibilityChanged();
    void gridWidthChanged();
    void gridHeightChanged();
    void gridMovementChanged();
    void minorColorChanged();
    void majorColorChanged();
    void minorBarWidthChanged();
    void majorBarWidthChanged();
    void verticalMinorTickScaleChanged();
    void horizontalMinorTickScaleChanged();

private:
    friend class AxisRenderer;
    qreal m_smoothing = 1.0;
    QVector3D m_iResolution;
    int m_origo = 0;
    QVector4D m_barsVisibility = QVector4D(1, 1, 1, 1);
    qreal m_gridWidth = 100;
    qreal m_gridHeight = 100;
    QPointF m_gridMovement;
    QColor m_minorColor = QColor(150, 150, 150);
    QColor m_majorColor = QColor(255, 255, 255);
    qreal m_minorBarWidth = 1.0;
    qreal m_majorBarWidth = 2.0;
    qreal m_verticalMinorTickScale = 0.1;
    qreal m_horizontalMinorTickScale = 0.1;
};

QT_END_NAMESPACE

#endif // AXISGRID_H
