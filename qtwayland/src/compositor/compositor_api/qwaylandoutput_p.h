/****************************************************************************
**
** Copyright (C) 2017-2016 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2017 Klarälvdalens Datakonsult AB (KDAB).
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWAYLANDOUTPUT_P_H
#define QWAYLANDOUTPUT_P_H

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
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandClient>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandXdgOutputV1>

#include <QtWaylandCompositor/private/qwayland-server-wayland.h>

#include <QtCore/QRect>
#include <QtCore/QVector>

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

struct QWaylandSurfaceViewMapper
{
    QWaylandSurfaceViewMapper()
    {}

    QWaylandSurfaceViewMapper(QWaylandSurface *s, QWaylandView *v)
        : surface(s)
        , views(1, v)
    {}

    QWaylandView *maybePrimaryView() const
    {
        for (int i = 0; i < views.size(); i++) {
            if (surface && surface->primaryView() == views.at(i))
                return views.at(i);
        }
        return nullptr;
    }

    QWaylandSurface *surface = nullptr;
    QVector<QWaylandView *> views;
    bool has_entered = false;
};

class Q_WAYLAND_COMPOSITOR_EXPORT QWaylandOutputPrivate : public QObjectPrivate, public QtWaylandServer::wl_output
{
public:
    QWaylandOutputPrivate();

    ~QWaylandOutputPrivate() override;
    static QWaylandOutputPrivate *get(QWaylandOutput *output) { return output->d_func(); }

    void addView(QWaylandView *view, QWaylandSurface *surface);
    void removeView(QWaylandView *view, QWaylandSurface *surface);

    void sendGeometry(const Resource *resource);
    void sendGeometryInfo();

    void sendMode(const Resource *resource, const QWaylandOutputMode &mode);
    void sendModesInfo();

    void handleWindowPixelSizeChanged();

    QPointer<QWaylandXdgOutputV1> xdgOutput;

protected:
    void output_bind_resource(Resource *resource) override;

private:
    void _q_handleMaybeWindowPixelSizeChanged();
    void _q_handleWindowDestroyed();

    QWaylandCompositor *compositor = nullptr;
    QWindow *window = nullptr;
    QString manufacturer;
    QString model;
    QPoint position;
    QVector<QWaylandOutputMode> modes;
    int currentMode = -1;
    int preferredMode = -1;
    QRect availableGeometry;
    QVector<QWaylandSurfaceViewMapper> surfaceViews;
    QSize physicalSize;
    QWaylandOutput::Subpixel subpixel = QWaylandOutput::SubpixelUnknown;
    QWaylandOutput::Transform transform = QWaylandOutput::TransformNormal;
    int scaleFactor = 1;
    bool sizeFollowsWindow = false;
    bool initialized = false;
    QSize windowPixelSize;

    Q_DECLARE_PUBLIC(QWaylandOutput)
    Q_DISABLE_COPY(QWaylandOutputPrivate)

    friend class QWaylandXdgOutputManagerV1Private;
};


QT_END_NAMESPACE

#endif  /*QWAYLANDOUTPUT_P_H*/
