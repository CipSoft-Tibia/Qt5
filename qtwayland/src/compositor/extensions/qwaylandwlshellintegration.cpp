// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwaylandwlshellintegration_p.h"

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandWlShellSurface>
#include <QtWaylandCompositor/QWaylandQuickShellSurfaceItem>
#include <QtWaylandCompositor/QWaylandSeat>

QT_BEGIN_NAMESPACE

namespace QtWayland {

WlShellIntegration::WlShellIntegration(QWaylandQuickShellSurfaceItem *item)
    : QWaylandQuickShellIntegration(item)
    , m_item(item)
    , m_shellSurface(qobject_cast<QWaylandWlShellSurface *>(item->shellSurface()))
{
    m_item->setSurface(m_shellSurface->surface());
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::startMove, this, &WlShellIntegration::handleStartMove);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::startResize, this, &WlShellIntegration::handleStartResize);
    connect(m_shellSurface->surface(), &QWaylandSurface::redraw, this, &WlShellIntegration::handleRedraw);
    connect(m_shellSurface->surface(), &QWaylandSurface::offsetForNextFrame, this, &WlShellIntegration::adjustOffsetForNextFrame);
    connect(m_shellSurface->surface(), &QWaylandSurface::hasContentChanged, this, &WlShellIntegration::handleSurfaceHasContentChanged);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::setDefaultToplevel, this, &WlShellIntegration::handleSetDefaultTopLevel);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::setTransient, this, &WlShellIntegration::handleSetTransient);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::setMaximized, this, &WlShellIntegration::handleSetMaximized);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::setFullScreen, this, &WlShellIntegration::handleSetFullScreen);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::setPopup, this, &WlShellIntegration::handleSetPopup);
    connect(m_shellSurface.data(), &QWaylandWlShellSurface::destroyed, this, &WlShellIntegration::handleShellSurfaceDestroyed);
}

WlShellIntegration::~WlShellIntegration()
{
    m_item->setSurface(nullptr);
}

void WlShellIntegration::handleStartMove(QWaylandSeat *seat)
{
    grabberState = GrabberState::Move;
    moveState.seat = seat;
    moveState.initialized = false;
}

void WlShellIntegration::handleStartResize(QWaylandSeat *seat, QWaylandWlShellSurface::ResizeEdge edges)
{
    grabberState = GrabberState::Resize;
    resizeState.seat = seat;
    resizeState.resizeEdges = edges;
    resizeState.initialSize = m_shellSurface->surface()->destinationSize();
    resizeState.initialized = false;
}

void WlShellIntegration::handleSetDefaultTopLevel()
{
    // Take focus if the policy allows
    if (m_shellSurface->shell()->focusPolicy() == QWaylandShell::AutomaticFocus)
        m_item->takeFocus();

    // In order to restore the window state, the client calls setDefaultToplevel()
    // so we need to unset the flags here but we save the previous state and move
    // to the initial position when redrawing
    nextState = State::Windowed;

    // Any handlers for making maximized or fullscreen state track the size of
    // the designated output, are unneeded now that we're going to windowed
    // state.
    nonwindowedState.output = nullptr;
    disconnect(nonwindowedState.sizeChangedConnection);
}

void WlShellIntegration::handleSetTransient(QWaylandSurface *parentSurface, const QPoint &relativeToParent, bool inactive)
{
    Q_UNUSED(parentSurface);
    Q_UNUSED(relativeToParent);

    // Take focus if the policy allows and it's not inactive
    if (m_shellSurface->shell()->focusPolicy() == QWaylandShell::AutomaticFocus && !inactive)
        m_item->takeFocus();
}

