// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhistorystate.h"
#include "qhistorystate_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QHistoryState
  \inmodule QtStateMachine

  \brief The QHistoryState class provides a means of returning to a previously active substate.

  \since 4.6
  \ingroup statemachine

  A history state is a pseudo-state that represents the child state that the
  parent state was in the last time the parent state was exited. A transition
  with a history state as its target is in fact a transition to one or more
  other child states of the parent state. QHistoryState is part of \l{Qt State Machine Overview}
  {Qt State Machine Framework}.

  Use the setDefaultState() function to set the state that should be entered
  if the parent state has never been entered.  Example:

  \code
  QStateMachine machine;

  QState *s1 = new QState();
  QState *s11 = new QState(s1);
  QState *s12 = new QState(s1);

  QHistoryState *s1h = new QHistoryState(s1);
  s1h->setDefaultState(s11);

  machine.addState(s1);

  QState *s2 = new QState();
  machine.addState(s2);

  QPushButton *button = new QPushButton();
  // Clicking the button will cause the state machine to enter the child state
  // that s1 was in the last time s1 was exited, or the history state's default
  // state if s1 has never been entered.
  s1->addTransition(button, SIGNAL(clicked()), s1h);
  \endcode

  If more than one default state has to be entered, or if the transition to the default state(s)
  has to be acted upon, the defaultTransition should be set instead. Note that the eventTest()
  method of that transition will never be called: the selection and execution of the transition is
  done automatically when entering the history state.

  By default a history state is shallow, meaning that it won't remember nested
  states. This can be configured through the historyType property.
*/

/*!
  \property QHistoryState::defaultTransition

  \brief the default transition of this history state
*/

/*!
  \property QHistoryState::defaultState

  \brief the default state of this history state
*/

/*!
  \property QHistoryState::historyType

  \brief the type of history that this history state records

  The default value of this property is QHistoryState::ShallowHistory.
*/

/*!
  \enum QHistoryState::HistoryType

  This enum specifies the type of history that a QHistoryState records.

  \value ShallowHistory Only the immediate child states of the parent state
  are recorded. In this case a transition with the history state as its
  target will end up in the immediate child state that the parent was in the
  last time it was exited. This is the default.

  \value DeepHistory Nested states are recorded. In this case a transition
  with the history state as its target will end up in the most deeply nested
  descendant state the parent was in the last time it was exited.
*/

namespace {
class DefaultStateTransition: public QAbstractTransition
{
    Q_OBJECT

public:
    DefaultStateTransition(QHistoryState *source, QAbstractState *target);

protected:
    // It doesn't matter whether this transition matches any event or not. It is always associated
    // with a QHistoryState, and as soon as the state-machine detects that it enters a history
    // state, it will handle this transition as a special case. The history state itself is never
    // entered either: either the stored configuration will be used, or the target(s) of this
    // transition are used.
    bool eventTest(QEvent *event)  override { Q_UNUSED(event); return false; }
    void onTransition(QEvent *event) override { Q_UNUSED(event); }
};
}

QHistoryStatePrivate::QHistoryStatePrivate()
    : QAbstractStatePrivate(HistoryState)
    , defaultTransition(nullptr)
    , historyType(QHistoryState::ShallowHistory)
{
}

DefaultStateTransition::DefaultStateTransition(QHistoryState *source, QAbstractState *target)
    : QAbstractTransition()
{
    setParent(source);
    setTargetState(target);
}

/*!
  Constructs a new shallow history state with the given \a parent state.
*/
QHistoryState::QHistoryState(QState *parent)
    : QAbstractState(*new QHistoryStatePrivate, parent)
{
}
/*!
  Constructs a new history state of the given \a type, with the given \a
  parent state.
*/
QHistoryState::QHistoryState(HistoryType type, QState *parent)
    : QAbstractState(*new QHistoryStatePrivate, parent)
{
    Q_D(QHistoryState);
    d->historyType = type;
}

/*!
  Destroys this history state.
*/
QHistoryState::~QHistoryState()
{
}

