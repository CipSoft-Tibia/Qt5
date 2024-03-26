// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemview_p_p.h"
#include "qquickitemviewfxitem_p_p.h"
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQml/QQmlInfo>
#include "qplatformdefs.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcItemViewDelegateLifecycle, "qt.quick.itemview.lifecycle")

// Default cacheBuffer for all views.
#ifndef QML_VIEW_DEFAULTCACHEBUFFER
#define QML_VIEW_DEFAULTCACHEBUFFER 320
#endif

FxViewItem::FxViewItem(QQuickItem *i, QQuickItemView *v, bool own, QQuickItemViewAttached *attached)
    : QQuickItemViewFxItem(i, own, QQuickItemViewPrivate::get(v))
    , view(v)
    , attached(attached)
{
}

QQuickItemViewChangeSet::QQuickItemViewChangeSet()
    : active(false)
{
    reset();
}

bool QQuickItemViewChangeSet::hasPendingChanges() const
{
    return !pendingChanges.isEmpty();
}

void QQuickItemViewChangeSet::applyChanges(const QQmlChangeSet &changeSet)
{
    pendingChanges.apply(changeSet);

    int moveId = -1;
    int moveOffset = 0;

    for (const QQmlChangeSet::Change &r : changeSet.removes()) {
        itemCount -= r.count;
        if (moveId == -1 && newCurrentIndex >= r.index + r.count) {
            newCurrentIndex -= r.count;
            currentChanged = true;
        } else if (moveId == -1 && newCurrentIndex >= r.index && newCurrentIndex < r.index + r.count) {
            // current item has been removed.
            if (r.isMove()) {
                moveId = r.moveId;
                moveOffset = newCurrentIndex - r.index;
            } else {
                currentRemoved = true;
                newCurrentIndex = -1;
                if (itemCount)
                    newCurrentIndex = qMin(r.index, itemCount - 1);
            }
            currentChanged = true;
        }
    }
    for (const QQmlChangeSet::Change &i : changeSet.inserts()) {
        if (moveId == -1) {
            if (itemCount && newCurrentIndex >= i.index) {
                newCurrentIndex += i.count;
                currentChanged = true;
            } else if (newCurrentIndex < 0) {
                newCurrentIndex = 0;
                currentChanged = true;
            } else if (newCurrentIndex == 0 && !itemCount) {
                // this is the first item, set the initial current index
                currentChanged = true;
            }
        } else if (moveId == i.moveId) {
            newCurrentIndex = i.index + moveOffset;
        }
        itemCount += i.count;
    }
}

void QQuickItemViewChangeSet::applyBufferedChanges(const QQuickItemViewChangeSet &other)
{
    if (!other.hasPendingChanges())
        return;

    pendingChanges.apply(other.pendingChanges);
    itemCount = other.itemCount;
    newCurrentIndex = other.newCurrentIndex;
    currentChanged = other.currentChanged;
    currentRemoved = other.currentRemoved;
}

void QQuickItemViewChangeSet::prepare(int currentIndex, int count)
{
    if (active)
        return;
    reset();
    active = true;
    itemCount = count;
    newCurrentIndex = currentIndex;
}

void QQuickItemViewChangeSet::reset()
{
    itemCount = 0;
    newCurrentIndex = -1;
    pendingChanges.clear();
    removedItems.clear();
    active = false;
    currentChanged = false;
    currentRemoved = false;
}

//-----------------------------------

QQuickItemView::QQuickItemView(QQuickFlickablePrivate &dd, QQuickItem *parent)
    : QQuickFlickable(dd, parent)
{
    Q_D(QQuickItemView);
    d->init();
}

QQuickItemView::~QQuickItemView()
{
    Q_D(QQuickItemView);
    d->clear(true);
    if (d->ownModel)
        delete d->model;
    delete d->header;
    delete d->footer;
}


QQuickItem *QQuickItemView::currentItem() const
{
    Q_D(const QQuickItemView);
    return d->currentItem ? d->currentItem->item : nullptr;
}

QVariant QQuickItemView::model() const
{
    Q_D(const QQuickItemView);
    return d->modelVariant;
}

void QQuickItemView::setModel(const QVariant &m)
{
    Q_D(QQuickItemView);
    QVariant model = m;
    if (model.userType() == qMetaTypeId<QJSValue>())
        model = model.value<QJSValue>().toVariant();

    if (d->modelVariant == model)
        return;
    if (d->model) {
        disconnect(d->model, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                this, SLOT(modelUpdated(QQmlChangeSet,bool)));
        disconnect(d->model, SIGNAL(initItem(int,QObject*)), this, SLOT(initItem(int,QObject*)));
        disconnect(d->model, SIGNAL(createdItem(int,QObject*)), this, SLOT(createdItem(int,QObject*)));
        disconnect(d->model, SIGNAL(destroyingItem(QObject*)), this, SLOT(destroyingItem(QObject*)));
        if (QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel*>(d->model)) {
            disconnect(delegateModel, SIGNAL(itemPooled(int,QObject*)), this, SLOT(onItemPooled(int,QObject*)));
            disconnect(delegateModel, SIGNAL(itemReused(int,QObject*)), this, SLOT(onItemReused(int,QObject*)));
        }
    }

    QQmlInstanceModel *oldModel = d->model;

    d->clear();
    d->model = nullptr;
    d->setPosition(d->contentStartOffset());
    d->modelVariant = model;

    QObject *object = qvariant_cast<QObject*>(model);
    QQmlInstanceModel *vim = nullptr;
    if (object && (vim = qobject_cast<QQmlInstanceModel *>(object))) {
        if (d->ownModel) {
            delete oldModel;
            d->ownModel = false;
        }
        d->model = vim;
    } else {
        if (!d->ownModel) {
            d->model = new QQmlDelegateModel(qmlContext(this), this);
            d->ownModel = true;
            if (isComponentComplete())
                static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();
        } else {
            d->model = oldModel;
        }
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model))
            dataModel->setModel(model);
    }

    if (d->model) {
        d->bufferMode = QQuickItemViewPrivate::BufferBefore | QQuickItemViewPrivate::BufferAfter;
        connect(d->model, SIGNAL(createdItem(int,QObject*)), this, SLOT(createdItem(int,QObject*)));
        connect(d->model, SIGNAL(initItem(int,QObject*)), this, SLOT(initItem(int,QObject*)));
        connect(d->model, SIGNAL(destroyingItem(QObject*)), this, SLOT(destroyingItem(QObject*)));
        if (QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel*>(d->model)) {
            connect(delegateModel, SIGNAL(itemPooled(int,QObject*)), this, SLOT(onItemPooled(int,QObject*)));
            connect(delegateModel, SIGNAL(itemReused(int,QObject*)), this, SLOT(onItemReused(int,QObject*)));
        }
        if (isComponentComplete()) {
            d->updateSectionCriteria();
            d->refill();
            /* Setting currentIndex to -2 ensures that we always enter the "currentIndex changed"
               code path in setCurrentIndex, updating bindings depending on currentIndex.*/
            d->currentIndex = -2;
            setCurrentIndex(d->model->count() > 0 ? 0 : -1);
            d->updateViewport();

#if QT_CONFIG(quick_viewtransitions)
            if (d->transitioner && d->transitioner->populateTransition) {
                d->transitioner->setPopulateTransitionEnabled(true);
                d->forceLayoutPolish();
            }
#endif
        }

        connect(d->model, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                this, SLOT(modelUpdated(QQmlChangeSet,bool)));
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model))
            QObjectPrivate::connect(dataModel, &QQmlDelegateModel::delegateChanged, d, &QQuickItemViewPrivate::applyDelegateChange);
        emit countChanged();
    }
    emit modelChanged();
    d->moveReason = QQuickItemViewPrivate::Other;
}

QQmlComponent *QQuickItemView::delegate() const
{
    Q_D(const QQuickItemView);
    if (d->model) {
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model))
            return dataModel->delegate();
    }

    return nullptr;
}

void QQuickItemView::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickItemView);
    if (delegate == this->delegate())
        return;
    if (!d->ownModel) {
        d->model = new QQmlDelegateModel(qmlContext(this));
        d->ownModel = true;
        if (isComponentComplete())
            static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();
    }
    if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(d->model)) {
        int oldCount = dataModel->count();
        dataModel->setDelegate(delegate);
        if (oldCount != dataModel->count())
            emit countChanged();
    }
    emit delegateChanged();
    d->delegateValidated = false;
}


int QQuickItemView::count() const
{
    Q_D(const QQuickItemView);
    if (!d->model)
        return 0;
    return d->model->count();
}

int QQuickItemView::currentIndex() const
{
    Q_D(const QQuickItemView);
    return d->currentIndex;
}

void QQuickItemView::setCurrentIndex(int index)
{
    Q_D(QQuickItemView);
    if (d->inRequest)  // currently creating item
        return;
    d->currentIndexCleared = (index == -1);

    d->applyPendingChanges();
    if (index == d->currentIndex)
        return;
    if (isComponentComplete() && d->isValid()) {
        d->moveReason = QQuickItemViewPrivate::SetIndex;
        d->updateCurrent(index);
    } else if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}


bool QQuickItemView::isWrapEnabled() const
{
    Q_D(const QQuickItemView);
    return d->wrap;
}

void QQuickItemView::setWrapEnabled(bool wrap)
{
    Q_D(QQuickItemView);
    if (d->wrap == wrap)
        return;
    d->wrap = wrap;
    emit keyNavigationWrapsChanged();
}

bool QQuickItemView::isKeyNavigationEnabled() const
{
    Q_D(const QQuickItemView);
    return d->explicitKeyNavigationEnabled ? d->keyNavigationEnabled : d->interactive;
}

void QQuickItemView::setKeyNavigationEnabled(bool keyNavigationEnabled)
{
    // TODO: default binding to "interactive" can be removed in Qt 6; it only exists for compatibility reasons.
    Q_D(QQuickItemView);
    const bool wasImplicit = !d->explicitKeyNavigationEnabled;
    if (wasImplicit)
        QObject::disconnect(this, &QQuickFlickable::interactiveChanged, this, &QQuickItemView::keyNavigationEnabledChanged);

    d->explicitKeyNavigationEnabled = true;

    // Ensure that we emit the change signal in case there is no different in value.
    if (d->keyNavigationEnabled != keyNavigationEnabled || wasImplicit) {
        d->keyNavigationEnabled = keyNavigationEnabled;
        emit keyNavigationEnabledChanged();
    }
}

