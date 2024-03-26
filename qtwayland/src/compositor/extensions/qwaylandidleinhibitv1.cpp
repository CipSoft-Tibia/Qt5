// Copyright (C) 2019 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/private/qwaylandsurface_p.h>

#include "qwaylandidleinhibitv1_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QWaylandIdleInhibitManagerV1
    \inmodule QtWaylandCompositor
    \since 5.14
    \brief Provides an extension that allows to inhibit the idle behavior of the compositor.
    \sa QWaylandSurface::inhibitsIdle

    The QWaylandIdleInhibitV1 extension provides a way for a client to inhibit the idle behavior of
    the compositor when a specific surface is visually relevant to the user.

    QWaylandIdleInhibitManagerV1 corresponds to the Wayland interface, \c zwp_idle_inhibit_manager_v1.

    Inhibited surfaces have the QWaylandSurface::inhibitsIdle property set to \c true.
*/

/*!
    \qmltype IdleInhibitManagerV1
    \instantiates QWaylandIdleInhibitManagerV1
    \inqmlmodule QtWayland.Compositor
    \since 5.14
    \brief Provides an extension that allows to inhibit the idle behavior of the compositor.
    \sa WaylandSurface::inhibitsIdle

    The IdleInhibitManagerV1 extension provides a way for a client to inhibit the idle behavior of
    the compositor when a specific surface is visually relevant to the user.

    IdleInhibitManagerV1 corresponds to the Wayland interface, \c zwp_idle_inhibit_manager_v1.

    To provide the functionality of the extension in a compositor, create an instance of the
    IdleInhibitManagerV1 component and add it to the list of extensions supported by the compositor:

    \qml
    import QtWayland.Compositor

    WaylandCompositor {
        IdleInhibitManagerV1 {
            // ...
        }
    }
    \endqml

    Inhibited surfaces have the WaylandSurface::inhibitsIdle property set to \c true.
*/

/*!
    Constructs a QWaylandIdleInhibitManagerV1 object.
*/
QWaylandIdleInhibitManagerV1::QWaylandIdleInhibitManagerV1()
    : QWaylandCompositorExtensionTemplate<QWaylandIdleInhibitManagerV1>(*new QWaylandIdleInhibitManagerV1Private())
{
}

/*!
    Constructs a QWaylandIdleInhibitManagerV1 object for the provided \a compositor.
*/
QWaylandIdleInhibitManagerV1::QWaylandIdleInhibitManagerV1(QWaylandCompositor *compositor)
    : QWaylandCompositorExtensionTemplate<QWaylandIdleInhibitManagerV1>(compositor, *new QWaylandIdleInhibitManagerV1Private())
{
}

/*!
    Destructs a QWaylandIdleInhibitManagerV1 object.
*/
QWaylandIdleInhibitManagerV1::~QWaylandIdleInhibitManagerV1() = default;

/*!
    Initializes the extension.
*/
void QWaylandIdleInhibitManagerV1::initialize()
{
    Q_D(QWaylandIdleInhibitManagerV1);

    QWaylandCompositorExtensionTemplate::initialize();
    QWaylandCompositor *compositor = static_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor) {
        qCWarning(qLcWaylandCompositor) << "Failed to find QWaylandCompositor when initializing QWaylandIdleInhibitManagerV1";
        return;
    }
    d->init(compositor->display(), d->interfaceVersion());
}

/*!
    Returns the Wayland interface for the QWaylandIdleInhibitManagerV1.
*/
const wl_interface *QWaylandIdleInhibitManagerV1::interface()
{
    return QWaylandIdleInhibitManagerV1Private::interface();
}


void QWaylandIdleInhibitManagerV1Private::zwp_idle_inhibit_manager_v1_create_inhibitor(Resource *resource, uint id, wl_resource *surfaceResource)
{
    auto *surface = QWaylandSurface::fromResource(surfaceResource);
    if (!surface) {
        qCWarning(qLcWaylandCompositor) << "Couldn't find surface requested for creating an inhibitor";
        wl_resource_post_error(resource->handle, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "invalid wl_surface@%d", wl_resource_get_id(surfaceResource));
        return;
    }

    auto *surfacePrivate = QWaylandSurfacePrivate::get(surface);
    if (!surfacePrivate) {
        wl_resource_post_no_memory(resource->handle);
        return;
    }

    auto *inhibitor = new Inhibitor(surface, resource->client(), id, resource->version());
    if (!inhibitor) {
        wl_resource_post_no_memory(resource->handle);
        return;
    }
    surfacePrivate->idleInhibitors.append(inhibitor);

    if (surfacePrivate->idleInhibitors.size() == 1)
        Q_EMIT surface->inhibitsIdleChanged();
}


QWaylandIdleInhibitManagerV1Private::Inhibitor::Inhibitor(QWaylandSurface *surface,
                                                          wl_client *client,
                                                          quint32 id, quint32 version)
    : QtWaylandServer::zwp_idle_inhibitor_v1(client, id, qMin<quint32>(version, interfaceVersion()))
    , m_surface(surface)
{
    Q_ASSERT(surface);
}

void QWaylandIdleInhibitManagerV1Private::Inhibitor::zwp_idle_inhibitor_v1_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void QWaylandIdleInhibitManagerV1Private::Inhibitor::zwp_idle_inhibitor_v1_destroy(Resource *resource)
{
    if (m_surface) {
        auto *surfacePrivate = QWaylandSurfacePrivate::get(m_surface.data());
        Q_ASSERT(surfacePrivate->idleInhibitors.contains(this));
        surfacePrivate->idleInhibitors.removeOne(this);

        if (surfacePrivate->idleInhibitors.isEmpty())
            Q_EMIT m_surface.data()->inhibitsIdleChanged();
    }

    wl_resource_destroy(resource->handle);
}

QT_END_NAMESPACE

#include "moc_qwaylandidleinhibitv1.cpp"
