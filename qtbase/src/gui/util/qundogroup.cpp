// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qundogroup.h"
#include "qundostack.h"
#include "qundostack_p.h"

QT_BEGIN_NAMESPACE

class QUndoGroupPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QUndoGroup)
public:
    QUndoGroupPrivate() : active(nullptr) {}

    QUndoStack *active;
    QList<QUndoStack*> stack_list;
};

/*!
    \class QUndoGroup
    \brief The QUndoGroup class is a group of QUndoStack objects.
    \since 4.2
    \inmodule QtGui

    For an overview of the Qt's undo framework, see the
    \l{qundo.html}{overview}.

    An application often has multiple undo stacks, one for each opened document. At the
    same time, an application usually has one undo action and one redo action, which
    triggers undo or redo in the active document.

    QUndoGroup is a group of QUndoStack objects, one of which may be active. It has
    an undo() and redo() slot, which calls QUndoStack::undo() and QUndoStack::redo()
    for the active stack. It also has the functions createUndoAction() and createRedoAction().
    The actions returned by these functions behave in the same way as those returned by
    QUndoStack::createUndoAction() and QUndoStack::createRedoAction() of the active
    stack.

    Stacks are added to a group with addStack() and removed with removeStack(). A stack
    is implicitly added to a group when it is created with the group as its parent
    QObject.

    It is the programmer's responsibility to specify which stack is active by
    calling QUndoStack::setActive(), usually when the associated document window receives focus.
    The active stack may also be set with setActiveStack(), and is returned by activeStack().

    When a stack is added to a group using addStack(), the group does not take ownership
    of the stack. This means the stack has to be deleted separately from the group. When
    a stack is deleted, it is automatically removed from a group. A stack may belong to
    only one group. Adding it to another group will cause it to be removed from the previous
    group.

    A QUndoGroup is also useful in conjunction with QUndoView. If a QUndoView is
    set to watch a group using QUndoView::setGroup(), it will update itself to display
    the active stack.
*/

/*!
    Creates an empty QUndoGroup object with parent \a parent.

    \sa addStack()
*/

QUndoGroup::QUndoGroup(QObject *parent)
    : QObject(*new QUndoGroupPrivate(), parent)
{
}

/*!
    Destroys the QUndoGroup.
*/
QUndoGroup::~QUndoGroup()
{
    // Ensure all QUndoStacks no longer refer to this group.
    Q_D(QUndoGroup);
    QList<QUndoStack *>::iterator it = d->stack_list.begin();
    QList<QUndoStack *>::iterator end = d->stack_list.end();
    while (it != end) {
        (*it)->d_func()->group = nullptr;
        ++it;
    }
}

/*!
    Adds \a stack to this group. The group does not take ownership of the stack. Another
    way of adding a stack to a group is by specifying the group as the stack's parent
    QObject in QUndoStack::QUndoStack(). In this case, the stack is deleted when the
    group is deleted, in the usual manner of QObjects.

    \sa removeStack(), stacks(), QUndoStack::QUndoStack()
*/

void QUndoGroup::addStack(QUndoStack *stack)
{
    Q_D(QUndoGroup);

    if (d->stack_list.contains(stack))
        return;
    d->stack_list.append(stack);

    if (QUndoGroup *other = stack->d_func()->group)
        other->removeStack(stack);
    stack->d_func()->group = this;
}

/*!
    Removes \a stack from this group. If the stack was the active stack in the group,
    the active stack becomes 0.

    \sa addStack(), stacks(), QUndoStack::~QUndoStack()
*/

void QUndoGroup::removeStack(QUndoStack *stack)
{
    Q_D(QUndoGroup);

    if (d->stack_list.removeAll(stack) == 0)
        return;
    if (stack == d->active)
        setActiveStack(nullptr);
    stack->d_func()->group = nullptr;
}

/*!
    Returns a list of stacks in this group.

    \sa addStack(), removeStack()
*/

QList<QUndoStack*> QUndoGroup::stacks() const
{
    Q_D(const QUndoGroup);
    return d->stack_list;
}

/*!
    Sets the active stack of this group to \a stack.

    If the stack is not a member of this group, this function does nothing.

    Synonymous with calling QUndoStack::setActive() on \a stack.

    The actions returned by createUndoAction() and createRedoAction() will now behave
    in the same way as those returned by \a stack's QUndoStack::createUndoAction()
    and QUndoStack::createRedoAction().

    \sa QUndoStack::setActive(), activeStack()
*/

