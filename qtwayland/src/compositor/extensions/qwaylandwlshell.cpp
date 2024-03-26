// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwaylandwlshell.h"
#include "qwaylandwlshell_p.h"

#if QT_CONFIG(wayland_compositor_quick)
#include "qwaylandwlshellintegration_p.h"
#endif
#include <QtWaylandCompositor/private/qwaylandutils_p.h>

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandView>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandClient>

#include <QtCore/QObject>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QWaylandSurfaceRole QWaylandWlShellSurfacePrivate::s_role("wl_shell_surface");

QWaylandWlShellPrivate::QWaylandWlShellPrivate()
{
}

void QWaylandWlShellPrivate::shell_get_shell_surface(Resource *resource, uint32_t id, struct ::wl_resource *surface_res)
{
    Q_Q(QWaylandWlShell);
    QWaylandSurface *surface = QWaylandSurface::fromResource(surface_res);

    QWaylandResource shellSurfaceResource(wl_resource_create(resource->client(), &wl_shell_surface_interface,
                                                             wl_resource_get_version(resource->handle), id));

    // XXX FIXME
    // The role concept was formalized in wayland 1.7, so that release adds one error
    // code for each interface that implements a role, and we are supposed to pass here
    // the newly constructed resource and the correct error code so that if setting the
    // role fails, a proper error can be sent to the client.
    // However we're still using wayland 1.4, which doesn't have interface specific role
    // errors, so the best we can do is to use wl_display's object_id error.
    wl_resource *displayRes = wl_client_get_object(resource->client(), 1);
    if (!surface->setRole(QWaylandWlShellSurface::role(), displayRes, WL_DISPLAY_ERROR_INVALID_OBJECT))
        return;

    emit q->wlShellSurfaceRequested(surface, shellSurfaceResource);

    QWaylandWlShellSurface *shellSurface = QWaylandWlShellSurface::fromResource(shellSurfaceResource.resource());
    if (!shellSurface) {
        // A QWaylandWlShellSurface was not created in response to the wlShellSurfaceRequested
        // signal, so we create one as fallback here instead.
        shellSurface = new QWaylandWlShellSurface(q, surface, shellSurfaceResource);
    }

    m_shellSurfaces.append(shellSurface);
    emit q->wlShellSurfaceCreated(shellSurface);
}

void QWaylandWlShellPrivate::unregisterShellSurface(QWaylandWlShellSurface *shellSurface)
{
    if (!m_shellSurfaces.removeOne(shellSurface))
        qWarning("Unexpected state. Can't find registered shell surface.");
}

QWaylandWlShellSurfacePrivate::QWaylandWlShellSurfacePrivate()
{
}

QWaylandWlShellSurfacePrivate::~QWaylandWlShellSurfacePrivate()
{
}

void QWaylandWlShellSurfacePrivate::ping(uint32_t serial)
{
    m_pings.insert(serial);
    send_ping(serial);
}

void QWaylandWlShellSurfacePrivate::setWindowType(Qt::WindowType windowType)
{
    if (m_windowType == windowType)
        return;
    m_windowType = windowType;

    Q_Q(QWaylandWlShellSurface);
    emit q->windowTypeChanged();
}

void QWaylandWlShellSurfacePrivate::shell_surface_destroy_resource(Resource *)
{
    Q_Q(QWaylandWlShellSurface);

    delete q;
}

void QWaylandWlShellSurfacePrivate::shell_surface_move(Resource *resource,
                struct wl_resource *input_device_super,
                uint32_t serial)
{
    Q_UNUSED(resource);
    Q_UNUSED(serial);

    Q_Q(QWaylandWlShellSurface);
    QWaylandSeat *input_device = QWaylandSeat::fromSeatResource(input_device_super);
    emit q->startMove(input_device);
}

void QWaylandWlShellSurfacePrivate::shell_surface_resize(Resource *resource,
                  struct wl_resource *input_device_super,
                  uint32_t serial,
                  uint32_t edges)
{
    Q_UNUSED(resource);
    Q_UNUSED(serial);
    Q_Q(QWaylandWlShellSurface);

    QWaylandSeat *input_device = QWaylandSeat::fromSeatResource(input_device_super);
    emit q->startResize(input_device, QWaylandWlShellSurface::ResizeEdge(edges));
}

void QWaylandWlShellSurfacePrivate::shell_surface_set_toplevel(Resource *resource)
{
    Q_UNUSED(resource);
    Q_Q(QWaylandWlShellSurface);
    setWindowType(Qt::WindowType::Window);
    emit q->setDefaultToplevel();
}

