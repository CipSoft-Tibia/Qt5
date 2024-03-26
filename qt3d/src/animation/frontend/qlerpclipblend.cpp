// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlerpclipblend.h"
#include "qlerpclipblend_p.h"

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {

/*!
    \qmltype LerpClipBlend
    \instantiates Qt3DAnimation::QLerpClipBlend
    \inqmlmodule Qt3D.Animation

    \brief Performs a linear interpolation of two animation clips based on a
    normalized factor.

    \since 5.9

    LerpClipBlend can be useful to create advanced animation effects based on
    individual animation clips. For instance, given a player character,, lerp
    blending could be used to combine a walking animation clip with an injured
    animation clip based on a blend factor that increases the more the player
    gets injured. This would then allow with blend factor == 0 to have a non
    injured walking player, with blend factor == 1 a fully injured player, with
    blend factor == 0.5 a partially walking and injured player.

    \sa BlendedClipAnimator
*/

/*!
    \class Qt3DAnimation::QLerpClipBlend
    \inmodule Qt3DAnimation
    \inherits Qt3DAnimation::QAbstractClipBlendNode

    \brief Performs a linear interpolation of two animation clips based on a
    normalized factor.

    \since 5.9

    QLerpClipBlend can be useful to create advanced animation effects based on
    individual animation clips. For instance, given a player character, lerp
    blending could be used to combine a walking animation clip with an injured
    animation clip based on a blend factor that increases the more the player
    gets injured. This would then allow with blend factor == 0 to have a non
    injured walking player, with blend factor == 1 a fully injured player, with
    blend factor == 0.5 a partially walking and injured player.

    \sa QBlendedClipAnimator
*/

QLerpClipBlendPrivate::QLerpClipBlendPrivate()
    : QAbstractClipBlendNodePrivate()
    , m_startClip(nullptr)
    , m_endClip(nullptr)
    , m_blendFactor(0.0f)
{
}

QLerpClipBlend::QLerpClipBlend(Qt3DCore::QNode *parent)
    : QAbstractClipBlendNode(*new QLerpClipBlendPrivate(), parent)
{
}

QLerpClipBlend::QLerpClipBlend(QLerpClipBlendPrivate &dd, Qt3DCore::QNode *parent)
    : QAbstractClipBlendNode(dd, parent)
{
}

QLerpClipBlend::~QLerpClipBlend()
{
}

/*!
    \qmlproperty real LerpClipBlend::blendFactor

    Specifies the blending factor between 0 and 1 to control the blending of
    two animation clips.
*/
/*!
    \property QLerpClipBlend::blendFactor

    Specifies the blending factor between 0 and 1 to control the blending of
    two animation clips.
 */
float QLerpClipBlend::blendFactor() const
{
    Q_D(const QLerpClipBlend);
    return d->m_blendFactor;
}

/*!
    \qmlproperty AbstractClipBlendNode LerpClipBlend::startClip

    Holds the sub-tree that should be used as the start clip for this
    lerp blend node. That is, the clip returned by this blend node when
    the blendFactor is set to a value of 0.
*/
/*!
    \property QLerpClipBlend::startClip

    Holds the sub-tree that should be used as the start clip for this
    lerp blend node. That is, the clip returned by this blend node when
    the blendFactor is set to a value of 0.
*/
Qt3DAnimation::QAbstractClipBlendNode *QLerpClipBlend::startClip() const
{
    Q_D(const QLerpClipBlend);
    return d->m_startClip;
}

/*!
    \qmlproperty AbstractClipBlendNode LerpClipBlend::endClip

    Holds the sub-tree that should be used as the end clip for this
    lerp blend node. That is, the clip returned by this blend node when
    the blendFactor is set to a value of 1.
*/
/*!
    \property QLerpClipBlend::endClip

    Holds the sub-tree that should be used as the start clip for this
    lerp blend node. That is, the clip returned by this blend node when
    the blendFactor is set to a value of 1.
*/
Qt3DAnimation::QAbstractClipBlendNode *QLerpClipBlend::endClip() const
{
    Q_D(const QLerpClipBlend);
    return d->m_endClip;
}

void QLerpClipBlend::setBlendFactor(float blendFactor)
{
    Q_D(QLerpClipBlend);

    if (d->m_blendFactor == blendFactor)
        return;

    d->m_blendFactor = blendFactor;
    emit blendFactorChanged(blendFactor);
}

void QLerpClipBlend::setStartClip(Qt3DAnimation::QAbstractClipBlendNode *startClip)
{
    Q_D(QLerpClipBlend);
    if (d->m_startClip == startClip)
        return;

    if (d->m_startClip)
        d->unregisterDestructionHelper(d->m_startClip);

    if (startClip && !startClip->parent())
        startClip->setParent(this);
    d->m_startClip = startClip;

    // Ensures proper bookkeeping
    if (d->m_startClip)
        d->registerDestructionHelper(d->m_startClip, &QLerpClipBlend::setStartClip, d->m_startClip);
    emit startClipChanged(startClip);
}

void QLerpClipBlend::setEndClip(Qt3DAnimation::QAbstractClipBlendNode *endClip)
{
    Q_D(QLerpClipBlend);
    if (d->m_endClip == endClip)
        return;

    if (d->m_endClip)
        d->unregisterDestructionHelper(d->m_endClip);

    if (endClip && !endClip->parent())
        endClip->setParent(this);
    d->m_endClip = endClip;

    // Ensures proper bookkeeping
    if (d->m_endClip)
        d->registerDestructionHelper(d->m_endClip, &QLerpClipBlend::setEndClip, d->m_endClip);
    emit endClipChanged(endClip);
}

} // Qt3DAnimation

QT_END_NAMESPACE

#include "moc_qlerpclipblend.cpp"
