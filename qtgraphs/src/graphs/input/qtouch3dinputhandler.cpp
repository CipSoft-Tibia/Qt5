// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtouch3dinputhandler_p.h"
#include "abstract3dcontroller_p.h"
#include <QtCore/QTimer>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

static const float maxTapAndHoldJitter = 20.0f;
static const int maxPinchJitter = 10;
#if defined (Q_OS_ANDROID) || defined(Q_OS_IOS)
static const int maxSelectionJitter = 10;
#else
static const int maxSelectionJitter = 5;
#endif
static const int tapAndHoldTime = 250;
static const float rotationSpeed = 200.0f;
static const float touchZoomDrift = 0.02f;

/*!
 * \class QTouch3DInputHandler
 * \inmodule QtGraphs
 * \brief Basic touch display based input handler.
 *
 * QTouch3DInputHandler is the basic input handler for touch screen devices.
 *
 * Default touch input handler has the following functionalty:
 * \table
 *      \header
 *          \li Gesture
 *          \li Action
 *      \row
 *          \li Touch-And-Move
 *          \li Rotate graph within limits set for Q3DCamera
 *      \row
 *          \li Tap
 *          \li Select the item tapped or remove selection if none.
 *              May open the secondary view depending on the
 *              \l {QAbstract3DGraph::selectionMode}{selection mode}.
 *      \row
 *          \li Tap-And-Hold
 *          \li Same as tap.
 *      \row
 *          \li Pinch
 *          \li Zoom in/out within the allowable zoom range set for Q3DCamera.
 *      \row
 *          \li Tap on the primary view when the secondary view is visible
 *          \li Closes the secondary view.
 *          \note Secondary view is available only for Q3DBars and Q3DSurface graphs.
 * \endtable
 *
 * Rotation, zoom, and selection can each be individually disabled using
 * corresponding Q3DInputHandler properties.
 */

/*!
 * \qmltype TouchInputHandler3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates QTouch3DInputHandler
 * \inherits InputHandler3D
 * \brief Basic touch display based input handler.
 *
 * TouchInputHandler3D is the basic input handler for touch screen devices.
 *
 * See QTouch3DInputHandler documentation for more details.
 */

/*!
 * Constructs the basic touch display input handler. An optional \a parent parameter can be given
 * and is then passed to QObject constructor.
 */
QTouch3DInputHandler::QTouch3DInputHandler(QObject *parent)
    : Q3DInputHandler(new QTouch3DInputHandlerPrivate(this), parent)
{
}

/*!
 *  Destroys the input handler.
 */
QTouch3DInputHandler::~QTouch3DInputHandler()
{
}

/*!
 * Override this to change handling of touch events.
 * Touch event is given in the \a event.
 */
void QTouch3DInputHandler::touchEvent(QTouchEvent *event)
{
    Q_D(QTouch3DInputHandler);
    QList<QTouchEvent::TouchPoint> points;
    points = event->points();

    if (!scene()->isSlicingActive() && points.size() == 2) {
        d->m_holdTimer->stop();
        QPointF distance = points.at(0).position() - points.at(1).position();
        QPoint midPoint = ((points.at(0).position() + points.at(1).position()) / 2.0).toPoint();
        d->handlePinchZoom(distance.manhattanLength(), midPoint);
    } else if (points.size() == 1) {
        QPointF pointerPos = points.at(0).position();
        if (event->type() == QEvent::TouchBegin) {
            // Flush input state
            d->m_inputState = QAbstract3DInputHandlerPrivate::InputStateNone;
            if (scene()->isSlicingActive()) {
                if (isSelectionEnabled()) {
                    if (scene()->isPointInPrimarySubView(pointerPos.toPoint()))
                        setInputView(InputViewOnPrimary);
                    else if (scene()->isPointInSecondarySubView(pointerPos.toPoint()))
                        setInputView(InputViewOnSecondary);
                    else
                        setInputView(InputViewNone);
                }
            } else {
                // Handle possible tap-and-hold selection
                if (isSelectionEnabled()) {
                    d->m_startHoldPos = pointerPos;
                    d->m_touchHoldPos = d->m_startHoldPos;
                    d->m_holdTimer->start();
                    setInputView(InputViewOnPrimary);
                }
                // Start rotating
                if (isRotationEnabled()) {
                    d->m_inputState = QAbstract3DInputHandlerPrivate::InputStateRotating;
                    setInputPosition(pointerPos.toPoint());
                    setInputView(InputViewOnPrimary);
                }
            }
        } else if (event->type() == QEvent::TouchEnd) {
            setInputView(InputViewNone);
            d->m_holdTimer->stop();
            // Handle possible selection
            if (!scene()->isSlicingActive()
                    && QAbstract3DInputHandlerPrivate::InputStatePinching
                    != d->m_inputState) {
                d->handleSelection(pointerPos);
            }
        } else if (event->type() == QEvent::TouchUpdate) {
            if (!scene()->isSlicingActive()) {
                d->m_touchHoldPos = pointerPos;
                // Handle rotation
                d->handleRotation(pointerPos);
            }
        }
    } else {
        d->m_holdTimer->stop();
    }
}

