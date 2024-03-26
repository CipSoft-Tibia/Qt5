// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemparticle_p.h"
#include <QtQuick/qsgnode.h>
#include <QTimer>
#include <QQmlComponent>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ItemParticle
    \instantiates QQuickItemParticle
    \inqmlmodule QtQuick.Particles
    \inherits ParticlePainter
    \brief For specifying a delegate to paint particles.
    \ingroup qtquick-particles

*/


/*!
    \qmlmethod QtQuick.Particles::ItemParticle::freeze(Item item)

    Suspends the flow of time for the logical particle which \a item represents,
    allowing you to control its movement.
*/

/*!
    \qmlmethod QtQuick.Particles::ItemParticle::unfreeze(Item item)

    Restarts the flow of time for the logical particle which \a item represents,
    allowing it to be moved by the particle system again.
*/

/*!
    \qmlmethod QtQuick.Particles::ItemParticle::take(Item item, bool prioritize)

    Asks the ItemParticle to take over control of \a item positioning temporarily.
    It will follow the movement of a logical particle when one is available.

    By default items form a queue when waiting for a logical particle, but if
    \a prioritize is \c true, then it will go immediately to the head of the
    queue.

    ItemParticle does not take ownership of the item, and will relinquish
    control when the logical particle expires. Commonly at this point you will
    want to put it back in the queue, you can do this with the below line in
    the delegate definition:

    \code
    ItemParticle.onDetached: itemParticleInstance.take(delegateRootItem);
    \endcode

    or delete it, such as with the below line in the delegate definition:

    \code
    ItemParticle.onDetached: delegateRootItem.destroy();
    \endcode
*/

/*!
    \qmlmethod QtQuick.Particles::ItemParticle::give(Item item)

    Orders the ItemParticle to give you control of the \a item. It will cease
    controlling it and the item will lose its association to the logical
    particle.
*/

/*!
    \qmlproperty bool QtQuick.Particles::ItemParticle::fade

    If true, the item will automatically be faded in and out
    at the ends of its lifetime. If false, you will have to
    implement any entry effect yourself.

    Default is true.
*/
/*!
    \qmlproperty Component QtQuick.Particles::ItemParticle::delegate

    An instance of the delegate will be created for every logical particle, and
    moved along with it. As an alternative to using delegate, you can create
    Item instances yourself and hand them to the ItemParticle to move using the
    \l take method.

    Any delegate instances created by ItemParticle will be destroyed when
    the logical particle expires.
*/

QQuickItemParticle::QQuickItemParticle(QQuickItem *parent) :
    QQuickParticlePainter(parent), m_fade(true), m_lastT(0), m_activeCount(0), m_delegate(nullptr)
{
    setFlag(QQuickItem::ItemHasContents);
    clock = new Clock(this);
    connect(this, &QQuickItemParticle::systemChanged, this, &QQuickItemParticle::reconnectSystem);
    connect(this, &QQuickItemParticle::parentChanged, this, &QQuickItemParticle::reconnectParent);
    connect(this, &QQuickItemParticle::enabledChanged, this, &QQuickItemParticle::updateClock);
    reconnectSystem(m_system);
    reconnectParent(parent);
}

QQuickItemParticle::~QQuickItemParticle()
{
    delete clock;
    qDeleteAll(m_managed);
}

void QQuickItemParticle::freeze(QQuickItem* item)
{
    m_stasis << item;
}


void QQuickItemParticle::unfreeze(QQuickItem* item)
{
    m_stasis.remove(item);
}

void QQuickItemParticle::take(QQuickItem *item, bool prioritize)
{
    if (prioritize)
        m_pendingItems.push_front(item);
    else
        m_pendingItems.push_back(item);
}

void QQuickItemParticle::give(QQuickItem *item)
{
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* data : std::as_const(m_system->groupData[groupId]->data)) {
            if (data->delegate == item){
                m_deletables << item;
                data->delegate = nullptr;
                m_system->groupData[groupId]->kill(data);
                return;
            }
        }
    }
}

void QQuickItemParticle::initialize(int gIdx, int pIdx)
{
    Q_UNUSED(gIdx);
    Q_UNUSED(pIdx);
}

void QQuickItemParticle::commit(int, int)
{
}

void QQuickItemParticle::processDeletables()
{
    foreach (QQuickItem* item, m_deletables){
        if (m_fade)
            item->setOpacity(0.);
        item->setVisible(false);
        QQuickItemParticleAttached* mpa;
        if ((mpa = qobject_cast<QQuickItemParticleAttached*>(qmlAttachedPropertiesObject<QQuickItemParticle>(item)))) {
            if (mpa->m_parentItem != nullptr)
                item->setParentItem(mpa->m_parentItem);
            mpa->detach();
        }
        int idx = -1;
        if ((idx = m_managed.indexOf(item)) != -1) {
            m_managed.takeAt(idx);
            delete item;
        }
        m_activeCount--;
    }
    m_deletables.clear();
}

