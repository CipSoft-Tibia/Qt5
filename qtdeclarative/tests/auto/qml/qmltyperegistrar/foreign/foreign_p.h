// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FOREIGN_P_H
#define FOREIGN_P_H

#include <QtCore/qobject.h>

// qmltyperegistrar will assume this file is reachable under <private/foreign_p.h>
// It's not true, but this is how it works on actual private headers in Qt.
// See the trick in tst_qmltyperegistrar's CMakeLists.txt to turn on the --private-includes option.
class ForeignPrivate : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void happens();
};

#endif // FOREIGN_P_H