void QWaylandWlShellSurfacePrivate::shell_surface_set_transient(Resource *resource,
                      struct wl_resource *parent_surface_resource,
                      int x,
                      int y,
                      uint32_t flags)
{

    Q_UNUSED(resource);
    Q_Q(QWaylandWlShellSurface);
    QWaylandSurface *parent_surface = QWaylandSurface::fromResource(parent_surface_resource);
    setWindowType(Qt::WindowType::SubWindow);
    emit q->setTransient(parent_surface, QPoint(x,y), flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE);
}

void QWaylandWlShellSurfacePrivate::shell_surface_set_fullscreen(Resource *resource,
                       uint32_t method,
                       uint32_t framerate,
                       struct wl_resource *output_resource)
{
    Q_UNUSED(resource);
    Q_UNUSED(method);
    Q_UNUSED(framerate);
    Q_Q(QWaylandWlShellSurface);
    QWaylandOutput *output = output_resource
            ? QWaylandOutput::fromResource(output_resource)
            : nullptr;
    setWindowType(Qt::WindowType::Window);
    emit q->setFullScreen(QWaylandWlShellSurface::FullScreenMethod(method), framerate, output);
}

void QWaylandWlShellSurfacePrivate::shell_surface_set_popup(Resource *resource, wl_resource *input_device, uint32_t serial, wl_resource *parent, int32_t x, int32_t y, uint32_t flags)
{
    Q_UNUSED(resource);
    Q_UNUSED(serial);
    Q_UNUSED(flags);
    Q_Q(QWaylandWlShellSurface);
    QWaylandSeat *input = QWaylandSeat::fromSeatResource(input_device);
    QWaylandSurface *parentSurface = QWaylandSurface::fromResource(parent);
    setWindowType(Qt::WindowType::Popup);
    emit q->setPopup(input, parentSurface, QPoint(x,y));

}

void QWaylandWlShellSurfacePrivate::shell_surface_set_maximized(Resource *resource,
                       struct wl_resource *output_resource)
{
    Q_UNUSED(resource);
    Q_Q(QWaylandWlShellSurface);
    QWaylandOutput *output = output_resource
            ? QWaylandOutput::fromResource(output_resource)
            : nullptr;
    setWindowType(Qt::WindowType::Window);
    emit q->setMaximized(output);
}

void QWaylandWlShellSurfacePrivate::shell_surface_pong(Resource *resource,
                        uint32_t serial)
{
    Q_UNUSED(resource);
    Q_Q(QWaylandWlShellSurface);
    if (m_pings.remove(serial))
        emit q->pong();
    else
        qWarning("Received an unexpected pong!");
}

void QWaylandWlShellSurfacePrivate::shell_surface_set_title(Resource *resource,
                             const QString &title)
{
    Q_UNUSED(resource);
    if (title == m_title)
        return;
    Q_Q(QWaylandWlShellSurface);
    m_title = title;
    emit q->titleChanged();
}

void QWaylandWlShellSurfacePrivate::shell_surface_set_class(Resource *resource,
                             const QString &className)
{
    Q_UNUSED(resource);
    if (className == m_className)
        return;
    Q_Q(QWaylandWlShellSurface);
    m_className = className;
    emit q->classNameChanged();
}

/*!
 * \qmltype WlShell
 * \instantiates QWaylandWlShell
 * \inqmlmodule QtWayland.Compositor.WlShell
 * \since 5.8
 * \brief Provides an extension for desktop-style user interfaces.
 *
 * The WlShell extension provides a way to associate a ShellSurface
 * with a regular Wayland surface. Using the shell surface interface, the client
 * can request that the surface is resized, moved, and so on.
 *
 * WlShell corresponds to the Wayland interface \c wl_shell.
 *
 * To provide the functionality of the shell extension in a compositor, create
 * an instance of the WlShell component and add it to the list of extensions
 * supported by the compositor:
 *
 * \qml
 * import QtWayland.Compositor.WlShell
 *
 * WaylandCompositor {
 *     WlShell {
 *         // ...
 *     }
 * }
 * \endqml
 */

/*!
 * \class QWaylandWlShell
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandWlShell class is an extension for desktop-style user interfaces.
 *
 * The QWaylandWlShell extension provides a way to associate a QWaylandWlShellSurface with
 * a regular Wayland surface. Using the shell surface interface, the client
 * can request that the surface is resized, moved, and so on.
 *
 * WlShell corresponds to the Wayland interface \c wl_shell.
 */

/*!
 * Constructs a QWaylandWlShell object.
 */
