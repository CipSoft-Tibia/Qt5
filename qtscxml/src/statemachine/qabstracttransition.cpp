// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qabstracttransition.h"
#include "qabstracttransition_p.h"
#include "qabstractstate.h"
#include "qhistorystate.h"
#include "qstate.h"
#include "qstatemachine.h"

QT_BEGIN_NAMESPACE

/*!
  \class QAbstractTransition
  \inmodule QtStateMachine

  \brief The QAbstractTransition class is the base class of transitions between QAbstractState objects.

  \since 4.6
  \ingroup statemachine

  The QAbstractTransition class is the abstract base class of transitions
  between states (QAbstractState objects) of a
  QStateMachine. QAbstractTransition is part of \l{Qt State Machine Overview}
  {Qt State Machine Framework}.

  The sourceState() function returns the source of the transition. The
  targetStates() function returns the targets of the transition. The machine()
  function returns the state machine that the transition is part of.

  The triggered() signal is emitted when the transition has been triggered.

  Transitions can cause animations to be played. Use the addAnimation()
  function to add an animation to the transition.

  \section1 Subclassing

  The eventTest() function is called by the state machine to determine whether
  an event should trigger the transition. In your reimplementation you
  typically check the event type and cast the event object to the proper type,
  and check that one or more properties of the event meet your criteria.

  The onTransition() function is called when the transition is triggered;
  reimplement this function to perform custom processing for the transition.
*/

/*!
    \property QAbstractTransition::sourceState

    \brief the source state (parent) of this transition
*/

/*!
    \property QAbstractTransition::targetState

    \brief the target state of this transition

    If a transition has no target state, the transition may still be
    triggered, but this will not cause the state machine's configuration to
    change (i.e. the current state will not be exited and re-entered).
*/

/*!
    \property QAbstractTransition::targetStates

    \brief the target states of this transition

    If multiple states are specified, all must be descendants of the same
    parallel group state.
*/

/*!
    \property QAbstractTransition::transitionType

    \brief indicates whether this transition is an internal transition, or an external transition.

    Internal and external transitions behave the same, except for the case of a transition whose
    source state is a compound state and whose target(s) is a descendant of the source. In such a
    case, an internal transition will not exit and re-enter its source state, while an external one
    will.

    By default, the type is an external transition.
*/

/*!
  \enum QAbstractTransition::TransitionType

  This enum specifies the kind of transition. By default, the type is an external transition.

  \value ExternalTransition Any state that is the source state of a transition (which is not a
                            target-less transition) is left, and re-entered when necessary.
  \value InternalTransition If the target state of a transition is a sub-state of a compound state,
                            and that compound state is the source state, an internal transition will
                            not leave the source state.

  \sa QAbstractTransition::transitionType
*/

QStateMachine *QAbstractTransitionPrivate::machine() const
{
    if (QState *source = sourceState())
        return source->machine();
    Q_Q(const QAbstractTransition);
    if (QHistoryState *parent = qobject_cast<QHistoryState *>(q->parent()))
        return parent->machine();
    return nullptr;
}

bool QAbstractTransitionPrivate::callEventTest(QEvent *e)
{
    Q_Q(QAbstractTransition);
    return q->eventTest(e);
}

void QAbstractTransitionPrivate::callOnTransition(QEvent *e)
{
    Q_Q(QAbstractTransition);
    q->onTransition(e);
}

QState *QAbstractTransitionPrivate::sourceState() const
{
    return qobject_cast<QState*>(parent);
}

void QAbstractTransitionPrivate::emitTriggered()
{
    Q_Q(QAbstractTransition);
    emit q->triggered(QAbstractTransition::QPrivateSignal());
}

/*!
  Constructs a new QAbstractTransition object with the given \a sourceState.
*/
QAbstractTransition::QAbstractTransition(QState *sourceState)
    : QObject(*new QAbstractTransitionPrivate, sourceState)
{
}

/*!
  \internal
*/
QAbstractTransition::QAbstractTransition(QAbstractTransitionPrivate &dd,
                                         QState *parent)
    : QObject(dd, parent)
{
}

/*!
  Destroys this transition.
*/
QAbstractTransition::~QAbstractTransition()
{
}

/*!
  Returns the source state of this transition, or \nullptr if this
  transition has no source state.
*/
QState *QAbstractTransition::sourceState() const
{
    Q_D(const QAbstractTransition);
    return d->sourceState();
}

/*!
  Returns the target state of this transition, or \nullptr if the
  transition has no target.
*/
QAbstractState *QAbstractTransition::targetState() const
{
    Q_D(const QAbstractTransition);
    if (d->targetStates.isEmpty())
        return nullptr;
    return d->targetStates.first().data();
}

/*!
  Sets the \a target state of this transition.
*/
void QAbstractTransition::setTargetState(QAbstractState* target)
{
    Q_D(QAbstractTransition);
    if ((d->targetStates.size() == 1 && target == d->targetStates.at(0).data()) ||
         (d->targetStates.isEmpty() && target == nullptr)) {
        return;
    }
    if (!target)
        d->targetStates.clear();
    else
        setTargetStates(QList<QAbstractState*>() << target);
    emit targetStateChanged(QPrivateSignal());
}

