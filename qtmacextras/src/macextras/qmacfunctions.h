/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtMacExtras module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMACFUNCTIONS_H
#define QMACFUNCTIONS_H

#if 0
#pragma qt_class(QtMac)
#endif

#include <QtMacExtras/qmacextrasglobal.h>

typedef struct CGImage *CGImageRef;
typedef struct CGContext *CGContextRef;

Q_FORWARD_DECLARE_OBJC_CLASS(NSData);
Q_FORWARD_DECLARE_OBJC_CLASS(NSImage);

QT_BEGIN_NAMESPACE

class QByteArray;
class QMenu;
class QMenuBar;
class QPixmap;
class QString;
class QUrl;
class QWindow;

namespace QtMac
{
#if QT_DEPRECATED_SINCE(5,3)
QT_DEPRECATED_X("Use QByteArray::toNSData") Q_MACEXTRAS_EXPORT NSData *toNSData(const QByteArray &data);
QT_DEPRECATED_X("Use QByteArray::fromNSData") Q_MACEXTRAS_EXPORT QByteArray fromNSData(const NSData *data);
#endif

#if QT_DEPRECATED_SINCE(5, 12)
QT_DEPRECATED_X("Use QPixmap::toImage and QImage::toCGImage") Q_MACEXTRAS_EXPORT CGImageRef toCGImageRef(const QPixmap &pixmap);
#endif
Q_MACEXTRAS_EXPORT QPixmap fromCGImageRef(CGImageRef image);

#if QT_DEPRECATED_SINCE(5, 12)
# if defined(QT_PLATFORM_UIKIT)
QT_DEPRECATED_X("Use UIGraphicsGetCurrentContext()")
# else
QT_DEPRECATED_X("Use NSGraphicsContext.currentContext.CGContext")
# endif
Q_MACEXTRAS_EXPORT CGContextRef currentCGContext();
#endif

#ifdef Q_OS_OSX
#if QT_DEPRECATED_SINCE(5, 12)
QT_DEPRECATED_X("Use NSApp.dockTile.badgeLabel") Q_MACEXTRAS_EXPORT void setBadgeLabelText(const QString &text);
QT_DEPRECATED_X("Use NSApp.dockTile.badgeLabel") Q_MACEXTRAS_EXPORT QString badgeLabelText();

QT_DEPRECATED_X("Use QPixmap::toImage, QImage::toCGImage, and -[NSImage initWithCGImage:size:]")
Q_MACEXTRAS_EXPORT NSImage *toNSImage(const QPixmap &pixmap);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
Q_MACEXTRAS_EXPORT bool isMainWindow(QWindow *window);
#endif
#endif // Q_OS_OSX

#if defined(QT_PLATFORM_UIKIT) && !defined(Q_OS_WATCHOS)
#if QT_DEPRECATED_SINCE(5, 12)
QT_DEPRECATED_X("Use UIApplication.sharedApplication.applicationIconBadgeNumber") Q_MACEXTRAS_EXPORT void setApplicationIconBadgeNumber(int number) __attribute__((availability(ios_app_extension,unavailable)));
QT_DEPRECATED_X("Use UIApplication.sharedApplication.applicationIconBadgeNumber") Q_MACEXTRAS_EXPORT int applicationIconBadgeNumber() __attribute__((availability(ios_app_extension,unavailable)));
#endif
#endif // defined(QT_PLATFORM_UIKIT) && !defined(Q_OS_WATCHOS)
}

QT_END_NAMESPACE

#endif // QMACFUNCTIONS_H
