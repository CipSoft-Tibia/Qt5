// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qimage.h>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>

#if defined(__ARM_NEON__)

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_neon(quint32 *dst, const uchar *src, int len)
{
    if (!len)
        return;

    const quint32 *const end = dst + len;

    // align dst on 128 bits
    const int offsetToAlignOn16Bytes = (reinterpret_cast<quintptr>(dst) >> 2) & 0x3;
    for (int i = 0; i < qMin(len, offsetToAlignOn16Bytes); ++i) {
        *dst++ = qRgb(src[0], src[1], src[2]);
        src += 3;
    }

    if ((len - offsetToAlignOn16Bytes) >= 16) {
        const quint32 *const simdEnd = end - 15;
        uint8x16x4_t dstVector;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        dstVector.val[0] = vdupq_n_u8(0xff);
#else
        dstVector.val[3] = vdupq_n_u8(0xff);
#endif
        do {
            uint8x16x3_t srcVector = vld3q_u8(src);
            src += 3 * 16;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            dstVector.val[1] = srcVector.val[0];
            dstVector.val[2] = srcVector.val[1];
            dstVector.val[3] = srcVector.val[2];
#else
            dstVector.val[0] = srcVector.val[2];
            dstVector.val[1] = srcVector.val[1];
            dstVector.val[2] = srcVector.val[0];
#endif
            vst4q_u8(reinterpret_cast<uint8_t*>(dst), dstVector);
            dst += 16;
        } while (dst < simdEnd);
    }

    int i = 0;
    int length = end - dst;
    SIMD_EPILOGUE(i, length, 15) {
        *dst++ = qRgb(src[0], src[1], src[2]);
        src += 3;
    }
}

void convert_RGB888_to_RGB32_neon(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGB888);
    Q_ASSERT(dest->format == QImage::Format_RGB32 || dest->format == QImage::Format_ARGB32 || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const uchar *src_data = (uchar *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        qt_convert_rgb888_to_rgb32_neon(dest_data, src_data, src->width);
        src_data += src->bytes_per_line;
        dest_data = (quint32 *)((uchar*)dest_data + dest->bytes_per_line);
    }
}

QT_END_NAMESPACE

#endif // defined(__ARM_NEON__)
