// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventloop.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qelapsedtimer.h"

#include "qobject_p.h"
#include "qeventloop_p.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QEventLoop
    \inmodule QtCore
    \brief The QEventLoop class provides a means of entering and leaving an event loop.

    At any time, you can create a QEventLoop object and call exec()
    on it to start a local event loop. From within the event loop,
    calling exit() will force exec() to return.

    \sa QAbstractEventDispatcher
*/

/*!
    \enum QEventLoop::ProcessEventsFlag

    This enum controls the types of events processed by the
    processEvents() functions.

    \value AllEvents All events. Note that
    \l{QEvent::DeferredDelete}{DeferredDelete} events are processed
    specially. See QObject::deleteLater() for more details.

    \value ExcludeUserInputEvents Do not process user input events,
    such as ButtonPress and KeyPress. Note that the events are not
    discarded; they will be delivered the next time processEvents() is
    called without the ExcludeUserInputEvents flag.

    \value ExcludeSocketNotifiers Do not process socket notifier
    events. Note that the events are not discarded; they will be
    delivered the next time processEvents() is called without the
    ExcludeSocketNotifiers flag.

    \value WaitForMoreEvents Wait for events if no pending events are
    available.

    \omitvalue X11ExcludeTimers
    \omitvalue EventLoopExec
    \omitvalue DialogExec
    \omitvalue ApplicationExec

    \sa processEvents()
*/

/*!
    Constructs an event loop object with the given \a parent.
*/
QEventLoop::QEventLoop(QObject *parent)
    : QObject(*new QEventLoopPrivate, parent)
{
    Q_D(QEventLoop);
    if (!QCoreApplication::instance() && QCoreApplicationPrivate::threadRequiresCoreApplication()) {
        qWarning("QEventLoop: Cannot be used without QApplication");
    } else {
        d->threadData.loadRelaxed()->ensureEventDispatcher();
    }
}

/*!
    Destroys the event loop object.
*/
QEventLoop::~QEventLoop()
{ }


/*!
    Processes some pending events that match \a flags.
    Returns \c true if pending events were handled;
    otherwise returns \c false.

    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input; i.e. by using the \l ExcludeUserInputEvents flag.

    This function is simply a wrapper for
    QAbstractEventDispatcher::processEvents(). See the documentation
    for that function for details.
*/
bool QEventLoop::processEvents(ProcessEventsFlags flags)
{
    Q_D(QEventLoop);
    auto threadData = d->threadData.loadRelaxed();
    if (!threadData->hasEventDispatcher())
        return false;
    return threadData->eventDispatcher.loadRelaxed()->processEvents(flags);
}

/*!
    Enters the main event loop and waits until exit() is called.
    Returns the value that was passed to exit().

    If \a flags are specified, only events of the types allowed by
    the \a flags will be processed.

    It is necessary to call this function to start event handling. The
    main event loop receives events from the window system and
    dispatches these to the application widgets.

    Generally speaking, no user interaction can take place before
    calling exec(). As a special case, modal widgets like QMessageBox
    can be used before calling exec(), because modal widgets
    use their own local event loop.

    To make your application perform idle processing (i.e. executing a
    special function whenever there are no pending events), use a
    QTimer with 0 timeout. More sophisticated idle processing schemes
    can be achieved using processEvents().

    \sa QCoreApplication::quit(), exit(), processEvents()
*/
int QEventLoop::exec(ProcessEventsFlags flags)
{
    Q_D(QEventLoop);
    auto threadData = d->threadData.loadRelaxed();

    //we need to protect from race condition with QThread::exit
    QMutexLocker locker(&static_cast<QThreadPrivate *>(QObjectPrivate::get(threadData->thread.loadAcquire()))->mutex);
    if (threadData->quitNow)
        return -1;

    if (d->inExec) {
        qWarning("QEventLoop::exec: instance %p has already called exec()", this);
        return -1;
    }

    struct LoopReference {
        QEventLoopPrivate *d;
        QMutexLocker<QMutex> &locker;

        bool exceptionCaught;
        LoopReference(QEventLoopPrivate *d, QMutexLocker<QMutex> &locker) : d(d), locker(locker), exceptionCaught(true)
        {
            d->inExec = true;
            d->exit.storeRelease(false);

            auto threadData = d->threadData.loadRelaxed();
            ++threadData->loopLevel;
            threadData->eventLoops.push(d->q_func());

            locker.unlock();
        }

        ~LoopReference()
        {
            if (exceptionCaught) {
                qWarning("Qt has caught an exception thrown from an event handler. Throwing\n"
                         "exceptions from an event handler is not supported in Qt.\n"
                         "You must not let any exception whatsoever propagate through Qt code.");
            }
            locker.relock();
            auto threadData = d->threadData.loadRelaxed();
            QEventLoop *eventLoop = threadData->eventLoops.pop();
            Q_ASSERT_X(eventLoop == d->q_func(), "QEventLoop::exec()", "internal error");
            Q_UNUSED(eventLoop); // --release warning
            d->inExec = false;
            --threadData->loopLevel;
        }
    };
    LoopReference ref(d, locker);

    // remove posted quit events when entering a new event loop
    QCoreApplication *app = QCoreApplication::instance();
    if (app && app->thread() == thread())
        QCoreApplication::removePostedEvents(app, QEvent::Quit);

    while (!d->exit.loadAcquire())
        processEvents(flags | WaitForMoreEvents | EventLoopExec);

    ref.exceptionCaught = false;
    return d->returnCode.loadRelaxed();
}

