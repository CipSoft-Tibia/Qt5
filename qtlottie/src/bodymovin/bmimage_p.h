// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMIMAGE_P_H
#define BMIMAGE_P_H

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

#include <QImage>
#include <QPointF>

#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmspatialproperty_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMImage : public BMBase
{
public:
    BMImage() = default;
    explicit BMImage(const BMImage &other);
    BMImage(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent = nullptr);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    QPointF position() const;
    qreal radius() const;

    QPointF getCenter() const { return m_center; }
    QImage getImage() const { return m_image; }

protected:
    BMSpatialProperty m_position;
    BMProperty<qreal> m_radius;

    QImage m_image;
    QPointF m_center;
};

QT_END_NAMESPACE

#endif // BMIMAGE_P_H
