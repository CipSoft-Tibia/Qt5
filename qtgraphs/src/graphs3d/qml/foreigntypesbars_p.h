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

#ifndef FOREIGNTYPESBARS_P_H
#define FOREIGNTYPESBARS_P_H

#include <QtCore/private/qglobal_p.h>
#include <QtGraphs/qbar3dseries.h>
#include <QtGraphs/qbardataproxy.h>
#include <QtGraphs/qitemmodelbardataproxy.h>

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

DEFINE_FOREIGN_CREATABLE_TYPE(QItemModelBarDataProxy, ItemModelBarDataProxy, 6)

DEFINE_FOREIGN_REPLACED_TYPE(QBar3DSeries, QBar3DSeries, Bar3DSeries)

DEFINE_FOREIGN_UNCREATABLE_TYPE(QBarDataProxy, BarDataProxy)

QT_END_NAMESPACE

#endif // FOREIGNTYPESBARS_P_H
