// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "eventcapturer.h"

#include <QDebug>
#include <QMetaEnum>
#include <QMouseEvent>
#include <QTimer>

/*!
    Installs an event filter on a particular object to record specific events
    that can be retrieved as C++ source code.

    For example:

    \code
    EventCapturer eventCapturer;

    view.show();

    eventCapturer.startCapturing(&view, 5000);

    // interact with the view here, in order for the events to be captured

    qDebug() << "\n";
    const auto capturedEvents = eventCapturer.capturedEvents();
    for (CapturedEvent event : capturedEvents)
        qDebug().noquote() << event.cppCommand();
    \endcode

    It is recommended to set the \c Qt::FramelessWindowHint flag on the view
    (this code has not been tested under other usage):

    view.setFlags(view.flags() | Qt::FramelessWindowHint);
*/

EventCapturer::EventCapturer(QObject *parent) :
    QObject(parent),
    mEventSource(nullptr),
    mStopCaptureKey(Qt::Key_Escape),
    mMoveEventTrimFlags(TrimNone),
    mDuration(0),
    mLastCaptureTime(0)
{
    mCapturedEventTypes << QEvent::MouseButtonPress << QEvent::MouseButtonRelease << QEvent::MouseButtonDblClick << QEvent::MouseMove;
}

void EventCapturer::startCapturing(QObject *eventSource, int duration)
{
    mEventSource = eventSource;

    if (!mEventSource)
        return;

    mEventSource->installEventFilter(this);
    mDelayTimer.start();
    mDuration = duration;
    mLastCaptureTime = 0;

    QTimer::singleShot(mDuration, this, SLOT(stopCapturing()));
}

void EventCapturer::setStopCaptureKey(Qt::Key stopCaptureKey)
{
    mStopCaptureKey = stopCaptureKey;
}

/*!
    Move events generate a lot of clutter, and for most cases they're not
    necessary. Here's a list of scenarios where various trim flags make sense:

    Scenario                            Flags

    Record the mouse cursor             TrimNone
    Record mouseover/hover effects      TrimNone
    Dragging/flicking                   TrimAll
*/
void EventCapturer::setMoveEventTrimFlags(MoveEventTrimFlags trimFlags)
{
    mMoveEventTrimFlags = trimFlags;
}

QSet<QEvent::Type> EventCapturer::capturedEventTypes()
{
    return mCapturedEventTypes;
}

void EventCapturer::setCapturedEventTypes(QSet<QEvent::Type> types)
{
    mCapturedEventTypes = types;
}

QList<CapturedEvent> EventCapturer::capturedEvents() const
{
    if (mMoveEventTrimFlags == TrimNone || mEvents.isEmpty())
        return mEvents;

    // We can't easily trim "trailing" move events as they come in without
    // storing them in some form, so we just do it all here.

    int firstEventIndex = 0;
    int lastEventIndex = mEvents.size() - 1;
    // The accumulated delay of all of the move events that we remove.
    // We keep this in order to maintain the correct timing between events.
    int accumulatedDelay = 0;

    bool encounteredNonMoveEvent = false;
    if (mMoveEventTrimFlags.testFlag(TrimLeading)) {
        for (int eventIndex = 0; !encounteredNonMoveEvent && eventIndex < mEvents.size(); ++eventIndex) {
            const CapturedEvent event = mEvents.at(eventIndex);
            if (event.type() != QEvent::MouseMove) {
                encounteredNonMoveEvent = true;
                firstEventIndex = eventIndex;
            } else {
                accumulatedDelay += event.delay();
            }
        }
    }

    if (mMoveEventTrimFlags.testFlag(TrimTrailing)) {
        encounteredNonMoveEvent = false;
        for (int eventIndex = mEvents.size() - 1; !encounteredNonMoveEvent && eventIndex >= 0; --eventIndex) {
            const CapturedEvent event = mEvents.at(eventIndex);
            if (event.type() != QEvent::MouseMove) {
                encounteredNonMoveEvent = true;
                lastEventIndex = eventIndex;
                // Don't need to bother with delays for trailing mouse moves, as there is nothing after them.
            }
        }
    }

    // Before we go any further, we need to copy the subset of commands while
    // the indices are still valid - we could be removing from the middle of
    // the commands next. Also, the function is const, so we can't remove from
    // mEvents anyway. :)
    QList<CapturedEvent> events = mEvents.mid(firstEventIndex, (lastEventIndex - firstEventIndex) + 1);

    if (mMoveEventTrimFlags.testFlag(TrimAfterReleases)) {
        bool lastNonMoveEventWasRelease = false;
        for (int eventIndex = 0; eventIndex < events.size(); ) {
            CapturedEvent &event = events[eventIndex];
            if (event.type() == QEvent::MouseMove && lastNonMoveEventWasRelease) {
                accumulatedDelay += event.delay();
                events.remove(eventIndex);
            } else {
                lastNonMoveEventWasRelease = event.type() == QEvent::MouseButtonRelease;
                if (event.type() == QEvent::MouseButtonPress) {
                    event.setDelay(event.delay() + accumulatedDelay);
                    accumulatedDelay = 0;
                }
                ++eventIndex;
            }
        }
    }

    return events;
}

bool EventCapturer::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == mStopCaptureKey) {
        stopCapturing();
        return true;
    }

    if (object != mEventSource)
        return false;

    if (!mCapturedEventTypes.contains(event->type()))
        return false;

    if (event->type() == QEvent::MouseButtonPress) {
        captureEvent(event);
    } else if (event->type() == QEvent::MouseButtonRelease) {
        captureEvent(event);
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        captureEvent(event);
    } else if (event->type() == QEvent::MouseMove) {
        captureEvent(event);
    } else {
        qWarning() << "No support for event type" << QMetaEnum::fromType<QEvent::Type>().valueToKey(event->type());
    }
    return false;
}

void EventCapturer::stopCapturing()
{
    if (mEventSource) {
        mEventSource->removeEventFilter(this);
        mEventSource = 0;
        mDuration = 0;
        mLastCaptureTime = 0;
    }
}

void EventCapturer::captureEvent(const QEvent *event)
{
    qDebug() << "captured" << event->type();
    CapturedEvent capturedEvent(*event, mDelayTimer.elapsed() - mLastCaptureTime);
    mEvents.append(capturedEvent);
    mLastCaptureTime = mDelayTimer.elapsed();
}
