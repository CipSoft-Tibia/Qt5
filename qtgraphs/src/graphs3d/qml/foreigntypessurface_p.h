// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef FOREIGNTYPESSURFACE_P_H
#define FOREIGNTYPESSURFACE_P_H

#include <QtCore/private/qglobal_p.h>
#include <QtGraphs/qheightmapsurfacedataproxy.h>
#include <QtGraphs/qitemmodelsurfacedataproxy.h>
#include <QtGraphs/qsurface3dseries.h>
#include <QtGraphs/qsurfacedataproxy.h>

QT_BEGIN_NAMESPACE

#define DEFINE_FOREIGN_BASE_ATTRIBUTES(type, name, minor) \
    Q_GADGET \
    QML_NAMED_ELEMENT(name) \
    QML_FOREIGN(type) \
    QML_ADDED_IN_VERSION(6, minor)

#define DEFINE_FOREIGN_UNCREATABLE_TYPE(type, name) \
    struct type##GraphsForeign \
    { \
        DEFINE_FOREIGN_BASE_ATTRIBUTES(type, name, 6) \
        QML_UNCREATABLE("") \
    };

#define DEFINE_FOREIGN_CREATABLE_TYPE(type, name, minor) \
    struct type##GraphsForeign \
    { \
        DEFINE_FOREIGN_BASE_ATTRIBUTES(type, name, minor) \
    };

#define DEFINE_FOREIGN_REPLACED_TYPE(type, name, better) \
    struct type##GraphsForeign \
    { \
        DEFINE_FOREIGN_BASE_ATTRIBUTES(type, name, 6) \
        QML_UNCREATABLE("Trying to create uncreatable: " #name ", use " #better " instead.") \
    };

DEFINE_FOREIGN_CREATABLE_TYPE(QHeightMapSurfaceDataProxy, HeightMapSurfaceDataProxy, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QItemModelSurfaceDataProxy, ItemModelSurfaceDataProxy, 6)

DEFINE_FOREIGN_REPLACED_TYPE(QSurface3DSeries, QSurface3DSeries, Surface3DSeries)

DEFINE_FOREIGN_UNCREATABLE_TYPE(QSurfaceDataProxy, SurfaceDataProxy)

QT_END_NAMESPACE

#endif // FOREIGNTYPESSURFACE_P_H
