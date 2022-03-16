/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qframegraphnode.h"
#include "qframegraphnode_p.h"
#include <Qt3DRender/qframegraphnodecreatedchange.h>
#include <Qt3DCore/qpropertyupdatedchange.h>

#include <Qt3DCore/QNode>

#include <QQueue>

using namespace Qt3DCore;

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

QFrameGraphNodePrivate::QFrameGraphNodePrivate()
    : QNodePrivate()
{
}

/*!
    \class Qt3DRender::QFrameGraphNode
    \inmodule Qt3DRender
    \since 5.5

    \brief Base class of all FrameGraph configuration nodes.

    This class is rarely instanced directly since it doesn't provide
    any frame graph specific behavior, although it can be convenient
    to use for grouping other nodes together in dynamic frame graphs.
    The actual behavior comes from the subclasses.

    The subclasses are:
    \table
    \header
        \li class
        \li description
    \row
        \li Qt3DRender::QCameraSelector
        \li Select camera from all available cameras in the scene
    \row
        \li Qt3DRender::QClearBuffers
        \li Specify which buffers to clear and to what values
    \row
        \li Qt3DRender::QDispatchCompute
        \li Specify Compute operation kernels
    \row
        \li Qt3DRender::QFrustumCulling
        \li Enable frustum culling
    \row
        \li Qt3DRender::QLayerFilter
        \li Select which layers to draw
    \row
        \li Qt3DRender::QNoDraw
        \li Disable drawing
    \row
        \li Qt3DRender::QRenderPassFilter
        \li Select which render passes to draw
    \row
        \li Qt3DRender::QRenderStateSet
        \li Set render states
    \row
        \li Qt3DRender::QRenderSurfaceSelector
        \li Select which surface to draw to
    \row
        \li Qt3DRender::QRenderTargetSelector
        \li Select which QRenderTarget to draw to
    \row
        \li Qt3DRender::QSortPolicy
        \li Specify how entities are sorted to determine draw order
    \row
        \li Qt3DRender::QTechniqueFilter
        \li Select which techniques to draw
    \row
        \li Qt3DRender::QViewport
        \li Specify viewport
    \row
        \li Qt3DRender::QMemoryBarrier
        \li Places a memory barrier
    \endtable

 */

/*!
    \qmltype FrameGraphNode
    \inqmlmodule Qt3D.Render
    \instantiates Qt3DRender::QFrameGraphNode
    \inherits Node
    \since 5.5
    \brief Base class of all FrameGraph configuration nodes.

    This class is rarely instanced directly since it doesn't provide
    any frame graph specific behavior, although it can be convenient
    to use for grouping other nodes together in dynamic frame graphs.
    The actual behavior comes from the subclasses.

    The subclasses are:
    \table
    \header
        \li class
        \li description
    \row
        \li CameraSelector
        \li Select camera from all available cameras in the scene
    \row
        \li ClearBuffers
        \li Specify which buffers to clear and to what values
    \row
        \li DispatchCompute
        \li Specify compute operation kernels
    \row
        \li FrustumCulling
        \li Enable frustum culling
    \row
        \li LayerFilter
        \li Select which layers to draw
    \row
        \li NoDraw
        \li Disable drawing
    \row
        \li RenderPassFilter
        \li Select which render passes to draw
    \row
        \li RenderStateSet
        \li Set render states
    \row
        \li RenderSurfaceSelector
        \li Select which surface to draw to
    \row
        \li RenderTargetSelector
        \li Select which RenderTarget to draw to
    \row
        \li SortPolicy
        \li Specify how entities are sorted to determine draw order
    \row
        \li TechniqueFilter
        \li Select which techniques to draw
    \row
        \li Viewport
        \li Specify viewport
    \row
        \li MemoryBarrier
        \li Places a memory barrier
    \endtable
*/

/*!
    The constructor creates an instance with the specified \a parent.
 */
QFrameGraphNode::QFrameGraphNode(QNode *parent)
    : QNode(*new QFrameGraphNodePrivate, parent)
{
}

/*! \internal */
QFrameGraphNode::~QFrameGraphNode()
{
}

/*!
    Returns a pointer to the parent frame graph node.

    If the parent of this node is not a frame graph node,
    this method will recursively look for a parent node that is a frame graph node.
 */
QFrameGraphNode *QFrameGraphNode::parentFrameGraphNode() const
{
    QFrameGraphNode *parentFGNode = nullptr;
    QNode *parentN = parentNode();

    while (parentN) {
        if ((parentFGNode = qobject_cast<QFrameGraphNode *>(parentN)) != nullptr)
            break;
        parentN = parentN->parentNode();
    }
    return parentFGNode;
}

/*!
    \internal
 * Returns a list of the children that are frame graph nodes.
 * If this function encounters a child node that is not a frame graph node,
 * it will go through the children of the child node and look for frame graph nodes.
 * If any of these are not frame graph nodes, they will be further searched as
 * if they were direct children of this node.
 */
QVector<QFrameGraphNode *> QFrameGraphNodePrivate::childFrameGraphNodes() const
{
    Q_Q(const QFrameGraphNode);
    QVector<QFrameGraphNode *> result;
    QQueue<QNode *> queue;
    queue.append(q->childNodes().toList());
    result.reserve(queue.size());
    while (!queue.isEmpty()) {
        auto *child = queue.dequeue();
        auto *childFGNode = qobject_cast<QFrameGraphNode *>(child);
        if (childFGNode != nullptr)
            result.push_back(childFGNode);
        else
            queue.append(child->childNodes().toList());
    }
    return result;
}

/*! \internal */
QFrameGraphNode::QFrameGraphNode(QFrameGraphNodePrivate &dd, QNode *parent)
    : QNode(dd, parent)
{
}

Qt3DCore::QNodeCreatedChangeBasePtr QFrameGraphNode::createNodeCreationChange() const
{
    // connect to the parentChanged signal here rather than constructor because
    // until now there's no backend node to notify when parent changes
    connect(this, &QNode::parentChanged, this, &QFrameGraphNode::onParentChanged);

    return QFrameGraphNodeCreatedChangeBasePtr::create(this);
}

void QFrameGraphNode::onParentChanged(QObject *)
{
    const auto parentID = parentFrameGraphNode() ? parentFrameGraphNode()->id() : Qt3DCore::QNodeId();
    auto parentChange = Qt3DCore::QPropertyUpdatedChangePtr::create(id());
    parentChange->setPropertyName("parentFrameGraphUpdated");
    parentChange->setValue(QVariant::fromValue(parentID));
    const bool blocked = blockNotifications(false);
    notifyObservers(parentChange);
    blockNotifications(blocked);
}

} // namespace Qt3DRender

QT_END_NAMESPACE