/*!
    Process pending events that match \a flags for a maximum of \a
    maxTime milliseconds, or until there are no more events to
    process, whichever is shorter.
    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input, i.e. by using the \l ExcludeUserInputEvents flag.

    \b{Notes:}
    \list
    \li This function does not process events continuously; it
       returns after all available events are processed.
    \li Specifying the \l WaitForMoreEvents flag makes no sense
       and will be ignored.
    \endlist
*/
void QEventLoop::processEvents(ProcessEventsFlags flags, int maxTime)
{
    Q_D(QEventLoop);
    if (!d->threadData.loadRelaxed()->hasEventDispatcher())
        return;

    QElapsedTimer start;
    start.start();
    while (processEvents(flags & ~WaitForMoreEvents)) {
        if (start.elapsed() > maxTime)
            break;
    }
}

/*!
    Tells the event loop to exit with a return code.

    After this function has been called, the event loop returns from
    the call to exec(). The exec() function returns \a returnCode.

    By convention, a \a returnCode of 0 means success, and any non-zero
    value indicates an error.

    Note that unlike the C library function of the same name, this
    function \e does return to the caller -- it is event processing that
    stops.

    \sa QCoreApplication::quit(), quit(), exec()
*/
void QEventLoop::exit(int returnCode)
{
    Q_D(QEventLoop);
    auto threadData = d->threadData.loadAcquire();
    if (!threadData->hasEventDispatcher())
        return;

    d->returnCode.storeRelaxed(returnCode);
    d->exit.storeRelease(true);
    threadData->eventDispatcher.loadRelaxed()->interrupt();
}

/*!
    Returns \c true if the event loop is running; otherwise returns
    false. The event loop is considered running from the time when
    exec() is called until exit() is called.

    \sa exec(), exit()
 */
bool QEventLoop::isRunning() const
{
    Q_D(const QEventLoop);
    return !d->exit.loadAcquire();
}

/*!
    Wakes up the event loop.

    \sa QAbstractEventDispatcher::wakeUp()
*/
void QEventLoop::wakeUp()
{
    Q_D(QEventLoop);
    auto threadData = d->threadData.loadAcquire();
    if (!threadData->hasEventDispatcher())
        return;
    threadData->eventDispatcher.loadRelaxed()->wakeUp();
}


/*!
    \reimp
*/
bool QEventLoop::event(QEvent *event)
{
    if (event->type() == QEvent::Quit) {
        quit();
        return true;
    } else {
        return QObject::event(event);
    }
}

/*!
    Tells the event loop to exit normally.

    Same as exit(0).

    \sa QCoreApplication::quit(), exit()
*/
void QEventLoop::quit()
{ exit(0); }

// If any of these trigger, the Type bits will interfere with the pointer values:
static_assert(alignof(QEventLoop) >= 4);
static_assert(alignof(QThread) >= 4);
static_assert(alignof(QCoreApplication) >= 4);

/*!
    \class QEventLoopLocker
    \inmodule QtCore
    \brief The QEventLoopLocker class provides a means to quit an event loop when it is no longer needed.
    \since 5.0

    The QEventLoopLocker operates on particular objects - either a QCoreApplication
    instance, a QEventLoop instance or a QThread instance.

    This makes it possible to, for example, run a batch of jobs with an event loop
    and exit that event loop after the last job is finished. That is accomplished
    by keeping a QEventLoopLocker with each job instance.

    The variant which operates on QCoreApplication makes it possible to finish
    asynchronously running jobs after the last gui window has been closed. This
    can be useful for example for running a job which uploads data to a network.

    \sa QEventLoop, QCoreApplication
*/

/*!
    Creates an event locker operating on the QCoreApplication.

    The application will quit when there are no more QEventLoopLockers operating on it.

    \sa QCoreApplication::quit(), QCoreApplication::isQuitLockEnabled()
 */
QEventLoopLocker::QEventLoopLocker() noexcept
    : QEventLoopLocker{QCoreApplication::instance(), Type::Application}
{

}

/*!
    Creates an event locker operating on the \a loop.

    This particular QEventLoop will quit when there are no more QEventLoopLockers operating on it.

    \sa QEventLoop::quit()
 */
QEventLoopLocker::QEventLoopLocker(QEventLoop *loop) noexcept
    : QEventLoopLocker{loop, Type::EventLoop}
{

}

/*!
    Creates an event locker operating on the \a thread.

    This particular QThread will quit when there are no more QEventLoopLockers operating on it.

    \sa QThread::quit()
 */
QEventLoopLocker::QEventLoopLocker(QThread *thread) noexcept
    : QEventLoopLocker{thread, Type::Thread}
{

}

/*!
    Destroys this event loop locker object
 */
QEventLoopLocker::~QEventLoopLocker()
{
    visit([](auto p) { p->d_func()->deref(); });
}

/*!
    \internal
*/
QEventLoopLocker::QEventLoopLocker(void *ptr, Type t) noexcept
    : p{quintptr(ptr) | quintptr(t)}
{
    visit([](auto p) { p->d_func()->ref(); });
}

/*!
    \internal
*/
template <typename Func>
void QEventLoopLocker::visit(Func f) const
{
    const auto ptr = pointer();
    if (!ptr)
        return;
    switch (type()) {
    case Type::EventLoop:   return f(static_cast<QEventLoop *>(ptr));
    case Type::Thread:      return f(static_cast<QThread *>(ptr));
    case Type::Application: return f(static_cast<QCoreApplication *>(ptr));
    }
    Q_UNREACHABLE();
}

QT_END_NAMESPACE

#include "moc_qeventloop.cpp"
