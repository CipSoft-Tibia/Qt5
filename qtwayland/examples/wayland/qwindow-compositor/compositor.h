/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Wayland module
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WINDOWCOMPOSITOR_H
#define WINDOWCOMPOSITOR_H

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandView>
#include <QtWaylandCompositor/QWaylandWlShellSurface>
#include <QtWaylandCompositor/QWaylandXdgSurfaceV5>
#include <QTimer>
#include <QOpenGLTextureBlitter>

QT_BEGIN_NAMESPACE

class QWaylandWlShell;
class QWaylandWlShellSurface;
class QWaylandXdgShellV5;
class QOpenGLTexture;
class Compositor;

class View : public QWaylandView
{
    Q_OBJECT
public:
    View(Compositor *compositor);
    QOpenGLTexture *getTexture();
    QOpenGLTextureBlitter::Origin textureOrigin() const;
    QPointF position() const { return m_position; }
    void setPosition(const QPointF &pos) { m_position = pos; }
    QSize size() const;
    bool isCursor() const;
    bool hasShell() const { return m_wlShellSurface; }
    void setParentView(View *parent) { m_parentView = parent; }
    View *parentView() const { return m_parentView; }
    QPointF parentPosition() const { return m_parentView ? (m_parentView->position() + m_parentView->parentPosition()) : QPointF(); }
    QSize windowSize() { return m_xdgSurface ? m_xdgSurface->windowGeometry().size() :  surface() ? surface()->size() : m_size; }
    QPoint offset() const { return m_offset; }

    qreal animationFactor() const {return m_animationFactor; }
    void setAnimationFactor(qreal f) {m_animationFactor = f; }

signals:
    void animationDone();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    friend class Compositor;
    Compositor *m_compositor = nullptr;
    GLenum m_textureTarget = GL_TEXTURE_2D;
    QOpenGLTexture *m_texture = nullptr;
    QOpenGLTextureBlitter::Origin m_origin;
    QPointF m_position;
    QSize m_size;
    QWaylandWlShellSurface *m_wlShellSurface = nullptr;
    QWaylandXdgSurfaceV5 *m_xdgSurface = nullptr;
    QWaylandXdgPopupV5 *m_xdgPopup = nullptr;
    View *m_parentView = nullptr;
    QPoint m_offset;
    qreal m_animationFactor = 1.0;
    QBasicTimer m_animationTimer;
    bool m_animationCountUp;

public slots:
    void onXdgSetMaximized();
    void onXdgUnsetMaximized();
    void onXdgSetFullscreen(QWaylandOutput *output);
    void onXdgUnsetFullscreen();
    void onOffsetForNextFrame(const QPoint &offset);

    void startAnimation(bool countUp);
    void cancelAnimation();
};

class Compositor : public QWaylandCompositor
{
    Q_OBJECT
public:
    Compositor(QWindow *window);
    ~Compositor() override;
    void create() override;

    void startRender();
    void endRender();

    QList<View*> views() const { return m_views; }
    void raise(View *view);

    void handleMouseEvent(QWaylandView *target, QMouseEvent *me);
    void handleResize(View *target, const QSize &initialSize, const QPoint &delta, int edge);
    void handleDrag(View *target, QMouseEvent *me);

    QWaylandClient *popupClient() const;
    void closePopups();
protected:
    void adjustCursorSurface(QWaylandSurface *surface, int hotspotX, int hotspotY);

signals:
    void startMove();
    void startResize(int edge, bool anchored);
    void dragStarted(View *dragIcon);
    void frameOffset(const QPoint &offset);

public slots:
    void triggerRender();

private slots:
    void surfaceHasContentChanged();
    void surfaceDestroyed();
    void viewSurfaceDestroyed();
    void onStartMove();
    void onWlStartResize(QWaylandSeat *seat, QWaylandWlShellSurface::ResizeEdge edges);
    void onXdgStartResize(QWaylandSeat *seat, QWaylandXdgSurfaceV5::ResizeEdge edges);

    void startDrag();


    void onSurfaceCreated(QWaylandSurface *surface);
    void onWlShellSurfaceCreated(QWaylandWlShellSurface *wlShellSurface);
    void onXdgSurfaceCreated(QWaylandXdgSurfaceV5 *xdgSurface);
    void onXdgPopupRequested(QWaylandSurface *surface, QWaylandSurface *parent, QWaylandSeat *seat,
                             const QPoint &position, const QWaylandResource &resource);
    void onSetTransient(QWaylandSurface *parentSurface, const QPoint &relativeToParent, bool inactive);
    void onSetPopup(QWaylandSeat *seat, QWaylandSurface *parent, const QPoint &relativeToParent);

    void onSubsurfaceChanged(QWaylandSurface *child, QWaylandSurface *parent);
    void onSubsurfacePositionChanged(const QPoint &position);

    void updateCursor();
    void viewAnimationDone();
private:
    View *findView(const QWaylandSurface *s) const;
    QWindow *m_window = nullptr;
    QList<View*> m_views;
    QWaylandWlShell *m_wlShell = nullptr;
    QWaylandXdgShellV5 *m_xdgShell = nullptr;
    QWaylandView m_cursorView;
    int m_cursorHotspotX;
    int m_cursorHotspotY;
};


QT_END_NAMESPACE

#endif // WINDOWCOMPOSITOR_H
