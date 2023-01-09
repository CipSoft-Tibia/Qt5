/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlthread_p.h"

#include <private/qfieldlist_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class QQmlThreadPrivate : public QThread
{
public:
    QQmlThreadPrivate(QQmlThread *);
    QQmlThread *q;

    void run() override;

    inline QMutex &mutex() { return _mutex; }
    inline void lock() { _mutex.lock(); }
    inline void unlock() { _mutex.unlock(); }
    inline void wait() { _wait.wait(&_mutex); }
    inline void wakeOne() { _wait.wakeOne(); }
    inline void wakeAll() { _wait.wakeAll(); }

    quint32 m_threadProcessing:1; // Set when the thread is processing messages
    quint32 m_mainProcessing:1; // Set when the main thread is processing messages
    quint32 m_shutdown:1; // Set by main thread to request a shutdown
    quint32 m_mainThreadWaiting:1; // Set by main thread if it is waiting for the message queue to empty

    typedef QFieldList<QQmlThread::Message, &QQmlThread::Message::next> MessageList;
    MessageList threadList;
    MessageList mainList;

    QQmlThread::Message *mainSync;

    void triggerMainEvent();
    void triggerThreadEvent();

    void mainEvent();
    void threadEvent();

protected:
    bool event(QEvent *) override;

private:
    struct MainObject : public QObject {
        MainObject(QQmlThreadPrivate *p);
        bool event(QEvent *e) override;
        QQmlThreadPrivate *p;
    };
    MainObject m_mainObject;

    QMutex _mutex;
    QWaitCondition _wait;
};

QQmlThreadPrivate::MainObject::MainObject(QQmlThreadPrivate *p)
: p(p)
{
}

// Trigger mainEvent in main thread.  Must be called from thread.
void QQmlThreadPrivate::triggerMainEvent()
{
#if QT_CONFIG(thread)
    Q_ASSERT(q->isThisThread());
#endif
    QCoreApplication::postEvent(&m_mainObject, new QEvent(QEvent::User));
}