int QQuickItemView::cacheBuffer() const
{
    Q_D(const QQuickItemView);
    return d->buffer;
}

void QQuickItemView::setCacheBuffer(int b)
{
    Q_D(QQuickItemView);
    if (b < 0) {
        qmlWarning(this) << "Cannot set a negative cache buffer";
        return;
    }

    if (d->buffer != b) {
        d->buffer = b;
        if (isComponentComplete()) {
            d->bufferMode = QQuickItemViewPrivate::BufferBefore | QQuickItemViewPrivate::BufferAfter;
            d->refillOrLayout();
        }
        emit cacheBufferChanged();
    }
}

int QQuickItemView::displayMarginBeginning() const
{
    Q_D(const QQuickItemView);
    return d->displayMarginBeginning;
}

void QQuickItemView::setDisplayMarginBeginning(int margin)
{
    Q_D(QQuickItemView);
    if (d->displayMarginBeginning != margin) {
        d->displayMarginBeginning = margin;
        if (isComponentComplete()) {
            d->forceLayoutPolish();
        }
        emit displayMarginBeginningChanged();
    }
}

int QQuickItemView::displayMarginEnd() const
{
    Q_D(const QQuickItemView);
    return d->displayMarginEnd;
}

void QQuickItemView::setDisplayMarginEnd(int margin)
{
    Q_D(QQuickItemView);
    if (d->displayMarginEnd != margin) {
        d->displayMarginEnd = margin;
        if (isComponentComplete()) {
            d->forceLayoutPolish();
        }
        emit displayMarginEndChanged();
    }
}

Qt::LayoutDirection QQuickItemView::layoutDirection() const
{
    Q_D(const QQuickItemView);
    return d->layoutDirection;
}

void QQuickItemView::setLayoutDirection(Qt::LayoutDirection layoutDirection)
{
    Q_D(QQuickItemView);
    if (d->layoutDirection != layoutDirection) {
        d->layoutDirection = layoutDirection;
        d->regenerate();
        emit layoutDirectionChanged();
        emit effectiveLayoutDirectionChanged();
    }
}

Qt::LayoutDirection QQuickItemView::effectiveLayoutDirection() const
{
    Q_D(const QQuickItemView);
    if (d->effectiveLayoutMirror)
        return d->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
    else
        return d->layoutDirection;
}

QQuickItemView::VerticalLayoutDirection QQuickItemView::verticalLayoutDirection() const
{
    Q_D(const QQuickItemView);
    return d->verticalLayoutDirection;
}

void QQuickItemView::setVerticalLayoutDirection(VerticalLayoutDirection layoutDirection)
{
    Q_D(QQuickItemView);
    if (d->verticalLayoutDirection != layoutDirection) {
        d->verticalLayoutDirection = layoutDirection;
        d->regenerate();
        emit verticalLayoutDirectionChanged();
    }
}

QQmlComponent *QQuickItemView::header() const
{
    Q_D(const QQuickItemView);
    return d->headerComponent;
}

QQuickItem *QQuickItemView::headerItem() const
{
    Q_D(const QQuickItemView);
    return d->header ? d->header->item : nullptr;
}

void QQuickItemView::setHeader(QQmlComponent *headerComponent)
{
    Q_D(QQuickItemView);
    if (d->headerComponent != headerComponent) {
        d->applyPendingChanges();
        delete d->header;
        d->header = nullptr;
        d->headerComponent = headerComponent;

        d->markExtentsDirty();

        if (isComponentComplete()) {
            d->updateHeader();
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        } else {
            emit headerItemChanged();
        }
        emit headerChanged();
    }
}

QQmlComponent *QQuickItemView::footer() const
{
    Q_D(const QQuickItemView);
    return d->footerComponent;
}

QQuickItem *QQuickItemView::footerItem() const
{
    Q_D(const QQuickItemView);
    return d->footer ? d->footer->item : nullptr;
}

void QQuickItemView::setFooter(QQmlComponent *footerComponent)
{
    Q_D(QQuickItemView);
    if (d->footerComponent != footerComponent) {
        d->applyPendingChanges();
        delete d->footer;
        d->footer = nullptr;
        d->footerComponent = footerComponent;

        if (isComponentComplete()) {
            d->updateFooter();
            d->updateViewport();
            d->fixupPosition();
        } else {
            emit footerItemChanged();
        }
        emit footerChanged();
    }
}

QQmlComponent *QQuickItemView::highlight() const
{
    Q_D(const QQuickItemView);
    return d->highlightComponent;
}

void QQuickItemView::setHighlight(QQmlComponent *highlightComponent)
{
    Q_D(QQuickItemView);
    if (highlightComponent != d->highlightComponent) {
        d->applyPendingChanges();
        d->highlightComponent = highlightComponent;
        d->createHighlight();
        if (d->currentItem)
            d->updateHighlight();
        emit highlightChanged();
    }
}

QQuickItem *QQuickItemView::highlightItem() const
{
    Q_D(const QQuickItemView);
    return d->highlight ? d->highlight->item : nullptr;
}

bool QQuickItemView::highlightFollowsCurrentItem() const
{
    Q_D(const QQuickItemView);
    return d->autoHighlight;
}

void QQuickItemView::setHighlightFollowsCurrentItem(bool autoHighlight)
{
    Q_D(QQuickItemView);
    if (d->autoHighlight != autoHighlight) {
        d->autoHighlight = autoHighlight;
        if (autoHighlight)
            d->updateHighlight();
        emit highlightFollowsCurrentItemChanged();
    }
}

QQuickItemView::HighlightRangeMode QQuickItemView::highlightRangeMode() const
{
    Q_D(const QQuickItemView);
    return static_cast<QQuickItemView::HighlightRangeMode>(d->highlightRange);
}

void QQuickItemView::setHighlightRangeMode(HighlightRangeMode mode)
{
    Q_D(QQuickItemView);
    if (d->highlightRange == mode)
        return;
    d->highlightRange = mode;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    if (isComponentComplete()) {
        d->updateViewport();
        d->moveReason = QQuickItemViewPrivate::Other;
        d->fixupPosition();
    }
    emit highlightRangeModeChanged();
}

//###Possibly rename these properties, since they are very useful even without a highlight?
qreal QQuickItemView::preferredHighlightBegin() const
{
    Q_D(const QQuickItemView);
    return d->highlightRangeStart;
}

void QQuickItemView::setPreferredHighlightBegin(qreal start)
{
    Q_D(QQuickItemView);
    d->highlightRangeStartValid = true;
    if (d->highlightRangeStart == start)
        return;
    d->highlightRangeStart = start;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    if (isComponentComplete()) {
        d->updateViewport();
        if (!isMoving() && !isFlicking()) {
            d->moveReason = QQuickItemViewPrivate::Other;
            d->fixupPosition();
        }
    }
    emit preferredHighlightBeginChanged();
}

void QQuickItemView::resetPreferredHighlightBegin()
{
    Q_D(QQuickItemView);
    d->highlightRangeStartValid = false;
    if (d->highlightRangeStart == 0)
        return;
    d->highlightRangeStart = 0;
    if (isComponentComplete()) {
        d->updateViewport();
        if (!isMoving() && !isFlicking()) {
            d->moveReason = QQuickItemViewPrivate::Other;
            d->fixupPosition();
        }
    }
    emit preferredHighlightBeginChanged();
}

qreal QQuickItemView::preferredHighlightEnd() const
{
    Q_D(const QQuickItemView);
    return d->highlightRangeEnd;
}

void QQuickItemView::setPreferredHighlightEnd(qreal end)
{
    Q_D(QQuickItemView);
    d->highlightRangeEndValid = true;
    if (d->highlightRangeEnd == end)
        return;
    d->highlightRangeEnd = end;
    d->haveHighlightRange = d->highlightRange != NoHighlightRange && d->highlightRangeStart <= d->highlightRangeEnd;
    if (isComponentComplete()) {
        d->updateViewport();
        if (!isMoving() && !isFlicking()) {
            d->moveReason = QQuickItemViewPrivate::Other;
            d->fixupPosition();
        }
    }
    emit preferredHighlightEndChanged();
}

void QQuickItemView::resetPreferredHighlightEnd()
{
    Q_D(QQuickItemView);
    d->highlightRangeEndValid = false;
    if (d->highlightRangeEnd == 0)
        return;
    d->highlightRangeEnd = 0;
    if (isComponentComplete()) {
        d->updateViewport();
        if (!isMoving() && !isFlicking()) {
            d->moveReason = QQuickItemViewPrivate::Other;
            d->fixupPosition();
        }
    }
    emit preferredHighlightEndChanged();
}

int QQuickItemView::highlightMoveDuration() const
{
    Q_D(const QQuickItemView);
    return d->highlightMoveDuration;
}

void QQuickItemView::setHighlightMoveDuration(int duration)
{
    Q_D(QQuickItemView);
    if (d->highlightMoveDuration != duration) {
        d->highlightMoveDuration = duration;
        emit highlightMoveDurationChanged();
    }
}

bool QQuickItemView::reuseItems() const
{
    return bool(d_func()->reusableFlag == QQmlDelegateModel::Reusable);
}

void QQuickItemView::setReuseItems(bool reuse)
{
    Q_D(QQuickItemView);
    if (reuseItems() == reuse)
        return;

    d->reusableFlag = reuse ? QQmlDelegateModel::Reusable : QQmlDelegateModel::NotReusable;

    if (!reuse && d->model) {
        // When we're told to not reuse items, we
        // immediately, as documented, drain the pool.
        d->model->drainReusableItemsPool(0);
    }

    emit reuseItemsChanged();
}

#if QT_CONFIG(quick_viewtransitions)
QQuickTransition *QQuickItemView::populateTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->populateTransition : nullptr;
}

void QQuickItemView::setPopulateTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->populateTransition != transition) {
        d->transitioner->populateTransition = transition;
        emit populateTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::addTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->addTransition : nullptr;
}

void QQuickItemView::setAddTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->addTransition != transition) {
        d->transitioner->addTransition = transition;
        emit addTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::addDisplacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->addDisplacedTransition : nullptr;
}

