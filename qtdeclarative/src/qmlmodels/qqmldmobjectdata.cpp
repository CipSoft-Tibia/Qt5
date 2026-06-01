// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qqmldmobjectdata_p.h>

QT_BEGIN_NAMESPACE

QQmlDMObjectData::QQmlDMObjectData(const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
        VDMObjectDelegateDataType *dataType,
        int index, int row, int column,
        QObject *object)
    : QQmlDelegateModelItem(metaType, dataType, index, row, column)
    , object(object)
{
    new QQmlDMObjectDataMetaObject(this, dataType);
}

QQmlRefPointer<QQmlContextData> QQmlDMObjectData::initProxy()
{
    QQmlRefPointer<QQmlContextData> ctxt = QQmlContextData::createChild(contextData);
    incubationTask->proxiedObject = object;
    incubationTask->proxyContext = ctxt;
    ctxt->setContextObject(this);
    // We don't own the proxied object. We need to clear it if it goes away.
    QObject::connect(object, &QObject::destroyed,
                     this, &QQmlDelegateModelItem::childContextObjectDestroyed);
    return ctxt;
}

QT_END_NAMESPACE