// Trigger even in thread.  Must be called from main thread.
void QQmlThreadPrivate::triggerThreadEvent()
{
#if QT_CONFIG(thread)
    Q_ASSERT(!q->isThisThread());
#endif
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

bool QQmlThreadPrivate::MainObject::event(QEvent *e)
{
    if (e->type() == QEvent::User)
        p->mainEvent();
    return QObject::event(e);
}

QQmlThreadPrivate::QQmlThreadPrivate(QQmlThread *q)
: q(q), m_threadProcessing(false), m_mainProcessing(false), m_shutdown(false),
  m_mainThreadWaiting(false), mainSync(nullptr), m_mainObject(this)
{
    setObjectName(QStringLiteral("QQmlThread"));
    // This size is aligned with the recursion depth limits in the parser/codegen. In case of
    // absurd content we want to hit the recursion checks instead of running out of stack.
    setStackSize(8 * 1024 * 1024);
}

bool QQmlThreadPrivate::event(QEvent *e)
{
    if (e->type() == QEvent::User)
        threadEvent();
    return QThread::event(e);
}

void QQmlThreadPrivate::run()
{
    lock();

    wakeOne();

    unlock();

    q->startupThread();
    exec();
    q->shutdownThread();
}

void QQmlThreadPrivate::mainEvent()
{
    lock();

    m_mainProcessing = true;

    while (!mainList.isEmpty() || mainSync) {
        bool isSync = mainSync != nullptr;
        QQmlThread::Message *message = isSync?mainSync:mainList.takeFirst();
        unlock();

        message->call(q);
        delete message;

        lock();

        if (isSync) {
            mainSync = nullptr;
            wakeOne();
        }
    }

    m_mainProcessing = false;

    unlock();
}

void QQmlThreadPrivate::threadEvent()
{
    lock();

    for (;;) {
        if (!threadList.isEmpty()) {
            m_threadProcessing = true;

            QQmlThread::Message *message = threadList.first();

            unlock();

            message->call(q);

            lock();

            delete threadList.takeFirst();
        } else if (m_shutdown) {
            quit();
            wakeOne();
            unlock();

            return;
        } else {
            wakeOne();

            m_threadProcessing = false;

            unlock();

            return;
        }
    }
}

QQmlThread::QQmlThread()
: d(new QQmlThreadPrivate(this))
{
}

QQmlThread::~QQmlThread()
{
    delete d;
}

void QQmlThread::startup()
{
    d->lock();
    d->start();
    d->wait();
    d->unlock();
    d->moveToThread(d);
}

void QQmlThread::shutdown()
{
    d->lock();
    Q_ASSERT(!d->m_shutdown);

    d->m_shutdown = true;
    for (;;) {
        if (d->mainSync || !d->mainList.isEmpty()) {
            d->unlock();
            d->mainEvent();
            d->lock();
        } else if (!d->threadList.isEmpty()) {
            d->wait();
        } else {
            break;
        }
    }

    if (QCoreApplication::closingDown())
        d->quit();
    else
        d->triggerThreadEvent();

    d->unlock();
    d->QThread::wait();
}

bool QQmlThread::isShutdown() const
{
    return d->m_shutdown;
}

QMutex &QQmlThread::mutex()
{
    return d->mutex();
}

void QQmlThread::lock()
{
    d->lock();
}

void QQmlThread::unlock()
{
    d->unlock();
}

void QQmlThread::wakeOne()
{
    d->wakeOne();
}

void QQmlThread::wakeAll()
{
    d->wakeAll();
}

void QQmlThread::wait()
{
    d->wait();
}

bool QQmlThread::isThisThread() const
{
    return QThread::currentThread() == d;
}

QThread *QQmlThread::thread() const
{
    return const_cast<QThread *>(static_cast<const QThread *>(d));
}

// Called when the thread starts.  Do startup stuff in here.
void QQmlThread::startupThread()
{
}

// Called when the thread shuts down.  Do cleanup in here.
void QQmlThread::shutdownThread()
{
}

void QQmlThread::internalCallMethodInThread(Message *message)
{
#if !QT_CONFIG(thread)
    message->call(this);
    delete message;
    return;
#endif

    Q_ASSERT(!isThisThread());
    d->lock();
    Q_ASSERT(d->m_mainThreadWaiting == false);

    bool wasEmpty = d->threadList.isEmpty();
    d->threadList.append(message);
    if (wasEmpty && d->m_threadProcessing == false)
        d->triggerThreadEvent();

    d->m_mainThreadWaiting = true;

    do {
        if (d->mainSync) {
            QQmlThread::Message *message = d->mainSync;
            unlock();
            message->call(this);
            delete message;
            lock();
            d->mainSync = nullptr;
            wakeOne();
        } else {
            d->wait();
        }
    } while (d->mainSync || !d->threadList.isEmpty());

    d->m_mainThreadWaiting = false;
    d->unlock();
}

void QQmlThread::internalCallMethodInMain(Message *message)
{
#if !QT_CONFIG(thread)
    message->call(this);
    delete message;
    return;
#endif

    Q_ASSERT(isThisThread());

    d->lock();

    Q_ASSERT(d->mainSync == nullptr);
    d->mainSync = message;

    if (d->m_mainThreadWaiting) {
        d->wakeOne();
    } else if (d->m_mainProcessing) {
        // Do nothing - it is already looping
    } else {
        d->triggerMainEvent();
    }

    while (d->mainSync) {
        if (d->m_shutdown) {
            delete d->mainSync;
            d->mainSync = nullptr;
            break;
        }
        d->wait();
    }

    d->unlock();
}

void QQmlThread::internalPostMethodToThread(Message *message)
{
#if !QT_CONFIG(thread)
    internalPostMethodToMain(message);
    return;
#endif
    Q_ASSERT(!isThisThread());
    d->lock();
    bool wasEmpty = d->threadList.isEmpty();
    d->threadList.append(message);
    if (wasEmpty && d->m_threadProcessing == false)
        d->triggerThreadEvent();
    d->unlock();
}

void QQmlThread::internalPostMethodToMain(Message *message)
{
#if QT_CONFIG(thread)
    Q_ASSERT(isThisThread());
#endif
    d->lock();
    bool wasEmpty = d->mainList.isEmpty();
    d->mainList.append(message);
    if (wasEmpty && d->m_mainProcessing == false)
        d->triggerMainEvent();
    d->unlock();
}

void QQmlThread::waitForNextMessage()
{
#if QT_CONFIG(thread)
    Q_ASSERT(!isThisThread());
#endif
    d->lock();
    Q_ASSERT(d->m_mainThreadWaiting == false);

    d->m_mainThreadWaiting = true;

    if (d->mainSync || !d->threadList.isEmpty()) {
        if (d->mainSync) {
            QQmlThread::Message *message = d->mainSync;
            unlock();
            message->call(this);
            delete message;
            lock();
            d->mainSync = nullptr;
            wakeOne();
        } else {
            d->wait();
        }
    }

    d->m_mainThreadWaiting = false;
    d->unlock();
}


QT_END_NAMESPACE
