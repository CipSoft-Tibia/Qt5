// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DRENDER_QSCENEEXPORTPLUGIN_P_H
#define QT3DRENDER_QSCENEEXPORTPLUGIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

#include <private/qt3drender_global_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

#define QSceneExportFactoryInterface_iid "org.qt-project.Qt3DRender.QSceneExportFactoryInterface 5.9"

class QSceneExporter;

class Q_3DRENDERSHARED_PRIVATE_EXPORT QSceneExportPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QSceneExportPlugin(QObject *parent = nullptr);
    ~QSceneExportPlugin();

    virtual QSceneExporter *create(const QString &key, const QStringList &paramList);
};

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_QSCENEEXPORTPLUGIN_P_H
