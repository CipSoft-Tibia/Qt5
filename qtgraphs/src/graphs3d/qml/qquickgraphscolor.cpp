// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquickgraphscolor_p.h"

QT_BEGIN_NAMESPACE

QQuickGraphsColor::QQuickGraphsColor(QObject *parent)
    : QObject(parent)
{}

void QQuickGraphsColor::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged(color);
    }
}

QColor QQuickGraphsColor::color() const
{
    return m_color;
}

QT_END_NAMESPACE