void QUndoGroup::setActiveStack(QUndoStack *stack)
{
    Q_D(QUndoGroup);
    if (d->active == stack)
        return;

    if (d->active != nullptr) {
        disconnect(d->active, SIGNAL(canUndoChanged(bool)),
                    this, SIGNAL(canUndoChanged(bool)));
        disconnect(d->active, SIGNAL(undoTextChanged(QString)),
                    this, SIGNAL(undoTextChanged(QString)));
        disconnect(d->active, SIGNAL(canRedoChanged(bool)),
                    this, SIGNAL(canRedoChanged(bool)));
        disconnect(d->active, SIGNAL(redoTextChanged(QString)),
                    this, SIGNAL(redoTextChanged(QString)));
        disconnect(d->active, SIGNAL(indexChanged(int)),
                    this, SIGNAL(indexChanged(int)));
        disconnect(d->active, SIGNAL(cleanChanged(bool)),
                    this, SIGNAL(cleanChanged(bool)));
    }

    d->active = stack;

    if (d->active == nullptr) {
        emit canUndoChanged(false);
        emit undoTextChanged(QString());
        emit canRedoChanged(false);
        emit redoTextChanged(QString());
        emit cleanChanged(true);
        emit indexChanged(0);
    } else {
        connect(d->active, SIGNAL(canUndoChanged(bool)),
                this, SIGNAL(canUndoChanged(bool)));
        connect(d->active, SIGNAL(undoTextChanged(QString)),
                this, SIGNAL(undoTextChanged(QString)));
        connect(d->active, SIGNAL(canRedoChanged(bool)),
                this, SIGNAL(canRedoChanged(bool)));
        connect(d->active, SIGNAL(redoTextChanged(QString)),
                this, SIGNAL(redoTextChanged(QString)));
        connect(d->active, SIGNAL(indexChanged(int)),
                this, SIGNAL(indexChanged(int)));
        connect(d->active, SIGNAL(cleanChanged(bool)),
                this, SIGNAL(cleanChanged(bool)));
        emit canUndoChanged(d->active->canUndo());
        emit undoTextChanged(d->active->undoText());
        emit canRedoChanged(d->active->canRedo());
        emit redoTextChanged(d->active->redoText());
        emit cleanChanged(d->active->isClean());
        emit indexChanged(d->active->index());
    }

    emit activeStackChanged(d->active);
}

/*!
    Returns the active stack of this group.

    If none of the stacks are active, or if the group is empty, this function
    returns \nullptr.

    \sa setActiveStack(), QUndoStack::setActive()
*/

QUndoStack *QUndoGroup::activeStack() const
{
    Q_D(const QUndoGroup);
    return d->active;
}

#ifndef QT_NO_ACTION

/*!
    Creates an undo QAction object with parent \a parent.

    Triggering this action will cause a call to QUndoStack::undo() on the active stack.
    The text of this action will always be the text of the command which will be undone
    in the next call to undo(), prefixed by \a prefix. If there is no command available
    for undo, if the group is empty or if none of the stacks are active, this action will
    be disabled.

    If \a prefix is empty, the default template "Undo %1" is used instead of prefix.
    Before Qt 4.8, the prefix "Undo" was used by default.

    \sa createRedoAction(), canUndo(), QUndoCommand::text()
*/

QAction *QUndoGroup::createUndoAction(QObject *parent, const QString &prefix) const
{
    QAction *action = new QAction(parent);
    action->setEnabled(canUndo());

    QString effectivePrefix = prefix;
    QString defaultText;
    if (prefix.isEmpty()) {
        effectivePrefix = tr("Undo %1");
        defaultText = tr("Undo", "Default text for undo action");
    }

    QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, undoText());

    connect(this, &QUndoGroup::canUndoChanged, action, &QAction::setEnabled);
    connect(this, &QUndoGroup::undoTextChanged, action, [=](const QString &text) {
        QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, text);
    });
    connect(action, &QAction::triggered, this, &QUndoGroup::undo);

    return action;
}

/*!
    Creates an redo QAction object with parent \a parent.

    Triggering this action will cause a call to QUndoStack::redo() on the active stack.
    The text of this action will always be the text of the command which will be redone
    in the next call to redo(), prefixed by \a prefix. If there is no command available
    for redo, if the group is empty or if none of the stacks are active, this action will
    be disabled.

    If \a prefix is empty, the default template "Redo %1" is used instead of prefix.
    Before Qt 4.8, the prefix "Redo" was used by default.

    \sa createUndoAction(), canRedo(), QUndoCommand::text()
*/

QAction *QUndoGroup::createRedoAction(QObject *parent, const QString &prefix) const
{
    QAction *action = new QAction(parent);
    action->setEnabled(canRedo());

    QString effectivePrefix = prefix;
    QString defaultText;
    if (prefix.isEmpty()) {
        effectivePrefix = tr("Redo %1");
        defaultText = tr("Redo", "Default text for redo action");
    }

    QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, redoText());

    connect(this, &QUndoGroup::canRedoChanged, action, &QAction::setEnabled);
    connect(this, &QUndoGroup::redoTextChanged, action, [=](const QString &text) {
        QUndoStackPrivate::setPrefixedText(action, effectivePrefix, defaultText, text);
    });
    connect(action, &QAction::triggered, this, &QUndoGroup::redo);
    return action;
}

