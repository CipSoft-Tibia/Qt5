// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTEVENTDISPATCHER_P_H
#define QABSTRACTEVENTDISPATCHER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qabstracteventdispatcher.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

Q_AUTOTEST_EXPORT qsizetype qGlobalPostedEventsCount();

class Q_CORE_EXPORT QAbstractEventDispatcherPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractEventDispatcher)
public:
    QAbstractEventDispatcherPrivate();
    ~QAbstractEventDispatcherPrivate() override;

    QList<QAbstractNativeEventFilter *> eventFilters;

    static int allocateTimerId();
    static void releaseTimerId(int id);
};

QT_END_NAMESPACE

#endif // QABSTRACTEVENTDISPATCHER_P_H