QWaylandWlShell::QWaylandWlShell()
    : QWaylandShellTemplate<QWaylandWlShell>(*new QWaylandWlShellPrivate())
{ }

/*!
 * Constructs a QWaylandWlShell object for the provided \a compositor.
 */
QWaylandWlShell::QWaylandWlShell(QWaylandCompositor *compositor)
    : QWaylandShellTemplate<QWaylandWlShell>(compositor, *new QWaylandWlShellPrivate())
{ }


/*!
 * Initializes the WlShell extension.
 */
void QWaylandWlShell::initialize()
{
    Q_D(QWaylandWlShell);
    QWaylandShellTemplate::initialize();
    QWaylandCompositor *compositor = qobject_cast<QWaylandCompositor *>(extensionContainer());
    if (!compositor) {
        qWarning() << "Failed to find QWaylandCompositor when initializing QWaylandWlShell";
        return;
    }
    d->init(compositor->display(), 1);
}

QList<QWaylandWlShellSurface *> QWaylandWlShell::shellSurfaces() const
{
    Q_D(const QWaylandWlShell);
    return d->m_shellSurfaces;
}

QList<QWaylandWlShellSurface *> QWaylandWlShell::shellSurfacesForClient(QWaylandClient *client) const
{
    Q_D(const QWaylandWlShell);
    QList<QWaylandWlShellSurface *> surfsForClient;
    for (QWaylandWlShellSurface *shellSurface : d->m_shellSurfaces) {
        if (shellSurface->surface() && shellSurface->surface()->client() == client)
            surfsForClient.append(shellSurface);
    }
    return surfsForClient;
}

QList<QWaylandWlShellSurface *> QWaylandWlShell::mappedPopups() const
{
    Q_D(const QWaylandWlShell);
    QList<QWaylandWlShellSurface *> popupSurfaces;
    for (QWaylandWlShellSurface *shellSurface : d->m_shellSurfaces) {
        if (shellSurface->windowType() == Qt::WindowType::Popup
                && shellSurface->surface() && shellSurface->surface()->hasContent()) {
            popupSurfaces.append(shellSurface);
        }
    }
    return popupSurfaces;
}

QWaylandClient *QWaylandWlShell::popupClient() const
{
    Q_D(const QWaylandWlShell);
    for (QWaylandWlShellSurface *shellSurface : d->m_shellSurfaces) {
        if (shellSurface->windowType() == Qt::WindowType::Popup
                && shellSurface->surface() && shellSurface->surface()->hasContent()) {
            return shellSurface->surface()->client();
        }
    }
    return nullptr;
}

void QWaylandWlShell::closeAllPopups()
{
    const auto mapped = mappedPopups();
    for (QWaylandWlShellSurface *shellSurface : mapped)
        shellSurface->sendPopupDone();
}

/*!
 * Returns the Wayland interface for the QWaylandWlShell.
 */
const struct wl_interface *QWaylandWlShell::interface()
{
    return QWaylandWlShellPrivate::interface();
}

/*!
 * \qmlsignal void WlShell::wlShellSurfaceRequested(WaylandSurface surface, WaylandResource resource)
 *
 * This signal is emitted when the client has requested a \c wl_shell_surface to be associated with
 * \a surface. The handler for this signal may create a shell surface for \a resource and initialize
 * it within the scope of the signal emission. Otherwise a WlShellSurface will be created
 * automatically.
 */

/*!
 * \fn void QWaylandWlShell::wlShellSurfaceRequested(QWaylandSurface *surface, const QWaylandResource &resource)
 *
 * This signal is emitted when the client has requested a \c wl_shell_surface to be associated with
 * \a surface. The handler for this signal may create a shell surface for \a resource and initialize
 * it within the scope of the signal emission. Otherwise a QWaylandWlShellSurface will be created
 * automatically.
 */

/*!
 * \qmlsignal void WlShell::wlShellSurfaceCreated(WlShellSurface shellSurface)
 *
 * This signal is emitted when the client has created a \c wl_shell_surface.
 * A common use case is to let the handler of this signal instantiate a ShellSurfaceItem or
 * WaylandQuickItem for displaying \a shellSurface in a QtQuick scene.
 */

/*!
 * \fn void QWaylandWlShell::wlShellSurfaceCreated(QWaylandWlShellSurface *shellSurface)
 *
 * This signal is emitted when the client has created a \c wl_shell_surface.
 * A common use case is to let the handler of this signal instantiate a QWaylandShellSurfaceItem or
 * QWaylandQuickItem for displaying \a shellSurface in a QtQuick scene.
 */

