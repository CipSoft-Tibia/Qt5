// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qwaylandshmbackingstore_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandsubsurface_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandabstractdecoration_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qtemporaryfile.h>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QMutexLocker>

#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef Q_OS_LINUX
#  include <sys/syscall.h>
// from linux/memfd.h:
#  ifndef MFD_CLOEXEC
#    define MFD_CLOEXEC     0x0001U
#  endif
#  ifndef MFD_ALLOW_SEALING
#    define MFD_ALLOW_SEALING 0x0002U
#  endif
// from bits/fcntl-linux.h
#  ifndef F_ADD_SEALS
#    define F_ADD_SEALS 1033
#  endif
#  ifndef F_SEAL_SEAL
#    define F_SEAL_SEAL 0x0001
#  endif
#  ifndef F_SEAL_SHRINK
#    define F_SEAL_SHRINK 0x0002
#  endif
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandShmBuffer::QWaylandShmBuffer(QWaylandDisplay *display,
                     const QSize &size, QImage::Format format, qreal scale)
{
    int stride = size.width() * 4;
    int alloc = stride * size.height();
    int fd = -1;

#ifdef SYS_memfd_create
    fd = syscall(SYS_memfd_create, "wayland-shm", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd >= 0)
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL);
#endif

    QScopedPointer<QFile> filePointer;

    if (fd == -1) {
        auto tmpFile = new QTemporaryFile (QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) +
                                       QLatin1String("/wayland-shm-XXXXXX"));
        tmpFile->open();
        filePointer.reset(tmpFile);
    } else {
        auto file = new QFile;
        file->open(fd, QIODevice::ReadWrite | QIODevice::Unbuffered, QFile::AutoCloseHandle);
        filePointer.reset(file);
    }
    if (!filePointer->isOpen() || !filePointer->resize(alloc)) {
        qWarning("QWaylandShmBuffer: failed: %s", qUtf8Printable(filePointer->errorString()));
        return;
    }
    fd = filePointer->handle();

    // map ourselves: QFile::map() will unmap when the object is destroyed,
    // but we want this mapping to persist (unmapping in destructor)
    uchar *data = (uchar *)
            mmap(nullptr, alloc, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == (uchar *) MAP_FAILED) {
        qErrnoWarning("QWaylandShmBuffer: mmap failed");
        return;
    }

    QWaylandShm* shm = display->shm();
    wl_shm_format wl_format = shm->formatFrom(format);
    mImage = QImage(data, size.width(), size.height(), stride, format);
    mImage.setDevicePixelRatio(scale);

    mShmPool = wl_shm_create_pool(shm->object(), fd, alloc);
    init(wl_shm_pool_create_buffer(mShmPool,0, size.width(), size.height(),
                                       stride, wl_format));
}

QWaylandShmBuffer::~QWaylandShmBuffer(void)
{
    delete mMarginsImage;
    if (mImage.constBits())
        munmap((void *) mImage.constBits(), mImage.sizeInBytes());
    if (mShmPool)
        wl_shm_pool_destroy(mShmPool);
}

QImage *QWaylandShmBuffer::imageInsideMargins(const QMargins &marginsIn)
{
    QMargins margins = marginsIn * mImage.devicePixelRatio();

    if (!margins.isNull() && margins != mMargins) {
        if (mMarginsImage) {
            delete mMarginsImage;
        }
        uchar *bits = const_cast<uchar *>(mImage.constBits());
        uchar *b_s_data = bits + margins.top() * mImage.bytesPerLine() + margins.left() * 4;
        int b_s_width = mImage.size().width() - margins.left() - margins.right();
        int b_s_height = mImage.size().height() - margins.top() - margins.bottom();
        mMarginsImage = new QImage(b_s_data, b_s_width,b_s_height,mImage.bytesPerLine(),mImage.format());
        mMarginsImage->setDevicePixelRatio(mImage.devicePixelRatio());
    }
    if (margins.isNull()) {
        delete mMarginsImage;
        mMarginsImage = nullptr;
    }

    mMargins = margins;
    if (!mMarginsImage)
        return &mImage;

    return mMarginsImage;

}

