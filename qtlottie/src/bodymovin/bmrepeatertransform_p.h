// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMREPEATERTRANSFORM_P_H
#define BMREPEATERTRANSFORM_P_H

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

#include <QtBodymovin/private/bmbasictransform_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMRepeaterTransform : public BMBasicTransform
{
public:
    BMRepeaterTransform() = default;
    explicit BMRepeaterTransform(const BMRepeaterTransform &other);
    BMRepeaterTransform(const QJsonObject &definition, const QVersionNumber &version,
                        BMBase *parent);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    qreal startOpacity() const;
    qreal endOpacity() const;

    void setInstanceCount(int copies);
    qreal opacityAtInstance(int instance) const;

protected:
    int m_copies = 0;
    BMProperty<qreal> m_startOpacity;
    BMProperty<qreal> m_endOpacity;
    QList<qreal> m_opacities;
};

QT_END_NAMESPACE

#endif // BMREPEATERTRANSFORM_P_H
