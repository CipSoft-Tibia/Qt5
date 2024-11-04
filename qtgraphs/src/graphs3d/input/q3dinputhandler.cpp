// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "graphsglobal_p.h"
#include "q3dinputhandler_p.h"
#include "qquickgraphsitem_p.h"

QT_BEGIN_NAMESPACE

static const int halfSizeZoomLevel = 50;
static const int oneToOneZoomLevel = 100;
static const int driftTowardCenterLevel = 175;
static const float wheelZoomDrift = 0.1f;

static const int nearZoomRangeDivider = 12;
static const int midZoomRangeDivider = 60;
static const int farZoomRangeDivider = 120;

#if !defined(Q_OS_IOS)
static const float rotationSpeed = 100.0f;
#endif

/*!
 * \class Q3DInputHandler
 * \inmodule QtGraphs
 * \ingroup graphs_3D
 * \brief Basic wheel mouse based input handler.
 *
 * Q3DInputHandler is the basic input handler for wheel mouse type of input
 * devices.
 *
 * Default input handler has the following functionalty:
 * \table
 *      \header
 *          \li Mouse action
 *          \li Action
 *      \row
 *          \li Drag with right button pressed
 *          \li Rotate graph within limits.
 *      \row
 *          \li Left click
 *          \li Select item under cursor or remove selection if none.
 *              May open the secondary view depending on the
 *              \l {QAbstract3DGraph::selectionMode}{selection mode}.
 *      \row
 *          \li Mouse wheel
 *          \li Zoom in/out within the allowable zoom range.
 *      \row
 *          \li Left click on the primary view when the secondary view is visible
 *          \li Closes the secondary view.
 *          \note Secondary view is available only for Q3DBars and Q3DSurface graphs.
 * \endtable
 *
 * Rotation, zoom, and selection can each be individually disabled using
 * corresponding properties of this class.
 */

/*!
 * \qmltype InputHandler3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml_3D
 * \instantiates Q3DInputHandler
 * \brief Basic wheel mouse based input handler.
 *
 * InputHandler3D is the basic input handler for wheel mouse type of input
 * devices.
 *
 * See Q3DInputHandler documentation for more details.
 */

/*!
 * \qmlproperty bool InputHandler3D::rotationEnabled
 *
 * Defines whether this input handler allows graph rotation.
 * Defaults to \c{true}.
 */

/*!
 * \qmlproperty bool InputHandler3D::zoomEnabled
 *
 * Defines whether this input handler allows graph zooming.
 * Defaults to \c{true}.
 */

/*!
 * \qmlproperty bool InputHandler3D::selectionEnabled
 *
 * Defines whether this input handler allows selection from the graph.
 * Defaults to \c{true}.
 */

/*!
 * \qmlproperty bool InputHandler3D::zoomAtTargetEnabled
 *
 * Defines whether zooming changes the camera target to the position of the
 * input at the time of the zoom. Defaults to \c{true}.
 */

/*!
 * \internal
 */
Q3DInputHandler::Q3DInputHandler(Q3DInputHandlerPrivate *d, QObject *parent)
    : QAbstract3DInputHandler(d, parent)
{
    QObject::connect(this,
                     &QAbstract3DInputHandler::sceneChanged,
                     d,
                     &Q3DInputHandlerPrivate::handleSceneChange);
}

/*!
 * Constructs the basic mouse input handler. An optional \a parent parameter can
 * be given and is then passed to QObject constructor.
 */
Q3DInputHandler::Q3DInputHandler(QObject *parent)
    : QAbstract3DInputHandler(new Q3DInputHandlerPrivate(this), parent)
{}

/*!
 *  Destroys the input handler.
 */
Q3DInputHandler::~Q3DInputHandler() {}

// Input event listeners
/*!
 * Override this to change handling of mouse press events.
 * Mouse press event is given in the \a event and the mouse position in \a
 * mousePos.
 */
void Q3DInputHandler::mousePressEvent(QMouseEvent *event, const QPoint &mousePos)
{
#if defined(Q_OS_IOS)
    Q_UNUSED(event);
    Q_UNUSED(mousePos);
#else
    Q_D(Q3DInputHandler);
    if (Qt::LeftButton == event->button()) {
        if (isSelectionEnabled()) {
            if (scene()->isSlicingActive()) {
                if (scene()->isPointInPrimarySubView(mousePos))
                    setInputView(QAbstract3DInputHandler::InputView::OnPrimary);
                else if (scene()->isPointInSecondarySubView(mousePos))
                    setInputView(QAbstract3DInputHandler::InputView::OnSecondary);
                else
                    setInputView(QAbstract3DInputHandler::InputView::None);
            } else {
                // update mouse positions to prevent jumping when releasing or
                // repressing a button
                d->m_inputState = QAbstract3DInputHandlerPrivate::InputState::Selecting;
                setInputPosition(mousePos);
                scene()->setSelectionQueryPosition(mousePos);
                setInputView(QAbstract3DInputHandler::InputView::OnPrimary);
            }
        }
    } else if (Qt::MiddleButton == event->button()) {
        if (isRotationEnabled()) {
            // reset rotations
            setInputPosition(QPoint(0, 0));
        }
    } else if (Qt::RightButton == event->button()) {
        if (isRotationEnabled()) {
            // disable rotating when in slice view
            if (!scene()->isSlicingActive())
                d->m_inputState = QAbstract3DInputHandlerPrivate::InputState::Rotating;
            // update mouse positions to prevent jumping when releasing or repressing
            // a button
            setInputPosition(mousePos);
        }
    }
#endif
}

