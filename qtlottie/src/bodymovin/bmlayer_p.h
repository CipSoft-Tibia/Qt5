// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BMLAYER_P_H
#define BMLAYER_P_H

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

#include <QtBodymovin/private/bmbase_p.h>

QT_BEGIN_NAMESPACE

class LottieRenderer;

class BODYMOVIN_EXPORT BMLayer : public BMBase
{
public:
    enum MatteClipMode {NoClip, Alpha, InvertedAlpha, Luminence, InvertedLuminence};

    BMLayer() = default;
   explicit  BMLayer (const BMLayer &other);
    ~BMLayer() override;

    BMBase *clone() const override;

    static BMLayer *construct(QJsonObject definition, const QVersionNumber &version);

    bool active(int frame) const override;

    void parse(const QJsonObject &definition) override;

    void updateProperties(int frame) override;
    void render(LottieRenderer &renderer) const override;

    BMBase *findChild(const QString &childName) override;

    bool isClippedLayer() const;
    bool isMaskLayer() const;
    MatteClipMode clipMode() const;

    int layerId() const;
    BMBasicTransform *transform() const;

protected:
    void renderEffects(LottieRenderer &renderer) const;

    virtual BMLayer *resolveLinkedLayer();
    virtual BMLayer *linkedLayer() const;

    int m_layerIndex = 0;
    int m_startFrame;
    int m_endFrame;
    qreal m_startTime;
    int m_blendMode;
    bool m_3dLayer = false;
    BMBase *m_effects = nullptr;
    qreal m_stretch;
    BMBasicTransform *m_layerTransform = nullptr;

    int m_parentLayer = 0;
    int m_td = 0;
    MatteClipMode m_clipMode = NoClip;

private:
    void parseEffects(const QJsonArray &definition, BMBase *effectRoot = nullptr);

    BMLayer *m_linkedLayer = nullptr;
};

QT_END_NAMESPACE

#endif // BMLAYER_P_H
