/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "browser_message_filter_qt.h"

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/plugin_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#include "common/qt_messages.h"
#include "profile_io_data_qt.h"
#include "type_conversion.h"

namespace QtWebEngineCore {

BrowserMessageFilterQt::BrowserMessageFilterQt(int /*render_process_id*/, Profile *profile)
    : BrowserMessageFilter(QtMsgStart)
    , m_profileData(ProfileIODataQt::FromBrowserContext(profile))
{
}

// The following is based on chrome/browser/plugins/plugin_info_message_filter.cc:
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.Chromium file.

bool BrowserMessageFilterQt::OnMessageReceived(const IPC::Message& message)
{
    IPC_BEGIN_MESSAGE_MAP(BrowserMessageFilterQt, message)
        IPC_MESSAGE_HANDLER(QtWebEngineHostMsg_AllowStorageAccess, OnAllowStorageAccess)
        IPC_MESSAGE_HANDLER_DELAY_REPLY(QtWebEngineHostMsg_RequestStorageAccessSync,
                                        OnRequestStorageAccessSync)
        IPC_MESSAGE_HANDLER(QtWebEngineHostMsg_RequestStorageAccessAsync,
                            OnRequestStorageAccessAsync)
        IPC_MESSAGE_UNHANDLED(return false)
    IPC_END_MESSAGE_MAP()
    return true;
}

void BrowserMessageFilterQt::OnAllowStorageAccess(int /*render_frame_id*/,
                                                  const GURL &origin_url,
                                                  const GURL &top_origin_url,
                                                  int /*storage_type*/,
                                                  bool *allowed)
{
    *allowed = m_profileData->canGetCookies(toQt(top_origin_url), toQt(origin_url));
}

void BrowserMessageFilterQt::OnRequestStorageAccessSync(int render_frame_id,
                                                        const GURL& origin_url,
                                                        const GURL& top_origin_url,
                                                        int storage_type,
                                                        IPC::Message* reply_msg)
{
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    base::Callback<void(bool)> callback = base::Bind(
            &BrowserMessageFilterQt::OnRequestStorageAccessSyncResponse,
            base::WrapRefCounted(this), reply_msg);
    OnRequestStorageAccess(render_frame_id,
                           origin_url,
                           top_origin_url,
                           storage_type,
                           callback);
}

void BrowserMessageFilterQt::OnRequestStorageAccessSyncResponse(IPC::Message *reply_msg, bool allowed)
{
    QtWebEngineHostMsg_RequestStorageAccessSync::WriteReplyParams(reply_msg, allowed);
    Send(reply_msg);
}

void BrowserMessageFilterQt::OnRequestStorageAccessAsync(int render_frame_id,
                                                         int request_id,
                                                         const GURL& origin_url,
                                                         const GURL& top_origin_url,
                                                         int storage_type)
{
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    base::Callback<void(bool)> callback = base::Bind(
            &BrowserMessageFilterQt::OnRequestStorageAccessAsyncResponse,
            base::WrapRefCounted(this), render_frame_id, request_id);
    OnRequestStorageAccess(render_frame_id,
                           origin_url,
                           top_origin_url,
                           storage_type,
                           callback);
}

void BrowserMessageFilterQt::OnRequestStorageAccessAsyncResponse(int render_frame_id,
                                                                 int request_id,
                                                                 bool allowed)
{
    Send(new QtWebEngineMsg_RequestStorageAccessAsyncResponse(render_frame_id, request_id, allowed));
}

void BrowserMessageFilterQt::OnRequestStorageAccess(int /*render_frame_id*/,
                                                    const GURL &origin_url,
                                                    const GURL &top_origin_url,
                                                    int /*storage_type*/,
                                                    base::Callback<void(bool)> callback)
{
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    bool allowed = m_profileData->canGetCookies(toQt(top_origin_url), toQt(origin_url));

    callback.Run(allowed);
}

} // namespace QtWebEngineCore