void QQuickItemView::setAddDisplacedTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->addDisplacedTransition != transition) {
        d->transitioner->addDisplacedTransition = transition;
        emit addDisplacedTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::moveTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->moveTransition : nullptr;
}

void QQuickItemView::setMoveTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->moveTransition != transition) {
        d->transitioner->moveTransition = transition;
        emit moveTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::moveDisplacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->moveDisplacedTransition : nullptr;
}

void QQuickItemView::setMoveDisplacedTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->moveDisplacedTransition != transition) {
        d->transitioner->moveDisplacedTransition = transition;
        emit moveDisplacedTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::removeTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->removeTransition : nullptr;
}

void QQuickItemView::setRemoveTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->removeTransition != transition) {
        d->transitioner->removeTransition = transition;
        emit removeTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::removeDisplacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->removeDisplacedTransition : nullptr;
}

void QQuickItemView::setRemoveDisplacedTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->removeDisplacedTransition != transition) {
        d->transitioner->removeDisplacedTransition = transition;
        emit removeDisplacedTransitionChanged();
    }
}

QQuickTransition *QQuickItemView::displacedTransition() const
{
    Q_D(const QQuickItemView);
    return d->transitioner ? d->transitioner->displacedTransition : nullptr;
}

void QQuickItemView::setDisplacedTransition(QQuickTransition *transition)
{
    Q_D(QQuickItemView);
    d->createTransitioner();
    if (d->transitioner->displacedTransition != transition) {
        d->transitioner->displacedTransition = transition;
        emit displacedTransitionChanged();
    }
}
#endif

void QQuickItemViewPrivate::positionViewAtIndex(int index, int mode)
{
    if (!isValid())
        return;
    if (mode < QQuickItemView::Beginning || mode > QQuickItemView::SnapPosition)
        return;

    Q_Q(QQuickItemView);
    q->cancelFlick();
    applyPendingChanges();
    const int modelCount = model->count();
    int idx = qMax(qMin(index, modelCount - 1), 0);

    const auto viewSize = size();
    qreal pos = isContentFlowReversed() ? -position() - viewSize : position();
    FxViewItem *item = visibleItem(idx);
    qreal maxExtent = calculatedMaxExtent();
    if (!item) {
        qreal itemPos = positionAt(idx);
        changedVisibleIndex(idx);
        // save the currently visible items in case any of them end up visible again
        const QList<FxViewItem *> oldVisible = visibleItems;
        visibleItems.clear();
        setPosition(qMin(itemPos, maxExtent));
        // now release the reference to all the old visible items.
        for (FxViewItem *item : oldVisible)
            releaseItem(item, reusableFlag);
        item = visibleItem(idx);
    }
    if (item) {
        const bool stickyHeader = hasStickyHeader();
        const bool stickyFooter = hasStickyFooter();
        const qreal stickyHeaderSize = stickyHeader ? headerSize() : 0;
        const qreal stickyFooterSize = stickyFooter ? footerSize() : 0;

        const qreal itemPos = item->position();
        switch (mode) {
        case QQuickItemView::Beginning:
            pos = itemPos;
            if (header && (index < 0 || stickyHeader))
                pos -= headerSize();
            break;
        case QQuickItemView::Center:
            pos = itemPos - (viewSize - item->size())/2;
            break;
        case QQuickItemView::End:
            pos = itemPos - viewSize + item->size();
            if (footer && (index >= modelCount || stickyFooter))
                pos += footerSize();
            break;
        case QQuickItemView::Visible:
            if (itemPos > pos + viewSize - stickyFooterSize)
                pos = item->endPosition() - viewSize + stickyFooterSize;
            else if (item->endPosition() <= pos - stickyHeaderSize)
                pos = itemPos - stickyHeaderSize;
            break;
        case QQuickItemView::Contain:
            if (item->endPosition() >= pos + viewSize + stickyFooterSize)
                pos = itemPos - viewSize + item->size() + stickyFooterSize;
            if (itemPos - stickyHeaderSize < pos)
                pos = itemPos - stickyHeaderSize;
            break;
        case QQuickItemView::SnapPosition:
            pos = itemPos - highlightRangeStart - stickyHeaderSize;
            break;
        }
        pos = qMin(pos, maxExtent);
        qreal minExtent = calculatedMinExtent();
        pos = qMax(pos, minExtent);
        moveReason = QQuickItemViewPrivate::Other;
        setPosition(pos);

        if (highlight) {
            if (autoHighlight)
                resetHighlightPosition();
            updateHighlight();
        }
    }
    fixupPosition();
}

void QQuickItemView::positionViewAtIndex(int index, int mode)
{
    Q_D(QQuickItemView);
    if (!d->isValid() || index < 0 || index >= d->model->count())
        return;
    d->positionViewAtIndex(index, mode);
}


void QQuickItemView::positionViewAtBeginning()
{
    Q_D(QQuickItemView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(-1, Beginning);
}

void QQuickItemView::positionViewAtEnd()
{
    Q_D(QQuickItemView);
    if (!d->isValid())
        return;
    d->positionViewAtIndex(d->model->count(), End);
}

static FxViewItem * fxViewItemAtPosition(const QList<FxViewItem *> &items, qreal x, qreal y)
{
    for (FxViewItem *item : items) {
        if (item->contains(x, y))
            return item;
    }
    return nullptr;
}

int QQuickItemView::indexAt(qreal x, qreal y) const
{
    Q_D(const QQuickItemView);
    const FxViewItem *item = fxViewItemAtPosition(d->visibleItems, x, y);
    return item ? item->index : -1;
}

QQuickItem *QQuickItemView::itemAt(qreal x, qreal y) const
{
    Q_D(const QQuickItemView);
    const FxViewItem *item = fxViewItemAtPosition(d->visibleItems, x, y);
    return item ? item->item : nullptr;
}

QQuickItem *QQuickItemView::itemAtIndex(int index) const
{
    Q_D(const QQuickItemView);
    const FxViewItem *item = d->visibleItem(index);
    return item ? item->item : nullptr;
}

void QQuickItemView::forceLayout()
{
    Q_D(QQuickItemView);
    if (isComponentComplete() && (d->currentChanges.hasPendingChanges() || d->forceLayout))
        d->layout();
}

void QQuickItemViewPrivate::applyPendingChanges()
{
    Q_Q(QQuickItemView);
    if (q->isComponentComplete() && currentChanges.hasPendingChanges())
        layout();
}

int QQuickItemViewPrivate::findMoveKeyIndex(QQmlChangeSet::MoveKey key, const QVector<QQmlChangeSet::Change> &changes) const
{
    for (int i=0; i<changes.size(); i++) {
        for (int j=changes[i].index; j<changes[i].index + changes[i].count; j++) {
            if (changes[i].moveKey(j) == key)
                return j;
        }
    }
    return -1;
}

qreal QQuickItemViewPrivate::minExtentForAxis(const AxisData &axisData, bool forXAxis) const
{
    Q_Q(const QQuickItemView);

    qreal highlightStart;
    qreal highlightEnd;
    qreal endPositionFirstItem = 0;
    qreal extent = -startPosition() + axisData.startMargin;
    if (isContentFlowReversed()) {
        if (model && model->count())
            endPositionFirstItem = positionAt(model->count()-1);
        else
            extent += headerSize();
        highlightStart = highlightRangeEndValid ? size() - highlightRangeEnd : size();
        highlightEnd = highlightRangeStartValid ? size() - highlightRangeStart : size();
        extent += footerSize();
        qreal maxExtentAlongAxis = forXAxis ? q->maxXExtent() : q->maxYExtent();
        if (extent < maxExtentAlongAxis)
            extent = maxExtentAlongAxis;
    } else {
        endPositionFirstItem = endPositionAt(0);
        highlightStart = highlightRangeStart;
        highlightEnd = highlightRangeEnd;
        extent += headerSize();
    }
    if (haveHighlightRange && highlightRange == QQuickItemView::StrictlyEnforceRange) {
        extent += highlightStart;
        FxViewItem *firstItem = visibleItem(0);
        if (firstItem)
            extent -= firstItem->sectionSize();
        extent = isContentFlowReversed()
                            ? qMin(extent, endPositionFirstItem + highlightEnd)
                            : qMax(extent, -(endPositionFirstItem - highlightEnd));
    }
    return extent;
}

qreal QQuickItemViewPrivate::maxExtentForAxis(const AxisData &axisData, bool forXAxis) const
{
    Q_Q(const QQuickItemView);

    qreal highlightStart;
    qreal highlightEnd;
    qreal lastItemPosition = 0;
    qreal extent = 0;
    if (isContentFlowReversed()) {
        highlightStart = highlightRangeEndValid ? size() - highlightRangeEnd : size();
        highlightEnd = highlightRangeStartValid ? size() - highlightRangeStart : size();
        lastItemPosition = endPosition();
    } else {
        highlightStart = highlightRangeStart;
        highlightEnd = highlightRangeEnd;
        if (model && model->count())
            lastItemPosition = positionAt(model->count()-1);
    }
    if (!model || !model->count()) {
        if (!isContentFlowReversed())
            maxExtent = header ? -headerSize() : 0;
        extent += forXAxis ? q->width() : q->height();
    } else if (haveHighlightRange && highlightRange == QQuickItemView::StrictlyEnforceRange) {
        extent = -(lastItemPosition - highlightStart);
        if (highlightEnd != highlightStart) {
            extent = isContentFlowReversed()
                    ? qMax(extent, -(endPosition() - highlightEnd))
                    : qMin(extent, -(endPosition() - highlightEnd));
        }
    } else {
        extent = -(endPosition() - (forXAxis ? q->width() : q->height()));
    }
    if (isContentFlowReversed()) {
        extent -= headerSize();
        extent -= axisData.endMargin;
    } else {
        extent -= footerSize();
        extent -= axisData.endMargin;
        qreal minExtentAlongAxis = forXAxis ? q->minXExtent() : q->minYExtent();
        if (extent > minExtentAlongAxis)
            extent = minExtentAlongAxis;
    }

    return extent;
}

qreal QQuickItemViewPrivate::calculatedMinExtent() const
{
    Q_Q(const QQuickItemView);
    qreal minExtent;
    if (layoutOrientation() == Qt::Vertical)
        minExtent = isContentFlowReversed() ? q->maxYExtent() - size(): -q->minYExtent();
    else
        minExtent = isContentFlowReversed() ? q->maxXExtent() - size(): -q->minXExtent();
    return minExtent;

}

qreal QQuickItemViewPrivate::calculatedMaxExtent() const
{
    Q_Q(const QQuickItemView);
    qreal maxExtent;
    if (layoutOrientation() == Qt::Vertical)
        maxExtent = isContentFlowReversed() ? q->minYExtent() - size(): -q->maxYExtent();
    else
        maxExtent = isContentFlowReversed() ? q->minXExtent() - size(): -q->maxXExtent();
    return maxExtent;
}

void QQuickItemViewPrivate::applyDelegateChange()
{
    releaseVisibleItems(QQmlDelegateModel::NotReusable);
    releaseItem(currentItem, QQmlDelegateModel::NotReusable);
    currentItem = nullptr;
    updateSectionCriteria();
    refill();
    moveReason = QQuickItemViewPrivate::SetIndex;
    updateCurrent(currentIndex);
    if (highlight && currentItem) {
        if (autoHighlight)
            resetHighlightPosition();
        updateTrackedItem();
    }
    moveReason = QQuickItemViewPrivate::Other;
    updateViewport();
}

// for debugging only
void QQuickItemViewPrivate::checkVisible() const
{
    int skip = 0;
    for (int i = 0; i < visibleItems.size(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == -1) {
            ++skip;
        } else if (item->index != visibleIndex + i - skip) {
            qFatal("index %d %d %d", visibleIndex, i, item->index);
        }
    }
}

// for debugging only
void QQuickItemViewPrivate::showVisibleItems() const
{
    qDebug() << "Visible items:";
    for (FxViewItem *item : visibleItems) {
        qDebug() << "\t" << item->index
                 << item->item->objectName()
                 << item->position();
    }
}

void QQuickItemViewPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change,
                                                const QRectF &oldGeometry)
{
    Q_Q(QQuickItemView);
    QQuickFlickablePrivate::itemGeometryChanged(item, change, oldGeometry);
    if (!q->isComponentComplete())
        return;

    if (header && header->item == item) {
        updateHeader();
        markExtentsDirty();
        updateViewport();
        if (!q->isMoving() && !q->isFlicking())
            fixupPosition();
    } else if (footer && footer->item == item) {
        updateFooter();
        markExtentsDirty();
        updateViewport();
        if (!q->isMoving() && !q->isFlicking())
            fixupPosition();
    }

    if (currentItem && currentItem->item == item) {
        // don't allow item movement transitions to trigger a re-layout and
        // start new transitions
        bool prevInLayout = inLayout;
#if QT_CONFIG(quick_viewtransitions)
        if (!inLayout) {
            FxViewItem *actualItem = transitioner ? visibleItem(currentIndex) : nullptr;
            if (actualItem && actualItem->transitionRunning())
                inLayout = true;
        }
#endif
        updateHighlight();
        inLayout = prevInLayout;
    }

    if (trackedItem && trackedItem->item == item)
        q->trackedPositionChanged();
}

