// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTVERSIONCHECKS_H
#define QTVERSIONCHECKS_H

#if 0
#pragma qt_class(QtVersionChecks)
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qtconfiginclude.h>

/*
   QT_VERSION is (major << 16) | (minor << 8) | patch.
*/
#define QT_VERSION      QT_VERSION_CHECK(QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH)
/*
   can be used like #if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
*/
#define QT_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

/*
   Helper macros to make some simple code active in Qt 6 or Qt 7 only,
   like:
     struct QT6_ONLY(Q_CORE_EXPORT) QTrivialClass
     {
         void QT7_ONLY(Q_CORE_EXPORT) void operate();
     }
*/
#if QT_VERSION_MAJOR == 7 || defined(QT_BOOTSTRAPPED)
#  define QT7_ONLY(...)         __VA_ARGS__
#  define QT6_ONLY(...)
#elif QT_VERSION_MAJOR == 6
#  define QT7_ONLY(...)
#  define QT6_ONLY(...)         __VA_ARGS__
#else
#  error Qt major version not 6 or 7
#endif

/* Macro and tag type to help overload resolution on functions
   that are, e.g., QT_REMOVED_SINCE'ed. Example use:

   #if QT_CORE_REMOVED_SINCE(6, 4)
   int size() const;
   #endif
   qsizetype size(QT6_DECL_NEW_OVERLOAD) const;

   in the normal cpp file:

   qsizetype size(QT6_IMPL_NEW_OVERLOAD) const {
      ~~~
   }

   in removed_api.cpp:

   int size() const { return int(size(QT6_CALL_NEW_OVERLOAD)); }
*/
#ifdef Q_QDOC
# define QT6_DECL_NEW_OVERLOAD
# define QT6_DECL_NEW_OVERLOAD_TAIL
# define QT6_IMPL_NEW_OVERLOAD
# define QT6_IMPL_NEW_OVERLOAD_TAIL
# define QT6_CALL_NEW_OVERLOAD
# define QT6_CALL_NEW_OVERLOAD_TAIL
#else
# define QT6_DECL_NEW_OVERLOAD QT6_ONLY(Qt::Disambiguated_t = Qt::Disambiguated)
# define QT6_DECL_NEW_OVERLOAD_TAIL QT6_ONLY(, QT6_DECL_NEW_OVERLOAD)
# define QT6_IMPL_NEW_OVERLOAD QT6_ONLY(Qt::Disambiguated_t)
# define QT6_IMPL_NEW_OVERLOAD_TAIL QT6_ONLY(, QT6_IMPL_NEW_OVERLOAD)
# define QT6_CALL_NEW_OVERLOAD QT6_ONLY(Qt::Disambiguated)
# define QT6_CALL_NEW_OVERLOAD_TAIL QT6_ONLY(, QT6_CALL_NEW_OVERLOAD)
#endif

#endif /* QTVERSIONCHECKS_H */
