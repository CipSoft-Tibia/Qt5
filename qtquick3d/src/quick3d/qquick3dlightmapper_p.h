// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DLIGHTMAPPER_P_H
#define QQUICK3DLIGHTMAPPER_P_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DLightmapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float opacityThreshold READ opacityThreshold WRITE setOpacityThreshold NOTIFY opacityThresholdChanged)
    Q_PROPERTY(float bias READ bias WRITE setBias NOTIFY biasChanged)
    Q_PROPERTY(bool adaptiveBiasEnabled READ isAdaptiveBiasEnabled WRITE setAdaptiveBiasEnabled NOTIFY adaptiveBiasEnabledChanged)
    Q_PROPERTY(bool indirectLightEnabled READ isIndirectLightEnabled WRITE setIndirectLightEnabled NOTIFY indirectLightEnabledChanged)
    Q_PROPERTY(int samples READ samples WRITE setSamples NOTIFY samplesChanged)
    Q_PROPERTY(int indirectLightWorkgroupSize READ indirectLightWorkgroupSize WRITE setIndirectLightWorkgroupSize NOTIFY indirectLightWorkgroupSizeChanged)
    Q_PROPERTY(int bounces READ bounces WRITE setBounces NOTIFY bouncesChanged)
    Q_PROPERTY(float indirectLightFactor READ indirectLightFactor WRITE setIndirectLightFactor NOTIFY indirectLightFactorChanged)

    QML_NAMED_ELEMENT(Lightmapper)

public:
    float opacityThreshold() const;
    float bias() const;
    bool isAdaptiveBiasEnabled() const;
    bool isIndirectLightEnabled() const;
    int samples() const;
    int indirectLightWorkgroupSize() const;
    int bounces() const;
    float indirectLightFactor() const;

public Q_SLOTS:
    void setOpacityThreshold(float opacity);
    void setBias(float bias);
    void setAdaptiveBiasEnabled(bool enabled);
    void setIndirectLightEnabled(bool enabled);
    void setSamples(int count);
    void setIndirectLightWorkgroupSize(int size);
    void setBounces(int count);
    void setIndirectLightFactor(float factor);

Q_SIGNALS:
    void changed();
    void opacityThresholdChanged();
    void biasChanged();
    void adaptiveBiasEnabledChanged();
    void indirectLightEnabledChanged();
    void samplesChanged();
    void indirectLightWorkgroupSizeChanged();
    void bouncesChanged();
    void indirectLightFactorChanged();

private:
    // keep the defaults in sync with the default values in QSSGLightmapperOptions
    float m_opacityThreshold = 0.5f;
    float m_bias = 0.005f;
    bool m_adaptiveBias = true;
    bool m_indirectLight = true;
    int m_samples = 256;
    int m_workgroupSize = 32;
    int m_bounces = 3;
    float m_indirectFactor = 1.0f;
};

QT_END_NAMESPACE

#endif // QQUICK3DLIGHTMAPPER_P_H