void QQuickItemView::destroyRemoved()
{
    Q_D(QQuickItemView);

#if QT_CONFIG(quick_viewtransitions)
    bool hasRemoveTransition = false;
    bool hasRemoveTransitionAsTarget = false;
    if (d->transitioner) {
        hasRemoveTransition = d->transitioner->canTransition(QQuickItemViewTransitioner::RemoveTransition, false);
        hasRemoveTransitionAsTarget = d->transitioner->canTransition(QQuickItemViewTransitioner::RemoveTransition, true);
    }
#endif

    for (QList<FxViewItem*>::Iterator it = d->visibleItems.begin();
            it != d->visibleItems.end();) {
        FxViewItem *item = *it;
        if (item->index == -1 && (!item->attached || item->attached->delayRemove() == false)) {
#if QT_CONFIG(quick_viewtransitions)
            if (hasRemoveTransitionAsTarget) {
                // don't remove from visibleItems until next layout()
                d->runDelayedRemoveTransition = true;
                QObject::disconnect(item->attached, SIGNAL(delayRemoveChanged()), this, SLOT(destroyRemoved()));
                ++it;
            } else {
                if (hasRemoveTransition)
                    d->runDelayedRemoveTransition = true;
#endif
                d->releaseItem(item, d->reusableFlag);
                it = d->visibleItems.erase(it);
#if QT_CONFIG(quick_viewtransitions)
            }
#endif
        } else {
            ++it;
        }
    }

    // Correct the positioning of the items
    d->forceLayoutPolish();
}

void QQuickItemView::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_D(QQuickItemView);
    if (reset) {
        cancelFlick();
#if QT_CONFIG(quick_viewtransitions)
        if (d->transitioner)
            d->transitioner->setPopulateTransitionEnabled(true);
#endif
        d->moveReason = QQuickItemViewPrivate::SetIndex;
        d->regenerate();
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->resetHighlightPosition();
            d->updateTrackedItem();
        }
        d->moveReason = QQuickItemViewPrivate::Other;
        emit countChanged();
#if QT_CONFIG(quick_viewtransitions)
        if (d->transitioner && d->transitioner->populateTransition)
            d->forceLayoutPolish();
#endif
    } else {
        if (d->inLayout) {
            d->bufferedChanges.prepare(d->currentIndex, d->itemCount);
            d->bufferedChanges.applyChanges(changeSet);
        } else {
            if (d->bufferedChanges.hasPendingChanges()) {
                d->currentChanges.applyBufferedChanges(d->bufferedChanges);
                d->bufferedChanges.reset();
            }
            d->currentChanges.prepare(d->currentIndex, d->itemCount);
            d->currentChanges.applyChanges(changeSet);
        }
        polish();
    }
}

void QQuickItemView::animStopped()
{
    Q_D(QQuickItemView);
    d->bufferMode = QQuickItemViewPrivate::BufferBefore | QQuickItemViewPrivate::BufferAfter;
    d->refillOrLayout();
    if (d->haveHighlightRange && d->highlightRange == QQuickItemView::StrictlyEnforceRange)
        d->updateHighlight();
}


void QQuickItemView::trackedPositionChanged()
{
    Q_D(QQuickItemView);
    if (!d->trackedItem || !d->currentItem)
        return;

    if (d->inLayout) {
        polish();
        return;
    }

    if (d->moveReason == QQuickItemViewPrivate::SetIndex) {
        qreal trackedPos = d->trackedItem->position();
        qreal trackedSize = d->trackedItem->size();
        qreal viewPos = d->isContentFlowReversed() ? -d->position()-d->size() : d->position();
        qreal pos = viewPos;
        if (d->haveHighlightRange) {
            if (trackedPos > pos + d->highlightRangeEnd - trackedSize)
                pos = trackedPos - d->highlightRangeEnd + trackedSize;
            if (trackedPos < pos + d->highlightRangeStart)
                pos = trackedPos - d->highlightRangeStart;
            if (d->highlightRange != StrictlyEnforceRange) {
                qreal maxExtent = d->calculatedMaxExtent();
                if (pos > maxExtent)
                    pos = maxExtent;
                qreal minExtent = d->calculatedMinExtent();
                if (pos < minExtent)
                    pos = minExtent;
            }
        } else {
            if (d->trackedItem != d->currentItem) {
                // also make section header visible
                trackedPos -= d->currentItem->sectionSize();
                trackedSize += d->currentItem->sectionSize();
            }
            qreal trackedEndPos = d->trackedItem->endPosition();
            qreal toItemPos = d->currentItem->position();
            qreal toItemEndPos = d->currentItem->endPosition();
            if (d->showHeaderForIndex(d->currentIndex)) {
                qreal startOffset = -d->contentStartOffset();
                trackedPos -= startOffset;
                trackedEndPos -= startOffset;
                toItemPos -= startOffset;
                toItemEndPos -= startOffset;
            } else if (d->showFooterForIndex(d->currentIndex)) {
                qreal endOffset = d->footerSize();
                if (d->layoutOrientation() == Qt::Vertical) {
                    if (d->isContentFlowReversed())
                        endOffset += d->vData.startMargin;
                    else
                        endOffset += d->vData.endMargin;
                } else {
                    if (d->isContentFlowReversed())
                        endOffset += d->hData.startMargin;
                    else
                        endOffset += d->hData.endMargin;
                }
                trackedPos += endOffset;
                trackedEndPos += endOffset;
                toItemPos += endOffset;
                toItemEndPos += endOffset;
            }

            if (trackedEndPos >= viewPos + d->size()
                && toItemEndPos >= viewPos + d->size()) {
                if (trackedEndPos <= toItemEndPos) {
                    pos = trackedEndPos - d->size();
                    if (trackedSize > d->size())
                        pos = trackedPos;
                } else {
                    pos = toItemEndPos - d->size();
                    if (d->currentItem->size() > d->size())
                        pos = d->currentItem->position();
                }
            }
            if (trackedPos < pos && toItemPos < pos)
                pos = qMax(trackedPos, toItemPos);
        }
        if (viewPos != pos) {
            d->calcVelocity = true;
            d->setPosition(pos);
            d->calcVelocity = false;
        }
    }
}

void QQuickItemView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickItemView);
    d->markExtentsDirty();
    if (isComponentComplete() && (d->isValid() || !d->visibleItems.isEmpty()))
        d->forceLayoutPolish();
    QQuickFlickable::geometryChange(newGeometry, oldGeometry);
}

qreal QQuickItemView::minYExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Horizontal)
        return QQuickFlickable::minYExtent();

    if (d->vData.minExtentDirty) {
        d->minExtent = d->minExtentForAxis(d->vData, false);
        d->vData.minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QQuickItemView::maxYExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Horizontal)
        return QQuickFlickable::maxYExtent();

    if (d->vData.maxExtentDirty) {
        d->maxExtent = d->maxExtentForAxis(d->vData, false);
        d->vData.maxExtentDirty = false;
    }

    return d->maxExtent;
}

qreal QQuickItemView::minXExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Vertical)
        return QQuickFlickable::minXExtent();

    if (d->hData.minExtentDirty) {
        d->minExtent = d->minExtentForAxis(d->hData, true);
        d->hData.minExtentDirty = false;
    }

    return d->minExtent;
}

qreal QQuickItemView::maxXExtent() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Vertical)
        return QQuickFlickable::maxXExtent();

    if (d->hData.maxExtentDirty) {
        d->maxExtent = d->maxExtentForAxis(d->hData, true);
        d->hData.maxExtentDirty = false;
    }

    return d->maxExtent;
}

