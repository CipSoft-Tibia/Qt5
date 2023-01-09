/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "avfcameradebug.h"
#include "avfcamerasession.h"
#include "avfcameraservice.h"
#include "avfcameracontrol.h"
#include "avfcamerarenderercontrol.h"
#include "avfcameradevicecontrol.h"
#include "avfaudioinputselectorcontrol.h"
#include "avfmediavideoprobecontrol.h"
#include "avfcameraviewfindersettingscontrol.h"
#include "avfimageencodercontrol.h"
#include "avfcamerautility.h"
#include "avfcamerawindowcontrol.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>

#include <QtCore/qdebug.h>

QT_USE_NAMESPACE

int AVFCameraSession::m_defaultCameraIndex;
QList<AVFCameraInfo> AVFCameraSession::m_cameraDevices;

@interface AVFCameraSessionObserver : NSObject

- (AVFCameraSessionObserver *) initWithCameraSession:(AVFCameraSession*)session;
- (void) processRuntimeError:(NSNotification *)notification;
- (void) processSessionStarted:(NSNotification *)notification;
- (void) processSessionStopped:(NSNotification *)notification;

@end

@implementation AVFCameraSessionObserver
{
@private
    AVFCameraSession *m_session;
    AVCaptureSession *m_captureSession;
}

- (AVFCameraSessionObserver *) initWithCameraSession:(AVFCameraSession*)session
{
    if (!(self = [super init]))
        return nil;

    self->m_session = session;
    self->m_captureSession = session->captureSession();

    [m_captureSession retain];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(processRuntimeError:)
                                                 name:AVCaptureSessionRuntimeErrorNotification
                                               object:m_captureSession];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(processSessionStarted:)
                                                 name:AVCaptureSessionDidStartRunningNotification
                                               object:m_captureSession];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(processSessionStopped:)
                                                 name:AVCaptureSessionDidStopRunningNotification
                                               object:m_captureSession];

    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVCaptureSessionRuntimeErrorNotification
                                                  object:m_captureSession];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVCaptureSessionDidStartRunningNotification
                                                  object:m_captureSession];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:AVCaptureSessionDidStopRunningNotification
                                                  object:m_captureSession];
    [m_captureSession release];
    [super dealloc];
}

- (void) processRuntimeError:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processRuntimeError", Qt::AutoConnection);
}

- (void) processSessionStarted:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processSessionStarted", Qt::AutoConnection);
}

- (void) processSessionStopped:(NSNotification *)notification
{
    Q_UNUSED(notification);
    QMetaObject::invokeMethod(m_session, "processSessionStopped", Qt::AutoConnection);
}

@end

AVFCameraSession::AVFCameraSession(AVFCameraService *service, QObject *parent)
   : QObject(parent)
   , m_service(service)
   , m_capturePreviewWindowOutput(nullptr)
   , m_state(QCamera::UnloadedState)
   , m_active(false)
   , m_videoInput(nil)
   , m_defaultCodec(0)
{
    m_captureSession = [[AVCaptureSession alloc] init];
    m_observer = [[AVFCameraSessionObserver alloc] initWithCameraSession:this];

    //configuration is commited during transition to Active state
    [m_captureSession beginConfiguration];
}

AVFCameraSession::~AVFCameraSession()
{
    if (m_capturePreviewWindowOutput) {
        m_capturePreviewWindowOutput->setLayer(nil);
    }

    if (m_videoInput) {
        [m_captureSession removeInput:m_videoInput];
        [m_videoInput release];
    }

    [m_observer release];
    [m_captureSession release];
}

int AVFCameraSession::defaultCameraIndex()
{
    updateCameraDevices();
    return m_defaultCameraIndex;
}

const QList<AVFCameraInfo> &AVFCameraSession::availableCameraDevices()
{
    updateCameraDevices();
    return m_cameraDevices;
}

AVFCameraInfo AVFCameraSession::cameraDeviceInfo(const QByteArray &device)
{
    updateCameraDevices();

    for (const AVFCameraInfo &info : qAsConst(m_cameraDevices)) {
        if (info.deviceId == device)
            return info;
    }

    return AVFCameraInfo();
}

