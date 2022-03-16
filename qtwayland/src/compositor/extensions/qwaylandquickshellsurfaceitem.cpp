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

#include "qwaylandquickshellsurfaceitem.h"
#include "qwaylandquickshellsurfaceitem_p.h"

#include <QtWaylandCompositor/QWaylandShellSurface>
#include <QGuiApplication>

QT_BEGIN_NAMESPACE

QWaylandQuickShellSurfaceItem *QWaylandQuickShellSurfaceItemPrivate::maybeCreateAutoPopup(QWaylandShellSurface* shellSurface)
{
    if (!m_autoCreatePopupItems)
        return nullptr;

    Q_Q(QWaylandQuickShellSurfaceItem);
    auto *popupItem = new QWaylandQuickShellSurfaceItem(q);
    popupItem->setShellSurface(shellSurface);
    popupItem->setAutoCreatePopupItems(true);
    QObject::connect(popupItem, &QWaylandQuickShellSurfaceItem::surfaceDestroyed, [popupItem](){
        popupItem->deleteLater();
    });
    return popupItem;
}

/*!
 * \qmltype ShellSurfaceItem
 * \inherits WaylandQuickItem
 * \inqmlmodule QtWayland.Compositor
 * \since 5.8
 * \brief A Qt Quick item type for displaying and interacting with a ShellSurface.
 *
 * This type is used to render \c wl_shell, \c xdg_shell or \c ivi_application surfaces as part of
 * a Qt Quick scene. It handles moving and resizing triggered by clicking on the window decorations.
 *
 * \sa WaylandQuickItem, WlShellSurface, XdgSurfaceV5, IviSurface
 */

/*!
 * \class QWaylandQuickShellSurfaceItem
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandQuickShellSurfaceItem class provides a Qt Quick item that represents a QWaylandShellSurface.
 *
 * This class is used to render \c wl_shell, \c xdg_shell or \c ivi_application surfaces as part of
 * a Qt Quick scene. It handles moving and resizing triggered by clicking on the window decorations.
 *
 * \sa QWaylandQuickItem, QWaylandWlShellSurface, QWaylandXdgSurfaceV5, QWaylandIviSurface
 */

/*!
 * Constructs a QWaylandQuickWlShellSurfaceItem with the given \a parent.
 */
QWaylandQuickShellSurfaceItem::QWaylandQuickShellSurfaceItem(QQuickItem *parent)
    : QWaylandQuickItem(*new QWaylandQuickShellSurfaceItemPrivate(), parent)
{
}

QWaylandQuickShellSurfaceItem::~QWaylandQuickShellSurfaceItem()
{
    Q_D(QWaylandQuickShellSurfaceItem);
    delete d->m_shellIntegration;
}

/*!
 * \internal
 */
QWaylandQuickShellSurfaceItem::QWaylandQuickShellSurfaceItem(QWaylandQuickShellSurfaceItemPrivate &dd, QQuickItem *parent)
    : QWaylandQuickItem(dd, parent)
{
}

/*!
 * \qmlproperty ShellSurface QtWaylandCompositor::ShellSurfaceItem::shellSurface
 *
 * This property holds the ShellSurface rendered by this ShellSurfaceItem.
 * It may either be an XdgSurfaceV5, WlShellSurface or IviSurface depending on which shell protocol
 * is in use.
 */

/*!
 * \property QWaylandQuickShellSurfaceItem::shellSurface
 *
 * This property holds the QWaylandShellSurface rendered by this QWaylandQuickShellSurfaceItem.
 * It may either be a QWaylandXdgSurfaceV5, QWaylandWlShellSurface or QWaylandIviSurface depending
 * on which shell protocol is in use.
 */
QWaylandShellSurface *QWaylandQuickShellSurfaceItem::shellSurface() const
{
    Q_D(const QWaylandQuickShellSurfaceItem);
    return d->m_shellSurface;
}

void QWaylandQuickShellSurfaceItem::setShellSurface(QWaylandShellSurface *shellSurface)
{
    Q_D(QWaylandQuickShellSurfaceItem);
    if (d->m_shellSurface == shellSurface)
        return;

    d->m_shellSurface = shellSurface;

    if (d->m_shellIntegration) {
        delete d->m_shellIntegration;
        d->m_shellIntegration = nullptr;
    }

    if (shellSurface)
        d->m_shellIntegration = shellSurface->createIntegration(this);

    emit shellSurfaceChanged();
}

/*!
 * \qmlproperty Item QtWaylandCompositor::ShellSurfaceItem::moveItem
 *
 * This property holds the move item for this ShellSurfaceItem. This is the item that will be moved
 * when the clients request the ShellSurface to be moved, maximized, resized etc. This property is
 * useful when implementing server-side decorations.
 */

/*!
 * \property QWaylandQuickShellSurfaceItem::moveItem
 *
 * This property holds the move item for this QWaylandQuickShellSurfaceItem. This is the item that
 * will be moved when the clients request the QWaylandShellSurface to be moved, maximized, resized
 * etc. This property is useful when implementing server-side decorations.
 */
QQuickItem *QWaylandQuickShellSurfaceItem::moveItem() const
{
    Q_D(const QWaylandQuickShellSurfaceItem);
    return d->m_moveItem ? d->m_moveItem : const_cast<QWaylandQuickShellSurfaceItem *>(this);
}

