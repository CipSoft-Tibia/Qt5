/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickbehavior_p.h"

#include "qquickanimation_p.h"
#include <qqmlcontext.h>
#include <qqmlinfo.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlengine_p.h>
#include <private/qabstractanimationjob_p.h>
#include <private/qquicktransition_p.h>

#include <private/qquickanimatorjob_p.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickBehaviorPrivate : public QObjectPrivate, public QAnimationJobChangeListener
{
    Q_DECLARE_PUBLIC(QQuickBehavior)
public:
    QQuickBehaviorPrivate() : animation(nullptr), animationInstance(nullptr), enabled(true), finalized(false)
      , blockRunningChanged(false) {}

    void animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;

    QQmlProperty property;
    QVariant targetValue;
    QPointer<QQuickAbstractAnimation> animation;
    QAbstractAnimationJob *animationInstance;
    bool enabled;
    bool finalized;
    bool blockRunningChanged;
};

/*!
    \qmltype Behavior
    \instantiates QQuickBehavior
    \inqmlmodule QtQuick
    \ingroup qtquick-transitions-animations
    \ingroup qtquick-interceptors
    \brief Defines a default animation for a property change.

    A Behavior defines the default animation to be applied whenever a
    particular property value changes.

    For example, the following Behavior defines a NumberAnimation to be run
    whenever the \l Rectangle's \c width value changes. When the MouseArea
    is clicked, the \c width is changed, triggering the behavior's animation:

    \snippet qml/behavior.qml 0

    Note that a property cannot have more than one assigned Behavior. To provide
    multiple animations within a Behavior, use ParallelAnimation or
    SequentialAnimation.

    If a \l{Qt Quick States}{state change} has a \l Transition that matches the same property as a
    Behavior, the \l Transition animation overrides the Behavior for that
    state change. For general advice on using Behaviors to animate state changes, see
    \l{Using Qt Quick Behaviors with States}.

    \sa {Animation and Transitions in Qt Quick}, {Qt Quick Examples - Animation#Behaviors}{Behavior example}, {Qt QML}
*/


QQuickBehavior::QQuickBehavior(QObject *parent)
    : QObject(*(new QQuickBehaviorPrivate), parent)
{
}

QQuickBehavior::~QQuickBehavior()
{
    Q_D(QQuickBehavior);
    delete d->animationInstance;
}

/*!
    \qmlproperty Animation QtQuick::Behavior::animation
    \default

    This property holds the animation to run when the behavior is triggered.
*/

QQuickAbstractAnimation *QQuickBehavior::animation()
{
    Q_D(QQuickBehavior);
    return d->animation;
}

void QQuickBehavior::setAnimation(QQuickAbstractAnimation *animation)
{
    Q_D(QQuickBehavior);
    if (d->animation) {
        qmlWarning(this) << tr("Cannot change the animation assigned to a Behavior.");
        return;
    }

    d->animation = animation;
    if (d->animation) {
        d->animation->setDefaultTarget(d->property);
        d->animation->setDisableUserControl();
    }
}


void QQuickBehaviorPrivate::animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State newState,QAbstractAnimationJob::State)
{
    if (!blockRunningChanged && animation)
        animation->notifyRunningChanged(newState == QAbstractAnimationJob::Running);
}

/*!
    \qmlproperty bool QtQuick::Behavior::enabled

    This property holds whether the behavior will be triggered when the tracked
    property changes value.

    By default a Behavior is enabled.
*/

bool QQuickBehavior::enabled() const
{
    Q_D(const QQuickBehavior);
    return d->enabled;
}