void QQuickItemView::setContentX(qreal pos)
{
    Q_D(QQuickItemView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QQuickItemViewPrivate::Other;
    QQuickFlickable::setContentX(pos);
}

void QQuickItemView::setContentY(qreal pos)
{
    Q_D(QQuickItemView);
    // Positioning the view manually should override any current movement state
    d->moveReason = QQuickItemViewPrivate::Other;
    QQuickFlickable::setContentY(pos);
}

qreal QQuickItemView::originX() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Horizontal
            && effectiveLayoutDirection() == Qt::RightToLeft
            && contentWidth() < width()) {
        return -d->lastPosition() - d->footerSize();
    }
    return QQuickFlickable::originX();
}

qreal QQuickItemView::originY() const
{
    Q_D(const QQuickItemView);
    if (d->layoutOrientation() == Qt::Vertical
            && d->verticalLayoutDirection == QQuickItemView::BottomToTop
            && contentHeight() < height()) {
        return -d->lastPosition() - d->footerSize();
    }
    return QQuickFlickable::originY();
}

void QQuickItemView::updatePolish()
{
    Q_D(QQuickItemView);
    QQuickFlickable::updatePolish();
    d->layout();
}

void QQuickItemView::componentComplete()
{
    Q_D(QQuickItemView);
    if (d->model && d->ownModel)
        static_cast<QQmlDelegateModel *>(d->model.data())->componentComplete();

    QQuickFlickable::componentComplete();

    d->updateSectionCriteria();
    d->updateHeader();
    d->updateFooter();
    d->updateViewport();
    d->setPosition(d->contentStartOffset());
#if QT_CONFIG(quick_viewtransitions)
    if (d->transitioner)
        d->transitioner->setPopulateTransitionEnabled(true);
#endif

    if (d->isValid()) {
        d->refill();
        d->moveReason = QQuickItemViewPrivate::SetIndex;
        if (d->currentIndex < 0 && !d->currentIndexCleared)
            d->updateCurrent(0);
        else
            d->updateCurrent(d->currentIndex);
        if (d->highlight && d->currentItem) {
            if (d->autoHighlight)
                d->resetHighlightPosition();
            d->updateTrackedItem();
        }
        d->moveReason = QQuickItemViewPrivate::Other;
        d->fixupPosition();
    }
    if (d->model && d->model->count())
        emit countChanged();
}



QQuickItemViewPrivate::QQuickItemViewPrivate()
    : itemCount(0)
    , buffer(QML_VIEW_DEFAULTCACHEBUFFER), bufferMode(BufferBefore | BufferAfter)
    , displayMarginBeginning(0), displayMarginEnd(0)
    , layoutDirection(Qt::LeftToRight), verticalLayoutDirection(QQuickItemView::TopToBottom)
    , visibleIndex(0)
    , currentIndex(-1), currentItem(nullptr)
    , trackedItem(nullptr), requestedIndex(-1)
    , highlightComponent(nullptr), highlight(nullptr)
    , highlightRange(QQuickItemView::NoHighlightRange)
    , highlightRangeStart(0), highlightRangeEnd(0)
    , highlightMoveDuration(150)
    , headerComponent(nullptr), header(nullptr), footerComponent(nullptr), footer(nullptr)
#if QT_CONFIG(quick_viewtransitions)
    , transitioner(nullptr)
#endif
    , minExtent(0), maxExtent(0)
    , ownModel(false), wrap(false)
    , keyNavigationEnabled(true)
    , explicitKeyNavigationEnabled(false)
    , inLayout(false), inViewportMoved(false), forceLayout(false), currentIndexCleared(false)
    , haveHighlightRange(false), autoHighlight(true), highlightRangeStartValid(false), highlightRangeEndValid(false)
    , fillCacheBuffer(false), inRequest(false)
#if QT_CONFIG(quick_viewtransitions)
    , runDelayedRemoveTransition(false)
#endif
    , delegateValidated(false), isClearing(false)
{
    bufferPause.addAnimationChangeListener(this, QAbstractAnimationJob::Completion);
    bufferPause.setLoopCount(1);
    bufferPause.setDuration(16);
}

QQuickItemViewPrivate::~QQuickItemViewPrivate()
{
#if QT_CONFIG(quick_viewtransitions)
    if (transitioner)
        transitioner->setChangeListener(nullptr);
    delete transitioner;
#endif
}

bool QQuickItemViewPrivate::isValid() const
{
    return model && model->count() && model->isValid();
}

qreal QQuickItemViewPrivate::position() const
{
    Q_Q(const QQuickItemView);
    return layoutOrientation() == Qt::Vertical ? q->contentY() : q->contentX();
}

qreal QQuickItemViewPrivate::size() const
{
    Q_Q(const QQuickItemView);
    return layoutOrientation() == Qt::Vertical ? q->height() : q->width();
}

qreal QQuickItemViewPrivate::startPosition() const
{
    return isContentFlowReversed() ? -lastPosition() : originPosition();
}

qreal QQuickItemViewPrivate::endPosition() const
{
    return isContentFlowReversed() ? -originPosition() : lastPosition();
}

qreal QQuickItemViewPrivate::contentStartOffset() const
{
    qreal pos = -headerSize();
    if (layoutOrientation() == Qt::Vertical) {
        if (isContentFlowReversed())
            pos -= vData.endMargin;
        else
            pos -= vData.startMargin;
    } else {
        if (isContentFlowReversed())
            pos -= hData.endMargin;
        else
            pos -= hData.startMargin;
    }
    return pos;
}

int QQuickItemViewPrivate::findLastVisibleIndex(int defaultValue) const
{
    for (auto it = visibleItems.rbegin(), end = visibleItems.rend(); it != end; ++it) {
        auto item = *it;
        if (item->index != -1)
            return item->index;
    }
    return defaultValue;
}

FxViewItem *QQuickItemViewPrivate::visibleItem(int modelIndex) const {
    if (modelIndex >= visibleIndex && modelIndex < visibleIndex + visibleItems.size()) {
        for (int i = modelIndex - visibleIndex; i < visibleItems.size(); ++i) {
            FxViewItem *item = visibleItems.at(i);
            if (item->index == modelIndex)
                return item;
        }
    }
    return nullptr;
}

FxViewItem *QQuickItemViewPrivate::firstItemInView() const {
    const qreal pos = isContentFlowReversed() ? -position()-size() : position();
    for (FxViewItem *item : visibleItems) {
        if (item->index != -1 && item->endPosition() > pos)
            return item;
    }
    return visibleItems.size() ? visibleItems.first() : 0;
}

int QQuickItemViewPrivate::findLastIndexInView() const
{
    const qreal viewEndPos = isContentFlowReversed() ? -position() : position() + size();
    for (auto it = visibleItems.rbegin(), end = visibleItems.rend(); it != end; ++it) {
        auto item = *it;
        if (item->index != -1 && item->position() <= viewEndPos)
            return item->index;
    }
    return -1;
}

// Map a model index to visibleItems list index.
// These may differ if removed items are still present in the visible list,
// e.g. doing a removal animation
int QQuickItemViewPrivate::mapFromModel(int modelIndex) const
{
    if (modelIndex < visibleIndex || modelIndex >= visibleIndex + visibleItems.size())
        return -1;
    for (int i = 0; i < visibleItems.size(); ++i) {
        FxViewItem *item = visibleItems.at(i);
        if (item->index == modelIndex)
            return i;
        if (item->index > modelIndex)
            return -1;
    }
    return -1; // Not in visibleList
}

void QQuickItemViewPrivate::init()
{
    Q_Q(QQuickItemView);
    q->setFlag(QQuickItem::ItemIsFocusScope);
    QObject::connect(q, SIGNAL(movementEnded()), q, SLOT(animStopped()));
    QObject::connect(q, &QQuickFlickable::interactiveChanged, q, &QQuickItemView::keyNavigationEnabledChanged);
    q->setFlickableDirection(QQuickFlickable::VerticalFlick);
}

void QQuickItemViewPrivate::updateCurrent(int modelIndex)
{
    Q_Q(QQuickItemView);
    applyPendingChanges();
    if (!q->isComponentComplete() || !isValid() || modelIndex < 0 || modelIndex >= model->count()) {
        if (currentItem) {
            if (currentItem->attached)
                currentItem->attached->setIsCurrentItem(false);
            releaseItem(currentItem, reusableFlag);
            currentItem = nullptr;
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
            emit q->currentItemChanged();
            updateHighlight();
        } else if (currentIndex != modelIndex) {
            currentIndex = modelIndex;
            emit q->currentIndexChanged();
        }
        return;
    }

    if (currentItem && currentIndex == modelIndex) {
        updateHighlight();
        return;
    }

    FxViewItem *oldCurrentItem = currentItem;
    int oldCurrentIndex = currentIndex;
    currentIndex = modelIndex;
    currentItem = createItem(modelIndex, QQmlIncubator::AsynchronousIfNested);
    if (oldCurrentItem && oldCurrentItem->attached && (!currentItem || oldCurrentItem->item != currentItem->item))
        oldCurrentItem->attached->setIsCurrentItem(false);
    if (currentItem) {
        currentItem->item->setFocus(true);
        if (currentItem->attached)
            currentItem->attached->setIsCurrentItem(true);
        initializeCurrentItem();
    }

    updateHighlight();
    if (oldCurrentIndex != currentIndex)
        emit q->currentIndexChanged();
    if (oldCurrentItem != currentItem
            && (!oldCurrentItem || !currentItem || oldCurrentItem->item != currentItem->item))
        emit q->currentItemChanged();
    releaseItem(oldCurrentItem, reusableFlag);
}