void WlShellIntegration::handleSetMaximized(QWaylandOutput *output)
{
    if (!m_item->view()->isPrimary())
        return;

    if (currentState == State::Maximized)
        return;

    QWaylandOutput *designatedOutput = output ? output : m_item->view()->output();
    if (!designatedOutput)
        return;

    if (currentState == State::Windowed)
        normalPosition = m_item->moveItem()->position();

    nextState = State::Maximized;
    finalPosition = designatedOutput->position() + designatedOutput->availableGeometry().topLeft();

    // Any prior output-resize handlers are irrelevant at this point
    disconnect(nonwindowedState.sizeChangedConnection);
    nonwindowedState.output = designatedOutput;
    nonwindowedState.sizeChangedConnection = connect(designatedOutput, &QWaylandOutput::availableGeometryChanged, this, &WlShellIntegration::handleMaximizedSizeChanged);
    handleMaximizedSizeChanged();
}

void WlShellIntegration::handleMaximizedSizeChanged()
{
    if (!m_shellSurface)
        return;

    if (nextState == State::Maximized) {
        QWaylandOutput *designatedOutput = nonwindowedState.output;
        auto scaleFactor = designatedOutput->scaleFactor();
        m_shellSurface->sendConfigure(designatedOutput->availableGeometry().size() / scaleFactor, QWaylandWlShellSurface::NoneEdge);
    }
}

void WlShellIntegration::handleSetFullScreen(QWaylandWlShellSurface::FullScreenMethod method, uint framerate, QWaylandOutput *output)
{
    Q_UNUSED(method);
    Q_UNUSED(framerate);

    if (!m_item->view()->isPrimary())
        return;

    if (currentState == State::FullScreen)
        return;

    QWaylandOutput *designatedOutput = output ? output : m_item->view()->output();
    if (!designatedOutput)
        return;

    if (currentState == State::Windowed)
        normalPosition = m_item->moveItem()->position();

    nextState = State::FullScreen;
    finalPosition = designatedOutput->position();

    // Any prior output-resize handlers are irrelevant at this point
    disconnect(nonwindowedState.sizeChangedConnection);
    nonwindowedState.output = designatedOutput;
    nonwindowedState.sizeChangedConnection = connect(designatedOutput, &QWaylandOutput::geometryChanged, this, &WlShellIntegration::handleFullScreenSizeChanged);
    handleFullScreenSizeChanged();
}

void WlShellIntegration::handleFullScreenSizeChanged()
{
    if (!m_shellSurface)
        return;

    if (nextState == State::FullScreen) {
        QWaylandOutput *designatedOutput = nonwindowedState.output;
        m_shellSurface->sendConfigure(designatedOutput->geometry().size(), QWaylandWlShellSurface::NoneEdge);
    }
}

void WlShellIntegration::handleSetPopup(QWaylandSeat *seat, QWaylandSurface *parent, const QPoint &relativeToParent)
{
    Q_UNUSED(seat);

    // Find the parent item on the same output
    QWaylandQuickShellSurfaceItem *parentItem = nullptr;
    const auto views = parent->views();
    for (QWaylandView *view : views) {
        if (view->output() == m_item->view()->output()) {
            QWaylandQuickShellSurfaceItem *item = qobject_cast<QWaylandQuickShellSurfaceItem*>(view->renderObject());
            if (item) {
                parentItem = item;
                break;
            }
        }
    }

    if (parentItem) {
        // Clear all the transforms for this ShellSurfaceItem. They are not
        // applicable when the item becomes a child to a surface that has its
        // own transforms. Otherwise the transforms would be applied twice.
        QQmlListProperty<QQuickTransform> t = m_item->transform();
        t.clear(&t);
        m_item->setRotation(0);
        m_item->setScale(1.0);
        m_item->setPosition(m_item->mapFromSurface(relativeToParent));
        m_item->setParentItem(parentItem);
    }

    isPopup = true;
    auto shell = m_shellSurface->shell();
    QWaylandQuickShellEventFilter::startFilter(m_shellSurface->surface()->client(), [shell]() {
        shell->closeAllPopups();
    });

    QObject::connect(m_shellSurface->surface(), &QWaylandSurface::hasContentChanged,
                     this, &WlShellIntegration::handleSurfaceHasContentChanged);
}

