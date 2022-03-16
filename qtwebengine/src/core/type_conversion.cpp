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

#include "type_conversion.h"

#include <content/public/common/favicon_url.h>
#include <ui/events/event_constants.h>
#include <ui/gfx/image/image_skia.h>
#include <QtCore/qcoreapplication.h>

namespace QtWebEngineCore {

QImage toQImage(const SkBitmap &bitmap)
{
    QImage image;
    switch (bitmap.colorType()) {
    case kUnknown_SkColorType:
    case kRGBA_F16_SkColorType:
    case kRGBA_F32_SkColorType:
        qWarning("Unknown or unsupported skia image format");
        break;
    case kAlpha_8_SkColorType:
        image = toQImage(bitmap, QImage::Format_Alpha8);
        break;
    case kRGB_565_SkColorType:
        image = toQImage(bitmap, QImage::Format_RGB16);
        break;
    case kARGB_4444_SkColorType:
        switch (bitmap.alphaType()) {
        case kUnknown_SkAlphaType:
            break;
        case kUnpremul_SkAlphaType:
            // not supported - treat as opaque
        case kOpaque_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_RGB444);
            break;
        case kPremul_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_ARGB4444_Premultiplied);
            break;
        }
        break;
    case kRGB_888x_SkColorType:
    case kRGBA_8888_SkColorType:
        switch (bitmap.alphaType()) {
        case kUnknown_SkAlphaType:
            break;
        case kOpaque_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_RGBX8888);
            break;
        case kPremul_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_RGBA8888_Premultiplied);
            break;
        case kUnpremul_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_RGBA8888);
            break;
        }
        break;
    case kBGRA_8888_SkColorType:
        // we are assuming little-endian arch here.
        switch (bitmap.alphaType()) {
        case kUnknown_SkAlphaType:
            break;
        case kOpaque_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_RGB32);
            break;
        case kPremul_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_ARGB32_Premultiplied);
            break;
        case kUnpremul_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_ARGB32);
            break;
        }
        break;
    case kRGB_101010x_SkColorType:
    case kRGBA_1010102_SkColorType:
        switch (bitmap.alphaType()) {
        case kUnknown_SkAlphaType:
            break;
        case kUnpremul_SkAlphaType:
            // not supported - treat as opaque
        case kOpaque_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_RGB30);
            break;
        case kPremul_SkAlphaType:
            image = toQImage(bitmap, QImage::Format_A2RGB30_Premultiplied);
            break;
        }
        break;
    case kGray_8_SkColorType:
        image = toQImage(bitmap, QImage::Format_Grayscale8);
        break;
    }
    return image;
}

QImage toQImage(const gfx::ImageSkiaRep &imageSkiaRep)
{
    QImage image = toQImage(imageSkiaRep.sk_bitmap());
    if (!image.isNull() && imageSkiaRep.scale() != 1.0f)
        image.setDevicePixelRatio(imageSkiaRep.scale());
    return image;
}

SkBitmap toSkBitmap(const QImage &image)
{
    SkBitmap bitmap;
    SkImageInfo imageInfo;

    switch (image.format()) {
    case QImage::Format_RGB32:
        imageInfo = SkImageInfo::Make(image.width(), image.height(), kBGRA_8888_SkColorType, kOpaque_SkAlphaType);
        break;
    case QImage::Format_ARGB32:
        imageInfo = SkImageInfo::Make(image.width(), image.height(), kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
        break;
    case QImage::Format_ARGB32_Premultiplied:
        imageInfo = SkImageInfo::Make(image.width(), image.height(), kBGRA_8888_SkColorType, kPremul_SkAlphaType);
        break;
    case QImage::Format_RGBX8888:
        imageInfo = SkImageInfo::Make(image.width(), image.height(), kRGBA_8888_SkColorType, kOpaque_SkAlphaType);
        break;
    case QImage::Format_RGBA8888:
        imageInfo = SkImageInfo::Make(image.width(), image.height(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        imageInfo = SkImageInfo::Make(image.width(), image.height(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        break;
    default:
        return toSkBitmap(image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    }

    bitmap.installPixels(imageInfo, (void *)image.bits(), image.bytesPerLine());

    // Ensure we copy the pixels
    SkBitmap bitmapCopy;
    bitmapCopy.allocPixels(imageInfo);
    bitmapCopy.writePixels(bitmap.pixmap());

    return bitmapCopy;
}

QIcon toQIcon(const std::vector<SkBitmap> &bitmaps)
{
    if (!bitmaps.size())
        return QIcon();

    QIcon icon;

    for (unsigned i = 0; i < bitmaps.size(); ++i) {
        SkBitmap bitmap = bitmaps[i];
        QImage image = toQImage(bitmap);

        icon.addPixmap(QPixmap::fromImage(image).copy());
    }

    return icon;
}

int flagsFromModifiers(Qt::KeyboardModifiers modifiers)
{
    int modifierFlags = ui::EF_NONE;
#if defined(Q_OS_OSX)
    if (!qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        if ((modifiers & Qt::ControlModifier) != 0)
            modifierFlags |= ui::EF_COMMAND_DOWN;
        if ((modifiers & Qt::MetaModifier) != 0)
            modifierFlags |= ui::EF_CONTROL_DOWN;
    } else
#endif
    {
        if ((modifiers & Qt::ControlModifier) != 0)
            modifierFlags |= ui::EF_CONTROL_DOWN;
        if ((modifiers & Qt::MetaModifier) != 0)
            modifierFlags |= ui::EF_COMMAND_DOWN;
    }
    if ((modifiers & Qt::ShiftModifier) != 0)
        modifierFlags |= ui::EF_SHIFT_DOWN;
    if ((modifiers & Qt::AltModifier) != 0)
        modifierFlags |= ui::EF_ALT_DOWN;
    return modifierFlags;
}

FaviconInfo::FaviconTypeFlags toQt(content::FaviconURL::IconType type)
{
    switch (type) {
    case content::FaviconURL::IconType::kFavicon:
        return FaviconInfo::Favicon;
    case content::FaviconURL::IconType::kTouchIcon:
        return FaviconInfo::TouchIcon;
    case content::FaviconURL::IconType::kTouchPrecomposedIcon:
        return FaviconInfo::TouchPrecomposedIcon;
    case content::FaviconURL::IconType::kInvalid:
        return FaviconInfo::InvalidIcon;
    }
    Q_UNREACHABLE();
    return FaviconInfo::InvalidIcon;
}

FaviconInfo toFaviconInfo(const content::FaviconURL &favicon_url)
{
    FaviconInfo info;
    info.url = toQt(favicon_url.icon_url);
    info.type = toQt(favicon_url.icon_type);
    // TODO: Add support for rel sizes attribute (favicon_url.icon_sizes):
    // http://www.w3schools.com/tags/att_link_sizes.asp
    info.size = QSize(0, 0);
    return info;
}

} // namespace QtWebEngineCore
