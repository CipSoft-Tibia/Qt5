// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSELECTABLE_P_H
#define QQUICKSELECTABLE_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickSelectable
{
public:
    virtual QQuickItem *selectionPointerHandlerTarget() const = 0;

    virtual bool startSelection(const QPointF &pos) = 0;
    virtual void setSelectionStartPos(const QPointF &pos) = 0;
    virtual void setSelectionEndPos(const QPointF &pos) = 0;
    virtual void clearSelection() = 0;
    virtual void normalizeSelection() = 0;

    virtual QRectF selectionRectangle() const = 0;
    virtual QSizeF scrollTowardsSelectionPoint(const QPointF &pos, const QSizeF &step) = 0;
};

QT_END_NAMESPACE

#endif
