// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGEOCODINGMANAGER_P_H
#define QGEOCODINGMANAGER_P_H

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

#include "qgeocodingmanager.h"

#include <QList>

QT_BEGIN_NAMESPACE

class QGeoCodingManagerEngine;

class QGeoCodingManagerPrivate
{
public:
    QGeoCodingManagerPrivate() = default;

    std::unique_ptr<QGeoCodingManagerEngine> engine;

private:
    Q_DISABLE_COPY(QGeoCodingManagerPrivate)
};

QT_END_NAMESPACE

#endif

