// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSURFACEDATAITEM_H
#define QSURFACEDATAITEM_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtGui/QVector3D>

QT_BEGIN_NAMESPACE

class QSurfaceDataItemPrivate;

class Q_GRAPHS_EXPORT QSurfaceDataItem
{
public:
    QSurfaceDataItem();
    QSurfaceDataItem(const QVector3D &position);
    QSurfaceDataItem(const QSurfaceDataItem &other);
    ~QSurfaceDataItem();

    QSurfaceDataItem &operator=(const QSurfaceDataItem &other);

    constexpr inline void setPosition(const QVector3D &pos) noexcept { m_position = pos; }
    constexpr inline QVector3D position() const noexcept { return m_position; }
    constexpr inline void setX(float value) noexcept { m_position.setX(value); }
    constexpr inline void setY(float value) noexcept { m_position.setY(value); }
    constexpr inline void setZ(float value) noexcept { m_position.setZ(value); }
    constexpr inline float x() const noexcept { return m_position.x(); }
    constexpr inline float y() const noexcept { return m_position.y(); }
    constexpr inline float z() const noexcept { return m_position.z(); }

protected:
    void createExtraData();

    QSurfaceDataItemPrivate *d_ptr;

private:
    QVector3D m_position;
};

QT_END_NAMESPACE

#endif
