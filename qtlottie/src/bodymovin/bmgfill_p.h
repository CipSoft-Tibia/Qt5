// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMGFILL_P_H
#define BMGFILL_P_H

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

#include <QVector4D>
#include <QGradient>

#include <QtBodymovin/private/bmgroup_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmspatialproperty_p.h>

QT_BEGIN_NAMESPACE

class BODYMOVIN_EXPORT BMGFill : public BMShape
{
public:
    BMGFill() = default;
    explicit BMGFill(const BMGFill &other);
    BMGFill(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent = nullptr);
    ~BMGFill() override;

    BMBase *clone() const override;

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    QGradient *value() const;
    QGradient::Type gradientType() const;
    QPointF startPoint() const;
    QPointF endPoint() const;
    qreal highlightLength() const;
    qreal highlightAngle() const;
    qreal opacity() const;

private:
    void setGradient();

protected:
    BMProperty<qreal> m_opacity;
    BMSpatialProperty m_startPoint;
    BMSpatialProperty m_endPoint;
    BMProperty<qreal> m_highlightLength;
    BMProperty<qreal> m_highlightAngle;
    QList<BMProperty4D<QVector4D>> m_colors;
    QGradient *m_gradient = nullptr;

};

QT_END_NAMESPACE

#endif // BMGFILL_P_H