void AVFCameraSession::updateCameraDevices()
{
#ifdef Q_OS_IOS
    // Cameras can't change dynamically on iOS. Update only once.
    if (!m_cameraDevices.isEmpty())
        return;
#else
    // On OS X, cameras can be added or removed. Update the list every time, but not more than
    // once every 500 ms
    static QElapsedTimer timer;
    if (timer.isValid() && timer.elapsed() < 500) // ms
        return;
#endif

    m_defaultCameraIndex = -1;
    m_cameraDevices.clear();

    AVCaptureDevice *defaultDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in videoDevices) {
        if (defaultDevice && [defaultDevice.uniqueID isEqualToString:device.uniqueID])
            m_defaultCameraIndex = m_cameraDevices.count();

        AVFCameraInfo info;
        info.deviceId = QByteArray([[device uniqueID] UTF8String]);
        info.description = QString::fromNSString([device localizedName]);

        // There is no API to get the camera sensor orientation, however, cameras are always
        // mounted in landscape on iDevices.
        //   - Back-facing cameras have the top side of the sensor aligned with the right side of
        //     the screen when held in portrait ==> 270 degrees clockwise angle
        //   - Front-facing cameras have the top side of the sensor aligned with the left side of
        //     the screen when held in portrait ==> 270 degrees clockwise angle
        // On OS X, the position will always be unspecified and the sensor orientation unknown.
        switch (device.position) {
        case AVCaptureDevicePositionBack:
            info.position = QCamera::BackFace;
            info.orientation = 270;
            break;
        case AVCaptureDevicePositionFront:
            info.position = QCamera::FrontFace;
            info.orientation = 270;
            break;
        default:
            info.position = QCamera::UnspecifiedPosition;
            info.orientation = 0;
            break;
        }

        m_cameraDevices.append(info);
    }

#ifndef Q_OS_IOS
    timer.restart();
#endif
}

void AVFCameraSession::setVideoOutput(AVFCameraRendererControl *output)
{
    m_videoOutput = output;
    if (output)
        output->configureAVCaptureSession(this);
}

void AVFCameraSession::setCapturePreviewOutput(AVFCameraWindowControl *output)
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO << output;
#endif

    if (m_capturePreviewWindowOutput == output)
        return;

    if (m_capturePreviewWindowOutput)
        m_capturePreviewWindowOutput->setLayer(nil);

    m_capturePreviewWindowOutput = output;

    if (m_capturePreviewWindowOutput) {
        AVCaptureVideoPreviewLayer *previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:m_captureSession];
        m_capturePreviewWindowOutput->setLayer(previewLayer);
        if (AVFCameraViewfinderSettingsControl2 *vfControl = m_service->viewfinderSettingsControl2()) {
            m_capturePreviewWindowOutput->setNativeSize(vfControl->viewfinderSettings().resolution());
        }
    }
}

AVCaptureDevice *AVFCameraSession::videoCaptureDevice() const
{
    if (m_videoInput)
        return m_videoInput.device;

    return nullptr;
}

QCamera::State AVFCameraSession::state() const
{
    if (m_active)
        return QCamera::ActiveState;

    return m_state == QCamera::ActiveState ? QCamera::LoadedState : m_state;
}

void AVFCameraSession::setState(QCamera::State newState)
{
    if (m_state == newState)
        return;

    qDebugCamera() << Q_FUNC_INFO << m_state << " -> " << newState;

    QCamera::State oldState = m_state;
    m_state = newState;

    //attach video input during Unloaded->Loaded transition
    if (oldState == QCamera::UnloadedState)
        attachVideoInputDevice();

    if (m_state == QCamera::ActiveState) {
        Q_EMIT readyToConfigureConnections();
        m_defaultCodec = 0;
        defaultCodec();

        bool activeFormatSet = applyImageEncoderSettings()
                             | applyViewfinderSettings();

        [m_captureSession commitConfiguration];

        if (activeFormatSet) {
            // According to the doc, the capture device must be locked before
            // startRunning to prevent the format we set to be overriden by the
            // session preset.
            [videoCaptureDevice() lockForConfiguration:nil];
        }

        [m_captureSession startRunning];

        if (activeFormatSet)
            [videoCaptureDevice() unlockForConfiguration];
    }

    if (oldState == QCamera::ActiveState) {
        [m_captureSession stopRunning];
        [m_captureSession beginConfiguration];
    }

    Q_EMIT stateChanged(m_state);
}

void AVFCameraSession::processRuntimeError()
{
    qWarning() << tr("Runtime camera error");
    Q_EMIT error(QCamera::CameraError, tr("Runtime camera error"));
}

