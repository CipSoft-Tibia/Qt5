// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMREPEATER_P_H
#define BMREPEATER_P_H

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

#include <QtBodymovin/bmglobal.h>
#include <QtBodymovin/private/bmshape_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmrepeatertransform_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class BODYMOVIN_EXPORT BMRepeater : public BMShape
{
public:
    BMRepeater() = default;
    explicit BMRepeater(const BMRepeater &other) = default;
    BMRepeater(const QJsonObject &definition, const QVersionNumber &version,
               BMBase *parent = nullptr);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    int copies() const;
    qreal offset() const;
    const BMRepeaterTransform &transform() const;

protected:
    BMProperty<int> m_copies;
    BMProperty<qreal> m_offset;
    BMRepeaterTransform m_transform;
};

QT_END_NAMESPACE

#endif // BMREPEATER_P_H
