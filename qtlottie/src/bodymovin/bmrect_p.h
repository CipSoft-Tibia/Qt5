// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMRECT_P_H
#define BMRECT_P_H

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

#include <QtBodymovin/private/bmshape_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmspatialproperty_p.h>
#include <QtBodymovin/private/bmfill_p.h>
#include <QtBodymovin/private/bmstroke_p.h>

QT_BEGIN_NAMESPACE

class BODYMOVIN_EXPORT BMRect : public BMShape
{
public:
    BMRect() = default;
    explicit BMRect(const BMRect &other);
    BMRect(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent = nullptr);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    bool setProperty(BMLiteral::PropertyType propertyType, QVariant value) override;

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;
    bool acceptsTrim() const override;

    QPointF position() const;
    QSizeF size() const;
    qreal roundness() const;

protected:
    BMSpatialProperty m_position;
    BMProperty2D<QSizeF> m_size;
    BMProperty<qreal> m_roundness;
};

QT_END_NAMESPACE

#endif // BMRECT_P_H