/*!
 * Override this to change handling of mouse release events.
 * Mouse release event is given in the \a event and the mouse position in \a
 * mousePos.
 */
void Q3DInputHandler::mouseReleaseEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
#if defined(Q_OS_IOS)
    Q_UNUSED(mousePos);
#else
    Q_D(Q3DInputHandler);
    if (QAbstract3DInputHandlerPrivate::InputState::Rotating == d->m_inputState) {
        // update mouse positions to prevent jumping when releasing or repressing a
        // button
        setInputPosition(mousePos);
    }
    d->m_inputState = QAbstract3DInputHandlerPrivate::InputState::None;
    setInputView(QAbstract3DInputHandler::InputView::None);
#endif
}

/*!
 * Override this to change handling of mouse move events.
 * Mouse move event is given in the \a event and the mouse position in \a
 * mousePos.
 */
void Q3DInputHandler::mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
#if defined(Q_OS_IOS)
    Q_UNUSED(mousePos);
#else
    Q_D(Q3DInputHandler);
    if (QAbstract3DInputHandlerPrivate::InputState::Rotating == d->m_inputState
        && isRotationEnabled()) {
        QQuickGraphsItem *item = this->item();
        // Calculate mouse movement since last frame
        float xRotation = item->cameraXRotation();
        float yRotation = item->cameraYRotation();
        float mouseMoveX = float(inputPosition().x() - mousePos.x())
                           / (scene()->viewport().width() / rotationSpeed);
        float mouseMoveY = float(inputPosition().y() - mousePos.y())
                           / (scene()->viewport().height() / rotationSpeed);
        // Apply to rotations
        xRotation -= mouseMoveX;
        yRotation -= mouseMoveY;
        item->setCameraXRotation(xRotation);
        item->setCameraYRotation(yRotation);

        setPreviousInputPos(inputPosition());
        setInputPosition(mousePos);
    }
#endif
}

#if QT_CONFIG(wheelevent)
/*!
 * Override this to change handling of wheel events.
 * The wheel event is given in the \a event.
 */
void Q3DInputHandler::wheelEvent(QWheelEvent *event)
{
    Q_D(Q3DInputHandler);
    if (isZoomEnabled()) {
        // disable zooming if in slice view
        if (scene()->isSlicingActive())
            return;

        // Adjust zoom level based on what zoom range we're in.
        QQuickGraphsItem *item = this->item();
        int zoomLevel = int(item->cameraZoomLevel());
        const int minZoomLevel = int(item->minCameraZoomLevel());
        const int maxZoomLevel = int(item->maxCameraZoomLevel());
        if (zoomLevel > oneToOneZoomLevel)
            zoomLevel += event->angleDelta().y() / nearZoomRangeDivider;
        else if (zoomLevel > halfSizeZoomLevel)
            zoomLevel += event->angleDelta().y() / midZoomRangeDivider;
        else
            zoomLevel += event->angleDelta().y() / farZoomRangeDivider;
        zoomLevel = qBound(minZoomLevel, zoomLevel, maxZoomLevel);

        if (isZoomAtTargetEnabled()) {
            if (!d->m_graph)
                d->handleSceneChange(scene());
            d->m_graph->setGraphPositionQueryPending(true);
            scene()->setGraphPositionQuery(event->position().toPoint());
            d->m_zoomAtTargetPending = true;
            // If zoom at target is enabled, we don't want to zoom yet, as that causes
            // jitter. Instead, we zoom next frame, when we apply the camera position.
            d->m_requestedZoomLevel = zoomLevel;
            d->m_driftMultiplier = wheelZoomDrift;
        } else {
            item->setCameraZoomLevel(zoomLevel);
        }
    }
}
#endif

/*!
 * \property Q3DInputHandler::rotationEnabled
 *
 * \brief Whether this input handler allows graph rotation.
 *
 * Defaults to \c{true}.
 */
void Q3DInputHandler::setRotationEnabled(bool enable)
{
    Q_D(Q3DInputHandler);
    if (d->m_rotationEnabled != enable) {
        d->m_rotationEnabled = enable;
        emit rotationEnabledChanged(enable);
    }
}

bool Q3DInputHandler::isRotationEnabled() const
{
    const Q_D(Q3DInputHandler);
    return d->m_rotationEnabled;
}

