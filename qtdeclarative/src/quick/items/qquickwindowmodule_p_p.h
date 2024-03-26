// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQUICKWINDOWMODULE_P_P_H
#define QQUICKWINDOWMODULE_P_P_H

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

#include "qquickwindow_p.h"
#include <QtQml/private/qv4persistent_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickWindowQmlImplPrivate : public QQuickWindowPrivate
{
public:
    QQuickWindowQmlImplPrivate();

    bool visible = false;
    bool visibleExplicitlySet = false;
    QQuickWindow::Visibility visibility = QQuickWindow::AutomaticVisibility;
    QV4::PersistentValue rootItemMarker;
};

QT_END_NAMESPACE

#endif // QQUICKWINDOWMODULE_P_P_H
