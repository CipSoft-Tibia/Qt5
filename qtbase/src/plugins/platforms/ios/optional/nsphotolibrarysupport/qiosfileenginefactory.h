// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSFILEENGINEFACTORY_H
#define QIOSFILEENGINEFACTORY_H

#include <QtCore/qstandardpaths.h>
#include <QtCore/private/qabstractfileengine_p.h>
#include "qiosfileengineassetslibrary.h"

QT_BEGIN_NAMESPACE

class QIOSFileEngineFactory : public QAbstractFileEngineHandler
{
public:
    QAbstractFileEngine* create(const QString &fileName) const
    {
        Q_CONSTINIT static QLatin1StringView assetsScheme("assets-library:");

#ifndef Q_OS_TVOS
        if (fileName.toLower().startsWith(assetsScheme))
            return new QIOSFileEngineAssetsLibrary(fileName);
#else
        Q_UNUSED(fileName);
#endif

        return 0;
    }
};

QT_END_NAMESPACE

#endif // QIOSFILEENGINEFACTORY_H
