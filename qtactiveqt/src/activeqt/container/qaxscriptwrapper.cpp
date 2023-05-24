// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "qaxobject.h"

#include <QtAxServer/qaxfactory.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

QAxBase *qax_create_object_wrapper(QObject *object)
{
    IDispatch *dispatch = nullptr;
    QAxObject *wrapper = nullptr;
    qAxFactory()->createObjectWrapper(object, &dispatch);
    if (dispatch) {
        wrapper = new QAxObject(dispatch, object);
        wrapper->setObjectName(object->objectName());
        dispatch->Release();
    }
    return wrapper;
}

QT_END_NAMESPACE