/*!
  Returns this history state's default transition. The default transition is
  taken when the history state has never been entered before. The target states
  of the default transition therefore make up the default state.

  \since 5.6
*/
QAbstractTransition *QHistoryState::defaultTransition() const
{
    Q_D(const QHistoryState);
    return d->defaultTransition;
}

/*!
  Sets this history state's default transition to be the given \a transition.
  This will set the source state of the \a transition to the history state.

  Note that the eventTest method of the \a transition will never be called.

  \since 5.6
*/
void QHistoryState::setDefaultTransition(QAbstractTransition *transition)
{
    Q_D(QHistoryState);
    d->defaultTransition.removeBindingUnlessInWrapper();
    if (d->defaultTransition.valueBypassingBindings() == transition)
        return;
    d->defaultTransition.setValueBypassingBindings(transition);
    if (transition)
        transition->setParent(this);
    d->defaultTransition.notify();
    emit defaultTransitionChanged(QHistoryState::QPrivateSignal());
}

QBindable<QAbstractTransition*> QHistoryState::bindableDefaultTransition()
{
    Q_D(QHistoryState);
    return &d->defaultTransition;
}

/*!
  Returns this history state's default state.  The default state indicates the
  state to transition to if the parent state has never been entered before.
*/
QAbstractState *QHistoryState::defaultState() const
{
    Q_D(const QHistoryState);
    return d->defaultTransition.value() ? d->defaultTransition->targetState() : nullptr;
}

static inline bool isSoleEntry(const QList<QAbstractState*> &states, const QAbstractState * state)
{
    return states.size() == 1 && states.first() == state;
}

/*!
  Sets this history state's default state to be the given \a state.
  \a state must be a sibling of this history state.

  Note that this function does not set \a state as the initial state
  of its parent.
*/
void QHistoryState::setDefaultState(QAbstractState *state)
{
    Q_D(QHistoryState);
    if (state && state->parentState() != parentState()) {
        qWarning("QHistoryState::setDefaultState: state %p does not belong "
                 "to this history state's group (%p)", state, parentState());
        return;
    }
    // evaluate the binding once
    auto *defaultTransition = d->defaultTransition.value();
    if (!defaultTransition || !isSoleEntry(defaultTransition->targetStates(), state)) {
        if (!defaultTransition || !qobject_cast<DefaultStateTransition*>(defaultTransition))
            d->defaultTransition.setValue(new DefaultStateTransition(this, state));
        else
            defaultTransition->setTargetState(state);
        emit defaultStateChanged(QHistoryState::QPrivateSignal());
    }
}

/*!
  Returns the type of history that this history state records.
*/
QHistoryState::HistoryType QHistoryState::historyType() const
{
    Q_D(const QHistoryState);
    return d->historyType;
}

/*!
  Sets the \a type of history that this history state records.
*/
void QHistoryState::setHistoryType(HistoryType type)
{
    Q_D(QHistoryState);
    d->historyType = type;
}

QBindable<QHistoryState::HistoryType> QHistoryState::bindableHistoryType()
{
    Q_D(QHistoryState);
    return &d->historyType;
}

/*!
  \reimp
*/
void QHistoryState::onEntry(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
void QHistoryState::onExit(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
bool QHistoryState::event(QEvent *e)
{
    return QAbstractState::event(e);
}

/*!
  \fn QHistoryState::defaultStateChanged()
  \since 5.4

  This signal is emitted when the defaultState property is changed.

  \sa QHistoryState::defaultState
*/

/*!
  \fn QHistoryState::historyTypeChanged()
  \since 5.4

  This signal is emitted when the historyType property is changed.

  \sa QHistoryState::historyType
*/

/*!
  \fn QHistoryState::defaultTransitionChanged()
  \since 5.6

  This signal is emitted when the defaultTransition property is changed.

  \sa QHistoryState::defaultTransition
*/

QT_END_NAMESPACE

#include "moc_qhistorystate.cpp"
#include "qhistorystate.moc"
