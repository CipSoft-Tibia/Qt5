// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLENUMVALUE_P_H
#define QQMLENUMVALUE_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

struct QQmlEnumValue
{
    QQmlEnumValue() {}
    QQmlEnumValue(const QString &n, int v) : namedValue(n), value(v) {}
    QString namedValue;
    int value = -1;
};

QT_END_NAMESPACE

#endif // QQMLENUMVALUE_P_H
