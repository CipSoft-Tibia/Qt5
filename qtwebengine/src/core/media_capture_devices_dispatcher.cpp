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

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

#include "media_capture_devices_dispatcher.h"

#include "javascript_dialog_manager_qt.h"
#include "type_conversion.h"
#include "web_contents_view_qt.h"
#include "web_engine_settings.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/media/webrtc/desktop_streams_registry.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/media_capture_devices.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/media_stream_request.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_manager_base.h"
#include "ui/base/l10n/l10n_util.h"

#if QT_CONFIG(webengine_webrtc)
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capturer.h"
#endif

#include <QtCore/qcoreapplication.h>

namespace QtWebEngineCore {

using content::BrowserThread;
using content::MediaStreamDevices;

namespace {

const content::MediaStreamDevice *findDeviceWithId(const content::MediaStreamDevices &devices, const std::string &deviceId)
{
    content::MediaStreamDevices::const_iterator iter = devices.begin();
    for (; iter != devices.end(); ++iter) {
        if (iter->id == deviceId) {
            return &(*iter);
        }
    }
    return 0;
}

// Based on chrome/browser/media/desktop_capture_access_handler.cc:
void getDevicesForDesktopCapture(content::MediaStreamDevices *devices, content::DesktopMediaID mediaId, bool captureAudio)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    // Add selected desktop source to the list.
    devices->push_back(content::MediaStreamDevice(content::MEDIA_DESKTOP_VIDEO_CAPTURE, mediaId.ToString(), "Screen"));
    if (captureAudio) {
        if (mediaId.type == content::DesktopMediaID::TYPE_WEB_CONTENTS) {
            devices->push_back(
                    content::MediaStreamDevice(content::MEDIA_DESKTOP_AUDIO_CAPTURE,
                                               mediaId.ToString(), "Tab audio"));
        } else {
            // Use the special loopback device ID for system audio capture.
            devices->push_back(content::MediaStreamDevice(
                    content::MEDIA_DESKTOP_AUDIO_CAPTURE,
                    media::AudioDeviceDescription::kLoopbackInputDeviceId,
                    "System Audio"));
        }
    }
}

content::DesktopMediaID getDefaultScreenId()
{
    // While this function is executing another thread may also want to create a
    // DesktopCapturer [1]. Unfortunately, creating a DesktopCapturer is not
    // thread safe on X11 due to the use of webrtc::XErrorTrap. It's safe to
    // disable this code on X11 since we don't actually need to create a
    // DesktopCapturer to get the screen id anyway
    // (ScreenCapturerLinux::GetSourceList always returns 0 as the id).
    //
    // [1]: webrtc::InProcessVideoCaptureDeviceLauncher::DoStartDesktopCaptureOnDeviceThread

#if QT_CONFIG(webengine_webrtc) && !defined(USE_X11)
    // Source id patterns are different across platforms.
    // On Linux, the hardcoded value "0" is used.
    // On Windows, the screens are enumerated consecutively in increasing order from 0.
    // On macOS the source ids are randomish numbers assigned by the OS.

    // In order to provide a correct screen id, we query for the available screen ids, and
    // select the first one as the main display id.
    // The code is based on the file
    // src/chrome/browser/extensions/api/desktop_capture/desktop_capture_base.cc.
    webrtc::DesktopCaptureOptions options =
            webrtc::DesktopCaptureOptions::CreateDefault();
    options.set_disable_effects(false);
    std::unique_ptr<webrtc::DesktopCapturer> screen_capturer(
            webrtc::DesktopCapturer::CreateScreenCapturer(options));

    if (screen_capturer) {
        webrtc::DesktopCapturer::SourceList screens;
        if (screen_capturer->GetSourceList(&screens)) {
            if (screens.size() > 0) {
                return content::DesktopMediaID(content::DesktopMediaID::TYPE_SCREEN, screens[0].id);
            }
        }
    }
#endif

    return content::DesktopMediaID(content::DesktopMediaID::TYPE_SCREEN, 0);
}

WebContentsAdapterClient::MediaRequestFlags mediaRequestFlagsForRequest(const content::MediaStreamRequest &request)
{
    WebContentsAdapterClient::MediaRequestFlags requestFlags = WebContentsAdapterClient::MediaNone;

    if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE)
        requestFlags |= WebContentsAdapterClient::MediaAudioCapture;
    else if (request.audio_type == content::MEDIA_DESKTOP_AUDIO_CAPTURE)
        requestFlags |= WebContentsAdapterClient::MediaDesktopAudioCapture;

