// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "lottierenderer_p.h"

QT_BEGIN_NAMESPACE

void LottieRenderer::setTrimmingState(LottieRenderer::TrimmingState trimmingState)
{
    m_trimmingState = trimmingState;
}

LottieRenderer::TrimmingState LottieRenderer::trimmingState() const
{
    return m_trimmingState;
}

void LottieRenderer::saveTrimmingState()
{
    m_trimStateStack.push(m_trimmingState);
}

void LottieRenderer::restoreTrimmingState()
{
    if (m_trimStateStack.size())
        m_trimmingState = m_trimStateStack.pop();
}

QT_END_NAMESPACE