QWaylandShmBackingStore::QWaylandShmBackingStore(QWindow *window, QWaylandDisplay *display)
    : QPlatformBackingStore(window)
    , mDisplay(display)
{
    QObject::connect(mDisplay, &QWaylandDisplay::connected, window, [this]() {
        auto copy = mBuffers;
        // clear available buffers so we create new ones
        // actual deletion is deferred till after resize call so we can copy
        // contents from the back buffer
        mBuffers.clear();
        mFrontBuffer = nullptr;
        // resize always resets mBackBuffer
        if (mRequestedSize.isValid() && waylandWindow())
            resize(mRequestedSize);
        qDeleteAll(copy);
    });
}

QWaylandShmBackingStore::~QWaylandShmBackingStore()
{
    if (QWaylandWindow *w = waylandWindow())
        w->setBackingStore(nullptr);

//    if (mFrontBuffer == waylandWindow()->attached())
//        waylandWindow()->attach(0);

    qDeleteAll(mBuffers);
}

QPaintDevice *QWaylandShmBackingStore::paintDevice()
{
    return contentSurface();
}

void QWaylandShmBackingStore::beginPaint(const QRegion &region)
{
    mPainting = true;
    ensureSize();

    if (mBackBuffer->image()->hasAlphaChannel()) {
        QPainter p(paintDevice());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        for (const QRect &rect : region)
            p.fillRect(rect, blank);
    }
}

void QWaylandShmBackingStore::endPaint()
{
    mPainting = false;
    if (mPendingFlush)
        flush(window(), mPendingRegion, QPoint());
}

void QWaylandShmBackingStore::ensureSize()
{
    waylandWindow()->setBackingStore(this);
    resize(mRequestedSize);
}

void QWaylandShmBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    // Invoked when the window is of type RasterSurface or when the window is
    // RasterGLSurface and there are no child widgets requiring OpenGL composition.

    // For the case of RasterGLSurface + having to compose, the composeAndFlush() is
    // called instead. The default implementation from QPlatformBackingStore is sufficient
    // however so no need to reimplement that.


    Q_UNUSED(window);
    Q_UNUSED(offset);

    if (mPainting) {
        mPendingRegion |= region;
        mPendingFlush = true;
        return;
    }

    mPendingFlush = false;
    mPendingRegion = QRegion();

    if (windowDecoration() && windowDecoration()->isDirty())
        updateDecorations();

    mFrontBuffer = mBackBuffer;

    QMargins margins = windowDecorationMargins();
    waylandWindow()->safeCommit(mFrontBuffer, region.translated(margins.left(), margins.top()));
}

void QWaylandShmBackingStore::resize(const QSize &size, const QRegion &)
{
    mRequestedSize = size;
}

QWaylandShmBuffer *QWaylandShmBackingStore::getBuffer(const QSize &size)
{
    const auto copy = mBuffers; // remove when ported to vector<unique_ptr> + remove_if
    for (QWaylandShmBuffer *b : copy) {
        if (!b->busy()) {
            if (b->size() == size) {
                return b;
            } else {
                mBuffers.remove(b);
                if (mBackBuffer == b)
                    mBackBuffer = nullptr;
                delete b;
            }
        }
    }

    static const size_t MAX_BUFFERS = 5;
    if (mBuffers.size() < MAX_BUFFERS) {
        QImage::Format format = QPlatformScreen::platformScreenForWindow(window())->format();
        QWaylandShmBuffer *b = new QWaylandShmBuffer(mDisplay, size, format, waylandWindow()->scale());
        mBuffers.push_front(b);
        return b;
    }
    return nullptr;
}

