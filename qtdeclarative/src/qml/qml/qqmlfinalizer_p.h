// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLFINALIZER_P_H
#define QQMLFINALIZER_P_H

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

#include <QtQml/private/qtqmlglobal_p.h>
#include <qobject.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlFinalizerHook
{
public:
    virtual ~QQmlFinalizerHook();
    virtual void componentFinalized() = 0;
};
#define QQmlFinalizerHook_iid "org.qt-project.Qt.QQmlFinalizerHook"
Q_DECLARE_INTERFACE(QQmlFinalizerHook, QQmlFinalizerHook_iid)

QT_END_NAMESPACE

#endif // QQMLFINALIZER_P_H
