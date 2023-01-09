/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandClient module of the Qt Toolkit.
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


#include "qwaylanddatadevice_p.h"

#include "qwaylanddatadevicemanager_p.h"
#include "qwaylanddataoffer_p.h"
#include "qwaylanddatasource_p.h"
#include "qwaylanddnd_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandabstractdecoration_p.h"
#include "qwaylandsurface_p.h"

#include <QtCore/QMimeData>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qguiapplication_p.h>

#if QT_CONFIG(clipboard)
#include <qpa/qplatformclipboard.h>
#endif
#include <qpa/qplatformdrag.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandDataDevice::QWaylandDataDevice(QWaylandDataDeviceManager *manager, QWaylandInputDevice *inputDevice)
    : QtWayland::wl_data_device(manager->get_data_device(inputDevice->wl_seat()))
    , m_display(manager->display())
    , m_inputDevice(inputDevice)
{
}

QWaylandDataDevice::~QWaylandDataDevice()
{
    if (wl_data_device_get_version(object()) >= WL_DATA_DEVICE_RELEASE_SINCE_VERSION)
        release();
}

QWaylandDataOffer *QWaylandDataDevice::selectionOffer() const
{
    return m_selectionOffer.data();
}

void QWaylandDataDevice::invalidateSelectionOffer()
{
    if (m_selectionOffer.isNull())
        return;

    m_selectionOffer.reset();

#if QT_CONFIG(clipboard)
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
#endif
}

QWaylandDataSource *QWaylandDataDevice::selectionSource() const
{
    return m_selectionSource.data();
}

void QWaylandDataDevice::setSelectionSource(QWaylandDataSource *source)
{
    if (source)
        connect(source, &QWaylandDataSource::cancelled, this, &QWaylandDataDevice::selectionSourceCancelled);
    set_selection(source ? source->object() : nullptr, m_inputDevice->serial());
    m_selectionSource.reset(source);
}

#if QT_CONFIG(draganddrop)
QWaylandDataOffer *QWaylandDataDevice::dragOffer() const
{
    return m_dragOffer.data();
}

bool QWaylandDataDevice::startDrag(QMimeData *mimeData, Qt::DropActions supportedActions, QWaylandWindow *icon)
{
    auto *seat = m_display->currentInputDevice();
    auto *origin = seat->pointerFocus();
    if (!origin)
        origin = seat->touchFocus();

    if (!origin) {
        qCDebug(lcQpaWayland) << "Couldn't start a drag because the origin window could not be found.";
        return false;
    }

    m_dragSource.reset(new QWaylandDataSource(m_display->dndSelectionHandler(), mimeData));

    if (wl_data_device_get_version(object()) >= 3)
        m_dragSource->set_actions(dropActionsToWl(supportedActions));

    connect(m_dragSource.data(), &QWaylandDataSource::cancelled, this, &QWaylandDataDevice::dragSourceCancelled);
    connect(m_dragSource.data(), &QWaylandDataSource::dndResponseUpdated, this, [this](bool accepted, Qt::DropAction action) {
            auto drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
            // in old versions drop action is not set, so we guess
            if (wl_data_source_get_version(m_dragSource->object()) < 3) {
                drag->setResponse(accepted);
            } else {
                QPlatformDropQtResponse response(accepted, action);
                drag->setResponse(response);
            }
    });
    connect(m_dragSource.data(), &QWaylandDataSource::dndDropped, this, [](bool accepted, Qt::DropAction action) {
        QPlatformDropQtResponse response(accepted, action);
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->setDropResponse(response);
    });
    connect(m_dragSource.data(), &QWaylandDataSource::finished, this, []() {
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->finishDrag();
    });

    start_drag(m_dragSource->object(), origin->wlSurface(), icon->wlSurface(), m_display->currentInputDevice()->serial());
    return true;
}

void QWaylandDataDevice::cancelDrag()
{
    m_dragSource.reset();
}
#endif

void QWaylandDataDevice::data_device_data_offer(struct ::wl_data_offer *id)
{
    new QWaylandDataOffer(m_display, id);
}

#if QT_CONFIG(draganddrop)
void QWaylandDataDevice::data_device_drop()
{
    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();

    QMimeData *dragData = nullptr;
    Qt::DropActions supportedActions;
    if (drag) {
        dragData = drag->mimeData();
        supportedActions = drag->supportedActions();
    } else if (m_dragOffer) {
        dragData = m_dragOffer->mimeData();
        supportedActions = m_dragOffer->supportedActions();
    } else {
        return;
    }

    QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(m_dragWindow, dragData, m_dragPoint, supportedActions,
                                                                          QGuiApplication::mouseButtons(),
                                                                          QGuiApplication::keyboardModifiers());

    if (drag) {
        auto drag =  static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag());
        drag->setDropResponse(response);
        drag->finishDrag();
    } else if (m_dragOffer) {
        m_dragOffer->finish();
    }
}