QTouch3DInputHandlerPrivate::QTouch3DInputHandlerPrivate(QTouch3DInputHandler *q)
    : Q3DInputHandlerPrivate(q),
      m_holdTimer(0)
{
    m_holdTimer = new QTimer();
    m_holdTimer->setSingleShot(true);
    m_holdTimer->setInterval(tapAndHoldTime);
    connect(m_holdTimer, &QTimer::timeout, this, &QTouch3DInputHandlerPrivate::handleTapAndHold);
}

QTouch3DInputHandlerPrivate::~QTouch3DInputHandlerPrivate()
{
    m_holdTimer->stop();
    delete m_holdTimer;
}

void QTouch3DInputHandlerPrivate::handlePinchZoom(float distance, const QPoint &pos)
{
    Q_Q(QTouch3DInputHandler);
    if (q->isZoomEnabled()) {
        int newDistance = distance;
        int prevDist = q->prevDistance();
        if (prevDist > 0 && qAbs(prevDist - newDistance) < maxPinchJitter)
            return;
        m_inputState = QAbstract3DInputHandlerPrivate::InputStatePinching;
        Q3DCamera *camera = q->scene()->activeCamera();
        int zoomLevel = int(camera->zoomLevel());
        const int minZoomLevel = int(camera->minZoomLevel());
        const int maxZoomLevel = int(camera->maxZoomLevel());
        float zoomRate = qSqrt(qSqrt(zoomLevel));
        if (newDistance > prevDist)
            zoomLevel += zoomRate;
        else
            zoomLevel -= zoomRate;
        zoomLevel = qBound(minZoomLevel, zoomLevel, maxZoomLevel);

        if (q->isZoomAtTargetEnabled()) {
            q->scene()->setGraphPositionQuery(pos);
            m_controller->setGraphPositionQueryPending(true);
            m_zoomAtTargetPending = true;
            // If zoom at target is enabled, we don't want to zoom yet, as that causes
            // jitter. Instead, we zoom next frame, when we apply the camera position.
            m_requestedZoomLevel = zoomLevel;
            m_driftMultiplier = touchZoomDrift;
        } else {
            camera->setZoomLevel(zoomLevel);
        }

        q->setPrevDistance(newDistance);
    }
}

void QTouch3DInputHandlerPrivate::handleTapAndHold()
{
    Q_Q(QTouch3DInputHandler);
    if (q->isSelectionEnabled()) {
        QPointF distance = m_startHoldPos - m_touchHoldPos;
        if (distance.manhattanLength() < maxTapAndHoldJitter) {
            m_inputState = QAbstract3DInputHandlerPrivate::InputStateSelecting;
            q->setInputPosition(m_touchHoldPos.toPoint());
            q->scene()->setSelectionQueryPosition(m_touchHoldPos.toPoint());
        }
    }
}

void QTouch3DInputHandlerPrivate::handleSelection(const QPointF &position)
{
    Q_Q(QTouch3DInputHandler);
    if (q->isSelectionEnabled()) {
        QPointF distance = m_startHoldPos - position;
        if (distance.manhattanLength() < maxSelectionJitter) {
            m_inputState = QAbstract3DInputHandlerPrivate::InputStateSelecting;
            q->scene()->setSelectionQueryPosition(position.toPoint());
        } else {
            m_inputState = QAbstract3DInputHandlerPrivate::InputStateNone;
            q->setInputView(QAbstract3DInputHandler::InputViewNone);
        }
        q->setPreviousInputPos(position.toPoint());
    }
}

void QTouch3DInputHandlerPrivate::handleRotation(const QPointF &position)
{
    Q_Q(QTouch3DInputHandler);
    if (q->isRotationEnabled()
            && QAbstract3DInputHandlerPrivate::InputStateRotating == m_inputState) {
        Q3DScene *scene = q->scene();
        Q3DCamera *camera = scene->activeCamera();
        float xRotation = camera->xRotation();
        float yRotation = camera->yRotation();
        QPointF inputPos = q->inputPosition();
        float mouseMoveX = float(inputPos.x() - position.x())
                / (scene->viewport().width() / rotationSpeed);
        float mouseMoveY = float(inputPos.y() - position.y())
                / (scene->viewport().height() / rotationSpeed);
        xRotation -= mouseMoveX;
        yRotation -= mouseMoveY;
        camera->setXRotation(xRotation);
        camera->setYRotation(yRotation);

        q->setPreviousInputPos(inputPos.toPoint());
        q->setInputPosition(position.toPoint());
    }
}

QT_END_NAMESPACE
