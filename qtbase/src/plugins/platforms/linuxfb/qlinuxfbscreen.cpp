// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlinuxfbscreen.h"
#include <QtFbSupport/private/qfbcursor_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QPainter>

#include <private/qcore_unix_p.h> // overrides QT_OPEN
#include <qimage.h>
#include <qdebug.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>

#include <linux/fb.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static int openFramebufferDevice(const QString &dev)
{
    int fd = -1;

    if (access(dev.toLatin1().constData(), R_OK|W_OK) == 0)
        fd = QT_OPEN(dev.toLatin1().constData(), O_RDWR);

    if (fd == -1) {
        if (access(dev.toLatin1().constData(), R_OK) == 0)
            fd = QT_OPEN(dev.toLatin1().constData(), O_RDONLY);
    }

    return fd;
}

static int determineDepth(const fb_var_screeninfo &vinfo)
{
    int depth = vinfo.bits_per_pixel;
    if (depth== 24) {
        depth = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (depth <= 0)
            depth = 24; // reset if color component lengths are not reported
    } else if (depth == 16) {
        depth = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (depth <= 0)
            depth = 16;
    }
    return depth;
}

static QRect determineGeometry(const fb_var_screeninfo &vinfo, const QRect &userGeometry)
{
    int xoff = vinfo.xoffset;
    int yoff = vinfo.yoffset;
    int w, h;
    if (userGeometry.isValid()) {
        w = userGeometry.width();
        h = userGeometry.height();
        if ((uint)w > vinfo.xres)
            w = vinfo.xres;
        if ((uint)h > vinfo.yres)
            h = vinfo.yres;

        int xxoff = userGeometry.x(), yyoff = userGeometry.y();
        if (xxoff != 0 || yyoff != 0) {
            if (xxoff < 0 || xxoff + w > (int)(vinfo.xres))
                xxoff = vinfo.xres - w;
            if (yyoff < 0 || yyoff + h > (int)(vinfo.yres))
                yyoff = vinfo.yres - h;
            xoff += xxoff;
            yoff += yyoff;
        } else {
            xoff += (vinfo.xres - w)/2;
            yoff += (vinfo.yres - h)/2;
        }
    } else {
        w = vinfo.xres;
        h = vinfo.yres;
    }

    if (w == 0 || h == 0) {
        qWarning("Unable to find screen geometry, using 320x240");
        w = 320;
        h = 240;
    }

    return QRect(xoff, yoff, w, h);
}

static QSizeF determinePhysicalSize(const fb_var_screeninfo &vinfo, const QSize &mmSize, const QSize &res)
{
    int mmWidth = mmSize.width(), mmHeight = mmSize.height();

    if (mmWidth <= 0 && mmHeight <= 0) {
        if (vinfo.width != 0 && vinfo.height != 0
            && vinfo.width != UINT_MAX && vinfo.height != UINT_MAX) {
            mmWidth = vinfo.width;
            mmHeight = vinfo.height;
        } else {
            const int dpi = 100;
            mmWidth = qRound(res.width() * 25.4 / dpi);
            mmHeight = qRound(res.height() * 25.4 / dpi);
        }
    } else if (mmWidth > 0 && mmHeight <= 0) {
        mmHeight = res.height() * mmWidth/res.width();
    } else if (mmHeight > 0 && mmWidth <= 0) {
        mmWidth = res.width() * mmHeight/res.height();
    }

    return QSize(mmWidth, mmHeight);
}

