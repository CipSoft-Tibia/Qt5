/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "qrenderstateset.h"
#include "qrenderstateset_p.h"

#include <Qt3DRender/qrenderstate.h>
#include <Qt3DCore/qpropertyupdatedchange.h>
#include <Qt3DCore/qpropertynodeaddedchange.h>
#include <Qt3DCore/qpropertynoderemovedchange.h>
#include <Qt3DRender/qframegraphnodecreatedchange.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {

QRenderStateSetPrivate::QRenderStateSetPrivate()
    : QFrameGraphNodePrivate()
{
}

/*!
    \class Qt3DRender::QRenderStateSet
    \inmodule Qt3DRender
    \since 5.5
    \brief The QRenderStateSet \l {QFrameGraphNode}{FrameGraph} node offers a way of
    specifying a set of QRenderState objects to be applied during the execution
    of a framegraph branch.

    States set on a QRenderStateSet are set globally, contrary to the per-material
    states that can be set on a QRenderPass. By default, an empty
    QRenderStateSet will result in all render states being disabled when
    executed. Adding a QRenderState state explicitly enables that render
    state at runtime.

    The RenderStateSet is enabled when added to the active frame graph:

    \code
    // using namespace Qt3DRender;

    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();

    QRenderSettings *renderSettings = new QRenderSettings();

    QViewport *viewport = new QViewport();
    QCameraSelector *cameraSelector = new QCameraSelector(viewport);

    QClearBuffers *clearBuffers = new QClearBuffers(cameraSelector);
    clearBuffers->setBuffers(QClearBuffers::ColorDepthBuffer);

    QRenderStateSet *renderStateSet = new QRenderStateSet(cameraSelector);
    QCullFace *cullFace = new QCullFace(renderStateSet);
    cullFace->setMode(QCullFace::Front);
    renderStateSet->addRenderState(cullFace);

    renderSettings->setActiveFrameGraph(viewport);

    rootEntity->addComponent(renderSettings);
    \endcode

    \sa QRenderState, QRenderPass
 */

/*!
    \qmltype RenderStateSet
    \inqmlmodule Qt3D.Render
    \since 5.5
    \inherits FrameGraphNode
    \instantiates Qt3DRender::QRenderStateSet
    \brief The RenderStateSet \l {FrameGraphNode}{FrameGraph} node offers a way of
    specifying a set of RenderState objects to be applied during the execution
    of a framegraph branch.

    States set on a RenderStateSet are set globally, contrary to the per-material
    states that can be set on a RenderPass. By default, an empty
    RenderStateSet will result in all render states being disabled when
    executed. Adding a RenderState state explicitly enables that render
    state at runtime.

    The RenderStateSet is enabled when added to the active frame graph:

    \qml
    import Qt3D.Core 2.0
    import Qt3D.Render 2.0
    import Qt3D.Extras 2.0

    Entity {
        id: rootNode
        components: [
            RenderSettings {
                activeFrameGraph: RenderSurfaceSelector {
                    ClearBuffers {
                        buffers : ClearBuffers.ColorDepthBuffer

                        CameraSelector {
                            camera: Camera {
                                position: Qt.vector3d(10, 0, 0)
                                viewCenter: Qt.vector3d(0, 0, 0)
                            }
                            RenderStateSet {
                                renderStates: [
                                    CullFace { mode: CullFace.Back }
                                ]
                            }
                        }
                    }
                }
            }
        ]

        Entity {
            id: sphereEntity
            components: [
                SphereMesh {},
                PhongMaterial {}
            ]
        }
    }
    \endqml

    \sa RenderState, RenderPass
 */

/*!
    \qmlproperty list<RenderState> RenderStateSet::renderStates
    Holds the list of RenderState objects used by the RenderStateSet.
*/

QRenderStateSet::QRenderStateSet(QNode *parent)
    : QFrameGraphNode(*new QRenderStateSetPrivate, parent)
{
}

/*! \internal */
QRenderStateSet::~QRenderStateSet()
{
}

/*!
    Adds a new QRenderState \a state to the QRenderStateSet instance.

    \note Not setting any QRenderState state on a QRenderStateSet instance
    implies all the render states will be disabled at render time.
 */
void QRenderStateSet::addRenderState(QRenderState *state)
{
    Q_ASSERT(state);
    Q_D(QRenderStateSet);

    if (!d->m_renderStates.contains(state)) {
        d->m_renderStates.append(state);

        // Ensures proper bookkeeping
        d->registerDestructionHelper(state, &QRenderStateSet::removeRenderState, d->m_renderStates);

        if (!state->parent())
            state->setParent(this);

        if (d->m_changeArbiter != nullptr) {
            const auto change = QPropertyNodeAddedChangePtr::create(id(), state);
            change->setPropertyName("renderState");
            d->notifyObservers(change);
        }
    }
}

/*!
    Removes the QRenderState \a state from the QRenderStateSet instance.
 */
void QRenderStateSet::removeRenderState(QRenderState *state)
{
    Q_ASSERT(state);
    Q_D(QRenderStateSet);

    if (d->m_changeArbiter != nullptr) {
        const auto change = QPropertyNodeRemovedChangePtr::create(id(), state);
        change->setPropertyName("renderState");
        d->notifyObservers(change);
    }
    d->m_renderStates.removeOne(state);
    // Remove bookkeeping connection
    d->unregisterDestructionHelper(state);
}

/*!
    Returns the list of QRenderState objects that compose the QRenderStateSet instance.
 */
QVector<QRenderState *> QRenderStateSet::renderStates() const
{
    Q_D(const QRenderStateSet);
    return d->m_renderStates;
}

Qt3DCore::QNodeCreatedChangeBasePtr QRenderStateSet::createNodeCreationChange() const
{
    auto creationChange = QFrameGraphNodeCreatedChangePtr<QRenderStateSetData>::create(this);
    auto &data = creationChange->data;
    Q_D(const QRenderStateSet);
    data.renderStateIds = qIdsForNodes(d->m_renderStates);
    return creationChange;
}

} // namespace Qt3DRender

QT_END_NAMESPACE
