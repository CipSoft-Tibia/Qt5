// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMBASICTRANSFORM_P_H
#define BMBASICTRANSFORM_P_H

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

#include <QPointF>

#include <QtBodymovin/private/bmshape_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmspatialproperty_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMBasicTransform : public BMShape
{
public:
    BMBasicTransform() = default;
    explicit BMBasicTransform(const BMBasicTransform &other);
    BMBasicTransform(const QJsonObject &definition, const QVersionNumber &version,
                     BMBase *parent = nullptr);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    QPointF anchorPoint() const;
    virtual QPointF position() const;
    QPointF scale() const;
    qreal rotation() const;
    qreal opacity() const;

protected:
    BMSpatialProperty m_anchorPoint;
    bool m_splitPosition = false;
    BMSpatialProperty m_position;
    BMProperty<qreal> m_xPos;
    BMProperty<qreal> m_yPos;
    BMProperty2D<QPointF> m_scale;
    BMProperty<qreal> m_rotation;
    BMProperty<qreal> m_opacity;
};

QT_END_NAMESPACE

#endif // BMBASICTRANSFORM_P_H
