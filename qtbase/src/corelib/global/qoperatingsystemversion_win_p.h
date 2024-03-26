// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPERATINGSYSTEMVERSION_WIN_P_H
#define QOPERATINGSYSTEMVERSION_WIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

OSVERSIONINFOEX qWindowsVersionInfo();

QT_END_NAMESPACE

#endif // QOPERATINGSYSTEMVERSION_WIN_P_H