/*!
 * \property Q3DInputHandler::zoomEnabled
 *
 * \brief Whether this input handler allows graph zooming.
 *
 * Defaults to \c{true}.
 */
void Q3DInputHandler::setZoomEnabled(bool enable)
{
    Q_D(Q3DInputHandler);
    if (d->m_zoomEnabled != enable) {
        d->m_zoomEnabled = enable;
        emit zoomEnabledChanged(enable);
    }
}

bool Q3DInputHandler::isZoomEnabled() const
{
    const Q_D(Q3DInputHandler);
    return d->m_zoomEnabled;
}

/*!
 * \property Q3DInputHandler::selectionEnabled
 *
 * \brief Whether this input handler allows selection from the graph.
 *
 * Defaults to \c{true}.
 */
void Q3DInputHandler::setSelectionEnabled(bool enable)
{
    Q_D(Q3DInputHandler);
    if (d->m_selectionEnabled != enable) {
        d->m_selectionEnabled = enable;
        emit selectionEnabledChanged(enable);
    }
}

bool Q3DInputHandler::isSelectionEnabled() const
{
    const Q_D(Q3DInputHandler);
    return d->m_selectionEnabled;
}

/*!
 * \property Q3DInputHandler::zoomAtTargetEnabled
 *
 * \brief Whether zooming should change the camera target so that the zoomed
 * point of the graph stays at the same location after the zoom.
 *
 * Defaults to \c{true}.
 */
void Q3DInputHandler::setZoomAtTargetEnabled(bool enable)
{
    Q_D(Q3DInputHandler);
    if (d->m_zoomAtTargetEnabled != enable) {
        d->m_zoomAtTargetEnabled = enable;
        emit zoomAtTargetEnabledChanged(enable);
    }
}

bool Q3DInputHandler::isZoomAtTargetEnabled() const
{
    const Q_D(Q3DInputHandler);
    return d->m_zoomAtTargetEnabled;
}

Q3DInputHandlerPrivate::Q3DInputHandlerPrivate(Q3DInputHandler *q)
    : QAbstract3DInputHandlerPrivate(q)
    , m_rotationEnabled(true)
    , m_zoomEnabled(true)
    , m_selectionEnabled(true)
    , m_zoomAtTargetEnabled(true)
    , m_zoomAtTargetPending(false)
    , m_graph(0)
    , m_requestedZoomLevel(0.0f)
    , m_driftMultiplier(0.0f)
{}

Q3DInputHandlerPrivate::~Q3DInputHandlerPrivate() {}

void Q3DInputHandlerPrivate::handleSceneChange(Q3DScene *scene)
{
    if (scene) {
        if (m_graph) {
            QObject::disconnect(m_graph,
                                &QQuickGraphsItem::queriedGraphPositionChanged,
                                this,
                                &Q3DInputHandlerPrivate::handleQueriedGraphPositionChange);
        }

        m_graph = qobject_cast<QQuickGraphsItem *>(scene->parent());
        if (m_graph) {
            QObject::connect(m_graph,
                             &QQuickGraphsItem::queriedGraphPositionChanged,
                             this,
                             &Q3DInputHandlerPrivate::handleQueriedGraphPositionChange);
        }
    }
}

void Q3DInputHandlerPrivate::handleQueriedGraphPositionChange()
{
    if (m_zoomAtTargetPending) {
        // Check if the zoom point is on graph
        QVector3D newTarget = m_graph->queriedGraphPosition();
        float currentZoom = m_requestedZoomLevel;
        float previousZoom = q_ptr->item()->cameraZoomLevel();
        q_ptr->item()->setCameraZoomLevel(currentZoom);
        float diffAdj = 0.0f;

        // If zooming in/out outside the graph, or zooming out after certain point,
        // move towards the center.
        if ((qAbs(newTarget.x()) > 2.0f || qAbs(newTarget.y()) > 2.0f || qAbs(newTarget.z()) > 2.0f)
            || (previousZoom > currentZoom && currentZoom <= driftTowardCenterLevel)) {
            newTarget = zeroVector;
            // Add some extra correction so that we actually reach the center
            // eventually
            diffAdj = m_driftMultiplier;
            if (previousZoom > currentZoom)
                diffAdj *= 2.0f; // Correct towards center little more when zooming out
        }

        float zoomFraction = 1.0f - (previousZoom / currentZoom);

        // Adjust camera towards the zoom point, attempting to keep the cursor at
        // same graph point
        QVector3D oldTarget = q_ptr->item()->cameraTargetPosition();
        QVector3D origDiff = newTarget - oldTarget;
        QVector3D diff = origDiff * zoomFraction + (origDiff.normalized() * diffAdj);
        if (diff.length() > origDiff.length())
            diff = origDiff;
        q_ptr->item()->setCameraTargetPosition(oldTarget + diff);

        if (q_ptr->scene()->selectionQueryPosition() == q_ptr->scene()->invalidSelectionPoint())
            m_zoomAtTargetPending = false;
    }
}

QT_END_NAMESPACE
