/****************************************************************************
**
** Copyright (C) 2017 Jolla Ltd, author: <giulio.camuffo@jollamobile.com>
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

#include "qwaylandview.h"
#include "qwaylandview_p.h"
#include "qwaylandsurface.h"
#include <QtWaylandCompositor/QWaylandSeat>
#include <QtWaylandCompositor/QWaylandCompositor>

#include <QtWaylandCompositor/private/qwaylandsurface_p.h>
#include <QtWaylandCompositor/private/qwaylandoutput_p.h>

#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

void QWaylandViewPrivate::markSurfaceAsDestroyed(QWaylandSurface *surface)
{
    Q_Q(QWaylandView);
    Q_ASSERT(surface == this->surface);

    setSurface(nullptr);
    QPointer<QWaylandView> deleteGuard(q);
    emit q->surfaceDestroyed();
    if (!deleteGuard.isNull())
        clearFrontBuffer();
}

/*!
 * \qmltype WaylandView
 * \inqmlmodule QtWayland.Compositor
 * \since 5.8
 * \brief Represents a view of a surface on an output.
 *
 * The WaylandView corresponds to the presentation of a surface on a specific
 * output, managing the buffers that contain the contents to be rendered.
 * You can have several views into the same surface.
 */

/*!
 * \class QWaylandView
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandView class represents a view of a surface on an output.
 *
 * The QWaylandView corresponds to the presentation of a surface on a specific
 * output, managing the buffers that contain the contents to be rendered.
 * You can have several views into the same surface.
 */

/*!
 * Constructs a QWaylandView with the given \a renderObject and \a parent.
 */
QWaylandView::QWaylandView(QObject *renderObject, QObject *parent)
    : QObject(*new QWaylandViewPrivate(),parent)
{
    d_func()->renderObject = renderObject;
}

/*!
 * Destroys the QWaylandView.
 */
QWaylandView::~QWaylandView()
{
    Q_D(QWaylandView);
    if (d->surface) {
        if (d->output)
            QWaylandOutputPrivate::get(d->output)->removeView(this, d->surface);

        QWaylandSurfacePrivate::get(d->surface)->derefView(this);
    }

}

/*!
* \internal
*  Didn't we decide to remove this property?
*/
QObject *QWaylandView::renderObject() const
{
    Q_D(const QWaylandView);
    return d->renderObject;
}

/*!
 * \qmlproperty WaylandSurface QtWaylandCompositor::WaylandView::surface
 *
 * This property holds the surface viewed by this WaylandView.
 */

/*!
 * \property QWaylandView::surface
 *
 * This property holds the surface viewed by this QWaylandView.
 */
QWaylandSurface *QWaylandView::surface() const
{
    Q_D(const QWaylandView);
    return d->surface;
}


void QWaylandViewPrivate::setSurface(QWaylandSurface *newSurface)
{
    Q_Q(QWaylandView);
    if (surface) {
        QWaylandSurfacePrivate::get(surface)->derefView(q);
        if (output)
            QWaylandOutputPrivate::get(output)->removeView(q, surface);
    }

    surface = newSurface;

    nextBuffer = QWaylandBufferRef();
    nextBufferCommitted = false;
    nextDamage = QRegion();

    if (surface) {
        QWaylandSurfacePrivate::get(surface)->refView(q);
        if (output)
            QWaylandOutputPrivate::get(output)->addView(q, surface);
    }
}

void QWaylandViewPrivate::clearFrontBuffer()
{
    if (!bufferLocked) {
        currentBuffer = QWaylandBufferRef();
        currentDamage = QRegion();
    }
}

void QWaylandView::setSurface(QWaylandSurface *newSurface)
{
    Q_D(QWaylandView);
    if (d->surface == newSurface)
        return;

    d->setSurface(newSurface);
    d->clearFrontBuffer();
    emit surfaceChanged();
}

/*!
 * \qmlproperty WaylandOutput QtWaylandCompositor::WaylandView::output
 *
 * This property holds the output on which this view displays its surface.
 */

/*!
 * \property QWaylandView::output
 *
 * This property holds the output on which this view displays its surface.
 */
QWaylandOutput *QWaylandView::output() const
{
    Q_D(const QWaylandView);
    return d->output;
}

void QWaylandView::setOutput(QWaylandOutput *newOutput)
{
    Q_D(QWaylandView);
    if (d->output == newOutput)
        return;

    if (d->output && d->surface)
        QWaylandOutputPrivate::get(d->output)->removeView(this, d->surface);

    d->output = newOutput;

    if (d->output && d->surface)
        QWaylandOutputPrivate::get(d->output)->addView(this, d->surface);

    emit outputChanged();
}

/*!
 * This function is called when a new \a buffer is committed to this view's surface.
 * \a damage contains the region that is different from the current buffer, i.e. the
 * region that needs to be updated.
 * The new \a buffer will become current on the next call to advance().
 *
 * Subclasses that reimplement this function \e must call the base implementation.
 */