void WlShellIntegration::handlePopupClosed()
{
    handlePopupRemoved();
    if (m_shellSurface)
        QObject::disconnect(m_shellSurface->surface(), &QWaylandSurface::hasContentChanged,
                            this, &WlShellIntegration::handleSurfaceHasContentChanged);
}

void WlShellIntegration::handlePopupRemoved()
{
    if (!m_shellSurface || m_shellSurface->shell()->mappedPopups().isEmpty())
        QWaylandQuickShellEventFilter::cancelFilter();
    isPopup = false;
}

qreal WlShellIntegration::devicePixelRatio() const
{
    return m_item->window() ? m_item->window()->devicePixelRatio() : 1;
}

void WlShellIntegration::handleShellSurfaceDestroyed()
{
    if (isPopup)
        handlePopupRemoved();

    // Disarm any handlers that might fire and attempt to use the now-stale pointer
    nonwindowedState.output = nullptr;
    disconnect(nonwindowedState.sizeChangedConnection);

    m_shellSurface = nullptr;
}

void WlShellIntegration::handleSurfaceHasContentChanged()
{
    if (m_shellSurface && m_shellSurface->surface()->destinationSize().isEmpty()
            && m_shellSurface->windowType() == Qt::WindowType::Popup) {
        handlePopupClosed();
    }
}

void WlShellIntegration::handleRedraw()
{
    if (currentState == nextState)
        return;

    m_item->moveItem()->setPosition(nextState == State::Windowed ? normalPosition : finalPosition);
    currentState = nextState;
}

void WlShellIntegration::adjustOffsetForNextFrame(const QPointF &offset)
{
    if (!m_item->view()->isPrimary())
        return;

    QQuickItem *moveItem = m_item->moveItem();
    moveItem->setPosition(moveItem->position() + m_item->mapFromSurface(offset));
}

bool WlShellIntegration::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        return filterMouseMoveEvent(mouseEvent);
    } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        return filterMouseReleaseEvent(mouseEvent);
    }
    return QWaylandQuickShellIntegration::eventFilter(object, event);
}

bool WlShellIntegration::filterMouseMoveEvent(QMouseEvent *event)
{
    if (grabberState == GrabberState::Resize) {
        Q_ASSERT(resizeState.seat == m_item->compositor()->seatFor(event));
        if (!resizeState.initialized) {
            resizeState.initialMousePos = event->scenePosition();
            resizeState.initialized = true;
            return true;
        }
        float scaleFactor = m_item->view()->output()->scaleFactor();
        QPointF delta = (event->scenePosition() - resizeState.initialMousePos) / scaleFactor * devicePixelRatio();
        QSize newSize = m_shellSurface->sizeForResize(resizeState.initialSize, delta, resizeState.resizeEdges);
        m_shellSurface->sendConfigure(newSize, resizeState.resizeEdges);
    } else if (grabberState == GrabberState::Move) {
        Q_ASSERT(moveState.seat == m_item->compositor()->seatFor(event));
        QQuickItem *moveItem = m_item->moveItem();
        if (!moveState.initialized) {
            moveState.initialOffset = moveItem->mapFromItem(nullptr, event->scenePosition());
            moveState.initialized = true;
            return true;
        }
        if (!moveItem->parentItem())
            return true;
        QPointF parentPos = moveItem->parentItem()->mapFromItem(nullptr, event->scenePosition());
        moveItem->setPosition(parentPos - moveState.initialOffset);
    }
    return false;
}

bool WlShellIntegration::filterMouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (grabberState != GrabberState::Default) {
        grabberState = GrabberState::Default;
        return true;
    }
    return false;
}

}

QT_END_NAMESPACE

#include "moc_qwaylandwlshellintegration_p.cpp"
