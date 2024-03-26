// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOCODINGMANAGERENGINE_P_H
#define QGEOCODINGMANAGERENGINE_P_H

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

#include "qgeocodingmanagerengine.h"

#include <QLocale>

QT_BEGIN_NAMESPACE

class QGeoCodingManagerEnginePrivate
{
public:
    QGeoCodingManagerEnginePrivate() = default;

    QString managerName;
    QLocale locale;
    int managerVersion = -1;

private:
    Q_DISABLE_COPY(QGeoCodingManagerEnginePrivate)
};

QT_END_NAMESPACE

#endif
