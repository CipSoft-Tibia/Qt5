// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstractclipblendnode.h"
#include "qabstractclipblendnode_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {

QAbstractClipBlendNodePrivate::QAbstractClipBlendNodePrivate()
    : Qt3DCore::QNodePrivate()
{
}

/*!
    \qmltype AbstractClipBlendNode
    \instantiates Qt3DAnimation::QAbstractClipBlendNode
    \inqmlmodule Qt3D.Animation
    \since 5.9

    \brief AbstractClipBlendNode is the base class for types used to construct animation blend
    trees.

    Animation blend trees are used with a BlendedClipAnimator to dynamically blend a set of
    animation clips together. The way in which the blending of animation clips is performed is
    controlled by the structure of the blend tree and the properties on the nodes it contains.

    The leaf nodes in a blend tree are containers for the input animation clips. These clips can be
    baked clips read from file via AnimationClipLoader, or they can be clips that you build within
    your application with AnimatitonClip and AnimationClipData. To include a clip in your blend
    tree, wrap it in a ClipBlendValue node.

    The interior nodes of a blend tree represent blending operations that will be applied to their
    arguments which hold the input clips or even entire sub-trees of other blend tree nodes.

    At present, the Qt 3D Animation module provides the following blend tree node types:

    \list
    \li Qt3D.Animation.ClipBlendValue
    \li Qt3D.Animation.LerpClipBlend
    \li Qt3D.Animation.QAdditiveClipBlend
    \endlist

    Additional node types representing other blending operations will be added in the future.

    \sa BlendedClipAnimator
*/

/*!
    \class Qt3DAnimation::QAbstractClipBlendNode
    \inherits Qt3DCore::QNode

    \inmodule Qt3DAnimation
    \since 5.9

    \brief QAbstractClipBlendNode is the base class for types used to construct animation blend
    trees.

    Animation blend trees are used with a QBlendedClipAnimator to dynamically blend a set of
    animation clips together. The way in which the blending of animation clips is performed is
    controlled by the structure of the blend tree and the properties on the nodes it contains.

    The leaf nodes in a blend tree are containers for the input animation clips. These clips can be
    baked clips read from file via QAnimationClipLoader, or they can be clips that you build within
    your application with QAnimatitonClip and QAnimationClipData. To include a clip in your blend
    tree, wrap it in a QClipBlendValue node.

    The interior nodes of a blend tree represent blending operations that will be applied to their
    arguments which hold the input clips or even entire sub-trees of other blend tree nodes.

    At present, the Qt 3D Animation module provides the following blend tree node types:

    \list
    \li Qt3DAnimation::QClipBlendValue
    \li Qt3DAnimation::QLerpClipBlend
    \li Qt3DAnimation::QAdditiveClipBlend
    \endlist

    Additional node types representing other blending operations will be added in the future.

    \sa QBlendedClipAnimator
*/

/*! \internal */
QAbstractClipBlendNode::QAbstractClipBlendNode(Qt3DCore::QNode *parent)
    : Qt3DCore::QNode(*new QAbstractClipBlendNodePrivate(), parent)
{
}

/*! \internal */
QAbstractClipBlendNode::QAbstractClipBlendNode(QAbstractClipBlendNodePrivate &dd, Qt3DCore::QNode *parent)
    : Qt3DCore::QNode(dd, parent)
{
}

QAbstractClipBlendNode::~QAbstractClipBlendNode()
{
}

} // Qt3DAnimation

QT_END_NAMESPACE

#include "moc_qabstractclipblendnode.cpp"