void QQuickItemViewPrivate::clear(bool onDestruction)
{
    Q_Q(QQuickItemView);

    isClearing = true;
    auto cleanup = qScopeGuard([this] { isClearing = false; });

    currentChanges.reset();
    bufferedChanges.reset();
    timeline.clear();

    releaseVisibleItems(QQmlInstanceModel::NotReusable);
    visibleIndex = 0;

#if QT_CONFIG(quick_viewtransitions)
    for (FxViewItem *item : std::as_const(releasePendingTransition)) {
        item->releaseAfterTransition = false;
        releaseItem(item, QQmlInstanceModel::NotReusable);
    }
    releasePendingTransition.clear();
#endif

    auto oldCurrentItem = currentItem;
    releaseItem(currentItem, QQmlDelegateModel::NotReusable);
    currentItem = nullptr;
    if (oldCurrentItem)
        emit q->currentItemChanged();
    createHighlight(onDestruction);
    trackedItem = nullptr;

    if (requestedIndex >= 0) {
        if (model)
            model->cancel(requestedIndex);
        requestedIndex = -1;
    }

    markExtentsDirty();
    itemCount = 0;
}


void QQuickItemViewPrivate::mirrorChange()
{
    Q_Q(QQuickItemView);
    regenerate();
    emit q->effectiveLayoutDirectionChanged();
}

void QQuickItemViewPrivate::animationFinished(QAbstractAnimationJob *)
{
    Q_Q(QQuickItemView);
    fillCacheBuffer = true;
    q->polish();
}

void QQuickItemViewPrivate::refill()
{
    qreal s = qMax(size(), qreal(0.));
    const auto pos = position();
    if (isContentFlowReversed())
        refill(-pos - displayMarginBeginning-s, -pos + displayMarginEnd);
    else
        refill(pos - displayMarginBeginning, pos + displayMarginEnd+s);
}

void QQuickItemViewPrivate::refill(qreal from, qreal to)
{
    Q_Q(QQuickItemView);
    if (!model || !model->isValid() || !q->isComponentComplete())
        return;
    if (q->size().isEmpty() && visibleItems.isEmpty())
        return;
    if (!model->count()) {
        updateHeader();
        updateFooter();
        updateViewport();
        return;
    }

    do {
        bufferPause.stop();
        if (currentChanges.hasPendingChanges() || bufferedChanges.hasPendingChanges() || currentChanges.active) {
            currentChanges.reset();
            bufferedChanges.reset();
            releaseVisibleItems(reusableFlag);
        }

        int prevCount = itemCount;
        itemCount = model->count();
        qreal bufferFrom = from - buffer;
        qreal bufferTo = to + buffer;
        qreal fillFrom = from;
        qreal fillTo = to;

        bool added = addVisibleItems(fillFrom, fillTo, bufferFrom, bufferTo, false);
        bool removed = removeNonVisibleItems(bufferFrom, bufferTo);

        if (requestedIndex == -1 && buffer && bufferMode != NoBuffer) {
            if (added) {
                // We've already created a new delegate this frame.
                // Just schedule a buffer refill.
                bufferPause.start();
            } else {
                if (bufferMode & BufferAfter)
                    fillTo = bufferTo;
                if (bufferMode & BufferBefore)
                    fillFrom = bufferFrom;
                added |= addVisibleItems(fillFrom, fillTo, bufferFrom, bufferTo, true);
            }
        }

        if (added || removed) {
            markExtentsDirty();
            updateBeginningEnd();
            visibleItemsChanged();
            updateHeader();
            updateFooter();
            updateViewport();
        }

        if (prevCount != itemCount)
            emit q->countChanged();
    } while (currentChanges.hasPendingChanges() || bufferedChanges.hasPendingChanges());
    storeFirstVisibleItemPosition();
}

void QQuickItemViewPrivate::regenerate(bool orientationChanged)
{
    Q_Q(QQuickItemView);
    if (q->isComponentComplete()) {
        if (orientationChanged) {
            delete header;
            header = nullptr;
            delete footer;
            footer = nullptr;
        }
        clear();
        updateHeader();
        updateFooter();
        updateViewport();
        setPosition(contentStartOffset());
        refill();
        updateCurrent(currentIndex);
    }
}

void QQuickItemViewPrivate::updateViewport()
{
    Q_Q(QQuickItemView);
    qreal extra = headerSize() + footerSize();
    qreal contentSize = isValid() || !visibleItems.isEmpty() ? (endPosition() - startPosition()) : 0.0;
    if (layoutOrientation() == Qt::Vertical)
        q->setContentHeight(contentSize + extra);
    else
        q->setContentWidth(contentSize + extra);
}

void QQuickItemViewPrivate::layout()
{
    Q_Q(QQuickItemView);
    if (inLayout)
        return;

    inLayout = true;

    // viewBounds contains bounds before any add/remove/move operation to the view
    QRectF viewBounds(q->contentX(),  q->contentY(), q->width(), q->height());

    if (!isValid() && !visibleItems.size()) {
        clear();
        setPosition(contentStartOffset());
        updateViewport();
#if QT_CONFIG(quick_viewtransitions)
        if (transitioner)
            transitioner->setPopulateTransitionEnabled(false);
#endif
        inLayout = false;
        return;
    }

#if QT_CONFIG(quick_viewtransitions)
    if (runDelayedRemoveTransition && transitioner
            && transitioner->canTransition(QQuickItemViewTransitioner::RemoveTransition, false)) {
        // assume that any items moving now are moving due to the remove - if they schedule
        // a different transition, that will override this one anyway
        for (int i=0; i<visibleItems.size(); i++)
            visibleItems[i]->transitionNextReposition(transitioner, QQuickItemViewTransitioner::RemoveTransition, false);
    }
#endif

    ChangeResult insertionPosChanges;
    ChangeResult removalPosChanges;
    if (!applyModelChanges(&insertionPosChanges, &removalPosChanges) && !forceLayout) {
        if (fillCacheBuffer) {
            fillCacheBuffer = false;
            refill();
        }
        inLayout = false;
        return;
    }
    forceLayout = false;

#if QT_CONFIG(quick_viewtransitions)
    if (transitioner && transitioner->canTransition(QQuickItemViewTransitioner::PopulateTransition, true)) {
        // Give the view one more chance to refill itself,
        // in case its size is changed such that more delegates become visible after component completed
        refill();
        for (FxViewItem *item : std::as_const(visibleItems)) {
            if (!item->transitionScheduledOrRunning())
                item->transitionNextReposition(transitioner, QQuickItemViewTransitioner::PopulateTransition, true);
        }
    }
#endif

    updateSections();
    layoutVisibleItems();
    storeFirstVisibleItemPosition();

#if QT_CONFIG(quick_viewtransitions)
    int lastIndexInView = findLastIndexInView();
#endif
    refill();
    markExtentsDirty();
    updateHighlight();

    if (!q->isMoving() && !q->isFlicking() && !movingFromHighlight()) {
        fixupPosition();
        refill();
    }

    updateHeader();
    updateFooter();
    updateViewport();
    updateUnrequestedPositions();

#if QT_CONFIG(quick_viewtransitions)
    if (transitioner) {
        // items added in the last refill() may need to be transitioned in - e.g. a remove
        // causes items to slide up into view
        if (lastIndexInView != -1 &&
            (transitioner->canTransition(QQuickItemViewTransitioner::MoveTransition, false)
                || transitioner->canTransition(QQuickItemViewTransitioner::RemoveTransition, false))) {
            translateAndTransitionItemsAfter(lastIndexInView, insertionPosChanges, removalPosChanges);
        }

        prepareVisibleItemTransitions();

        // We cannot use iterators here as erasing from a container invalidates them.
        for (int i = 0, count = releasePendingTransition.size(); i < count;) {
            auto success = prepareNonVisibleItemTransition(releasePendingTransition[i], viewBounds);
            // prepareNonVisibleItemTransition() may remove items while in fast flicking.
            // Invisible animating items are kicked in or out the viewPort.
            // Recheck count to test if the item got removed. In that case the same index points
            // to a different item now.
            const int old_count = count;
            count = releasePendingTransition.size();
            if (old_count > count)
                continue;

            if (!success) {
                releaseItem(releasePendingTransition[i], reusableFlag);
                releasePendingTransition.remove(i);
                --count;
            } else {
                ++i;
            }
        }

        for (int i=0; i<visibleItems.size(); i++)
            visibleItems[i]->startTransition(transitioner);
        for (int i=0; i<releasePendingTransition.size(); i++)
            releasePendingTransition[i]->startTransition(transitioner);

        transitioner->setPopulateTransitionEnabled(false);
        transitioner->resetTargetLists();
    }
#endif

    if (!currentItem)
        updateCurrent(currentIndex);

#if QT_CONFIG(quick_viewtransitions)
    runDelayedRemoveTransition = false;
#endif
    inLayout = false;
}

