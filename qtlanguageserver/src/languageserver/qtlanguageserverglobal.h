// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTLANGUAGESERVERGLOBAL_H
#define QTLANGUAGESERVERGLOBAL_H
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#    if defined(QT_BUILD_LANGUAGESERVER_LIB)
#        define Q_LANGUAGESERVER_EXPORT Q_DECL_EXPORT
#    else
#        define Q_LANGUAGESERVER_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define Q_LANGUAGESERVER_EXPORT
#endif

QT_END_NAMESPACE

#endif // QTLANGUAGESERVERGLOBAL_H