void QWaylandView::bufferCommitted(const QWaylandBufferRef &buffer, const QRegion &damage)
{
    Q_D(QWaylandView);
    QMutexLocker locker(&d->bufferMutex);
    d->nextBuffer = buffer;
    d->nextDamage = damage;
    d->nextBufferCommitted = true;
}

/*!
 * Updates the current buffer and damage region to the latest version committed by the client.
 * Returns true if new content was committed since the previous call to advance().
 * Otherwise returns false.
 *
 * \sa currentBuffer(), currentDamage()
 */
bool QWaylandView::advance()
{
    Q_D(QWaylandView);

    if (!d->nextBufferCommitted && !d->forceAdvanceSucceed)
        return false;

    if (d->bufferLocked)
        return false;

    if (d->surface && d->surface->primaryView() == this) {
        Q_FOREACH (QWaylandView *view, d->surface->views()) {
            if (view != this && view->allowDiscardFrontBuffer() && view->d_func()->currentBuffer == d->currentBuffer)
                view->discardCurrentBuffer();
        }
    }

    QMutexLocker locker(&d->bufferMutex);
    d->forceAdvanceSucceed = false;
    d->nextBufferCommitted = false;
    d->currentBuffer = d->nextBuffer;
    d->currentDamage = d->nextDamage;
    return true;
}

/*!
 * Force the view to discard its current buffer, to allow it to be reused on the client side.
 */
void QWaylandView::discardCurrentBuffer()
{
    Q_D(QWaylandView);
    QMutexLocker locker(&d->bufferMutex);
    d->currentBuffer = QWaylandBufferRef();
    d->forceAdvanceSucceed = true;
}

/*!
 * Returns a reference to this view's current buffer.
 */
QWaylandBufferRef QWaylandView::currentBuffer()
{
    Q_D(QWaylandView);
    QMutexLocker locker(&d->bufferMutex);
    return d->currentBuffer;
}

/*!
 * Returns the current damage region of this view.
 */
QRegion QWaylandView::currentDamage()
{
    Q_D(QWaylandView);
    QMutexLocker locker(&d->bufferMutex);
    return d->currentDamage;
}

/*!
 * \qmlproperty bool QtWaylandCompositor::WaylandView::bufferLocked
 *
 * This property holds whether the view's buffer is currently locked. When
 * the buffer is locked, advance() will not advance to the next buffer and
 * returns \c false.
 *
 * The default is \c false.
 */

/*!
 * \property QWaylandView::bufferLocked
 *
 * This property holds whether the view's buffer is currently locked. When
 * the buffer is locked, advance() will not advance to the next buffer
 * and returns \c false.
 *
 * The default is \c false.
 */
bool QWaylandView::isBufferLocked() const
{
    Q_D(const QWaylandView);
    return d->bufferLocked;
}

void QWaylandView::setBufferLocked(bool locked)
{
    Q_D(QWaylandView);
    if (d->bufferLocked == locked)
        return;
    d->bufferLocked = locked;
    emit bufferLockedChanged();
}
/*!
 * \qmlproperty bool QtWaylandCompositor::WaylandView::allowDiscardFrontBuffer
 *
 * By default, the view locks the current buffer until advance() is called. Set this property
 * to true to allow Qt to release the buffer when the primary view is no longer using it.
 *
 * This can be used to avoid the situation where a secondary view that updates on a lower
 * frequency will throttle the frame rate of the client application.
 */

/*!
 * \property QWaylandView::allowDiscardFrontBuffer
 *
 * By default, the view locks the current buffer until advance() is called. Set this property
 * to \c true to allow Qt to release the buffer when the primary view is no longer using it.
 *
 * This can be used to avoid the situation where a secondary view that updates on a lower
 * frequency will throttle the frame rate of the client application.
 */
bool QWaylandView::allowDiscardFrontBuffer() const
{
    Q_D(const QWaylandView);
    return d->allowDiscardFrontBuffer;
}

void QWaylandView::setAllowDiscardFrontBuffer(bool discard)
{
    Q_D(QWaylandView);
    if (d->allowDiscardFrontBuffer == discard)
        return;
    d->allowDiscardFrontBuffer = discard;
    emit allowDiscardFrontBufferChanged();
}

/*!
 * Makes this QWaylandView the primary view for the surface.
 *
 * It has no effect if this QWaylandView is not holding any QWaylandSurface
 *
 * \sa QWaylandSurface::primaryView
 */
void QWaylandView::setPrimary()
{
    Q_D(QWaylandView);
    if (d->surface)
        d->surface->setPrimaryView(this);
    else
        qWarning("Calling setPrimary() on a QWaylandView without a surface has no effect.");
}

/*!
 * Returns true if this QWaylandView is the primary view for the QWaylandSurface
 *
 * \sa QWaylandSurface::primaryView
 */
bool QWaylandView::isPrimary() const
{
    Q_D(const QWaylandView);
    return d->surface && d->surface->primaryView() == this;
}

/*!
 * Returns the Wayland surface resource for this QWaylandView.
 */
struct wl_resource *QWaylandView::surfaceResource() const
{
    Q_D(const QWaylandView);
    if (!d->surface)
        return nullptr;
    return d->surface->resource();
}

QT_END_NAMESPACE