void AVFCameraSession::processSessionStarted()
{
    qDebugCamera() << Q_FUNC_INFO;
    if (!m_active) {
        m_active = true;
        Q_EMIT activeChanged(m_active);
        Q_EMIT stateChanged(state());
    }
}

void AVFCameraSession::processSessionStopped()
{
    qDebugCamera() << Q_FUNC_INFO;
    if (m_active) {
        m_active = false;
        Q_EMIT activeChanged(m_active);
        Q_EMIT stateChanged(state());
    }
}

void AVFCameraSession::onCaptureModeChanged(QCamera::CaptureModes mode)
{
    Q_UNUSED(mode)

    const QCamera::State s = state();
    if (s == QCamera::LoadedState || s == QCamera::ActiveState) {
        applyImageEncoderSettings();
        applyViewfinderSettings();
    }
}

void AVFCameraSession::attachVideoInputDevice()
{
    //Attach video input device:
    if (m_service->videoDeviceControl()->isDirty()) {
        if (m_videoInput) {
            [m_captureSession removeInput:m_videoInput];
            [m_videoInput release];
            m_videoInput = nullptr;
            m_activeCameraInfo = AVFCameraInfo();
        }

        AVCaptureDevice *videoDevice = m_service->videoDeviceControl()->createCaptureDevice();

        NSError *error = nil;
        m_videoInput = [AVCaptureDeviceInput
                deviceInputWithDevice:videoDevice
                error:&error];

        if (!m_videoInput) {
            qWarning() << "Failed to create video device input";
        } else {
            if ([m_captureSession canAddInput:m_videoInput]) {
                m_activeCameraInfo = m_cameraDevices.at(m_service->videoDeviceControl()->selectedDevice());
                [m_videoInput retain];
                [m_captureSession addInput:m_videoInput];
            } else {
                qWarning() << "Failed to connect video device input";
            }
        }
    }
}

bool AVFCameraSession::applyImageEncoderSettings()
{
    if (AVFImageEncoderControl *control = m_service->imageEncoderControl())
        return control->applySettings();

    return false;
}

bool AVFCameraSession::applyViewfinderSettings()
{
    if (AVFCameraViewfinderSettingsControl2 *vfControl = m_service->viewfinderSettingsControl2()) {
        QCamera::CaptureModes currentMode = m_service->cameraControl()->captureMode();
        QCameraViewfinderSettings vfSettings(vfControl->requestedSettings());
        // Viewfinder and image capture solutions must be the same, if an image capture
        // resolution is set, it takes precedence over the viewfinder resolution.
        if (currentMode.testFlag(QCamera::CaptureStillImage)) {
            const QSize imageResolution(m_service->imageEncoderControl()->requestedSettings().resolution());
            if (!imageResolution.isNull() && imageResolution.isValid())
                vfSettings.setResolution(imageResolution);
        }

        vfControl->applySettings(vfSettings);

        if (m_capturePreviewWindowOutput)
            m_capturePreviewWindowOutput->setNativeSize(vfControl->viewfinderSettings().resolution());

        return !vfSettings.isNull();
    }

    return false;
}

void AVFCameraSession::addProbe(AVFMediaVideoProbeControl *probe)
{
    m_videoProbesMutex.lock();
    if (probe)
        m_videoProbes << probe;
    m_videoProbesMutex.unlock();
}

void AVFCameraSession::removeProbe(AVFMediaVideoProbeControl *probe)
{
    m_videoProbesMutex.lock();
    m_videoProbes.remove(probe);
    m_videoProbesMutex.unlock();
}

FourCharCode AVFCameraSession::defaultCodec()
{
    if (!m_defaultCodec) {
        if (AVCaptureDevice *device = videoCaptureDevice()) {
            AVCaptureDeviceFormat *format = device.activeFormat;
            if (!format || !format.formatDescription)
                return m_defaultCodec;
            m_defaultCodec = CMVideoFormatDescriptionGetCodecType(format.formatDescription);
        }
    }
    return m_defaultCodec;
}

void AVFCameraSession::onCameraFrameFetched(const QVideoFrame &frame)
{
    Q_EMIT newViewfinderFrame(frame);

    m_videoProbesMutex.lock();
    QSet<AVFMediaVideoProbeControl *>::const_iterator i = m_videoProbes.constBegin();
    while (i != m_videoProbes.constEnd()) {
        (*i)->newFrameProbed(frame);
        ++i;
    }
    m_videoProbesMutex.unlock();
}

#include "moc_avfcamerasession.cpp"