    if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE)
        requestFlags |= WebContentsAdapterClient::MediaVideoCapture;
    else if (request.video_type == content::MEDIA_DESKTOP_VIDEO_CAPTURE)
        requestFlags |= WebContentsAdapterClient::MediaDesktopVideoCapture;

    return requestFlags;
}

} // namespace

MediaCaptureDevicesDispatcher::PendingAccessRequest::PendingAccessRequest(const content::MediaStreamRequest &request,
                                                                          const RepeatingMediaResponseCallback &callback)
        : request(request)
        , callback(callback)
{
}

MediaCaptureDevicesDispatcher::PendingAccessRequest::~PendingAccessRequest()
{
}

void MediaCaptureDevicesDispatcher::handleMediaAccessPermissionResponse(content::WebContents *webContents, const QUrl &securityOrigin, WebContentsAdapterClient::MediaRequestFlags authorizationFlags)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    content::MediaStreamDevices devices;
    auto it = m_pendingRequests.find(webContents);
    if (it == m_pendingRequests.end() || it->second.empty())
        return;

    RequestsQueue &queue(it->second);
    content::MediaStreamRequest &request = queue.front().request;

    const QUrl requestSecurityOrigin(toQt(request.security_origin));
    bool securityOriginsMatch = (requestSecurityOrigin.host() == securityOrigin.host()
                                 && requestSecurityOrigin.scheme() == securityOrigin.scheme()
                                 && requestSecurityOrigin.port() == securityOrigin.port());
    if (!securityOriginsMatch)
        qWarning("Security origin mismatch for media access permission: %s requested and %s provided\n", qPrintable(requestSecurityOrigin.toString()), qPrintable(securityOrigin.toString()));

    bool microphoneRequested =
            (request.audio_type && authorizationFlags & WebContentsAdapterClient::MediaAudioCapture);
    bool webcamRequested =
            (request.video_type && authorizationFlags & WebContentsAdapterClient::MediaVideoCapture);
    bool desktopAudioRequested =
            (request.audio_type && authorizationFlags & WebContentsAdapterClient::MediaDesktopAudioCapture);
    bool desktopVideoRequested =
            (request.video_type && authorizationFlags & WebContentsAdapterClient::MediaDesktopVideoCapture);

    if (securityOriginsMatch) {
        if (microphoneRequested || webcamRequested) {
            switch (request.request_type) {
            case content::MEDIA_OPEN_DEVICE_PEPPER_ONLY:
                getDefaultDevices("", "", microphoneRequested, webcamRequested, &devices);
                break;
            case content::MEDIA_DEVICE_ACCESS:
            case content::MEDIA_GENERATE_STREAM:
                getDefaultDevices(request.requested_audio_device_id, request.requested_video_device_id,
                                  microphoneRequested, webcamRequested, &devices);
                break;
            }
        } else if (desktopVideoRequested) {
            getDevicesForDesktopCapture(&devices, getDefaultScreenId(), desktopAudioRequested);
        }
    }

    content::MediaResponseCallback callback = std::move(queue.front().callback);
    queue.pop_front();

    if (!queue.empty()) {
        // Post a task to process next queued request. It has to be done
        // asynchronously to make sure that calling infobar is not destroyed until
        // after this function returns.
        BrowserThread::PostTask(
                BrowserThread::UI, FROM_HERE, base::BindOnce(&MediaCaptureDevicesDispatcher::ProcessQueuedAccessRequest, base::Unretained(this), webContents));
    }

    std::move(callback).Run(devices, devices.empty() ? content::MEDIA_DEVICE_INVALID_STATE : content::MEDIA_DEVICE_OK,
                            std::unique_ptr<content::MediaStreamUI>());
}

MediaCaptureDevicesDispatcher *MediaCaptureDevicesDispatcher::GetInstance()
{
    return base::Singleton<MediaCaptureDevicesDispatcher>::get();
}

