/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsceneexportfactory_p.h"
#include "qsceneexportplugin_p.h"
#include "qsceneexporter_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>

#include <private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader, (QSceneExportFactoryInterface_iid, QLatin1String("/sceneparsers"), Qt::CaseInsensitive))
#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader, (QSceneExportFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))
#endif

QStringList QSceneExportFactory::keys(const QString &pluginPath)
{
#if QT_CONFIG(library)
    QStringList list;
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        list = directLoader()->keyMap().values();
        if (!list.isEmpty()) {
            const QString postFix = QLatin1String(" (from ")
                    + QDir::toNativeSeparators(pluginPath)
                    + QLatin1Char(')');
            const QStringList::iterator end = list.end();
            for (QStringList::iterator it = list.begin(); it != end; ++it)
                (*it).append(postFix);
        }
    }
    list.append(loader()->keyMap().values());
    return list;
#else
    return loader()->keyMap().values();
#endif
}

QSceneExporter *QSceneExportFactory::create(const QString &name, const QStringList &args,
                                            const QString &pluginPath)
{
#if QT_CONFIG(library)
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        if (QSceneExporter *ret = qLoadPlugin<QSceneExporter,
                QSceneExportPlugin>(directLoader(), name, args)) {
            return ret;
        }
    }
#endif
    return qLoadPlugin<QSceneExporter, QSceneExportPlugin>(loader(), name, args);
}

} // namespace Qt3DRender

QT_END_NAMESPACE
