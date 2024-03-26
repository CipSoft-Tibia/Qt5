// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktransitionmanager_p_p.h"

#include "qquicktransition_p.h"
#include "qquickstate_p_p.h"

#include <private/qqmlbinding_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlproperty_p.h>

#include <QtCore/qdebug.h>
#include <private/qanimationjobutil_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcStates)

class QQuickTransitionManagerPrivate
{
public:
    QQuickTransitionManagerPrivate()
        : state(nullptr), transitionInstance(nullptr) {}

    void applyBindings();
    typedef QList<QQuickSimpleAction> SimpleActionList;
    QQuickState *state;
    QQuickTransitionInstance *transitionInstance;
    QQuickStateOperation::ActionList bindingsList;
    SimpleActionList completeList;
};

QQuickTransitionManager::QQuickTransitionManager()
: d(new QQuickTransitionManagerPrivate)
{
}

void QQuickTransitionManager::setState(QQuickState *s)
{
    d->state = s;
}

QQuickTransitionManager::~QQuickTransitionManager()
{
    delete d->transitionInstance;
    d->transitionInstance = nullptr;
    delete d; d = nullptr;
}

bool QQuickTransitionManager::isRunning() const
{
    return d->transitionInstance && d->transitionInstance->isRunning();
}

void QQuickTransitionManager::complete()
{
    d->applyBindings();

    // Explicitly take a copy in case the write action triggers a script that modifies the list.
    QQuickTransitionManagerPrivate::SimpleActionList completeListCopy = d->completeList;
    for (const QQuickSimpleAction &action : std::as_const(completeListCopy))
        action.property().write(action.value());

    d->completeList.clear();

    if (d->state)
        static_cast<QQuickStatePrivate*>(QObjectPrivate::get(d->state))->complete();

    finished();
}

void QQuickTransitionManagerPrivate::applyBindings()
{
    for (const QQuickStateAction &action : std::as_const(bindingsList)) {
        if (auto binding = action.toBinding; binding) {
            binding.installOn(action.property, QQmlAnyBinding::RespectInterceptors);
        } else if (action.event) {
            if (action.reverseEvent)
                action.event->reverse();
            else
                action.event->execute();
        }

    }

    bindingsList.clear();
}

void QQuickTransitionManager::finished()
{
}

void QQuickTransitionManager::transition(const QList<QQuickStateAction> &list,
                                      QQuickTransition *transition,
                                      QObject *defaultTarget)
{
    RETURN_IF_DELETED(cancel());

    // The copy below is ON PURPOSE, because firing actions might involve scripts that modify the list.
    QQuickStateOperation::ActionList applyList = list;

    // Determine which actions are binding changes and disable any current bindings
    for (const QQuickStateAction &action : std::as_const(applyList)) {
        if (action.toBinding)
            d->bindingsList << action;
        if (action.fromBinding) {
            auto property = action.property;
            QQmlAnyBinding::removeBindingFrom(property); // Disable current binding
        }
        if (action.event && action.event->changesBindings()) {  //### assume isReversable()?
            d->bindingsList << action;
            action.event->clearBindings();
        }
    }

    // Animated transitions need both the start and the end value for
    // each property change.  In the presence of bindings, the end values
    // are non-trivial to calculate.  As a "best effort" attempt, we first
    // apply all the property and binding changes, then read all the actual
    // final values, then roll back the changes and proceed as normal.
    //
    // This doesn't catch everything, and it might be a little fragile in
    // some cases - but whatcha going to do?
    if (transition && !d->bindingsList.isEmpty()) {

        // Apply all the property and binding changes
        for (const QQuickStateAction &action : std::as_const(applyList)) {
            if (auto binding = action.toBinding; binding) {
                binding.installOn(action.property);
            } else if (!action.event) {
                QQmlPropertyPrivate::write(action.property, action.toValue, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
            } else if (action.event->isReversable()) {
                if (action.reverseEvent)
                    action.event->reverse();
                else
                    action.event->execute();
            }
        }

        // Read all the end values for binding changes.
        for (auto it = applyList.begin(), eit = applyList.end(); it != eit; ++it) {
            if (it->event) {
                it->event->saveTargetValues();
                continue;
            }
            const QQmlProperty &prop = it->property;
            if (it->toBinding || !it->toValue.isValid())
                it->toValue = prop.read();
        }

        // Revert back to the original values
        for (const QQuickStateAction &action : std::as_const(applyList)) {
            if (action.event) {
                if (action.event->isReversable()) {
                    action.event->clearBindings();
                    action.event->rewind();
                    action.event->clearBindings();  //### shouldn't be needed
                }
                continue;
            }

            if (action.toBinding) {
                auto property = action.property;
                QQmlAnyBinding::removeBindingFrom(property); // Make sure this is disabled during the transition
            }

            QQmlPropertyPrivate::write(action.property, action.fromValue, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
        }
    }

    if (transition) {
        QList<QQmlProperty> touched;
        QQuickTransitionInstance *oldInstance = d->transitionInstance;
        d->transitionInstance = transition->prepare(applyList, touched, this, defaultTarget);
        d->transitionInstance->start();
        if (oldInstance && oldInstance != d->transitionInstance)
            delete oldInstance;

        // Modify the action list to remove actions handled in the transition
        auto isHandledInTransition = [this, touched](const QQuickStateAction &action) {
            if (action.event) {
                return action.actionDone;
            } else {
                if (touched.contains(action.property)) {
                    if (action.toValue != action.fromValue)
                        d->completeList << QQuickSimpleAction(action, QQuickSimpleAction::EndState);
                    return true;
                }
            }
            return false;
        };
        auto newEnd = std::remove_if(applyList.begin(), applyList.end(), isHandledInTransition);
        applyList.erase(newEnd, applyList.end());
    }

    // Any actions remaining have not been handled by the transition and should
    // be applied immediately.  We skip applying bindings, as they are all
    // applied at the end in applyBindings() to avoid any nastiness mid
    // transition
    for (const QQuickStateAction &action : std::as_const(applyList)) {
        if (action.event && !action.event->changesBindings()) {
            if (action.event->isReversable() && action.reverseEvent)
                action.event->reverse();
            else
                action.event->execute();
        } else if (!action.event && !action.toBinding) {
            action.property.write(action.toValue);
        }
    }
    if (lcStates().isDebugEnabled()) {
        for (const QQuickStateAction &action : std::as_const(applyList)) {
            if (action.event)
                qCDebug(lcStates) << "no transition for event:" << action.event->type();
            else
                qCDebug(lcStates) << "no transition for:" << action.property.object()
                                  << action.property.name() << "from:" << action.fromValue
                                  << "to:" << action.toValue;
        }
    }

    if (!transition)
        complete();
}

void QQuickTransitionManager::cancel()
{
    if (d->transitionInstance && d->transitionInstance->isRunning())
        RETURN_IF_DELETED(d->transitionInstance->stop());

    for (const QQuickStateAction &action : std::as_const(d->bindingsList)) {
        if (action.toBinding && action.deletableToBinding) {
            auto property = action.property;
            QQmlAnyBinding::removeBindingFrom(property);
        } else if (action.event) {
            //### what do we do here?
        }

    }
    d->bindingsList.clear();
    d->completeList.clear();
}

QT_END_NAMESPACE