MediaCaptureDevicesDispatcher::MediaCaptureDevicesDispatcher()
{
    // MediaCaptureDevicesDispatcher is a singleton. It should be created on
    // UI thread. Otherwise, it will not receive
    // content::NOTIFICATION_WEB_CONTENTS_DESTROYED, and that will result in
    // possible use after free.
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    m_notificationsRegistrar.Add(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                                 content::NotificationService::AllSources());
}

MediaCaptureDevicesDispatcher::~MediaCaptureDevicesDispatcher()
{
}

void MediaCaptureDevicesDispatcher::Observe(int type, const content::NotificationSource &source, const content::NotificationDetails &details)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    if (type == content::NOTIFICATION_WEB_CONTENTS_DESTROYED) {
        content::WebContents *webContents = content::Source<content::WebContents>(source).ptr();
        m_pendingRequests.erase(webContents);
    }
}

void MediaCaptureDevicesDispatcher::processMediaAccessRequest(WebContentsAdapterClient *adapterClient, content::WebContents *webContents, const content::MediaStreamRequest &request, content::MediaResponseCallback callback)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    // Let's not support tab capture for now.
    if (request.video_type == content::MEDIA_TAB_VIDEO_CAPTURE || request.audio_type == content::MEDIA_TAB_AUDIO_CAPTURE) {
        std::move(callback).Run(content::MediaStreamDevices(), content::MEDIA_DEVICE_NOT_SUPPORTED, std::unique_ptr<content::MediaStreamUI>());
        return;
    }

    if (request.video_type == content::MEDIA_DESKTOP_VIDEO_CAPTURE || request.audio_type == content::MEDIA_DESKTOP_AUDIO_CAPTURE) {
        const bool screenCaptureEnabled =
                adapterClient->webEngineSettings()->testAttribute(WebEngineSettings::ScreenCaptureEnabled);
        const bool originIsSecure = content::IsOriginSecure(request.security_origin);
        if (!screenCaptureEnabled || !originIsSecure) {
            std::move(callback).Run(content::MediaStreamDevices(), content::MEDIA_DEVICE_INVALID_STATE, std::unique_ptr<content::MediaStreamUI>());
            return;
        }

        if (!request.requested_video_device_id.empty()) {
            // Non-empty device id from the chooseDesktopMedia() extension API.
            processDesktopCaptureAccessRequest(webContents, request, std::move(callback));
            return;
        }
    }

    enqueueMediaAccessRequest(webContents, request, std::move(callback));
    // We might not require this approval for pepper requests.
    adapterClient->runMediaAccessPermissionRequest(toQt(request.security_origin), mediaRequestFlagsForRequest(request));
}

void MediaCaptureDevicesDispatcher::processDesktopCaptureAccessRequest(content::WebContents *webContents, const content::MediaStreamRequest &request, content::MediaResponseCallback callback)
{
    content::MediaStreamDevices devices;

    if (request.video_type != content::MEDIA_DESKTOP_VIDEO_CAPTURE || request.requested_video_device_id.empty()) {
        std::move(callback).Run(devices, content::MEDIA_DEVICE_INVALID_STATE, std::unique_ptr<content::MediaStreamUI>());
        return;
    }

    content::WebContents *const web_contents_for_stream = content::WebContents::FromRenderFrameHost(
            content::RenderFrameHost::FromID(request.render_process_id, request.render_frame_id));
    content::RenderFrameHost *const main_frame = web_contents_for_stream ? web_contents_for_stream->GetMainFrame() : NULL;

    content::DesktopMediaID mediaId;
    if (main_frame) {
        // The extension name that the stream is registered with.
        std::string originalExtensionName;
        // Resolve DesktopMediaID for the specified device id.
        mediaId = getDesktopStreamsRegistry()->RequestMediaForStreamId(
                request.requested_video_device_id, main_frame->GetProcess()->GetID(),
                main_frame->GetRoutingID(), request.security_origin,
                &originalExtensionName);
    }

    // Received invalid device id.
    if (mediaId.type == content::DesktopMediaID::TYPE_NONE) {
        std::move(callback).Run(devices, content::MEDIA_DEVICE_INVALID_STATE, std::unique_ptr<content::MediaStreamUI>());
        return;
    }

    // Audio is only supported for screen capture streams.
    bool capture_audio = (mediaId.type == content::DesktopMediaID::TYPE_SCREEN && request.audio_type == content::MEDIA_DESKTOP_AUDIO_CAPTURE);

    getDevicesForDesktopCapture(&devices, mediaId, capture_audio);

    std::move(callback).Run(devices, devices.empty() ? content::MEDIA_DEVICE_INVALID_STATE : content::MEDIA_DEVICE_OK,
                            std::unique_ptr<content::MediaStreamUI>());
}