void QWaylandShmBackingStore::resize(const QSize &size)
{
    QMargins margins = windowDecorationMargins();
    qreal scale = waylandWindow()->scale();
    QSize sizeWithMargins = (size + QSize(margins.left()+margins.right(),margins.top()+margins.bottom())) * scale;

    // We look for a free buffer to draw into. If the buffer is not the last buffer we used,
    // that is mBackBuffer, and the size is the same we memcpy the old content into the new
    // buffer so that QPainter is happy to find the stuff it had drawn before. If the new
    // buffer has a different size it needs to be redrawn completely anyway, and if the buffer
    // is the same the stuff is there already.
    // You can exercise the different codepaths with weston, switching between the gl and the
    // pixman renderer. With the gl renderer release events are sent early so we can effectively
    // run single buffered, while with the pixman renderer we have to use two.
    QWaylandShmBuffer *buffer = getBuffer(sizeWithMargins);
    while (!buffer) {
        qCDebug(lcWaylandBackingstore, "QWaylandShmBackingStore: stalling waiting for a buffer to be released from the compositor...");

        mDisplay->blockingReadEvents();
        buffer = getBuffer(sizeWithMargins);
    }

    qsizetype oldSizeInBytes = mBackBuffer ? mBackBuffer->image()->sizeInBytes() : 0;
    qsizetype newSizeInBytes = buffer->image()->sizeInBytes();

    // mBackBuffer may have been deleted here but if so it means its size was different so we wouldn't copy it anyway
    if (mBackBuffer != buffer && oldSizeInBytes == newSizeInBytes)
        memcpy(buffer->image()->bits(), mBackBuffer->image()->constBits(), newSizeInBytes);

    mBackBuffer = buffer;

    // ensure the new buffer is at the beginning of the list so next time getBuffer() will pick
    // it if possible
    if (mBuffers.front() != buffer) {
        mBuffers.remove(buffer);
        mBuffers.push_front(buffer);
    }

    if (windowDecoration() && window()->isVisible() && oldSizeInBytes != newSizeInBytes)
        windowDecoration()->update();
}

QImage *QWaylandShmBackingStore::entireSurface() const
{
    return mBackBuffer->image();
}

QImage *QWaylandShmBackingStore::contentSurface() const
{
    return windowDecoration() ? mBackBuffer->imageInsideMargins(windowDecorationMargins()) : mBackBuffer->image();
}

void QWaylandShmBackingStore::updateDecorations()
{
    QPainter decorationPainter(entireSurface());
    decorationPainter.setCompositionMode(QPainter::CompositionMode_Source);
    QImage sourceImage = windowDecoration()->contentImage();

    qreal dp = sourceImage.devicePixelRatio();
    int dpWidth = int(sourceImage.width() / dp);
    int dpHeight = int(sourceImage.height() / dp);
    QTransform sourceMatrix;
    sourceMatrix.scale(dp, dp);
    QRect target; // needs to be in device independent pixels

    //Top
    target.setX(0);
    target.setY(0);
    target.setWidth(dpWidth);
    target.setHeight(windowDecorationMargins().top());
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));

    //Left
    target.setWidth(windowDecorationMargins().left());
    target.setHeight(dpHeight);
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));

    //Right
    target.setX(dpWidth - windowDecorationMargins().right());
    target.setWidth(windowDecorationMargins().right());
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));

    //Bottom
    target.setX(0);
    target.setY(dpHeight - windowDecorationMargins().bottom());
    target.setWidth(dpWidth);
    target.setHeight(windowDecorationMargins().bottom());
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));
}

QWaylandAbstractDecoration *QWaylandShmBackingStore::windowDecoration() const
{
    return waylandWindow()->decoration();
}

QMargins QWaylandShmBackingStore::windowDecorationMargins() const
{
    if (windowDecoration())
        return windowDecoration()->margins();
    return QMargins();
}

QWaylandWindow *QWaylandShmBackingStore::waylandWindow() const
{
    return static_cast<QWaylandWindow *>(window()->handle());
}

#if QT_CONFIG(opengl)
QImage QWaylandShmBackingStore::toImage() const
{
    // Invoked from QPlatformBackingStore::composeAndFlush() that is called
    // instead of flush() for widgets that have renderToTexture children
    // (QOpenGLWidget, QQuickWidget).

    return *contentSurface();
}
#endif  // opengl

}

QT_END_NAMESPACE
