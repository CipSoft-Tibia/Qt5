// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMSHAPETRANSFORM_P_H
#define BMSHAPETRANSFORM_P_H

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
#include <QtBodymovin/private/bmbasictransform_p.h>
#include <QtBodymovin/private/bmproperty_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMShapeTransform :  public BMBasicTransform
{
public:
    explicit BMShapeTransform(const BMShapeTransform &other);
    BMShapeTransform(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    qreal skew() const;
    qreal skewAxis() const;
    qreal shearX() const;
    qreal shearY() const;
    qreal shearAngle() const;

protected:
    BMProperty<qreal> m_skew;
    BMProperty<qreal> m_skewAxis;
    qreal m_shearX;
    qreal m_shearY;
    qreal m_shearAngle;
};

QT_END_NAMESPACE

#endif // BMSHAPETRANSFORM_P_H