void MediaCaptureDevicesDispatcher::enqueueMediaAccessRequest(content::WebContents *webContents,
                                                              const content::MediaStreamRequest &request,
                                                              content::MediaResponseCallback callback)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    RequestsQueue &queue = m_pendingRequests[webContents];
    queue.push_back(PendingAccessRequest(request, base::AdaptCallbackForRepeating(std::move(callback))));
}

void MediaCaptureDevicesDispatcher::ProcessQueuedAccessRequest(content::WebContents *webContents)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    auto it = m_pendingRequests.find(webContents);
    if (it == m_pendingRequests.end() || it->second.empty())
        return;

    RequestsQueue &queue(it->second);
    content::MediaStreamRequest &request = queue.front().request;

    WebContentsAdapterClient *adapterClient = WebContentsViewQt::from(static_cast<content::WebContentsImpl *>(webContents)->GetView())->client();
    adapterClient->runMediaAccessPermissionRequest(toQt(request.security_origin), mediaRequestFlagsForRequest(request));
}

void MediaCaptureDevicesDispatcher::getDefaultDevices(const std::string &audioDeviceId, const std::string &videoDeviceId,
                                                      bool audio, bool video, content::MediaStreamDevices *devices)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK(audio || video);

    if (audio) {
        const content::MediaStreamDevices &audioDevices = content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
        const content::MediaStreamDevice *device = findDeviceWithId(audioDevices, audioDeviceId);
        if (!device && !audioDevices.empty())
            device = &audioDevices.front();
        if (device)
            devices->push_back(*device);
    }

    if (video) {
        const content::MediaStreamDevices &videoDevices = content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
        const content::MediaStreamDevice *device = findDeviceWithId(videoDevices, videoDeviceId);
        if (!device && !videoDevices.empty())
            device = &videoDevices.front();
        if (device)
            devices->push_back(*device);
    }
}

DesktopStreamsRegistry *MediaCaptureDevicesDispatcher::getDesktopStreamsRegistry()
{
    if (!m_desktopStreamsRegistry)
        m_desktopStreamsRegistry.reset(new DesktopStreamsRegistry());
    return m_desktopStreamsRegistry.get();
}

void MediaCaptureDevicesDispatcher::OnMediaRequestStateChanged(int render_process_id, int render_frame_id, int page_request_id, const GURL &security_origin, content::MediaStreamType stream_type, content::MediaRequestState state)
{
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    BrowserThread::PostTask(
            BrowserThread::UI, FROM_HERE,
            base::Bind(
                    &MediaCaptureDevicesDispatcher::updateMediaRequestStateOnUIThread,
                    base::Unretained(this), render_process_id, render_frame_id,
                    page_request_id, security_origin, stream_type, state));
}

void MediaCaptureDevicesDispatcher::updateMediaRequestStateOnUIThread(int render_process_id,
                                                                      int render_frame_id,
                                                                      int page_request_id,
                                                                      const GURL & /*security_origin*/,
                                                                      content::MediaStreamType /*stream_type*/,
                                                                      content::MediaRequestState state)
{
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    // Cancel the request.
    if (state == content::MEDIA_REQUEST_STATE_CLOSING) {
        for (auto &pair : m_pendingRequests) {
            RequestsQueue &queue = pair.second;
            for (auto it = queue.begin(); it != queue.end(); ++it) {
                if (it->request.render_process_id == render_process_id
                        && it->request.render_frame_id == render_frame_id
                        && it->request.page_request_id == page_request_id) {
                    queue.erase(it);
                    return;
                }
            }
        }
    }
}

} // namespace QtWebEngineCore