static QImage::Format determineFormat(const fb_var_screeninfo &info, int depth)
{
    const fb_bitfield rgba[4] = { info.red, info.green,
                                  info.blue, info.transp };

    QImage::Format format = QImage::Format_Invalid;

    switch (depth) {
    case 32: {
        const fb_bitfield argb8888[4] = {{16, 8, 0}, {8, 8, 0},
                                         {0, 8, 0}, {24, 8, 0}};
        const fb_bitfield abgr8888[4] = {{0, 8, 0}, {8, 8, 0},
                                         {16, 8, 0}, {24, 8, 0}};
        if (memcmp(rgba, argb8888, 4 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_ARGB32;
        } else if (memcmp(rgba, argb8888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB32;
        } else if (memcmp(rgba, abgr8888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB32;
            // pixeltype = BGRPixel;
        }
        break;
    }
    case 24: {
        const fb_bitfield rgb888[4] = {{16, 8, 0}, {8, 8, 0},
                                       {0, 8, 0}, {0, 0, 0}};
        const fb_bitfield bgr888[4] = {{0, 8, 0}, {8, 8, 0},
                                       {16, 8, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB888;
        } else if (memcmp(rgba, bgr888, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_BGR888;
            // pixeltype = BGRPixel;
        }
        break;
    }
    case 18: {
        const fb_bitfield rgb666[4] = {{12, 6, 0}, {6, 6, 0},
                                       {0, 6, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb666, 3 * sizeof(fb_bitfield)) == 0)
            format = QImage::Format_RGB666;
        break;
    }
    case 16: {
        const fb_bitfield rgb565[4] = {{11, 5, 0}, {5, 6, 0},
                                       {0, 5, 0}, {0, 0, 0}};
        const fb_bitfield bgr565[4] = {{0, 5, 0}, {5, 6, 0},
                                       {11, 5, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb565, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB16;
        } else if (memcmp(rgba, bgr565, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB16;
            // pixeltype = BGRPixel;
        }
        break;
    }
    case 15: {
        const fb_bitfield rgb1555[4] = {{10, 5, 0}, {5, 5, 0},
                                        {0, 5, 0}, {15, 1, 0}};
        const fb_bitfield bgr1555[4] = {{0, 5, 0}, {5, 5, 0},
                                        {10, 5, 0}, {15, 1, 0}};
        if (memcmp(rgba, rgb1555, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB555;
        } else if (memcmp(rgba, bgr1555, 3 * sizeof(fb_bitfield)) == 0) {
            format = QImage::Format_RGB555;
            // pixeltype = BGRPixel;
        }
        break;
    }
    case 12: {
        const fb_bitfield rgb444[4] = {{8, 4, 0}, {4, 4, 0},
                                       {0, 4, 0}, {0, 0, 0}};
        if (memcmp(rgba, rgb444, 3 * sizeof(fb_bitfield)) == 0)
            format = QImage::Format_RGB444;
        break;
    }
    case 8:
        break;
    case 1:
        format = QImage::Format_Mono; //###: LSB???
        break;
    default:
        break;
    }

    return format;
}

static int openTtyDevice(const QString &device)
{
    const char *const devs[] = { "/dev/tty0", "/dev/tty", "/dev/console", nullptr };

    int fd = -1;
    if (device.isEmpty()) {
        for (const char * const *dev = devs; *dev; ++dev) {
            fd = QT_OPEN(*dev, O_RDWR);
            if (fd != -1)
                break;
        }
    } else {
        fd = QT_OPEN(QFile::encodeName(device).constData(), O_RDWR);
    }

    return fd;
}

static void switchToGraphicsMode(int ttyfd, bool doSwitch, int *oldMode)
{
    // Do not warn if the switch fails: the ioctl fails when launching from a
    // remote console and there is nothing we can do about it.  The matching
    // call in resetTty should at least fail then, too, so we do no harm.
    if (ioctl(ttyfd, KDGETMODE, oldMode) == 0) {
        if (doSwitch && *oldMode != KD_GRAPHICS)
            ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
    }
}

static void resetTty(int ttyfd, int oldMode)
{
    ioctl(ttyfd, KDSETMODE, oldMode);

    QT_CLOSE(ttyfd);
}

static void blankScreen(int fd, bool on)
{
    ioctl(fd, FBIOBLANK, on ? VESA_POWERDOWN : VESA_NO_BLANKING);
}

QLinuxFbScreen::QLinuxFbScreen(const QStringList &args)
    : mArgs(args), mFbFd(-1), mTtyFd(-1), mBlitter(nullptr)
{
    mMmap.data = nullptr;
}

QLinuxFbScreen::~QLinuxFbScreen()
{
    if (mFbFd != -1) {
        if (mMmap.data)
            munmap(mMmap.data - mMmap.offset, mMmap.size);
        close(mFbFd);
    }

    if (mTtyFd != -1)
        resetTty(mTtyFd, mOldTtyMode);

    delete mBlitter;
}

bool QLinuxFbScreen::initialize()
{
    QRegularExpression ttyRx("tty=(.*)"_L1);
    QRegularExpression fbRx("fb=(.*)"_L1);
    QRegularExpression mmSizeRx("mmsize=(\\d+)x(\\d+)"_L1);
    QRegularExpression sizeRx("size=(\\d+)x(\\d+)"_L1);
    QRegularExpression offsetRx("offset=(\\d+)x(\\d+)"_L1);

    QString fbDevice, ttyDevice;
    QSize userMmSize;
    QRect userGeometry;
    bool doSwitchToGraphicsMode = true;

    // Parse arguments
    for (const QString &arg : std::as_const(mArgs)) {
        QRegularExpressionMatch match;
        if (arg == "nographicsmodeswitch"_L1)
            doSwitchToGraphicsMode = false;
        else if (arg.contains(mmSizeRx, &match))
            userMmSize = QSize(match.captured(1).toInt(), match.captured(2).toInt());
        else if (arg.contains(sizeRx, &match))
            userGeometry.setSize(QSize(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(offsetRx, &match))
            userGeometry.setTopLeft(QPoint(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(ttyRx, &match))
            ttyDevice = match.captured(1);
        else if (arg.contains(fbRx, &match))
            fbDevice = match.captured(1);
    }

    if (fbDevice.isEmpty()) {
        fbDevice = "/dev/fb0"_L1;
        if (!QFile::exists(fbDevice))
            fbDevice = "/dev/graphics/fb0"_L1;
        if (!QFile::exists(fbDevice)) {
            qWarning("Unable to figure out framebuffer device. Specify it manually.");
            return false;
        }
    }

    // Open the device
    mFbFd = openFramebufferDevice(fbDevice);
    if (mFbFd == -1) {
        qErrnoWarning(errno, "Failed to open framebuffer %s", qPrintable(fbDevice));
        return false;
    }

    // Read the fixed and variable screen information
    fb_fix_screeninfo finfo;
    fb_var_screeninfo vinfo;
    memset(&vinfo, 0, sizeof(vinfo));
    memset(&finfo, 0, sizeof(finfo));

    if (ioctl(mFbFd, FBIOGET_FSCREENINFO, &finfo) != 0) {
        qErrnoWarning(errno, "Error reading fixed information");
        return false;
    }

    if (ioctl(mFbFd, FBIOGET_VSCREENINFO, &vinfo)) {
        qErrnoWarning(errno, "Error reading variable information");
        return false;
    }

    mDepth = determineDepth(vinfo);
    mBytesPerLine = finfo.line_length;
    QRect geometry = determineGeometry(vinfo, userGeometry);
    mGeometry = QRect(QPoint(0, 0), geometry.size());
    mFormat = determineFormat(vinfo, mDepth);
    mPhysicalSize = determinePhysicalSize(vinfo, userMmSize, geometry.size());

    // mmap the framebuffer
    mMmap.size = finfo.smem_len;
    uchar *data = (unsigned char *)mmap(nullptr, mMmap.size, PROT_READ | PROT_WRITE, MAP_SHARED, mFbFd, 0);
    if ((long)data == -1) {
        qErrnoWarning(errno, "Failed to mmap framebuffer");
        return false;
    }

    mMmap.offset = geometry.y() * mBytesPerLine + geometry.x() * mDepth / 8;
    mMmap.data = data + mMmap.offset;

    QFbScreen::initializeCompositor();
    mFbScreenImage = QImage(mMmap.data, geometry.width(), geometry.height(), mBytesPerLine, mFormat);

    mCursor = new QFbCursor(this);

    mTtyFd = openTtyDevice(ttyDevice);
    if (mTtyFd == -1)
        qErrnoWarning(errno, "Failed to open tty");

    switchToGraphicsMode(mTtyFd, doSwitchToGraphicsMode, &mOldTtyMode);
    blankScreen(mFbFd, false);

    return true;
}

QRegion QLinuxFbScreen::doRedraw()
{
    QRegion touched = QFbScreen::doRedraw();

    if (touched.isEmpty())
        return touched;

    if (!mBlitter)
        mBlitter = new QPainter(&mFbScreenImage);

    mBlitter->setCompositionMode(QPainter::CompositionMode_Source);
    for (const QRect &rect : touched)
        mBlitter->drawImage(rect, mScreenImage, rect);

    return touched;
}

// grabWindow() grabs "from the screen" not from the backingstores.
// In linuxfb's case it will also include the mouse cursor.
QPixmap QLinuxFbScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    if (!wid) {
        if (width < 0)
            width = mFbScreenImage.width() - x;
        if (height < 0)
            height = mFbScreenImage.height() - y;
        return QPixmap::fromImage(mFbScreenImage).copy(x, y, width, height);
    }

    QFbWindow *window = windowForId(wid);
    if (window) {
        const QRect geom = window->geometry();
        if (width < 0)
            width = geom.width() - x;
        if (height < 0)
            height = geom.height() - y;
        QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
        rect &= window->geometry();
        return QPixmap::fromImage(mFbScreenImage).copy(rect);
    }

    return QPixmap();
}

QT_END_NAMESPACE

#include "moc_qlinuxfbscreen.cpp"