void QQuickItemParticle::tick(int time)
{
    Q_UNUSED(time);//only needed because QTickAnimationProxy expects one
    processDeletables();
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* d : std::as_const(m_system->groupData[groupId]->data)) {
            if (!d->delegate && d->t != -1 && d->stillAlive(m_system)) {
                QQuickItem* parentItem = nullptr;
                if (!m_pendingItems.isEmpty()){
                    QQuickItem *item = m_pendingItems.front();
                    m_pendingItems.pop_front();
                    parentItem = item->parentItem();
                    d->delegate = item;
                }else if (m_delegate){
                    d->delegate = qobject_cast<QQuickItem*>(m_delegate->create(qmlContext(this)));
                    if (d->delegate)
                        m_managed << d->delegate;
                }
                if (d && d->delegate){//###Data can be zero if creating an item leads to a reset - this screws things up.
                    d->delegate->setX(d->curX(m_system) - d->delegate->width() / 2); //TODO: adjust for system?
                    d->delegate->setY(d->curY(m_system) - d->delegate->height() / 2);
                    QQuickItemParticleAttached* mpa = qobject_cast<QQuickItemParticleAttached*>(qmlAttachedPropertiesObject<QQuickItemParticle>(d->delegate));
                    if (mpa){
                        mpa->m_parentItem = parentItem;
                        mpa->m_mp = this;
                        mpa->attach();
                    }
                    d->delegate->setParentItem(this);
                    if (m_fade)
                        d->delegate->setOpacity(0.);
                    d->delegate->setVisible(false);//Will be set to true when we prepare the next frame
                    m_activeCount++;
                }
            }
        }
    }
}

void QQuickItemParticle::reset()
{
    QQuickParticlePainter::reset();

    // delete all managed items which had their logical particles cleared
    // but leave it alone if the logical particle is maintained
    QSet<QQuickItem*> lost = QSet<QQuickItem*>(m_managed.cbegin(), m_managed.cend());
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* d : std::as_const(m_system->groupData[groupId]->data)) {
            lost.remove(d->delegate);
        }
    }
    m_deletables.unite(lost);
    //TODO: This doesn't yet handle calling detach on taken particles in the system reset case
    processDeletables();
}


QSGNode* QQuickItemParticle::updatePaintNode(QSGNode* n, UpdatePaintNodeData* d)
{
    //Dummy update just to get painting tick
    if (m_pleaseReset)
        m_pleaseReset = false;

    if (clockShouldUpdate()) {
        prepareNextFrame();
        update(); //Get called again
    }
    if (n)
        n->markDirty(QSGNode::DirtyMaterial);
    return QQuickItem::updatePaintNode(n,d);
}

void QQuickItemParticle::prepareNextFrame()
{
    if (!m_system)
        return;
    qint64 timeStamp = m_system->systemSync(this);
    qreal curT = timeStamp/1000.0;
    qreal dt = curT - m_lastT;
    m_lastT = curT;
    if (!m_activeCount)
        return;

    //TODO: Size, better fade?
    for (auto groupId : groupIds()) {
        for (QQuickParticleData* data : std::as_const(m_system->groupData[groupId]->data)) {
            QQuickItem* item = data->delegate;
            if (!item)
                continue;
            float t = ((timeStamp / 1000.0f) - data->t) / data->lifeSpan;
            if (m_stasis.contains(item)) {
                data->t += dt;//Stasis effect
                continue;
            }
            if (t >= 1.0f){//Usually happens from load
                m_deletables << item;
                data->delegate = nullptr;
            }else{//Fade
                data->delegate->setVisible(true);
                if (m_fade){
                    float o = 1.f;
                    if (t <0.2f)
                        o = t * 5;
                    if (t > 0.8f)
                        o = (1-t)*5;
                    item->setOpacity(o);
                }
            }
            item->setX(data->curX(m_system) - item->width() / 2 - m_systemOffset.x());
            item->setY(data->curY(m_system) - item->height() / 2 - m_systemOffset.y());
        }
    }
}

QQuickItemParticleAttached *QQuickItemParticle::qmlAttachedProperties(QObject *object)
{
    return new QQuickItemParticleAttached(object);
}

bool QQuickItemParticle::clockShouldUpdate() const
{
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    return (m_system && m_system->isRunning() && !m_system->isPaused() && m_system->isEnabled()
            && ((parentItem && parentItem->isEnabled()) || !parentItem) && isEnabled());
}

void QQuickItemParticle::reconnectParent(QQuickItem *parentItem)
{
    updateClock();
    disconnect(m_parentEnabledStateConnection);
    if (parentItem) {
        m_parentEnabledStateConnection = connect(parentItem, &QQuickParticleSystem::enabledChanged,
                                                 this, &QQuickItemParticle::updateClock);
    }
}

void QQuickItemParticle::reconnectSystem(QQuickParticleSystem *system)
{
    updateClock();
    disconnect(m_systemRunStateConnection);
    disconnect(m_systemPauseStateConnection);
    disconnect(m_systemEnabledStateConnection);
    if (system) {
        m_systemRunStateConnection = connect(m_system, &QQuickParticleSystem::runningChanged, this, [this](){
            QQuickItemParticle::updateClock();
        });
        m_systemPauseStateConnection = connect(m_system, &QQuickParticleSystem::pausedChanged, this, [this](){
            QQuickItemParticle::updateClock();
        });
        m_systemEnabledStateConnection = connect(m_system, &QQuickParticleSystem::enabledChanged, this,
                                                 &QQuickItemParticle::updateClock);
    }
}

void QQuickItemParticle::updateClock()
{
    if (clockShouldUpdate()) {
        if (!clock->isRunning())
            clock->start();
    } else {
        if (clock->isRunning())
            clock->pause();
    }
}

QT_END_NAMESPACE

#include "moc_qquickitemparticle_p.cpp"
