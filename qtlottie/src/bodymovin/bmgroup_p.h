// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMGROUP_P_H
#define BMGROUP_P_H

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

#include <QJsonObject>
#include <QColor>

#include <QtBodymovin/private/bmshape_p.h>
#include <QtBodymovin/private/bmproperty_p.h>
#include <QtBodymovin/private/bmpathtrimmer_p.h>

QT_BEGIN_NAMESPACE

class BMFill;
class BMTrimPath;
class BMPathTrimmer;

class BODYMOVIN_EXPORT BMGroup : public BMShape
{
public:
    BMGroup() = default;
    BMGroup(const QJsonObject &definition, const QVersionNumber &version, BMBase *parent = nullptr);

    BMBase *clone() const override;

    void construct(const QJsonObject &definition, const QVersionNumber &version);

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    bool acceptsTrim() const override;
    void applyTrim(const BMTrimPath  &trimmer) override;
};

QT_END_NAMESPACE

#endif // BMGROUP_P_H