void QWaylandQuickShellSurfaceItem::setMoveItem(QQuickItem *moveItem)
{
    Q_D(QWaylandQuickShellSurfaceItem);
    moveItem = moveItem ? moveItem : this;
    if (this->moveItem() == moveItem)
        return;
    d->m_moveItem = moveItem;
    moveItemChanged();
}

/*!
 * \qmlproperty bool QtWaylandCompositor::ShellSurfaceItem::autoCreatePopupItems
 *
 * This property holds whether ShellSurfaceItems for popups parented to the shell
 * surface managed by this item should automatically be created.
 */

/*!
 * \property QWaylandQuickShellSurfaceItem::autoCreatePopupItems
 *
 * This property holds whether QWaylandQuickShellSurfaceItems for popups
 * parented to the shell surface managed by this item should automatically be created.
 */
bool QWaylandQuickShellSurfaceItem::autoCreatePopupItems()
{
    Q_D(const QWaylandQuickShellSurfaceItem);
    return d->m_autoCreatePopupItems;
}

void QWaylandQuickShellSurfaceItem::setAutoCreatePopupItems(bool enabled)
{
    Q_D(QWaylandQuickShellSurfaceItem);

    if (enabled == d->m_autoCreatePopupItems)
        return;

    d->m_autoCreatePopupItems = enabled;
    emit autoCreatePopupItemsChanged();
}

void QWaylandQuickShellSurfaceItem::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QWaylandQuickShellSurfaceItem);
    if (!d->m_shellIntegration->mouseMoveEvent(event))
        QWaylandQuickItem::mouseMoveEvent(event);
}

void QWaylandQuickShellSurfaceItem::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QWaylandQuickShellSurfaceItem);
    if (!d->m_shellIntegration->mouseReleaseEvent(event))
        QWaylandQuickItem::mouseReleaseEvent(event);
}

/*!
\class QWaylandQuickShellEventFilter
\brief QWaylandQuickShellEventFilter implements a Wayland popup grab
\internal
*/

void QWaylandQuickShellEventFilter::startFilter(QWaylandClient *client, CallbackFunction closePopups)
{
    if (!self)
        self = new QWaylandQuickShellEventFilter(qGuiApp);
    if (!self->eventFilterInstalled) {
        qGuiApp->installEventFilter(self);
        self->eventFilterInstalled = true;
        self->client = client;
        self->closePopups = closePopups;
    }
}

void QWaylandQuickShellEventFilter::cancelFilter()
{
    if (!self)
        return;
    if (self->eventFilterInstalled && !self->waitForRelease)
        self->stopFilter();
}

void QWaylandQuickShellEventFilter::stopFilter()
{
    if (eventFilterInstalled) {
        qGuiApp->removeEventFilter(this);
        eventFilterInstalled = false;
    }
}
QWaylandQuickShellEventFilter *QWaylandQuickShellEventFilter::self = nullptr;

QWaylandQuickShellEventFilter::QWaylandQuickShellEventFilter(QObject *parent)
    : QObject(parent)
{
}

bool QWaylandQuickShellEventFilter::eventFilter(QObject *receiver, QEvent *e)
{
    if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease) {
        bool press = e->type() == QEvent::MouseButtonPress;
        if (press && !waitForRelease) {
            // The user clicked something: we need to close popups unless this press is caught later
            if (!mousePressTimeout.isActive())
                mousePressTimeout.start(0, this);
        }

        QQuickItem *item = qobject_cast<QQuickItem*>(receiver);
        if (!item)
            return false;

        QMouseEvent *event = static_cast<QMouseEvent*>(e);
        QWaylandQuickShellSurfaceItem *shellSurfaceItem = qobject_cast<QWaylandQuickShellSurfaceItem*>(item);
        bool finalRelease = (event->type() == QEvent::MouseButtonRelease) && (event->buttons() == Qt::NoButton);
        bool popupClient = shellSurfaceItem && shellSurfaceItem->surface() && shellSurfaceItem->surface()->client() == client;

        if (waitForRelease) {
            // We are eating events until all mouse buttons are released
            if (finalRelease) {
                waitForRelease = false;
                stopFilter();
            }
            return true;
        }

        if (finalRelease && mousePressTimeout.isActive()) {
            // the user somehow managed to press and release the mouse button in 0 milliseconds
            qWarning("Badly written autotest detected");
            mousePressTimeout.stop();
            stopFilter();
        }

        if (press && !shellSurfaceItem && !QQmlProperty(item, QStringLiteral("qtwayland_blocking_overlay")).isValid()) {
            // the user clicked on something that's not blocking mouse events
            e->ignore(); //propagate the event to items below
            return true; // don't give the event to the item
        }

        mousePressTimeout.stop(); // we've got this

        if (press && !popupClient) {
            // The user clicked outside the active popup's client. The popups should
            // be closed, but the event filter will stay to catch the release-
            // event before removing itself.
            waitForRelease = true;
            closePopups();
            return true;
        }
    }

    return false;
}

void QWaylandQuickShellEventFilter::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mousePressTimeout.timerId()) {
        mousePressTimeout.stop();
        closePopups();
        stopFilter();
        // Don't wait for release: Since the press wasn't accepted,
        // the release won't be delivered.
    }
}

QT_END_NAMESPACE
