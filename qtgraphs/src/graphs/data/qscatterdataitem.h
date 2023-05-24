// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSCATTERDATAITEM_H
#define QSCATTERDATAITEM_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtGui/QQuaternion>

QT_BEGIN_NAMESPACE

class QScatterDataItemPrivate;

class Q_GRAPHS_EXPORT QScatterDataItem
{
public:
    QScatterDataItem();
    QScatterDataItem(const QVector3D &position);
    QScatterDataItem(const QVector3D &position, const QQuaternion &rotation);
    QScatterDataItem(const QScatterDataItem &other);
    ~QScatterDataItem();

    QScatterDataItem &operator=(const QScatterDataItem &other);

    constexpr inline void setPosition(const QVector3D &pos) noexcept { m_position = pos; }
    constexpr inline QVector3D position() const noexcept { return m_position; }
    constexpr inline void setRotation(const QQuaternion &rot) noexcept { m_rotation = rot; }
    inline QQuaternion rotation() const { return m_rotation; }
    constexpr inline void setX(float value) noexcept { m_position.setX(value); }
    constexpr inline void setY(float value) noexcept { m_position.setY(value); }
    constexpr inline void setZ(float value) noexcept { m_position.setZ(value); }
    constexpr inline float x() const noexcept { return m_position.x(); }
    constexpr inline float y() const noexcept { return m_position.y(); }
    constexpr inline float z() const noexcept { return m_position.z(); }

protected:
    void createExtraData();

    QScatterDataItemPrivate *d_ptr;

private:
    QVector3D m_position;
    QQuaternion m_rotation;
};

QT_END_NAMESPACE

#endif