/*!
 * \internal
 */
QByteArray QWaylandWlShell::interfaceName()
{
    return QWaylandWlShellPrivate::interfaceName();
}

/*!
 * \qmltype WlShellSurface
 * \instantiates QWaylandWlShellSurface
 * \inqmlmodule QtWayland.Compositor.WlShell
 * \since 5.8
 * \brief Provides a \c wl_shell_surface that offers desktop-style compositor-specific features to a surface.
 *
 * This type is part of the \l{WlShell} extension and provides a way to extend
 * the functionality of an existing WaylandSurface with features specific to desktop-style
 * compositors, such as resizing and moving the surface.
 *
 * It corresponds to the Wayland interface \c wl_shell_surface.
 */

/*!
 * \class QWaylandWlShellSurface
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandWlShellSurface class provides desktop-style compositor-specific features to a surface.
 *
 * This class is part of the QWaylandWlShell extension and provides a way to extend
 * the functionality of an existing QWaylandSurface with features specific to desktop-style
 * compositors, such as resizing and moving the surface.
 *
 * It corresponds to the Wayland interface \c wl_shell_surface.
 */

/*!
 * Constructs a QWaylandWlShellSurface.
 */
QWaylandWlShellSurface::QWaylandWlShellSurface()
    : QWaylandShellSurfaceTemplate<QWaylandWlShellSurface>(*new QWaylandWlShellSurfacePrivate)
{
}

/*!
 * Constructs a QWaylandWlShellSurface for \a surface and initializes it with the given \a shell and resource \a res.
 */
QWaylandWlShellSurface::QWaylandWlShellSurface(QWaylandWlShell *shell, QWaylandSurface *surface, const QWaylandResource &res)
    : QWaylandShellSurfaceTemplate<QWaylandWlShellSurface>(*new QWaylandWlShellSurfacePrivate)
{
    initialize(shell, surface, res);
}

QWaylandWlShellSurface::~QWaylandWlShellSurface()
{
    Q_D(QWaylandWlShellSurface);
    if (d->m_shell)
        QWaylandWlShellPrivate::get(d->m_shell)->unregisterShellSurface(this);
}

/*!
 * \qmlmethod void WlShellSurface::initialize(WlShell shell, WaylandSurface surface, WaylandResource resource)
 *
 * Initializes the WlShellSurface and associates it with the given \a shell, \a surface, and \a resource.
 */

/*!
 * Initializes the QWaylandWlShellSurface and associates it with the given \a shell, \a surface, and \a resource.
 */
void QWaylandWlShellSurface::initialize(QWaylandWlShell *shell, QWaylandSurface *surface, const QWaylandResource &resource)
{
    Q_D(QWaylandWlShellSurface);
    d->m_shell = shell;
    d->m_surface = surface;
    d->init(resource.resource());
    setExtensionContainer(surface);
    emit surfaceChanged();
    emit shellChanged();
    QWaylandCompositorExtension::initialize();
}

/*!
 * \internal
 */
void QWaylandWlShellSurface::initialize()
{
    QWaylandCompositorExtension::initialize();
}

const struct wl_interface *QWaylandWlShellSurface::interface()
{
    return QWaylandWlShellSurfacePrivate::interface();
}

/*!
 * \internal
 */
QByteArray QWaylandWlShellSurface::interfaceName()
{
    return QWaylandWlShellSurfacePrivate::interfaceName();
}

QSize QWaylandWlShellSurface::sizeForResize(const QSizeF &size, const QPointF &delta, QWaylandWlShellSurface::ResizeEdge edge)
{
    qreal width = size.width();
    qreal height = size.height();
    if (edge & LeftEdge)
        width -= delta.x();
    else if (edge & RightEdge)
        width += delta.x();

    if (edge & TopEdge)
        height -= delta.y();
    else if (edge & BottomEdge)
        height += delta.y();

    QSizeF newSize(qMax(width, 1.0), qMax(height, 1.0));
    return newSize.toSize();
}

/*!
 * \enum QWaylandWlShellSurface::ResizeEdge
 *
 * This enum type provides a way to specify an edge or corner of
 * the surface.
 *
 * \value NoneEdge No edge.
 * \value TopEdge The top edge.
 * \value BottomEdge The bottom edge.
 * \value LeftEdge The left edge.
 * \value TopLeftEdge The top left corner.
 * \value BottomLeftEdge The bottom left corner.
 * \value RightEdge The right edge.
 * \value TopRightEdge The top right corner.
 * \value BottomRightEdge The bottom right corner.
 */

