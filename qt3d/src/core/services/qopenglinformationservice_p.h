// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DCORE_QOPENGLINFORMATIONSERVICE_P_H
#define QT3DCORE_QOPENGLINFORMATIONSERVICE_P_H

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

#include <Qt3DCore/qt3dcore_global.h>
#include <QtCore/qstring.h>
#include <QtGui/qsurfaceformat.h>

#include <Qt3DCore/private/qservicelocator_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

class QOpenGLInformationServicePrivate;

class Q_3DCORESHARED_EXPORT QOpenGLInformationService : public QAbstractServiceProvider
{
    Q_OBJECT
public:
    virtual QSurfaceFormat format() const = 0;

protected:
    QOpenGLInformationService(const QString &description = QString());
    QOpenGLInformationService(QOpenGLInformationServicePrivate &dd);
};

} // namespace Qt3DCore

QT_END_NAMESPACE

#endif // QT3DCORE_QOPENGLINFORMATIONSERVICE_P_H