void QQuickBehavior::setEnabled(bool enabled)
{
    Q_D(QQuickBehavior);
    if (d->enabled == enabled)
        return;
    d->enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty Variant QtQuick::Behavior::targetValue

    This property holds the target value of the property being controlled by the Behavior.
    This value is set by the Behavior before the animation is started.

    \since QtQuick 2.13
*/
QVariant QQuickBehavior::targetValue() const
{
    Q_D(const QQuickBehavior);
    return d->targetValue;
}

/*!
    \readonly
    \qmlpropertygroup QtQuick::Behavior::targetProperty
    \qmlproperty string QtQuick::Behavior::targetProperty.name
    \qmlproperty Object QtQuick::Behavior::targetProperty.object

    \table
    \header
        \li Property
        \li Description
    \row
        \li name
        \li This property holds the name of the property being controlled by this Behavior.
    \row
        \li object
        \li This property holds the object of the property being controlled by this Behavior.
    \endtable

    This property can be used to define custom behaviors based on the name or the object of
    the property being controlled.

    The following example defines a Behavior fading out and fading in its target object
    when the property it controls changes:
    \qml
    // FadeBehavior.qml
    import QtQuick 2.15

    Behavior {
        id: root
        property Item fadeTarget: targetProperty.object
        SequentialAnimation {
            NumberAnimation {
                target: root.fadeTarget
                property: "opacity"
                to: 0
                easing.type: Easing.InQuad
            }
            PropertyAction { } // actually change the controlled property between the 2 other animations
            NumberAnimation {
                target: root.fadeTarget
                property: "opacity"
                to: 1
                easing.type: Easing.OutQuad
            }
        }
    }
    \endqml

    This can be used to animate a text when it changes:
    \qml
    import QtQuick 2.15

    Text {
        id: root
        property int counter
        text: counter
        FadeBehavior on text {}
        Timer {
            running: true
            repeat: true
            interval: 1000
            onTriggered: ++root.counter
        }
    }
    \endqml

    \since QtQuick 2.15
*/
QQmlProperty QQuickBehavior::targetProperty() const
{
    Q_D(const QQuickBehavior);
    return d->property;
}

void QQuickBehavior::write(const QVariant &value)
{
    Q_D(QQuickBehavior);
    const bool targetValueHasChanged = d->targetValue != value;
    if (targetValueHasChanged) {
        d->targetValue = value;
        emit targetValueChanged(); // emitting the signal here should allow
    }                              // d->enabled to change if scripted by the user.
    bool bypass = !d->enabled || !d->finalized || QQmlEnginePrivate::designerMode();
    if (!bypass)
        qmlExecuteDeferred(this);
    if (QQmlData::wasDeleted(d->animation) || bypass) {
        if (d->animationInstance)
            d->animationInstance->stop();
        QQmlPropertyPrivate::write(d->property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
        return;
    }

    bool behaviorActive = d->animation->isRunning();
    if (behaviorActive && !targetValueHasChanged)
        return;

    if (d->animationInstance
            && (d->animationInstance->duration() != -1
                || d->animationInstance->isRenderThreadProxy())
            && !d->animationInstance->isStopped()) {
        d->blockRunningChanged = true;
        d->animationInstance->stop();
    }
    // Render thread animations use "stop" to synchronize the property back
    // to the item, so we need to read the value after.
    const QVariant &currentValue = d->property.read();

    // Don't unnecessarily wake up the animation system if no real animation
    // is needed (value has not changed). If the Behavior was already
    // running, let it continue as normal to ensure correct behavior and state.
    if (!behaviorActive && d->targetValue == currentValue) {
        QQmlPropertyPrivate::write(d->property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
        return;
    }

    QQuickStateOperation::ActionList actions;
    QQuickStateAction action;
    action.property = d->property;
    action.fromValue = currentValue;
    action.toValue = value;
    actions << action;

    QList<QQmlProperty> after;
    QAbstractAnimationJob *prev = d->animationInstance;
    d->animationInstance = d->animation->transition(actions, after, QQuickAbstractAnimation::Forward);

    if (d->animationInstance && d->animation->threadingModel() == QQuickAbstractAnimation::RenderThread)
        d->animationInstance = new QQuickAnimatorProxyJob(d->animationInstance, d->animation);

    if (prev && prev != d->animationInstance)
        delete prev;

    if (d->animationInstance) {
        if (d->animationInstance != prev)
            d->animationInstance->addAnimationChangeListener(d, QAbstractAnimationJob::StateChange);
        d->animationInstance->start();
        d->blockRunningChanged = false;
    }
    if (!after.contains(d->property))
        QQmlPropertyPrivate::write(d->property, value, QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
}

void QQuickBehavior::setTarget(const QQmlProperty &property)
{
    Q_D(QQuickBehavior);
    d->property = property;
    if (d->animation)
        d->animation->setDefaultTarget(property);

    QQmlEnginePrivate *engPriv = QQmlEnginePrivate::get(qmlEngine(this));
    static int finalizedIdx = -1;
    if (finalizedIdx < 0)
        finalizedIdx = metaObject()->indexOfSlot("componentFinalized()");
    engPriv->registerFinalizeCallback(this, finalizedIdx);

    Q_EMIT targetPropertyChanged();
}

void QQuickBehavior::componentFinalized()
{
    Q_D(QQuickBehavior);
    d->finalized = true;
}

QT_END_NAMESPACE

#include "moc_qquickbehavior_p.cpp"
