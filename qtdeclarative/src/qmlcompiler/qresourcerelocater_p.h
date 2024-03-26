// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QRESOURCERELOCATER_P_H
#define QRESOURCERELOCATER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qtqmlcompilerexports_p.h>

#include <QtCore/qstring.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

int Q_QMLCOMPILER_PRIVATE_EXPORT qRelocateResourceFile(const QString &input, const QString &output);

QT_END_NAMESPACE

#endif // QRESOURCERELOCATER_P_H
