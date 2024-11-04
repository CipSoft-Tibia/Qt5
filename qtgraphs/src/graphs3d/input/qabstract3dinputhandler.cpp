// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qabstract3dinputhandler_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QAbstract3DInputHandler
 * \inmodule QtGraphs
 * \ingroup graphs_3D
 * \brief The base class for implementations of 3D input handlers.
 *
 * QAbstract3DInputHandler is the base class that is subclassed by different
 * input handling implementations that take input events and translate those to
 * camera and light movements. Input handlers also translate raw input events to
 * slicing and selection events in the scene.
 */

/*!
 * \enum QAbstract3DInputHandler::InputView
 *
 * Predefined input views for mouse and touch based input handlers.
 *
 * \value None
 *        Mouse or touch not on a view.
 * \value OnPrimary
 *        Mouse or touch input received on the primary view area. If secondary view is displayed when
 *        inputView becomes OnPrimary, secondary view is closed.
 * \value OnSecondary
 *        Mouse or touch input received on the secondary view area.
 */

/*!
 * \qmltype AbstractInputHandler3D
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml_3D
 * \instantiates QAbstract3DInputHandler
 * \brief Base type for all 3D graph input handlers.
 *
 * This type is uncreatable.
 *
 * For AbstractInputHandler3D enums, see \l{QAbstract3DInputHandler::InputView}.
 */

/*!
 * \internal
 */
QAbstract3DInputHandler::QAbstract3DInputHandler(QAbstract3DInputHandlerPrivate *d, QObject *parent)
    : QObject(parent)
    , d_ptr(d)
{}

/*!
 * Constructs the base class. An optional \a parent parameter can be given
 * and is then passed to QObject constructor.
 */
QAbstract3DInputHandler::QAbstract3DInputHandler(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAbstract3DInputHandlerPrivate(this))
{}

/*!
 *  Destroys the base class.
 */
QAbstract3DInputHandler::~QAbstract3DInputHandler() {}

// Input event listeners
/*!
 * Override this to handle mouse double click events.
 * Mouse double click event is given in the \a event.
 */
void QAbstract3DInputHandler::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

/*!
 * Override this to handle touch input events.
 * Touch event is given in the \a event.
 */
void QAbstract3DInputHandler::touchEvent(QTouchEvent *event)
{
    Q_UNUSED(event);
}

/*!
 * Override this to handle mouse press events.
 * Mouse press event is given in the \a event and the mouse position in \a
 * mousePos.
 */
void QAbstract3DInputHandler::mousePressEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
    Q_UNUSED(mousePos);
}

/*!
 * Override this to handle mouse release events.
 * Mouse release event is given in the \a event and the mouse position in \a
 * mousePos.
 */
void QAbstract3DInputHandler::mouseReleaseEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
    Q_UNUSED(mousePos);
}

/*!
 * Override this to handle mouse move events.
 * Mouse move event is given in the \a event and the mouse position in \a
 * mousePos.
 */
void QAbstract3DInputHandler::mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos)
{
    Q_UNUSED(event);
    Q_UNUSED(mousePos);
}

#if QT_CONFIG(wheelevent)
/*!
 * Override this to handle wheel events.
 * Wheel event is given in the \a event.
 */
void QAbstract3DInputHandler::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event);
}
#endif

// Property get/set
/*!
 * \property QAbstract3DInputHandler::inputView
 *
 * \brief The current enumerated input view based on the view of the processed
 * input events.
 *
 * One of the InputView enum values.
 *
 * When the view changes, the \c inputViewChanged signal is emitted.
 *
 * \sa InputView
 */
QAbstract3DInputHandler::InputView QAbstract3DInputHandler::inputView() const
{
    const Q_D(QAbstract3DInputHandler);
    return d->m_inputView;
}

void QAbstract3DInputHandler::setInputView(QAbstract3DInputHandler::InputView inputView)
{
    Q_D(QAbstract3DInputHandler);
    if (inputView != d->m_inputView) {
        d->m_inputView = inputView;
        emit inputViewChanged(inputView);
    }
}

/*!
 * \property QAbstract3DInputHandler::inputPosition
 *
 * \brief The last input position based on the processed input events.
 */
QPoint QAbstract3DInputHandler::inputPosition() const
{
    const Q_D(QAbstract3DInputHandler);
    return d->m_inputPosition;
}

void QAbstract3DInputHandler::setInputPosition(const QPoint &position, bool forceSelection)
{
    Q_D(QAbstract3DInputHandler);
    if (position != d->m_inputPosition) {
        if (forceSelection)
            d->m_inputState = QAbstract3DInputHandlerPrivate::InputState::Selecting;
        d->m_inputPosition = position;
        emit positionChanged(position);
    }
}

