// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef CHARTHELPERS_P_H
#define CHARTHELPERS_P_H

#include <QtCore/QtNumeric>
#include <QtCore/QPointF>

QT_BEGIN_NAMESPACE

static inline bool isValidValue(qreal value)
{
    if (qIsNaN(value) || qIsInf(value)) {
        qWarning("Ignored NaN, Inf, or -Inf value.");
        return false;
    }
    return true;
}

static inline bool isValidValue(qreal x, qreal y)
{
    return (isValidValue(x) && isValidValue(y));
}

static inline bool isValidValue(const QPointF point)
{
    return (isValidValue(point.x()) && isValidValue(point.y()));
}

QT_END_NAMESPACE

#endif // CHARTHELPERS_P_H
