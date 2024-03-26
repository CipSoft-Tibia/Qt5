// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFUTURESYNCHRONIZER_H
#define QFUTURESYNCHRONIZER_H

#include <QtCore/qfuture.h>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE


template <typename T>
class QFutureSynchronizer
{
    Q_DISABLE_COPY(QFutureSynchronizer)

public:
    Q_NODISCARD_CTOR QFutureSynchronizer() : m_cancelOnWait(false) { }
    Q_NODISCARD_CTOR
    explicit QFutureSynchronizer(QFuture<T> future)
        : m_cancelOnWait(false)
    { addFuture(std::move(future)); }
    ~QFutureSynchronizer()  { waitForFinished(); }

    void setFuture(QFuture<T> future)
    {
        waitForFinished();
        m_futures.clear();
        addFuture(std::move(future));
    }

    void addFuture(QFuture<T> future)
    {
        m_futures.append(std::move(future));
    }

    void waitForFinished()
    {
        if (m_cancelOnWait) {
            for (int i = 0; i < m_futures.size(); ++i) {
                 m_futures[i].cancel();
            }
        }

        for (int i = 0; i < m_futures.size(); ++i) {
             m_futures[i].waitForFinished();
         }
    }

    void clearFutures()
    {
        m_futures.clear();
    }

    QList<QFuture<T> > futures() const
    {
        return m_futures;
    }

    void setCancelOnWait(bool enabled)
    {
        m_cancelOnWait = enabled;
    }

    bool cancelOnWait() const
    {
        return m_cancelOnWait;
    }

protected:
    QList<QFuture<T>> m_futures;
    bool m_cancelOnWait;
};

QT_END_NAMESPACE

#endif // QFUTURESYNCHRONIZER_H