bool QQuickItemViewPrivate::applyModelChanges(ChangeResult *totalInsertionResult, ChangeResult *totalRemovalResult)
{
    Q_Q(QQuickItemView);
    if (!q->isComponentComplete() || !hasPendingChanges())
        return false;

    if (bufferedChanges.hasPendingChanges()) {
        currentChanges.applyBufferedChanges(bufferedChanges);
        bufferedChanges.reset();
    }

    updateUnrequestedIndexes();

    FxViewItem *prevVisibleItemsFirst = visibleItems.size() ? *visibleItems.constBegin() : nullptr;
    int prevItemCount = itemCount;
    int prevVisibleItemsCount = visibleItems.size();
    bool visibleAffected = false;
    bool viewportChanged = !currentChanges.pendingChanges.removes().isEmpty()
            || !currentChanges.pendingChanges.inserts().isEmpty();

    FxViewItem *prevFirstItemInView = firstItemInView();
    QQmlNullableValue<qreal> prevFirstItemInViewPos;
    int prevFirstItemInViewIndex = -1;
    if (prevFirstItemInView) {
        prevFirstItemInViewPos = prevFirstItemInView->position();
        prevFirstItemInViewIndex = prevFirstItemInView->index;
    }
    qreal prevVisibleItemsFirstPos = visibleItems.size() ? firstVisibleItemPosition : 0.0;

    totalInsertionResult->visiblePos = prevFirstItemInViewPos;
    totalRemovalResult->visiblePos = prevFirstItemInViewPos;

    const QVector<QQmlChangeSet::Change> &removals = currentChanges.pendingChanges.removes();
    const QVector<QQmlChangeSet::Change> &insertions = currentChanges.pendingChanges.inserts();
    ChangeResult insertionResult(prevFirstItemInViewPos);
    ChangeResult removalResult(prevFirstItemInViewPos);

    int removedCount = 0;
    for (const QQmlChangeSet::Change &r : removals) {
        itemCount -= r.count;
        if (applyRemovalChange(r, &removalResult, &removedCount))
            visibleAffected = true;
        if (!visibleAffected && needsRefillForAddedOrRemovedIndex(r.index))
            visibleAffected = true;
        const int correctedFirstVisibleIndex = prevFirstItemInViewIndex - removalResult.countChangeBeforeVisible;
        if (correctedFirstVisibleIndex >= 0 && r.index < correctedFirstVisibleIndex) {
            if (r.index + r.count < correctedFirstVisibleIndex)
                removalResult.countChangeBeforeVisible += r.count;
            else
                removalResult.countChangeBeforeVisible += (correctedFirstVisibleIndex  - r.index);
        }
    }
#if QT_CONFIG(quick_viewtransitions)
    if (runDelayedRemoveTransition) {
        QQmlChangeSet::Change removal;
        for (QList<FxViewItem*>::Iterator it = visibleItems.begin(); it != visibleItems.end();) {
            FxViewItem *item = *it;
            if (item->index == -1 && (!item->attached || !item->attached->delayRemove())) {
                removeItem(item, removal, &removalResult);
                removedCount++;
                it = visibleItems.erase(it);
            } else {
               ++it;
            }
        }
    }
#endif
    *totalRemovalResult += removalResult;
    if (!removals.isEmpty()) {
        updateVisibleIndex();

        // set positions correctly for the next insertion
        if (!insertions.isEmpty()) {
            repositionFirstItem(prevVisibleItemsFirst, prevVisibleItemsFirstPos, prevFirstItemInView, &insertionResult, &removalResult);
            layoutVisibleItems(removals.first().index);
            storeFirstVisibleItemPosition();
        }
    }

    QList<FxViewItem *> newItems;
    QList<MovedItem> movingIntoView;

    for (int i=0; i<insertions.size(); i++) {
        bool wasEmpty = visibleItems.isEmpty();
        if (applyInsertionChange(insertions[i], &insertionResult, &newItems, &movingIntoView))
            visibleAffected = true;
        if (!visibleAffected && needsRefillForAddedOrRemovedIndex(insertions[i].index))
            visibleAffected = true;
        if (wasEmpty && !visibleItems.isEmpty())
            resetFirstItemPosition();
        *totalInsertionResult += insertionResult;

        // set positions correctly for the next insertion
        if (i < insertions.size() - 1) {
            repositionFirstItem(prevVisibleItemsFirst, prevVisibleItemsFirstPos, prevFirstItemInView, &insertionResult, &removalResult);
            layoutVisibleItems(insertions[i].index);
            storeFirstVisibleItemPosition();
        }
        itemCount += insertions[i].count;
    }
    for (FxViewItem *item : std::as_const(newItems)) {
        if (item->attached)
            item->attached->emitAdd();
    }

#if QT_CONFIG(quick_viewtransitions)
    // for each item that was moved directly into the view as a result of a move(),
    // find the index it was moved from in order to set its initial position, so that we
    // can transition it from this "original" position to its new position in the view
    if (transitioner && transitioner->canTransition(QQuickItemViewTransitioner::MoveTransition, true)) {
        for (const MovedItem &m : std::as_const(movingIntoView)) {
            int fromIndex = findMoveKeyIndex(m.moveKey, removals);
            if (fromIndex >= 0) {
                if (prevFirstItemInViewIndex >= 0 && fromIndex < prevFirstItemInViewIndex)
                    repositionItemAt(m.item, fromIndex, -totalInsertionResult->sizeChangesAfterVisiblePos);
                else
                    repositionItemAt(m.item, fromIndex, totalInsertionResult->sizeChangesAfterVisiblePos);
                m.item->transitionNextReposition(transitioner, QQuickItemViewTransitioner::MoveTransition, true);
            }
        }
    }
#endif

    // reposition visibleItems.first() correctly so that the content y doesn't jump
    if (removedCount != prevVisibleItemsCount)
        repositionFirstItem(prevVisibleItemsFirst, prevVisibleItemsFirstPos, prevFirstItemInView, &insertionResult, &removalResult);

    // Whatever removed/moved items remain are no longer visible items.
#if QT_CONFIG(quick_viewtransitions)
    prepareRemoveTransitions(&currentChanges.removedItems);
#endif
    for (auto it = currentChanges.removedItems.begin();
         it != currentChanges.removedItems.end(); ++it) {
        releaseItem(it.value(), reusableFlag);
    }
    currentChanges.removedItems.clear();

    if (currentChanges.currentChanged) {
        if (currentChanges.currentRemoved && currentItem) {
            if (currentItem->item && currentItem->attached)
                currentItem->attached->setIsCurrentItem(false);
            auto oldCurrentItem = currentItem;
            releaseItem(currentItem, reusableFlag);
            currentItem = nullptr;
            if (oldCurrentItem)
                emit q->currentItemChanged();
        }
        if (!currentIndexCleared)
            updateCurrent(currentChanges.newCurrentIndex);
    }

    if (!visibleAffected)
        visibleAffected = !currentChanges.pendingChanges.changes().isEmpty();
    currentChanges.reset();

    updateSections();
    if (prevItemCount != itemCount)
        emit q->countChanged();
    if (!visibleAffected && viewportChanged)
        updateViewport();

    return visibleAffected;
}

bool QQuickItemViewPrivate::applyRemovalChange(const QQmlChangeSet::Change &removal, ChangeResult *removeResult, int *removedCount)
{
    Q_Q(QQuickItemView);
    bool visibleAffected = false;

    if (visibleItems.size() && removal.index + removal.count > visibleItems.constLast()->index) {
        if (removal.index > visibleItems.constLast()->index)
            removeResult->countChangeAfterVisibleItems += removal.count;
        else
            removeResult->countChangeAfterVisibleItems += ((removal.index + removal.count - 1) - visibleItems.constLast()->index);
    }

    QList<FxViewItem*>::Iterator it = visibleItems.begin();
    while (it != visibleItems.end()) {
        FxViewItem *item = *it;
        if (item->index == -1 || item->index < removal.index) {
            // already removed, or before removed items
            if (!visibleAffected && item->index < removal.index)
                visibleAffected = true;
            ++it;
        } else if (item->index >= removal.index + removal.count) {
            // after removed items
            item->index -= removal.count;
#if QT_CONFIG(quick_viewtransitions)
            if (removal.isMove())
                item->transitionNextReposition(transitioner, QQuickItemViewTransitioner::MoveTransition, false);
            else
                item->transitionNextReposition(transitioner, QQuickItemViewTransitioner::RemoveTransition, false);
#endif
            ++it;
        } else {
            // removed item
            visibleAffected = true;
            if (!removal.isMove() && item->item && item->attached)
                item->attached->emitRemove();

            if (item->item && item->attached && item->attached->delayRemove() && !removal.isMove()) {
                item->index = -1;
                QObject::connect(item->attached, SIGNAL(delayRemoveChanged()), q, SLOT(destroyRemoved()), Qt::QueuedConnection);
                ++it;
            } else {
                removeItem(item, removal, removeResult);
                if (!removal.isMove())
                    (*removedCount)++;
                it = visibleItems.erase(it);
            }
        }
    }

    return visibleAffected;
}

void QQuickItemViewPrivate::removeItem(FxViewItem *item, const QQmlChangeSet::Change &removal, ChangeResult *removeResult)
{
    if (removeResult->visiblePos.isValid()) {
        if (item->position() < removeResult->visiblePos)
            updateSizeChangesBeforeVisiblePos(item, removeResult);
        else
            removeResult->sizeChangesAfterVisiblePos += item->size();
    }
    if (removal.isMove()) {
        currentChanges.removedItems.replace(removal.moveKey(item->index), item);
#if QT_CONFIG(quick_viewtransitions)
        item->transitionNextReposition(transitioner, QQuickItemViewTransitioner::MoveTransition, true);
#endif
    } else {
        // track item so it is released later
        currentChanges.removedItems.insert(QQmlChangeSet::MoveKey(), item);
    }
    if (!removeResult->changedFirstItem && item == *visibleItems.constBegin())
        removeResult->changedFirstItem = true;
}

void QQuickItemViewPrivate::updateSizeChangesBeforeVisiblePos(FxViewItem *item, ChangeResult *removeResult)
{
    removeResult->sizeChangesBeforeVisiblePos += item->size();
}

void QQuickItemViewPrivate::repositionFirstItem(FxViewItem *prevVisibleItemsFirst,
                                                   qreal prevVisibleItemsFirstPos,
                                                   FxViewItem *prevFirstVisible,
                                                   ChangeResult *insertionResult,
                                                   ChangeResult *removalResult)
{
    const QQmlNullableValue<qreal> prevViewPos = insertionResult->visiblePos;

    // reposition visibleItems.first() correctly so that the content y doesn't jump
    if (visibleItems.size()) {
        if (prevVisibleItemsFirst && insertionResult->changedFirstItem)
            resetFirstItemPosition(prevVisibleItemsFirstPos);

        if (prevFirstVisible && prevVisibleItemsFirst == prevFirstVisible
                && prevFirstVisible != *visibleItems.constBegin()) {
            // the previous visibleItems.first() was also the first visible item, and it has been
            // moved/removed, so move the new visibleItems.first() to the pos of the previous one
            if (!insertionResult->changedFirstItem)
                resetFirstItemPosition(prevVisibleItemsFirstPos);

        } else if (prevViewPos.isValid()) {
            qreal moveForwardsBy = 0;
            qreal moveBackwardsBy = 0;

            // shift visibleItems.first() relative to the number of added/removed items
            const auto pos = visibleItems.constFirst()->position();
            if (pos > prevViewPos) {
                moveForwardsBy = insertionResult->sizeChangesAfterVisiblePos;
                moveBackwardsBy = removalResult->sizeChangesAfterVisiblePos;
            } else if (pos < prevViewPos) {
                moveForwardsBy = removalResult->sizeChangesBeforeVisiblePos;
                moveBackwardsBy = insertionResult->sizeChangesBeforeVisiblePos;
            }
            adjustFirstItem(moveForwardsBy, moveBackwardsBy, insertionResult->countChangeBeforeVisible - removalResult->countChangeBeforeVisible);
        }
        insertionResult->reset();
        removalResult->reset();
    }
}

#if QT_CONFIG(quick_viewtransitions)
void QQuickItemViewPrivate::createTransitioner()
{
    if (!transitioner) {
        transitioner = new QQuickItemViewTransitioner;
        transitioner->setChangeListener(this);
    }
}