/*!
 * \qmlmethod void WlShellSurface::sendConfigure(size s, enum edges)
 *
 * Sends a configure event to the client, suggesting that it resize its surface to
 * the provided size \a s. The \a edges provide a hint about how the surface
 * was resized.
 */

/*!
 * Sends a configure event to the client, suggesting that it resize its surface to
 * the provided \a size. The \a edges provide a hint about how the surface
 * was resized.
 */
void QWaylandWlShellSurface::sendConfigure(const QSize &size, ResizeEdge edges)
{
    Q_D(QWaylandWlShellSurface);
    if (!size.isValid()) {
        qWarning() << "Can't configure wl_shell_surface with an invalid size" << size;
        return;
    }
    d->send_configure(edges, size.width(), size.height());
}

/*!
 * \qmlmethod void WlShellSurface::sendPopupDone()
 *
 * Sends a popup_done event to the client to indicate that the user has clicked
 * somewhere outside the client's surfaces.
 */

/*!
 * Sends a popup_done event to the client to indicate that the user has clicked
 * somewhere outside the client's surfaces.
 */
void QWaylandWlShellSurface::sendPopupDone()
{
    Q_D(QWaylandWlShellSurface);
    d->send_popup_done();
}

#if QT_CONFIG(wayland_compositor_quick)
QWaylandQuickShellIntegration *QWaylandWlShellSurface::createIntegration(QWaylandQuickShellSurfaceItem *item)
{
    return new QtWayland::WlShellIntegration(item);
}
#endif

/*!
 * \qmlproperty WaylandSurface WlShellSurface::surface
 *
 * This property holds the \c wl_surface associated with this WlShellSurface.
 */

/*!
 * \property QWaylandWlShellSurface::surface
 *
 * This property holds the surface associated with this QWaylandWlShellSurface.
 */
QWaylandSurface *QWaylandWlShellSurface::surface() const
{
    Q_D(const QWaylandWlShellSurface);
    return d->m_surface;
}

/*!
 * \qmlproperty WlShell WlShellSurface::shell
 *
 * This property holds the shell associated with this WlShellSurface.
 */

/*!
 * \property QWaylandWlShellSurface::shell
 *
 * This property holds the shell associated with this QWaylandWlShellSurface.
 */
QWaylandWlShell *QWaylandWlShellSurface::shell() const
{
    Q_D(const QWaylandWlShellSurface);
    return d->m_shell;
}

/*!
 * \qmlproperty enum WlShellSurface::windowType
 *
 * This property holds the window type of the WlShellSurface.
 */

Qt::WindowType QWaylandWlShellSurface::windowType() const
{
    Q_D(const QWaylandWlShellSurface);
    return d->m_windowType;
}

/*!
 * \qmlproperty string WlShellSurface::title
 *
 * This property holds the title of the WlShellSurface.
 */

/*!
 * \property QWaylandWlShellSurface::title
 *
 * This property holds the title of the QWaylandWlShellSurface.
 */
QString QWaylandWlShellSurface::title() const
{
    Q_D(const QWaylandWlShellSurface);
    return d->m_title;
}

/*!
 * \qmlproperty string WlShellSurface::className
 *
 * This property holds the class name of the WlShellSurface.
 */

/*!
 * \property QWaylandWlShellSurface::className
 *
 * This property holds the class name of the QWaylandWlShellSurface.
 */
QString QWaylandWlShellSurface::className() const
{
    Q_D(const QWaylandWlShellSurface);
    return d->m_className;
}

QWaylandSurfaceRole *QWaylandWlShellSurface::role()
{
    return &QWaylandWlShellSurfacePrivate::s_role;
}

/*!
 * \qmlmethod void WlShellSurface::ping()
 *
 * Sends a ping event to the client. If the client replies to the event the pong
 * signal will be emitted.
 */

/*!
 * Sends a ping event to the client. If the client replies to the event the pong
 * signal will be emitted.
 */
void QWaylandWlShellSurface::ping()
{
    Q_D(QWaylandWlShellSurface);
    uint32_t serial = d->m_surface->compositor()->nextSerial();
    d->ping(serial);
}

/*!
 * Returns the QWaylandWlShellSurface object associated with the given \a resource, or null if no such object exists.
 */
QWaylandWlShellSurface *QWaylandWlShellSurface::fromResource(wl_resource *resource)
{
    if (auto p = QtWayland::fromResource<QWaylandWlShellSurfacePrivate *>(resource))
        return p->q_func();
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qwaylandwlshell.cpp"
