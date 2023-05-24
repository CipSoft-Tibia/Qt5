// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMFILLEFFECT_P_H
#define BMFILLEFFECT_P_H

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

#include <QtBodymovin/private/bmbase_p.h>
#include <QtBodymovin/private/bmproperty_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMFillEffect : public BMBase
{
public:
    BMFillEffect() = default;
    explicit BMFillEffect(const BMFillEffect &other);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    QColor color() const;
    qreal opacity() const;

protected:
    BMProperty4D<QVector4D> m_color;
    BMProperty<qreal> m_opacity;
};

QT_END_NAMESPACE

#endif // BMFILLEFFECT_P_H