void QWaylandDataDevice::data_device_enter(uint32_t serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer *id)
{
    auto *dragWaylandWindow = surface ? QWaylandWindow::fromWlSurface(surface) : nullptr;
    if (!dragWaylandWindow)
        return; // Ignore foreign surfaces

    m_dragWindow = dragWaylandWindow->window();
    m_dragPoint = calculateDragPosition(x, y, m_dragWindow);
    m_enterSerial = serial;

    QMimeData *dragData = nullptr;
    Qt::DropActions supportedActions;

    m_dragOffer.reset(static_cast<QWaylandDataOffer *>(wl_data_offer_get_user_data(id)));
    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();
    if (drag) {
        dragData = drag->mimeData();
        supportedActions = drag->supportedActions();
    } else if (m_dragOffer) {
        dragData = m_dragOffer->mimeData();
        supportedActions = m_dragOffer->supportedActions();
    }

    const QPlatformDragQtResponse &response = QWindowSystemInterface::handleDrag(m_dragWindow, dragData, m_dragPoint, supportedActions,
                                                                                 QGuiApplication::mouseButtons(),
                                                                                 QGuiApplication::keyboardModifiers());

    if (drag) {
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->setResponse(response);
    }

    sendResponse(supportedActions, response);
}

void QWaylandDataDevice::data_device_leave()
{
    if (m_dragWindow)
        QWindowSystemInterface::handleDrag(m_dragWindow, nullptr, QPoint(), Qt::IgnoreAction,
                                           QGuiApplication::mouseButtons(),
                                           QGuiApplication::keyboardModifiers());

    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();
    if (!drag) {
        m_dragOffer.reset();
    }
}

void QWaylandDataDevice::data_device_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    Q_UNUSED(time);

    QDrag *drag = static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->currentDrag();

    if (!drag && !m_dragOffer)
        return;

    m_dragPoint = calculateDragPosition(x, y, m_dragWindow);

    QMimeData *dragData = nullptr;
    Qt::DropActions supportedActions;
    if (drag) {
        dragData = drag->mimeData();
        supportedActions = drag->supportedActions();
    } else {
        dragData = m_dragOffer->mimeData();
        supportedActions = m_dragOffer->supportedActions();
    }

    const QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(m_dragWindow, dragData, m_dragPoint, supportedActions,
                                                                          QGuiApplication::mouseButtons(),
                                                                          QGuiApplication::keyboardModifiers());

    if (drag) {
        static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->setResponse(response);
    }

    sendResponse(supportedActions, response);
}
#endif // QT_CONFIG(draganddrop)

void QWaylandDataDevice::data_device_selection(wl_data_offer *id)
{
    if (id)
        m_selectionOffer.reset(static_cast<QWaylandDataOffer *>(wl_data_offer_get_user_data(id)));
    else
        m_selectionOffer.reset();

#if QT_CONFIG(clipboard)
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
#endif
}

void QWaylandDataDevice::selectionSourceCancelled()
{
    m_selectionSource.reset();
#if QT_CONFIG(clipboard)
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
#endif
}

#if QT_CONFIG(draganddrop)
void QWaylandDataDevice::dragSourceCancelled()
{
    static_cast<QWaylandDrag *>(QGuiApplicationPrivate::platformIntegration()->drag())->finishDrag();
    m_dragSource.reset();
}

QPoint QWaylandDataDevice::calculateDragPosition(int x, int y, QWindow *wnd) const
{
    QPoint pnt(wl_fixed_to_int(x), wl_fixed_to_int(y));
    if (wnd) {
        QWaylandWindow *wwnd = static_cast<QWaylandWindow*>(m_dragWindow->handle());
        if (wwnd && wwnd->decoration()) {
            pnt -= QPoint(wwnd->decoration()->margins().left(),
                          wwnd->decoration()->margins().top());
        }
    }
    return pnt;
}

void QWaylandDataDevice::sendResponse(Qt::DropActions supportedActions, const QPlatformDragQtResponse &response)
{
    if (response.isAccepted()) {
        if (wl_data_device_get_version(object()) >= 3)
            m_dragOffer->set_actions(dropActionsToWl(supportedActions), dropActionsToWl(response.acceptedAction()));

        m_dragOffer->accept(m_enterSerial, m_dragOffer->firstFormat());
    } else {
        m_dragOffer->accept(m_enterSerial, QString());
    }
}

int QWaylandDataDevice::dropActionsToWl(Qt::DropActions actions)
{

    int wlActions = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
    if (actions & Qt::CopyAction)
        wlActions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
    if (actions & (Qt::MoveAction | Qt::TargetMoveAction))
        wlActions |= WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;

    // wayland does not support LinkAction at the time of writing
    return wlActions;
}


#endif // QT_CONFIG(draganddrop)

}

QT_END_NAMESPACE
