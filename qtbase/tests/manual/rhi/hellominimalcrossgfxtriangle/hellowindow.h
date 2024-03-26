// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HELLOWINDOW_H
#define HELLOWINDOW_H

#include "window.h"

class HelloWindow : public Window
{
public:
    HelloWindow(QRhi::Implementation graphicsApi);

    void customInit() override;
    void customRender() override;

private:
    QShader getShader(const QString &name);

    std::unique_ptr<QRhiBuffer> m_vbuf;
    bool m_vbufReady = false;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_ps;

    float m_rotation = 0;
    float m_opacity = 1;
    int m_opacityDir = -1;
};

#endif