/*!
 * Returns the manhattan length between last two input positions.
 */
int QAbstract3DInputHandler::prevDistance() const
{
    const Q_D(QAbstract3DInputHandler);
    return d->m_prevDistance;
}

/*!
 * Sets the \a distance (manhattan length) between last two input positions.
 */
void QAbstract3DInputHandler::setPrevDistance(int distance)
{
    Q_D(QAbstract3DInputHandler);
    d->m_prevDistance = distance;
}

/*!
 * \property QAbstract3DInputHandler::scene
 *
 * \brief The 3D scene this abstract input handler is controlling.
 *
 * One input handler can control one scene. Setting a scene to an input handler
 * does not transfer the ownership of the scene.
 */
Q3DScene *QAbstract3DInputHandler::scene() const
{
    const Q_D(QAbstract3DInputHandler);
    return d->m_scene;
}

void QAbstract3DInputHandler::setScene(Q3DScene *scene)
{
    Q_D(QAbstract3DInputHandler);
    if (scene != d->m_scene) {
        if (d->m_scene) {
            QObject::disconnect(d->m_scene,
                                &Q3DScene::selectionQueryPositionChanged,
                                this,
                                &QAbstract3DInputHandler::handleSelection);
        }

        QObject::connect(scene,
                         &Q3DScene::selectionQueryPositionChanged,
                         this,
                         &QAbstract3DInputHandler::handleSelection);

        d->m_scene = scene;
        emit sceneChanged(scene);
    }
}

QQuickGraphsItem *QAbstract3DInputHandler::item() const
{
    const Q_D(QAbstract3DInputHandler);
    return d->m_item;
}

void QAbstract3DInputHandler::setItem(QQuickGraphsItem *item)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item = item;
}

/*!
 * Sets the previous input position to the point given by \a position.
 */
void QAbstract3DInputHandler::setPreviousInputPos(const QPoint &position)
{
    Q_D(QAbstract3DInputHandler);
    d->m_previousInputPos = position;
}

/*!
 * Returns the previous input position.
 */
QPoint QAbstract3DInputHandler::previousInputPos() const
{
    const Q_D(QAbstract3DInputHandler);
    return d->m_previousInputPos;
}

float QAbstract3DInputHandler::cameraZoomLevel()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->cameraZoomLevel();
}

void QAbstract3DInputHandler::setCameraZoomLevel(float level)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setCameraZoomLevel(level);
}

float QAbstract3DInputHandler::cameraXRotation()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->cameraXRotation();
}

void QAbstract3DInputHandler::setCameraXRotation(float rotation)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setCameraXRotation(rotation);
}

float QAbstract3DInputHandler::cameraYRotation()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->cameraYRotation();
}

void QAbstract3DInputHandler::setCameraYRotation(float rotation)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setCameraYRotation(rotation);
}

float QAbstract3DInputHandler::minCameraXRotation()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->minCameraXRotation();
}

void QAbstract3DInputHandler::setMinCameraXRotation(float rotation)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setMinCameraXRotation(rotation);
}

float QAbstract3DInputHandler::maxCameraXRotation()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->maxCameraXRotation();
}

void QAbstract3DInputHandler::setMaxCameraXRotation(float rotation)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setMaxCameraXRotation(rotation);
}

float QAbstract3DInputHandler::minCameraYRotation()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->minCameraYRotation();
}

void QAbstract3DInputHandler::setMinCameraYRotation(float rotation)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setMinCameraYRotation(rotation);
}

float QAbstract3DInputHandler::maxCameraYRotation()
{
    Q_D(QAbstract3DInputHandler);
    return d->m_item->maxCameraYRotation();
}

void QAbstract3DInputHandler::setMaxCameraYRotation(float rotation)
{
    Q_D(QAbstract3DInputHandler);
    d->m_item->setMaxCameraYRotation(rotation);
}

/*!
 * Converts an incoming selection query \a position to a selection.
 *
 * \sa Q3DScene::selectionQueryPosition
 */
void QAbstract3DInputHandler::handleSelection(const QPoint &position)
{
    setInputPosition(position, true);
}

QAbstract3DInputHandlerPrivate::QAbstract3DInputHandlerPrivate(QAbstract3DInputHandler *q)
    : m_prevDistance(0)
    , m_previousInputPos(QPoint(0, 0))
    , q_ptr(q)
    , m_inputView(QAbstract3DInputHandler::InputView::None)
    , m_inputPosition(QPoint(0, 0))
    , m_scene(0)
    , m_isDefaultHandler(false)
{}

QAbstract3DInputHandlerPrivate::~QAbstract3DInputHandlerPrivate() {}

QT_END_NAMESPACE
