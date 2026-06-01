// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QLOTTIEFILL_P_H
#define QLOTTIEFILL_P_H

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

#include <QtLottie/private/qlottiegroup_p.h>
#include <QtLottie/private/qlottieproperty_p.h>

QT_BEGIN_NAMESPACE

class Q_LOTTIE_EXPORT QLottieFill : public QLottieShape
{
public:
    QLottieFill() = default;
    explicit QLottieFill(const QLottieFill &other);
    QLottieFill(const QJsonObject &definition, QLottieBase *parent = nullptr);

    QLottieBase *clone() const override;

    void updateProperties(int frame) override;

    void render(QLottieRenderer &renderer) const override;

    QColor color() const;
    qreal opacity() const;

protected:
    QLottieProperty4D<QVector4D> m_color;
    QLottieProperty<qreal> m_opacity;
};

QT_END_NAMESPACE

#endif // QLOTTIEFILL_P_H
