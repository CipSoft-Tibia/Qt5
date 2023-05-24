// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMFILL_P_H
#define BMFILL_P_H

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

#include <QColor>
#include <QVector4D>

#include <QtBodymovin/private/bmgroup_p.h>
#include <QtBodymovin/private/bmproperty_p.h>

QT_BEGIN_NAMESPACE

class BODYMOVIN_EXPORT BMFill : public BMShape
{
public:
    BMFill() = default;
    explicit BMFill(const BMFill &other);
    BMFill(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent = nullptr);

    BMBase *clone() const override;

    void updateProperties(int frame) override;

    void render(LottieRenderer &renderer) const override;

    QColor color() const;
    qreal opacity() const;

protected:
    BMProperty4D<QVector4D> m_color;
    BMProperty<qreal> m_opacity;
};

QT_END_NAMESPACE

#endif // BMFILL_P_H
