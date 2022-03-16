/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandivisurface.h"
#include "qwaylandivisurface_p.h"
#include "qwaylandiviapplication_p.h"
#if QT_CONFIG(wayland_compositor_quick)
#include "qwaylandivisurfaceintegration_p.h"
#endif

#include <QtWaylandCompositor/QWaylandResource>
#include <QDebug>

QT_BEGIN_NAMESPACE

QWaylandSurfaceRole QWaylandIviSurfacePrivate::s_role("ivi_surface");

/*!
 * \qmltype IviSurface
 * \inqmlmodule QtWayland.Compositor
 * \since 5.8
 * \brief Provides a simple way to identify and resize a surface.
 *
 * This type is part of the \l{IviApplication} extension and provides a way to extend
 * the functionality of an existing WaylandSurface with a way to resize and identify it.
 *
 * It corresponds to the Wayland \c ivi_surface interface.
 */

/*!
 * \class QWaylandIviSurface
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandIviSurface class provides a simple way to identify and resize a surface.
 *
 * This class is part of the QWaylandIviApplication extension and provides a way to
 * extend the functionality of an existing QWaylandSurface with a way to resize and identify it.
 *
 * It corresponds to the Wayland \c ivi_surface interface.
 */

/*!
 * Constructs a QWaylandIviSurface.
 */
QWaylandIviSurface::QWaylandIviSurface()
    : QWaylandShellSurfaceTemplate<QWaylandIviSurface>(*new QWaylandIviSurfacePrivate())
{
}

/*!
 * Constructs a QWaylandIviSurface for \a surface and initializes it with the
 * given \a application, \a surface, \a iviId, and \a resource.
 */
QWaylandIviSurface::QWaylandIviSurface(QWaylandIviApplication *application, QWaylandSurface *surface, uint iviId, const QWaylandResource &resource)
    : QWaylandShellSurfaceTemplate<QWaylandIviSurface>(*new QWaylandIviSurfacePrivate())
{
    initialize(application, surface, iviId, resource);
}

/*!
 * \qmlmethod void QtWaylandCompositor::IviSurface::initialize(IviApplication iviApplication, WaylandSurface surface, int iviId, WaylandResource resource)
 *
 * Initializes the IviSurface, associating it with the given \a iviApplication, \a surface,
 * \a iviId, and \a resource.
 */

/*!
 * Initializes the QWaylandIviSurface, associating it with the given \a iviApplication, \a surface,
 * \a iviId, and \a resource.
 */
void QWaylandIviSurface::initialize(QWaylandIviApplication *iviApplication, QWaylandSurface *surface, uint iviId, const QWaylandResource &resource)
{
    Q_D(QWaylandIviSurface);

    d->m_iviApplication = iviApplication;
    d->m_surface = surface;
    d->m_iviId = iviId;

    d->init(resource.resource());
    setExtensionContainer(surface);

    emit surfaceChanged();
    emit iviIdChanged();

    QWaylandCompositorExtension::initialize();
}

/*!
 * \qmlproperty WaylandSurface QtWaylandCompositor::IviSurface::surface
 *
 * This property holds the surface associated with this IviSurface.
 */

/*!
 * \property QWaylandIviSurface::surface
 *
 * This property holds the surface associated with this QWaylandIviSurface.
 */
QWaylandSurface *QWaylandIviSurface::surface() const
{
    Q_D(const QWaylandIviSurface);
    return d->m_surface;
}

/*!
 * \qmlproperty int QtWaylandCompositor::IviSurface::iviId
 * \readonly
 *
 * This property holds the ivi id id of this IviSurface.
 */

/*!
 * \property QWaylandIviSurface::iviId
 *
 * This property holds the ivi id of this QWaylandIviSurface.
 */
uint QWaylandIviSurface::iviId() const
{
    Q_D(const QWaylandIviSurface);
    return d->m_iviId;
}

/*!
 * Returns the Wayland interface for the QWaylandIviSurface.
 */
const struct wl_interface *QWaylandIviSurface::interface()
{
    return QWaylandIviSurfacePrivate::interface();
}

QByteArray QWaylandIviSurface::interfaceName()
{
    return QWaylandIviSurfacePrivate::interfaceName();
}

/*!
 * Returns the surface role for the QWaylandIviSurface.
 */
QWaylandSurfaceRole *QWaylandIviSurface::role()
{
    return &QWaylandIviSurfacePrivate::s_role;
}

/*!
 * Returns the QWaylandIviSurface corresponding to the \a resource.
 */
QWaylandIviSurface *QWaylandIviSurface::fromResource(wl_resource *resource)
{
    auto iviSurfaceResource = QWaylandIviSurfacePrivate::Resource::fromResource(resource);
    if (!iviSurfaceResource)
        return nullptr;
    return static_cast<QWaylandIviSurfacePrivate *>(iviSurfaceResource->ivi_surface_object)->q_func();
}

/*!
 * \qmlmethod int QtWaylandCompositor::IviSurface::sendConfigure(size size)
 *
 * Sends a configure event to the client, telling it to resize the surface to the given \a size.
 */

/*!
 * Sends a configure event to the client, telling it to resize the surface to the given \a size.
 */
void QWaylandIviSurface::sendConfigure(const QSize &size)
{
    if (!size.isValid()) {
        qWarning() << "Can't configure ivi_surface with an invalid size" << size;
        return;
    }
    Q_D(QWaylandIviSurface);
    d->send_configure(size.width(), size.height());
}

#if QT_CONFIG(wayland_compositor_quick)
QWaylandQuickShellIntegration *QWaylandIviSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new QtWayland::IviSurfaceIntegration(item);
}
#endif

/*!
 * \internal
 */
void QWaylandIviSurface::initialize()
{
    QWaylandShellSurfaceTemplate::initialize();
}

QWaylandIviSurfacePrivate::QWaylandIviSurfacePrivate()
{
}

void QWaylandIviSurfacePrivate::ivi_surface_destroy_resource(QtWaylandServer::ivi_surface::Resource *resource)
{
    Q_UNUSED(resource);
    Q_Q(QWaylandIviSurface);
    QWaylandIviApplicationPrivate::get(m_iviApplication)->unregisterIviSurface(q);
    delete q;
}

void QWaylandIviSurfacePrivate::ivi_surface_destroy(QtWaylandServer::ivi_surface::Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

QT_END_NAMESPACE
