/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
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

#ifndef TYPE_CONVERSION_H
#define TYPE_CONVERSION_H

#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QIcon>
#include <QImage>
#include <QNetworkCookie>
#include <QRect>
#include <QString>
#include <QUrl>
#include <base/strings/nullable_string16.h>
#include "base/files/file_path.h"
#include "base/time/time.h"
#include "favicon_manager.h"
#include "net/cookies/canonical_cookie.h"
#include "third_party/blink/public/mojom/favicon/favicon_url.mojom-forward.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "third_party/skia/include/core/SkMatrix44.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "url/gurl.h"

QT_FORWARD_DECLARE_CLASS(QMatrix4x4)
QT_FORWARD_DECLARE_CLASS(QSslCertificate)

namespace gfx {
class ImageSkiaRep;
}

namespace net {
class X509Certificate;
}

namespace QtWebEngineCore {

inline QString toQt(const base::string16 &string)
{
#if defined(OS_WIN)
    return QString::fromStdWString(string.data());
#else
    return QString::fromUtf16(string.data());
#endif
}

inline QString toQt(const base::Optional<base::string16> &string)
{
    if (!string.has_value())
        return QString();
#if defined(OS_WIN)
    return QString::fromStdWString(string->data());
#else
    return QString::fromUtf16(string->data());
#endif
}

inline QString toQString(const std::string &string)
{
    return QString::fromStdString(string);
}

inline QByteArray toQByteArray(const std::string &string)
{
    return QByteArray::fromStdString(string);
}

// ### should probably be toQByteArray
inline QString toQt(const std::string &string)
{
    return toQString(string);
}

inline base::string16 toString16(const QString &qString)
{
#if defined(OS_WIN)
    return base::string16(qString.toStdWString());
#else
    return base::string16(qString.utf16());
#endif
}

inline base::NullableString16 toNullableString16(const QString &qString)
{
    return base::NullableString16(toString16(qString), qString.isNull());
}

inline base::Optional<base::string16> toOptionalString16(const QString &qString)
{
    if (qString.isNull())
        return base::nullopt;
    return base::make_optional(toString16(qString));
}

inline QUrl toQt(const GURL &url)
{
    if (url.is_valid())
        return QUrl::fromEncoded(toQByteArray(url.spec()));

    return QUrl(toQString(url.possibly_invalid_spec()));
}

inline GURL toGurl(const QUrl& url)
{
    return GURL(url.toEncoded().toStdString());
}

inline QPoint toQt(const gfx::Point &point)
{
    return QPoint(point.x(), point.y());
}

inline QPointF toQt(const gfx::Vector2dF &point)
{
    return QPointF(point.x(), point.y());
}

inline gfx::Point toGfx(const QPoint& point)
{
  return gfx::Point(point.x(), point.y());
}

inline gfx::PointF toGfx(const QPointF& point)
{
  return gfx::PointF(point.x(), point.y());
}

inline QRect toQt(const gfx::Rect &rect)
{
    return QRect(rect.x(), rect.y(), rect.width(), rect.height());
}

inline QRectF toQt(const gfx::RectF &rect)
{
    return QRectF(rect.x(), rect.y(), rect.width(), rect.height());
}

inline gfx::Size toGfx(const QSize &size)
{
    return gfx::Size(size.width(), size.height());
}

inline QSize toQt(const gfx::Size &size)
{
    return QSize(size.width(), size.height());
}

inline gfx::SizeF toGfx(const QSizeF& size)
{
  return gfx::SizeF(size.width(), size.height());
}

inline gfx::Rect toGfx(const QRect &rect)
{
    return gfx::Rect(rect.x(), rect.y(), rect.width(), rect.height());
}

inline QSizeF toQt(const gfx::SizeF &size)
{
    return QSizeF(size.width(), size.height());
}

inline QColor toQt(const SkColor &c)
{
    return QColor(SkColorGetR(c), SkColorGetG(c), SkColorGetB(c), SkColorGetA(c));
}

inline SkColor toSk(const QColor &c)
{
    return c.rgba();
}

inline QImage toQImage(const SkBitmap &bitmap, QImage::Format format)
{
    SkPixelRef *pixelRef = bitmap.pixelRef();
    return QImage((uchar *)pixelRef->pixels(), bitmap.width(), bitmap.height(), format);
}

QImage toQImage(const SkBitmap &bitmap);
QImage toQImage(const gfx::ImageSkiaRep &imageSkiaRep);
SkBitmap toSkBitmap(const QImage &image);

QIcon toQIcon(const std::vector<SkBitmap> &bitmaps);

void convertToQt(const SkMatrix44 &m, QMatrix4x4 &c);

inline QDateTime toQt(base::Time time)
{
    return QDateTime::fromMSecsSinceEpoch(time.ToJavaTime());
}

inline base::Time toTime(const QDateTime &dateTime) {
    return base::Time::FromJavaTime(dateTime.toMSecsSinceEpoch());
}

inline QNetworkCookie toQt(const net::CanonicalCookie & cookie)
{
    QNetworkCookie qCookie = QNetworkCookie(QByteArray::fromStdString(cookie.Name()), QByteArray::fromStdString(cookie.Value()));
    qCookie.setDomain(toQt(cookie.Domain()));
    if (!cookie.ExpiryDate().is_null())
        qCookie.setExpirationDate(toQt(cookie.ExpiryDate()));
    qCookie.setHttpOnly(cookie.IsHttpOnly());
    qCookie.setPath(toQt(cookie.Path()));
    qCookie.setSecure(cookie.IsSecure());
    return qCookie;
}

inline base::FilePath::StringType toFilePathString(const QString &str)
{
#if defined(OS_WIN)
    return QDir::toNativeSeparators(str).toStdWString();
#else
    return str.toStdString();
#endif
}

inline base::FilePath toFilePath(const QString &str)
{
    return base::FilePath(toFilePathString(str));
}

int flagsFromModifiers(Qt::KeyboardModifiers modifiers);

inline QStringList fromVector(const std::vector<base::string16> &vector)
{
    QStringList result;
    for (auto s: vector) {
      result.append(toQt(s));
    }
    return result;
}

FaviconInfo toFaviconInfo(const blink::mojom::FaviconURLPtr &favicon_url);

QList<QSslCertificate> toCertificateChain(net::X509Certificate *certificate);

} // namespace QtWebEngineCore

#endif // TYPE_CONVERSION_H
