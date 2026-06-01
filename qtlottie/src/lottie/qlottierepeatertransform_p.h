// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LOTTIEREPEATERTRANSFORM_P_H
#define LOTTIEREPEATERTRANSFORM_P_H

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

#include <QtLottie/private/qlottiebasictransform_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class Q_LOTTIE_EXPORT QLottieRepeaterTransform : public QLottieBasicTransform
{
public:
    QLottieRepeaterTransform() = default;
    explicit QLottieRepeaterTransform(const QLottieRepeaterTransform &other);
    QLottieRepeaterTransform(const QJsonObject &definition,
                        QLottieBase *parent);

    QLottieBase *clone() const override;

    void construct(const QJsonObject &definition);

    void updateProperties(int frame) override;
    void render(QLottieRenderer &renderer) const override;

    qreal startOpacity() const;
    qreal endOpacity() const;

    void setInstanceCount(int copies);
    qreal opacityAtInstance(int instance) const;

protected:
    int m_copies = 0;
    QLottieProperty<qreal> m_startOpacity;
    QLottieProperty<qreal> m_endOpacity;
    QList<qreal> m_opacities;
};

QT_END_NAMESPACE

#endif // LottieREPEATERTRANSFORM_P_H
