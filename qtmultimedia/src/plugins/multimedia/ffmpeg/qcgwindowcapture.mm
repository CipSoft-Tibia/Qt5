// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcgwindowcapture_p.h"
#include "private/qcapturablewindow_p.h"
#include "qffmpegsurfacecapturegrabber_p.h"
#include "private/qabstractvideobuffer_p.h"

#include "qscreen.h"
#include "qguiapplication.h"
#include <qmutex.h>
#include <qwaitcondition.h>

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <AppKit/NSScreen.h>
#include <AppKit/NSApplication.h>
#include <AppKit/NSWindow.h>

namespace {

std::optional<qreal> frameRateForWindow(CGWindowID /*wid*/)
{
    // TODO: detect the frame rate
    // if (window && window.screen) {
    //     CGDirectDisplayID displayID = [window.screen.deviceDescription[@"NSScreenNumber"]
    //     unsignedIntValue]; const auto displayRefreshRate =
    //     CGDisplayModeGetRefreshRate(CGDisplayCopyDisplayMode(displayID)); if (displayRefreshRate
    //     > 0 && displayRefreshRate < frameRate) frameRate = displayRefreshRate;
    // }

    return {};
}

}

QT_BEGIN_NAMESPACE

class QCGImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    QCGImageVideoBuffer(CGImageRef image) : QAbstractVideoBuffer(QVideoFrame::NoHandle)
    {
        auto provider = CGImageGetDataProvider(image);
        m_data = CGDataProviderCopyData(provider);
        m_bytesPerLine = CGImageGetBytesPerRow(image);
    }

    ~QCGImageVideoBuffer() override { CFRelease(m_data); }

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped) {
            m_mapMode = mode;

            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = static_cast<int>(m_bytesPerLine);
            mapData.data[0] = (uchar *)CFDataGetBytePtr(m_data);
            mapData.size[0] = static_cast<int>(CFDataGetLength(m_data));
        }

        return mapData;
    }

    void unmap() override { m_mapMode = QVideoFrame::NotMapped; }

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    CFDataRef m_data;
    size_t m_bytesPerLine = 0;
};

class QCGWindowCapture::Grabber : public QFFmpegSurfaceCaptureGrabber
{
public:
    Grabber(QCGWindowCapture &capture, CGWindowID wid) : m_capture(capture), m_wid(wid)
    {
        addFrameCallback(*this, &Grabber::onNewFrame);
        connect(this, &Grabber::errorUpdated, &capture, &QCGWindowCapture::updateError);

        if (auto screen = QGuiApplication::primaryScreen())
            setFrameRate(screen->refreshRate());

        start();
    }

    ~Grabber() override { stop(); }

    QVideoFrameFormat frameFormat() const
    {
        QMutexLocker<QMutex> locker(&m_formatMutex);
        while (!m_format)
            m_waitForFormat.wait(&m_formatMutex);
        return *m_format;
    }

protected:
    QVideoFrame grabFrame() override
    {
        if (auto rate = frameRateForWindow(m_wid))
            setFrameRate(*rate);

        auto imageRef = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow,
                                                m_wid, kCGWindowImageBoundsIgnoreFraming);
        if (!imageRef) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Cannot create image by window"));
            return {};
        }

        auto imageDeleter = qScopeGuard([imageRef]() { CGImageRelease(imageRef); });

        if (CGImageGetBitsPerPixel(imageRef) != 32
            || CGImageGetPixelFormatInfo(imageRef) != kCGImagePixelFormatPacked
            || CGImageGetByteOrderInfo(imageRef) != kCGImageByteOrder32Little) {
            qWarning() << "Unexpected image format. PixelFormatInfo:"
                       << CGImageGetPixelFormatInfo(imageRef)
                       << "BitsPerPixel:" << CGImageGetBitsPerPixel(imageRef) << "AlphaInfo"
                       << CGImageGetAlphaInfo(imageRef)
                       << "ByteOrderInfo:" << CGImageGetByteOrderInfo(imageRef);

            updateError(QPlatformSurfaceCapture::CapturingNotSupported,
                        QLatin1String("Not supported pixel format"));
            return {};
        }

        QVideoFrameFormat format(QSize(CGImageGetWidth(imageRef), CGImageGetHeight(imageRef)),
                                 QVideoFrameFormat::Format_BGRA8888);
        format.setFrameRate(frameRate());

        return QVideoFrame(new QCGImageVideoBuffer(imageRef), format);
    }

    void onNewFrame(QVideoFrame frame)
    {
        // Since writing of the format is supposed to be only from one thread,
        // the read-only comparison without a mutex is thread-safe
        if (!m_format || m_format != frame.surfaceFormat()) {
            QMutexLocker<QMutex> locker(&m_formatMutex);

            m_format = frame.surfaceFormat();

            locker.unlock();

            m_waitForFormat.notify_one();
        }

        emit m_capture.newVideoFrame(frame);
    }

private:
    QCGWindowCapture &m_capture;
    std::optional<QVideoFrameFormat> m_format;
    mutable QMutex m_formatMutex;
    mutable QWaitCondition m_waitForFormat;
    CGWindowID m_wid;
};

QCGWindowCapture::QCGWindowCapture() : QPlatformSurfaceCapture(WindowSource{})
{
    CGRequestScreenCaptureAccess();
}

QCGWindowCapture::~QCGWindowCapture() = default;

bool QCGWindowCapture::setActiveInternal(bool active)
{
    if (active) {
        if (!CGPreflightScreenCaptureAccess()) {
            updateError(QPlatformSurfaceCapture::CaptureFailed,
                        QLatin1String("Permissions denied"));
            return false;
        }

        auto window = source<WindowSource>();

        auto handle = QCapturableWindowPrivate::handle(window);
        if (!handle || !handle->id)
            updateError(QPlatformSurfaceCapture::NotFound, QLatin1String("Invalid window"));
        else
            m_grabber = std::make_unique<Grabber>(*this, handle->id);

    } else {
        m_grabber.reset();
    }

    return active == static_cast<bool>(m_grabber);
}

QVideoFrameFormat QCGWindowCapture::frameFormat() const
{
    return m_grabber ? m_grabber->frameFormat() : QVideoFrameFormat();
}

QT_END_NAMESPACE

#include "moc_qcgwindowcapture_p.cpp"
