// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef AXISTICKER_H
#define AXISTICKER_H

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

class AxisTicker : public QQuickShaderEffect
{
    Q_OBJECT
    Q_PROPERTY(QVector3D iResolution READ iResolution NOTIFY iResolutionChanged FINAL)
    Q_PROPERTY(qreal smoothing READ smoothing WRITE setSmoothing NOTIFY smoothingChanged FINAL)
    Q_PROPERTY(int origo READ origo WRITE setOrigo NOTIFY origoChanged FINAL)
    Q_PROPERTY(bool minorBarsVisible READ minorBarsVisible WRITE setMinorBarsVisible NOTIFY minorBarsVisibleChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qreal barsMovement READ barsMovement WRITE setBarsMovement NOTIFY barsMovementChanged FINAL)
    Q_PROPERTY(QColor minorColor READ minorColor WRITE setMinorColor NOTIFY minorColorChanged FINAL)
    Q_PROPERTY(QColor majorColor READ majorColor WRITE setMajorColor NOTIFY majorColorChanged FINAL)
    Q_PROPERTY(qreal minorBarWidth READ minorBarWidth WRITE setMinorBarWidth NOTIFY minorBarWidthChanged FINAL)
    Q_PROPERTY(qreal majorBarWidth READ majorBarWidth WRITE setMajorBarWidth NOTIFY majorBarWidthChanged FINAL)
    Q_PROPERTY(qreal minorTickScale READ minorTickScale WRITE setMinorTickScale NOTIFY minorTickScaleChanged FINAL)
    Q_PROPERTY(qreal minorBarsLength READ minorBarsLength WRITE setMinorBarsLength NOTIFY minorBarsLengthChanged FINAL)
    Q_PROPERTY(bool isHorizontal READ isHorizontal WRITE setIsHorizontal NOTIFY isHorizontalChanged FINAL)
    QML_ELEMENT
public:
    AxisTicker(QQuickItem *parent = nullptr);

    void setupShaders();

    QVector3D iResolution() const;

    qreal smoothing() const;
    void setSmoothing(qreal newSmoothing);

    int origo() const;
    void setOrigo(int newOrigo);

    bool minorBarsVisible() const;
    void setMinorBarsVisible(bool newMinorBarsVisible);

    qreal spacing() const;
    void setSpacing(qreal newSpacing);

    qreal barsMovement() const;
    void setBarsMovement(qreal newBarsMovement);

    QColor minorColor() const;
    void setMinorColor(const QColor &newMinorColor);

    QColor majorColor() const;
    void setMajorColor(const QColor &newMajorColor);

    qreal minorBarWidth() const;
    void setMinorBarWidth(qreal newMinorBarWidth);

    qreal majorBarWidth() const;
    void setMajorBarWidth(qreal newMajorBarWidth);

    qreal minorTickScale() const;
    void setMinorTickScale(qreal newMinorTickScale);

    qreal minorBarsLength() const;
    void setMinorBarsLength(qreal newMinorBarsLength);

    bool isHorizontal() const;
    void setIsHorizontal(bool newIsHorizontal);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

Q_SIGNALS:
    void iResolutionChanged();
    void smoothingChanged();
    void origoChanged();
    void minorBarsVisibleChanged();
    void spacingChanged();
    void barsMovementChanged();
    void minorColorChanged();
    void majorColorChanged();
    void minorBarWidthChanged();
    void majorBarWidthChanged();
    void minorTickScaleChanged();
    void minorBarsLengthChanged();
    void isHorizontalChanged();

private:
    friend class AxisRenderer;
    QVector3D m_iResolution;
    qreal m_smoothing = 1.0;
    int m_origo = 0;
    bool m_minorBarsVisible = true;
    qreal m_spacing = 100;
    qreal m_barsMovement = 0;
    QColor m_minorColor = QColor(150, 150, 150);
    QColor m_majorColor = QColor(255, 255, 255);
    qreal m_minorBarWidth = 1.0;
    qreal m_majorBarWidth = 2.0;
    qreal m_minorTickScale = 0.1;
    qreal m_minorBarsLength = 0.1;
    bool m_isHorizontal = false;
};

QT_END_NAMESPACE

#endif // AXISTICKER_H
