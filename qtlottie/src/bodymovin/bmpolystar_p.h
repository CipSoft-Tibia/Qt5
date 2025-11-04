// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMPOLYSTAR_P_H
#define BMPOLYSTAR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QRect>
#include <QPointF>
#include <QBrush>
#include <QPen>
#include <QPainterPath>

#include <QtBodymovin/private/bmshape_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmspatialproperty_p.h>
#include <QtBodymovin/private/bmfill_p.h>
#include <QtBodymovin/private/bmstroke_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMPolyStar : public BMShape
{
public:
    BMPolyStar() = default;
    explicit BMPolyStar(const BMPolyStar &other);
    BMPolyStar(const QJsonObject &definition, const QVersionNumber &version,
               BMBase *parent = nullptr);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    bool acceptsTrim() const override;

    QPointF position() const;
    qreal outerRadius() const;
    qreal innerRadius() const;
    qreal startAngle() const;
    int pointCount() const;
    bool isPolygonModeEnabled() const;

protected:
    BMSpatialProperty m_position;
    BMProperty<int> m_pointCount;
    BMProperty<qreal> m_outerRadius;
    BMProperty<qreal> m_innerRadius;
    BMProperty<qreal> m_startAngle;
    bool m_polygonMode = false;
};

QT_END_NAMESPACE

#endif // BMPOLYSTAR_P_H
