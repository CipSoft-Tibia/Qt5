// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDTABLETV2_P_H
#define QWAYLANDTABLETV2_P_H

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

#include <QtWaylandClient/private/qwayland-tablet-unstable-v2.h>

#include <QtWaylandClient/private/qtwaylandclientglobal_p.h>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QPointF>
#include <QtGui/QPointingDevice>
#include <QtGui/QInputDevice>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandDisplay;
class QWaylandInputDevice;
class QWaylandSurface;

class QWaylandTabletSeatV2;
class QWaylandTabletV2;
class QWaylandTabletToolV2;
class QWaylandTabletPadV2;

class Q_WAYLANDCLIENT_EXPORT QWaylandTabletManagerV2 : public QtWayland::zwp_tablet_manager_v2
{
public:
    explicit QWaylandTabletManagerV2(QWaylandDisplay *display, uint id, uint version);
};

class Q_WAYLANDCLIENT_EXPORT QWaylandTabletSeatV2 : public QObject, public QtWayland::zwp_tablet_seat_v2
{
    Q_OBJECT
public:
    explicit QWaylandTabletSeatV2(QWaylandTabletManagerV2 *manager, QWaylandInputDevice *seat);
    ~QWaylandTabletSeatV2() override;

    QWaylandInputDevice *seat() const { return m_seat; }

protected:
    void zwp_tablet_seat_v2_tablet_added(struct ::zwp_tablet_v2 *id) override;
    void zwp_tablet_seat_v2_tool_added(struct ::zwp_tablet_tool_v2 *id) override;
    void zwp_tablet_seat_v2_pad_added(struct ::zwp_tablet_pad_v2 *id) override;

private:
    QWaylandInputDevice *m_seat;
    QList<QWaylandTabletV2 *> m_tablets;
    QList<QWaylandTabletToolV2 *> m_tools;
    QList<QWaylandTabletPadV2 *> m_pads;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandTabletV2 : public QObject, public QtWayland::zwp_tablet_v2
{
    Q_OBJECT
public:
    explicit QWaylandTabletV2(::zwp_tablet_v2 *tablet);

protected:
//    void zwp_tablet_v2_name(const QString &name) override;
//    void zwp_tablet_v2_id(uint32_t vid, uint32_t pid) override;
//    void zwp_tablet_v2_path(const QString &path) override;
//    void zwp_tablet_v2_done() override;
    void zwp_tablet_v2_removed() override;
};

class Q_WAYLANDCLIENT_EXPORT QWaylandTabletToolV2 : public QObject, public QtWayland::zwp_tablet_tool_v2
{
    Q_OBJECT
public:
    QWaylandTabletToolV2(QWaylandTabletSeatV2 *tabletSeat, ::zwp_tablet_tool_v2 *tool);

protected:
    void zwp_tablet_tool_v2_type(uint32_t tool_type) override;
    void zwp_tablet_tool_v2_hardware_serial(uint32_t hardware_serial_hi, uint32_t hardware_serial_lo) override;
//    void zwp_tablet_tool_v2_hardware_id_wacom(uint32_t hardware_id_hi, uint32_t hardware_id_lo) override;
    void zwp_tablet_tool_v2_capability(uint32_t capability) override;
    void zwp_tablet_tool_v2_done() override;
    void zwp_tablet_tool_v2_removed() override;
    void zwp_tablet_tool_v2_proximity_in(uint32_t serial, struct ::zwp_tablet_v2 *tablet, struct ::wl_surface *surface) override;
    void zwp_tablet_tool_v2_proximity_out() override;
    void zwp_tablet_tool_v2_down(uint32_t serial) override;
    void zwp_tablet_tool_v2_up() override;
    void zwp_tablet_tool_v2_motion(wl_fixed_t x, wl_fixed_t y) override;
    void zwp_tablet_tool_v2_pressure(uint32_t pressure) override;
    void zwp_tablet_tool_v2_distance(uint32_t distance) override;
    void zwp_tablet_tool_v2_tilt(wl_fixed_t tilt_x, wl_fixed_t tilt_y) override;
    void zwp_tablet_tool_v2_rotation(wl_fixed_t degrees) override;
    void zwp_tablet_tool_v2_slider(int32_t position) override;
//    void zwp_tablet_tool_v2_wheel(wl_fixed_t degrees, int32_t clicks) override;
    void zwp_tablet_tool_v2_button(uint32_t serial, uint32_t button, uint32_t state) override;
    void zwp_tablet_tool_v2_frame(uint32_t time) override;

private:
    QWaylandTabletSeatV2 *m_tabletSeat;

    // Static state (sent before done event)
    QPointingDevice::PointerType m_pointerType = QPointingDevice::PointerType::Unknown;
    QInputDevice::DeviceType m_tabletDevice = QInputDevice::DeviceType::Unknown;
    type m_toolType = type_pen;
    bool m_hasRotation = false;
    quint64 m_uid = 0;

    // Accumulated state (applied on frame event)
    struct State {
        bool down = false;
        QPointer<QWaylandSurface> proximitySurface;
        bool enteredSurface = false; // Not enough with just proximitySurface, if the surface is deleted, we still want to send a leave event
        QPointF surfacePosition;
        uint distance = 0;
        qreal pressure = 0;
        qreal rotation = 0;
        qreal xTilt = 0;
        qreal yTilt = 0;
        qreal slider = 0;
        Qt::MouseButtons buttons = Qt::MouseButton::NoButton; // Actual buttons, down state -> left mouse is mapped inside the frame handler
        //auto operator<=>(const Point&) const = default; // TODO: use this when upgrading to C++20
        bool operator==(const State &o) const;
    } m_pending, m_applied;
};

// We don't actually use this, but need to handle the "removed" event to comply with the protocol
class Q_WAYLANDCLIENT_EXPORT QWaylandTabletPadV2 : public QObject, public QtWayland::zwp_tablet_pad_v2
{
    Q_OBJECT
public:
    explicit QWaylandTabletPadV2(::zwp_tablet_pad_v2 *pad);

protected:
//    void zwp_tablet_pad_v2_group(struct ::zwp_tablet_pad_group_v2 *pad_group) override;
//    void zwp_tablet_pad_v2_path(const QString &path) override;
//    void zwp_tablet_pad_v2_buttons(uint32_t buttons) override;
//    void zwp_tablet_pad_v2_done() override;
//    void zwp_tablet_pad_v2_button(uint32_t time, uint32_t button, uint32_t state) override;
//    void zwp_tablet_pad_v2_enter(uint32_t serial, struct ::zwp_tablet_v2 *tablet, struct ::wl_surface *surface) override;
//    void zwp_tablet_pad_v2_leave(uint32_t serial, struct ::wl_surface *surface) override;
    void zwp_tablet_pad_v2_removed() override;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDTABLETV2_P_H