#endif // QT_NO_ACTION

/*!
    Calls QUndoStack::undo() on the active stack.

    If none of the stacks are active, or if the group is empty, this function
    does nothing.

    \sa redo(), canUndo(), setActiveStack()
*/

void QUndoGroup::undo()
{
    Q_D(QUndoGroup);
    if (d->active != nullptr)
        d->active->undo();
}

/*!
    Calls QUndoStack::redo() on the active stack.

    If none of the stacks are active, or if the group is empty, this function
    does nothing.

    \sa undo(), canRedo(), setActiveStack()
*/


void QUndoGroup::redo()
{
    Q_D(QUndoGroup);
    if (d->active != nullptr)
        d->active->redo();
}

/*!
    Returns the value of the active stack's QUndoStack::canUndo().

    If none of the stacks are active, or if the group is empty, this function
    returns \c false.

    \sa canRedo(), setActiveStack()
*/

bool QUndoGroup::canUndo() const
{
    Q_D(const QUndoGroup);
    return d->active != nullptr && d->active->canUndo();
}

/*!
    Returns the value of the active stack's QUndoStack::canRedo().

    If none of the stacks are active, or if the group is empty, this function
    returns \c false.

    \sa canUndo(), setActiveStack()
*/

bool QUndoGroup::canRedo() const
{
    Q_D(const QUndoGroup);
    return d->active != nullptr && d->active->canRedo();
}

/*!
    Returns the value of the active stack's QUndoStack::undoText().

    If none of the stacks are active, or if the group is empty, this function
    returns an empty string.

    \sa redoText(), setActiveStack()
*/

QString QUndoGroup::undoText() const
{
    Q_D(const QUndoGroup);
    return d->active == nullptr ? QString() : d->active->undoText();
}

/*!
    Returns the value of the active stack's QUndoStack::redoText().

    If none of the stacks are active, or if the group is empty, this function
    returns an empty string.

    \sa undoText(), setActiveStack()
*/

QString QUndoGroup::redoText() const
{
    Q_D(const QUndoGroup);
    return d->active == nullptr ? QString() : d->active->redoText();
}

/*!
    Returns the value of the active stack's QUndoStack::isClean().

    If none of the stacks are active, or if the group is empty, this function
    returns \c true.

    \sa setActiveStack()
*/

bool QUndoGroup::isClean() const
{
    Q_D(const QUndoGroup);
    return d->active == nullptr || d->active->isClean();
}

/*! \fn void QUndoGroup::activeStackChanged(QUndoStack *stack)

    This signal is emitted whenever the active stack of the group changes. This can happen
    when setActiveStack() or QUndoStack::setActive() is called, or when the active stack
    is removed form the group. \a stack is the new active stack. If no stack is active,
    \a stack is 0.

    \sa setActiveStack(), QUndoStack::setActive()
*/

/*! \fn void QUndoGroup::indexChanged(int idx)

    This signal is emitted whenever the active stack emits QUndoStack::indexChanged()
    or the active stack changes.

    \a idx is the new current index, or 0 if the active stack is 0.

    \sa QUndoStack::indexChanged(), setActiveStack()
*/

/*! \fn void QUndoGroup::cleanChanged(bool clean)

    This signal is emitted whenever the active stack emits QUndoStack::cleanChanged()
    or the active stack changes.

    \a clean is the new state, or true if the active stack is 0.

    \sa QUndoStack::cleanChanged(), setActiveStack()
*/

/*! \fn void QUndoGroup::canUndoChanged(bool canUndo)

    This signal is emitted whenever the active stack emits QUndoStack::canUndoChanged()
    or the active stack changes.

    \a canUndo is the new state, or false if the active stack is 0.

    \sa QUndoStack::canUndoChanged(), setActiveStack()
*/

/*! \fn void QUndoGroup::canRedoChanged(bool canRedo)

    This signal is emitted whenever the active stack emits QUndoStack::canRedoChanged()
    or the active stack changes.

    \a canRedo is the new state, or false if the active stack is 0.

    \sa QUndoStack::canRedoChanged(), setActiveStack()
*/

/*! \fn void QUndoGroup::undoTextChanged(const QString &undoText)

    This signal is emitted whenever the active stack emits QUndoStack::undoTextChanged()
    or the active stack changes.

    \a undoText is the new state, or an empty string if the active stack is 0.

    \sa QUndoStack::undoTextChanged(), setActiveStack()
*/

/*! \fn void QUndoGroup::redoTextChanged(const QString &redoText)

    This signal is emitted whenever the active stack emits QUndoStack::redoTextChanged()
    or the active stack changes.

    \a redoText is the new state, or an empty string if the active stack is 0.

    \sa QUndoStack::redoTextChanged(), setActiveStack()
*/

QT_END_NAMESPACE

#include "moc_qundogroup.cpp"