void QQuickItemViewPrivate::prepareVisibleItemTransitions()
{
    Q_Q(QQuickItemView);
    if (!transitioner)
        return;

    // must call for every visible item to init or discard transitions
    QRectF viewBounds(q->contentX(), q->contentY(), q->width(), q->height());
    for (int i=0; i<visibleItems.size(); i++)
        visibleItems[i]->prepareTransition(transitioner, viewBounds);
}

void QQuickItemViewPrivate::prepareRemoveTransitions(QMultiHash<QQmlChangeSet::MoveKey, FxViewItem *> *removedItems)
{
    if (!transitioner)
        return;

    if (transitioner->canTransition(QQuickItemViewTransitioner::RemoveTransition, true)
            || transitioner->canTransition(QQuickItemViewTransitioner::RemoveTransition, false)) {
        for (auto it = removedItems->begin(); it != removedItems->end(); ) {
            bool isRemove = it.key().moveId < 0;
            if (isRemove) {
                FxViewItem *item = *it;
                item->trackGeometry(false);
                item->releaseAfterTransition = true;
                releasePendingTransition.append(item);
                item->transitionNextReposition(transitioner, QQuickItemViewTransitioner::RemoveTransition, true);
                it = removedItems->erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool QQuickItemViewPrivate::prepareNonVisibleItemTransition(FxViewItem *item, const QRectF &viewBounds)
{
    // Called for items that have been removed from visibleItems and may now be
    // transitioned out of the view. This applies to items that are being directly
    // removed, or moved to outside of the view, as well as those that are
    // displaced to a position outside of the view due to an insert or move.

    if (!transitioner)
        return false;

    if (item->scheduledTransitionType() == QQuickItemViewTransitioner::MoveTransition)
        repositionItemAt(item, item->index, 0);

    bool success = false;
    ACTION_IF_DELETED(item, success = item->prepareTransition(transitioner, viewBounds), return success);

    if (success) {
        item->releaseAfterTransition = true;
        return true;
    }
    return false;
}

void QQuickItemViewPrivate::viewItemTransitionFinished(QQuickItemViewTransitionableItem *item)
{
    for (int i=0; i<releasePendingTransition.size(); i++) {
        if (releasePendingTransition.at(i)->transitionableItem == item) {
            releaseItem(releasePendingTransition.takeAt(i), reusableFlag);
            return;
        }
    }
}
#endif

/*
  This may return 0 if the item is being created asynchronously.
  When the item becomes available, refill() will be called and the item
  will be returned on the next call to createItem().
*/
FxViewItem *QQuickItemViewPrivate::createItem(int modelIndex, QQmlIncubator::IncubationMode incubationMode)
{
    Q_Q(QQuickItemView);

    if (requestedIndex == modelIndex && incubationMode == QQmlIncubator::Asynchronous)
        return nullptr;

#if QT_CONFIG(quick_viewtransitions)
    for (int i=0; i<releasePendingTransition.size(); i++) {
        if (releasePendingTransition.at(i)->index == modelIndex
                && !releasePendingTransition.at(i)->isPendingRemoval()) {
            releasePendingTransition[i]->releaseAfterTransition = false;
            return releasePendingTransition.takeAt(i);
        }
    }
#endif

    inRequest = true;

    // The model will run this same range check internally but produce a warning and return nullptr.
    // Since we handle this result graciously in our code, we preempt this warning by checking the range ourselves.
    QObject* object = modelIndex < model->count() ? model->object(modelIndex, incubationMode) : nullptr;
    QQuickItem *item = qmlobject_cast<QQuickItem*>(object);

    if (!item) {
        if (!object) {
            if (requestedIndex == -1 && model->incubationStatus(modelIndex) == QQmlIncubator::Loading) {
                // The reason we didn't receive an item is because it's incubating async. We keep track
                // of this by assigning the index we're waiting for to 'requestedIndex'. This will e.g. let
                // the view avoid unnecessary layout calls until the item has been loaded.
                requestedIndex = modelIndex;
            }
        } else {
            model->release(object);
            if (!delegateValidated) {
                delegateValidated = true;
                QObject* delegate = q->delegate();
                qmlWarning(delegate ? delegate : q) << QQuickItemView::tr("Delegate must be of Item type");
            }
        }
        inRequest = false;
        return nullptr;
    } else {
        item->setParentItem(q->contentItem());
        if (requestedIndex == modelIndex)
            requestedIndex = -1;
        FxViewItem *viewItem = newViewItem(modelIndex, item);
        if (viewItem) {
            viewItem->index = modelIndex;
            // do other set up for the new item that should not happen
            // until after bindings are evaluated
            initializeViewItem(viewItem);
            unrequestedItems.remove(item);
        }
        inRequest = false;
        return viewItem;
    }
}

void QQuickItemView::createdItem(int index, QObject* object)
{
    Q_D(QQuickItemView);

    QQuickItem* item = qmlobject_cast<QQuickItem*>(object);
    if (!d->inRequest) {
        d->unrequestedItems.insert(item, index);
        d->requestedIndex = -1;
        if (d->hasPendingChanges())
            d->layout();
        else
            d->refill();
        if (d->unrequestedItems.contains(item))
            d->repositionPackageItemAt(item, index);
        else if (index == d->currentIndex)
            d->updateCurrent(index);
    }
}

void QQuickItemView::initItem(int, QObject *object)
{
    QQuickItem* item = qmlobject_cast<QQuickItem*>(object);
    if (item) {
        if (qFuzzyIsNull(item->z()))
            item->setZ(1);
        item->setParentItem(contentItem());
        QQuickItemPrivate::get(item)->setCulled(true);
    }
}

void QQuickItemView::destroyingItem(QObject *object)
{
    Q_D(QQuickItemView);
    QQuickItem* item = qmlobject_cast<QQuickItem*>(object);
    if (item) {
        item->setParentItem(nullptr);
        d->unrequestedItems.remove(item);
    }
}

void QQuickItemView::onItemPooled(int modelIndex, QObject *object)
{
    Q_UNUSED(modelIndex);

    if (auto *attached = d_func()->getAttachedObject(object))
        emit attached->pooled();
}

void QQuickItemView::onItemReused(int modelIndex, QObject *object)
{
    Q_UNUSED(modelIndex);

    if (auto *attached = d_func()->getAttachedObject(object))
        emit attached->reused();
}

bool QQuickItemViewPrivate::releaseItem(FxViewItem *item, QQmlInstanceModel::ReusableFlag reusableFlag)
{
    Q_Q(QQuickItemView);
    if (!item)
        return true;
    if (trackedItem == item)
        trackedItem = nullptr;
    item->trackGeometry(false);

    QQmlInstanceModel::ReleaseFlags flags = {};
    if (model && item->item) {
        flags = model->release(item->item, reusableFlag);
        if (!flags) {
            // item was not destroyed, and we no longer reference it.
            if (item->item->parentItem() == contentItem) {
                // Only cull the item if its parent item is still our contentItem.
                // One case where this can happen is moving an item out of one ObjectModel and into another.
                QQuickItemPrivate::get(item->item)->setCulled(true);
            }
            if (!isClearing)
                unrequestedItems.insert(item->item, model->indexOf(item->item, q));
        } else if (flags & QQmlInstanceModel::Destroyed) {
            item->item->setParentItem(nullptr);
        } else if (flags & QQmlInstanceModel::Pooled) {
            item->setVisible(false);
        }
    }
    delete item;
    return flags != QQmlInstanceModel::Referenced;
}

QQuickItem *QQuickItemViewPrivate::createHighlightItem() const
{
    return createComponentItem(highlightComponent, 0.0, true);
}

QQuickItem *QQuickItemViewPrivate::createComponentItem(QQmlComponent *component, qreal zValue, bool createDefault) const
{
    Q_Q(const QQuickItemView);

    QQuickItem *item = nullptr;
    if (component) {
        QQmlContext *context = component->creationContext();
        if (!context)
            context = qmlContext(q);

        if (QObject *nobj = component->beginCreate(context)) {
            item = qobject_cast<QQuickItem *>(nobj);
            if (!item)
                delete nobj;
        }
    } else if (createDefault) {
        item = new QQuickItem;
    }
    if (item) {
        if (qFuzzyIsNull(item->z()))
            item->setZ(zValue);
        QQml_setParent_noEvent(item, q->contentItem());
        item->setParentItem(q->contentItem());

        initializeComponentItem(item);
    }
    if (component)
        component->completeCreate();
    return item;
}

/*!
    \internal

    Allows derived classes to do any initialization required for \a item
    before completeCreate() is called on it. For example, any attached
    properties required by the item can be set.

    This is similar to initItem(), but as that has logic specific to
    delegate items, we use a separate function for non-delegates.
*/
void QQuickItemViewPrivate::initializeComponentItem(QQuickItem *item) const
{
    Q_UNUSED(item);
}

void QQuickItemViewPrivate::updateTrackedItem()
{
    Q_Q(QQuickItemView);
    FxViewItem *item = currentItem;
    if (highlight)
        item = highlight.get();
    trackedItem = item;

    if (trackedItem)
        q->trackedPositionChanged();
}

void QQuickItemViewPrivate::updateUnrequestedIndexes()
{
    Q_Q(QQuickItemView);
    for (QHash<QQuickItem*,int>::iterator it = unrequestedItems.begin(), end = unrequestedItems.end(); it != end; ++it)
        *it = model->indexOf(it.key(), q);
}

void QQuickItemViewPrivate::updateUnrequestedPositions()
{
    for (QHash<QQuickItem*,int>::const_iterator it = unrequestedItems.cbegin(), cend = unrequestedItems.cend(); it != cend; ++it) {
        if (it.value() >= 0)
            repositionPackageItemAt(it.key(), it.value());
    }
}

void QQuickItemViewPrivate::updateVisibleIndex()
{
    typedef QList<FxViewItem*>::const_iterator FxViewItemListConstIt;

    visibleIndex = 0;
    for (FxViewItemListConstIt it = visibleItems.constBegin(), cend = visibleItems.constEnd(); it != cend; ++it) {
        if ((*it)->index != -1) {
            visibleIndex = (*it)->index;
            break;
        }
    }
}

QT_END_NAMESPACE

#include "moc_qquickitemview_p.cpp"
