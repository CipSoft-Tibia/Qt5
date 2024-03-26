// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qimagescale_p.h"
#include "qimage.h"
#include <private/qsimd_p.h>

#if QT_CONFIG(thread) && !defined(Q_OS_WASM)
#include <qsemaphore.h>
#include <qthreadpool.h>
#include <private/qthreadpool_p.h>
#endif

#if defined(__ARM_NEON__)

QT_BEGIN_NAMESPACE

using namespace QImageScale;

template<typename T>
static inline void multithread_pixels_function(QImageScaleInfo *isi, int dh, const T &scaleSection)
{
#if QT_CONFIG(thread) && !defined(Q_OS_WASM)
    int segments = (qsizetype(isi->sh) * isi->sw) / (1<<16);
    segments = std::min(segments, dh);
    QThreadPool *threadPool = QThreadPoolPrivate::qtGuiInstance();
    if (segments > 1 && threadPool && !threadPool->contains(QThread::currentThread())) {
        QSemaphore semaphore;
        int y = 0;
        for (int i = 0; i < segments; ++i) {
            int yn = (dh - y) / (segments - i);
            threadPool->start([&, y, yn]() {
                scaleSection(y, y + yn);
                semaphore.release(1);
            });
            y += yn;
        }
        semaphore.acquire(segments);
        return;
    }
#endif
    scaleSection(0, dh);
}

inline static uint32x4_t qt_qimageScaleAARGBA_helper(const unsigned int *pix, int xyap, int Cxy, int step)
{
    uint32x2_t vpix32 = vmov_n_u32(*pix);
    uint16x4_t vpix16 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vpix32)));
    uint32x4_t vx = vmull_n_u16(vpix16, xyap);
    int i;
    for (i = (1 << 14) - xyap; i > Cxy; i -= Cxy) {
        pix += step;
        vpix32 = vmov_n_u32(*pix);
        vpix16 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vpix32)));
        vx = vaddq_u32(vx, vmull_n_u16(vpix16, Cxy));
    }
    pix += step;
    vpix32 = vmov_n_u32(*pix);
    vpix16 = vget_low_u16(vmovl_u8(vreinterpret_u8_u32(vpix32)));
    vx = vaddq_u32(vx, vmull_n_u16(vpix16, i));
    return vx;
}

template<bool RGB>
void qt_qimageScaleAARGBA_up_x_down_y_neon(QImageScaleInfo *isi, unsigned int *dest,
                                           int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    /* go through every scanline in the output buffer */
    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            int Cy = yapoints[y] >> 16;
            int yap = yapoints[y] & 0xffff;

            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                const unsigned int *sptr = ypoints[y] + xpoints[x];
                uint32x4_t vx = qt_qimageScaleAARGBA_helper(sptr, yap, Cy, sow);

                int xap = xapoints[x];
                if (xap > 0) {
                    uint32x4_t vr = qt_qimageScaleAARGBA_helper(sptr + 1, yap, Cy, sow);

                    vx = vmulq_n_u32(vx, 256 - xap);
                    vr = vmulq_n_u32(vr, xap);
                    vx = vaddq_u32(vx, vr);
                    vx = vshrq_n_u32(vx, 8);
                }
                vx = vshrq_n_u32(vx, 14);
                const uint16x4_t vx16 = vmovn_u32(vx);
                const uint8x8_t vx8 = vmovn_u16(vcombine_u16(vx16, vx16));
                *dptr = vget_lane_u32(vreinterpret_u32_u8(vx8), 0);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_x_up_y_neon(QImageScaleInfo *isi, unsigned int *dest,
                                           int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    /* go through every scanline in the output buffer */
    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                int Cx = xapoints[x] >> 16;
                int xap = xapoints[x] & 0xffff;

                const unsigned int *sptr = ypoints[y] + xpoints[x];
                uint32x4_t vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);

                int yap = yapoints[y];
                if (yap > 0) {
                    uint32x4_t vr = qt_qimageScaleAARGBA_helper(sptr + sow, xap, Cx, 1);

                    vx = vmulq_n_u32(vx, 256 - yap);
                    vr = vmulq_n_u32(vr, yap);
                    vx = vaddq_u32(vx, vr);
                    vx = vshrq_n_u32(vx, 8);
                }
                vx = vshrq_n_u32(vx, 14);
                const uint16x4_t vx16 = vmovn_u32(vx);
                const uint8x8_t vx8 = vmovn_u16(vcombine_u16(vx16, vx16));
                *dptr = vget_lane_u32(vreinterpret_u32_u8(vx8), 0);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template<bool RGB>
void qt_qimageScaleAARGBA_down_xy_neon(QImageScaleInfo *isi, unsigned int *dest,
                                       int dw, int dh, int dow, int sow)
{
    const unsigned int **ypoints = isi->ypoints;
    int *xpoints = isi->xpoints;
    int *xapoints = isi->xapoints;
    int *yapoints = isi->yapoints;

    auto scaleSection = [&] (int yStart, int yEnd) {
        for (int y = yStart; y < yEnd; ++y) {
            int Cy = yapoints[y] >> 16;
            int yap = yapoints[y] & 0xffff;

            unsigned int *dptr = dest + (y * dow);
            for (int x = 0; x < dw; x++) {
                const int Cx = xapoints[x] >> 16;
                const int xap = xapoints[x] & 0xffff;

                const unsigned int *sptr = ypoints[y] + xpoints[x];
                uint32x4_t vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);
                vx = vshrq_n_u32(vx, 4);
                uint32x4_t vr = vmulq_n_u32(vx, yap);

                int j;
                for (j = (1 << 14) - yap; j > Cy; j -= Cy) {
                    sptr += sow;
                    vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);
                    vx = vshrq_n_u32(vx, 4);
                    vx = vmulq_n_u32(vx, Cy);
                    vr = vaddq_u32(vr, vx);
                }
                sptr += sow;
                vx = qt_qimageScaleAARGBA_helper(sptr, xap, Cx, 1);
                vx = vshrq_n_u32(vx, 4);
                vx = vmulq_n_u32(vx, j);
                vr = vaddq_u32(vr, vx);

                vx = vshrq_n_u32(vr, 24);
                const uint16x4_t vx16 = vmovn_u32(vx);
                const uint8x8_t vx8 = vmovn_u16(vcombine_u16(vx16, vx16));
                *dptr = vget_lane_u32(vreinterpret_u32_u8(vx8), 0);
                if (RGB)
                    *dptr |= 0xff000000;
                dptr++;
            }
        }
    };
    multithread_pixels_function(isi, dh, scaleSection);
}

template void qt_qimageScaleAARGBA_up_x_down_y_neon<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                           int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_up_x_down_y_neon<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_neon<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                           int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_x_up_y_neon<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                          int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_neon<false>(QImageScaleInfo *isi, unsigned int *dest,
                                                       int dw, int dh, int dow, int sow);

template void qt_qimageScaleAARGBA_down_xy_neon<true>(QImageScaleInfo *isi, unsigned int *dest,
                                                      int dw, int dh, int dow, int sow);

QT_END_NAMESPACE

#endif
