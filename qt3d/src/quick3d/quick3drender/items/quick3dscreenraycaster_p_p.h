// Copyright (C) 2018 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DRENDER_RENDER_QUICK_QUICKSCREENRAYCASTER_P_P_H
#define QT3DRENDER_RENDER_QUICK_QUICKSCREENRAYCASTER_P_P_H

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

#include <Qt3DRender/private/qabstractraycaster_p.h>

#include <Qt3DQuickRender/private/qt3dquickrender_global_p.h>
#include <Qt3DQuick/private/quick3dnode_p.h>

#include <QtQml/QJSValue>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {
namespace Quick {

class Quick3DScreenRayCaster;

class Q_3DQUICKRENDERSHARED_PRIVATE_EXPORT Quick3DScreenRayCasterPrivate : public QAbstractRayCasterPrivate
{
public:
    explicit Quick3DScreenRayCasterPrivate() : QAbstractRayCasterPrivate() { }

    Q_DECLARE_PUBLIC(Quick3DScreenRayCaster)
};

} // namespace Quick
} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_RENDER_QUICK_QUICKSCREENRAYCASTER_P_P_H

