// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMIMAGELAYER_P_H
#define BMIMAGELAYER_P_H

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

#include <QtBodymovin/private/bmlayer_p.h>

QT_BEGIN_NAMESPACE

class QJsonObject;

class LottieRenderer;
class BMShape;
class BMTrimPath;
class BMBasicTransform;

class BODYMOVIN_EXPORT BMImageLayer : public BMLayer
{
public:
    BMImageLayer() = default;
    explicit BMImageLayer(const BMImageLayer &other);
    BMImageLayer(const QJsonObject &definition, const QVersionNumber &version);
    ~BMImageLayer() override;

    BMBase *clone() const override;

    void updateProperties(int frame) override;
    void render(LottieRenderer &render) const override;

protected:
    QList<int> m_maskProperties;

private:
    BMTrimPath *m_appliedTrim = nullptr;
};

QT_END_NAMESPACE

#endif // BMIMAGELAYER_P_H
