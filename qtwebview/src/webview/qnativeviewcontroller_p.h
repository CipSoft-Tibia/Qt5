// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNATIVEVIEWCONTROLLER_P_H
#define QNATIVEVIEWCONTROLLER_P_H

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

#include "qwebview_global.h"
#include <QtCore/qrect.h>
#include <QtGui/qwindow.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QNativeViewController
{
public:
    virtual ~QNativeViewController() {}
    virtual void setParentView(QObject *view) = 0;
    virtual QObject *parentView() const = 0;
    virtual void setGeometry(const QRect &geometry) = 0;
    virtual void setVisibility(QWindow::Visibility visibility) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual void init() { }
    virtual void setFocus(bool focus) { Q_UNUSED(focus); }
    virtual void updatePolish() { }
};

QT_END_NAMESPACE

#endif // QNATIVEVIEWCONTROLLER_P_H