/*!
  Returns the target states of this transition, or an empty list if this
  transition has no target states.
*/
QList<QAbstractState*> QAbstractTransition::targetStates() const
{
    Q_D(const QAbstractTransition);
    QList<QAbstractState*> result;
    for (int i = 0; i < d->targetStates.size(); ++i) {
        QAbstractState *target = d->targetStates.at(i).data();
        if (target)
            result.append(target);
    }
    return result;
}

/*!
  Sets the target states of this transition to be the given \a targets.
*/
void QAbstractTransition::setTargetStates(const QList<QAbstractState*> &targets)
{
    Q_D(QAbstractTransition);

    // Verify if any of the new target states is a null-pointer:
    for (int i = 0; i < targets.size(); ++i) {
        if (targets.at(i) == nullptr) {
            qWarning("QAbstractTransition::setTargetStates: target state(s) cannot be null");
            return;
        }
    }

    // First clean out any target states that got destroyed, but for which we still have a QPointer
    // around.
    for (int i = 0; i < d->targetStates.size(); ) {
        if (d->targetStates.at(i).isNull()) {
            d->targetStates.remove(i);
        } else {
            ++i;
        }
    }

    // Easy check: if both lists are empty, we're done.
    if (targets.isEmpty() && d->targetStates.isEmpty())
        return;

    bool sameList = true;

    if (targets.size() != d->targetStates.size()) {
        // If the sizes of the lists are different, we don't need to be smart: they're different. So
        // we can just set the new list as the targetStates.
        sameList = false;
    } else {
        QList<QPointer<QAbstractState>> copy(d->targetStates);
        for (int i = 0; i < targets.size(); ++i) {
            sameList &= copy.removeOne(targets.at(i));
            if (!sameList)
                break; // ok, we now know the lists are not the same, so stop the loop.
        }

        sameList &= copy.isEmpty();
    }

    if (sameList)
        return;

    d->targetStates.resize(targets.size());
    for (int i = 0; i < targets.size(); ++i) {
        d->targetStates[i] = targets.at(i);
    }

    emit targetStatesChanged(QPrivateSignal());
}

/*!
  Returns the type of the transition.
*/
QAbstractTransition::TransitionType QAbstractTransition::transitionType() const
{
    Q_D(const QAbstractTransition);
    return d->transitionType;
}

/*!
  Sets the type of the transition to \a type.
*/
void QAbstractTransition::setTransitionType(TransitionType type)
{
    Q_D(QAbstractTransition);
    d->transitionType = type;
}

QBindable<QAbstractTransition::TransitionType> QAbstractTransition::bindableTransitionType()
{
    Q_D(QAbstractTransition);
    return &d->transitionType;
}

/*!
  Returns the state machine that this transition is part of, or
  \nullptr if the transition is not part of a state machine.
*/
QStateMachine *QAbstractTransition::machine() const
{
    Q_D(const QAbstractTransition);
    return d->machine();
}

#if QT_CONFIG(animation)

/*!
  Adds the given \a animation to this transition.
  The transition does not take ownership of the animation.

  \sa removeAnimation(), animations()
*/
void QAbstractTransition::addAnimation(QAbstractAnimation *animation)
{
    Q_D(QAbstractTransition);
    if (!animation) {
        qWarning("QAbstractTransition::addAnimation: cannot add null animation");
        return;
    }
    d->animations.append(animation);
}

/*!
  Removes the given \a animation from this transition.

  \sa addAnimation()
*/
void QAbstractTransition::removeAnimation(QAbstractAnimation *animation)
{
    Q_D(QAbstractTransition);
    if (!animation) {
        qWarning("QAbstractTransition::removeAnimation: cannot remove null animation");
        return;
    }
    d->animations.removeOne(animation);
}

/*!
  Returns the list of animations associated with this transition, or an empty
  list if it has no animations.

  \sa addAnimation()
*/
QList<QAbstractAnimation*> QAbstractTransition::animations() const
{
    Q_D(const QAbstractTransition);
    return d->animations;
}

#endif

/*!
  \fn QAbstractTransition::eventTest(QEvent *event)

  This function is called to determine whether the given \a event should cause
  this transition to trigger. Reimplement this function and return true if the
  event should trigger the transition, otherwise return false.
*/

/*!
  \fn QAbstractTransition::onTransition(QEvent *event)

  This function is called when the transition is triggered. The given \a event
  is what caused the transition to trigger. Reimplement this function to
  perform custom processing when the transition is triggered.
*/

/*!
  \fn QAbstractTransition::triggered()

  This signal is emitted when the transition has been triggered (after
  onTransition() has been called).
*/

/*!
  \fn QAbstractTransition::targetStateChanged()
  \since 5.4

  This signal is emitted when the targetState property is changed.

  \sa QAbstractTransition::targetState
*/

/*!
  \fn QAbstractTransition::targetStatesChanged()
  \since 5.4

  This signal is emitted when the targetStates property is changed.

  \sa QAbstractTransition::targetStates
*/

/*!
  \reimp
*/
bool QAbstractTransition::event(QEvent *e)
{
    return QObject::event(e);
}

QT_END_NAMESPACE

#include "moc_qabstracttransition.cpp"
