/****************************************************************************
**
** Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2017 Jolla Ltd, author: <giulio.camuffo@jollamobile.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDSURFACE_P_H
#define QWAYLANDSURFACE_P_H

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

#include <QtWaylandCompositor/qtwaylandcompositorglobal.h>
#include <private/qobject_p.h>

#include <private/qwlclientbuffer_p.h>
#include <QtWaylandCompositor/qwaylandsurface.h>
#include <QtWaylandCompositor/qwaylandbufferref.h>

#include <QtWaylandCompositor/private/qwlregion_p.h>

#include <QtCore/QVector>
#include <QtCore/QRect>
#include <QtGui/QRegion>
#include <QtGui/QImage>
#include <QtGui/QWindow>

#include <QtCore/QTextStream>
#include <QtCore/QMetaType>

#include <wayland-util.h>

#include <QtWaylandCompositor/private/qwayland-server-wayland.h>
#include <QtWaylandCompositor/private/qwaylandviewporter_p.h>
#include <QtWaylandCompositor/private/qwaylandidleinhibitv1_p.h>

QT_BEGIN_NAMESPACE

class QWaylandCompositor;
class QWaylandSurface;
class QWaylandView;
class QWaylandSurfaceInterface;
class QWaylandInputMethodControl;

namespace QtWayland {
class FrameCallback;
}

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandSurfacePrivate : public QObjectPrivate, public QtWaylandServer::wl_surface
{
public:
    static QWaylandSurfacePrivate *get(QWaylandSurface *surface);

    QWaylandSurfacePrivate();
    ~QWaylandSurfacePrivate() override;

    void ref();
    void deref();

    void refView(QWaylandView *view);
    void derefView(QWaylandView *view);

    using QtWaylandServer::wl_surface::resource;

    void removeFrameCallback(QtWayland::FrameCallback *callback);

    void notifyViewsAboutDestruction();

#ifndef QT_NO_DEBUG
    static void addUninitializedSurface(QWaylandSurfacePrivate *surface);
    static void removeUninitializedSurface(QWaylandSurfacePrivate *surface);
    static bool hasUninitializedSurface();
#endif

    void initSubsurface(QWaylandSurface *parent, struct ::wl_client *client, int id, int version);
    bool isSubsurface() const { return subsurface; }
    QWaylandSurfacePrivate *parentSurface() const { return subsurface ? subsurface->parentSurface : nullptr; }

protected:
    void surface_destroy_resource(Resource *resource) override;

    void surface_destroy(Resource *resource) override;
    void surface_attach(Resource *resource,
                        struct wl_resource *buffer, int x, int y) override;
    void surface_damage(Resource *resource,
                        int32_t x, int32_t y, int32_t width, int32_t height) override;
    void surface_frame(Resource *resource,
                       uint32_t callback) override;
    void surface_set_opaque_region(Resource *resource,
                                   struct wl_resource *region) override;
    void surface_set_input_region(Resource *resource,
                                  struct wl_resource *region) override;
    void surface_commit(Resource *resource) override;
    void surface_set_buffer_transform(Resource *resource, int32_t transform) override;
    void surface_set_buffer_scale(Resource *resource, int32_t bufferScale) override;

    QtWayland::ClientBuffer *getBuffer(struct ::wl_resource *buffer);

public: //member variables
    QWaylandCompositor *compositor = nullptr;
    int refCount = 1;
    QWaylandClient *client = nullptr;
    QList<QWaylandView *> views;
    QRegion damage;
    QWaylandBufferRef bufferRef;
    QWaylandSurfaceRole *role = nullptr;
    QWaylandViewporterPrivate::Viewport *viewport = nullptr;

    struct {
        QWaylandBufferRef buffer;
        QRegion damage;
        QPoint offset;
        bool newlyAttached = false;
        QRegion inputRegion;
        int bufferScale = 1;
        QRectF sourceGeometry;
        QSize destinationSize;
        QRegion opaqueRegion;
    } pending;

    QPoint lastLocalMousePos;
    QPoint lastGlobalMousePos;

    QList<QtWayland::FrameCallback *> pendingFrameCallbacks;
    QList<QtWayland::FrameCallback *> frameCallbacks;

    QList<QPointer<QWaylandSurface>> subsurfaceChildren;

    QVector<QWaylandIdleInhibitManagerV1Private::Inhibitor *> idleInhibitors;

    QRegion inputRegion;
    QRegion opaqueRegion;

    QRectF sourceGeometry;
    QSize destinationSize;
    QSize bufferSize;
    int bufferScale = 1;
    bool isCursorSurface = false;
    bool destroyed = false;
    bool hasContent = false;
    bool isInitialized = false;
    Qt::ScreenOrientation contentOrientation = Qt::PrimaryOrientation;
    QWindow::Visibility visibility;
#if QT_CONFIG(im)
    QWaylandInputMethodControl *inputMethodControl = nullptr;
#endif

    class Subsurface : public QtWaylandServer::wl_subsurface
    {
    public:
        Subsurface(QWaylandSurfacePrivate *s) : surface(s) {}
        QWaylandSurfacePrivate *surfaceFromResource();

    protected:
        void subsurface_set_position(wl_subsurface::Resource *resource, int32_t x, int32_t y) override;
        void subsurface_place_above(wl_subsurface::Resource *resource, struct wl_resource *sibling) override;
        void subsurface_place_below(wl_subsurface::Resource *resource, struct wl_resource *sibling) override;
        void subsurface_set_sync(wl_subsurface::Resource *resource) override;
        void subsurface_set_desync(wl_subsurface::Resource *resource) override;

    private:
        friend class QWaylandSurfacePrivate;
        QWaylandSurfacePrivate *surface = nullptr;
        QWaylandSurfacePrivate *parentSurface = nullptr;
        QPoint position;
    };

    Subsurface *subsurface = nullptr;

#ifndef QT_NO_DEBUG
    static QList<QWaylandSurfacePrivate *> uninitializedSurfaces;
#endif
    Q_DECLARE_PUBLIC(QWaylandSurface)
    Q_DISABLE_COPY(QWaylandSurfacePrivate)
};

QT_END_NAMESPACE

#endif
