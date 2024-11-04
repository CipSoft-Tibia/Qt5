// Copyright (C) 2023 The Qt Company Ltd.
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

#ifndef FOREIGNTYPES_P_H
#define FOREIGNTYPES_P_H

#include <QtQml/qqml.h>

#include <QtCore/qabstractitemmodel.h>

#include <QtCore/private/qglobal_p.h>
#include <QtGraphs/q3dinputhandler.h>
#include <QtGraphs/q3dscene.h>
#include <QtGraphs/qabstract3daxis.h>
#include <QtGraphs/qabstract3dinputhandler.h>
#include <QtGraphs/qabstract3dseries.h>
#include <QtGraphs/qabstractdataproxy.h>
#include <QtGraphs/qcategory3daxis.h>
#include <QtGraphs/qcustom3ditem.h>
#include <QtGraphs/qcustom3dlabel.h>
#include <QtGraphs/qcustom3dvolume.h>
#include <QtGraphs/qlogvalue3daxisformatter.h>
#include <QtGraphs/qtouch3dinputhandler.h>
#include <QtGraphs/qvalue3daxis.h>
#include <QtGraphs/qvalue3daxisformatter.h>

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
        QML_UNCREATABLE("Trying to create uncreatable: " #name ".") \
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

struct Q3DSceneForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(Q3DScene)
};

DEFINE_FOREIGN_CREATABLE_TYPE(QCategory3DAxis, CategoryAxis3D, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QValue3DAxis, ValueAxis3D, 6)

DEFINE_FOREIGN_CREATABLE_TYPE(QCustom3DItem, Custom3DItem, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QCustom3DLabel, Custom3DLabel, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QLogValue3DAxisFormatter, LogValueAxis3DFormatter, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QValue3DAxisFormatter, ValueAxis3DFormatter, 6)

DEFINE_FOREIGN_CREATABLE_TYPE(Q3DInputHandler, InputHandler3D, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QCustom3DVolume, Custom3DVolume, 6)
DEFINE_FOREIGN_CREATABLE_TYPE(QTouch3DInputHandler, TouchInputHandler3D, 6)

DEFINE_FOREIGN_UNCREATABLE_TYPE(QAbstract3DAxis, AbstractAxis3D)
DEFINE_FOREIGN_UNCREATABLE_TYPE(QAbstract3DInputHandler, AbstractInputHandler3D)
DEFINE_FOREIGN_UNCREATABLE_TYPE(QAbstract3DSeries, Abstract3DSeries)
DEFINE_FOREIGN_UNCREATABLE_TYPE(QAbstractDataProxy, AbstractDataProxy)
DEFINE_FOREIGN_UNCREATABLE_TYPE(QAbstractItemModel, AbstractItemModel)

QT_END_NAMESPACE

#endif // FOREIGNTYPES_P_H
