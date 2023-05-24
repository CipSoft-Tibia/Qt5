// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QBARDATAITEM_H
#define QBARDATAITEM_H

#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QBarDataItemPrivate;

class Q_GRAPHS_EXPORT QBarDataItem
{
public:
    constexpr QBarDataItem() noexcept {};
    constexpr QBarDataItem(float value) noexcept { m_value = value; };
    constexpr QBarDataItem(float value, float angle) noexcept{ m_value = value; m_angle = angle; };
    QBarDataItem(const QBarDataItem &other);
    ~QBarDataItem();

    QBarDataItem &operator=(const QBarDataItem &other);

    constexpr inline void setValue(float val) noexcept { m_value = val; }
    constexpr inline float value() const noexcept { return m_value; }
    constexpr inline void setRotation(float angle) noexcept { m_angle = angle; }
    constexpr inline float rotation() const noexcept { return m_angle; }

protected:
    void createExtraData();

    QBarDataItemPrivate *d_ptr = nullptr;

private:
    float m_value = 0.f;
    float m_angle = 0.f;
};

QT_END_NAMESPACE

#endif
