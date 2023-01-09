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

#ifndef PERMISSION_MANAGER_QT_H
#define PERMISSION_MANAGER_QT_H

#include "base/callback.h"
#include "content/public/browser/permission_controller_delegate.h"

#include "profile_adapter.h"

#include <map>

namespace QtWebEngineCore {

class PermissionManagerQt : public content::PermissionControllerDelegate {

public:
    PermissionManagerQt();
    ~PermissionManagerQt();

    void permissionRequestReply(const QUrl &origin, ProfileAdapter::PermissionType type, ProfileAdapter::PermissionState reply);
    bool checkPermission(const QUrl &origin, ProfileAdapter::PermissionType type);

    // content::PermissionManager implementation:
    int RequestPermission(
        content::PermissionType permission,
        content::RenderFrameHost* render_frame_host,
        const GURL& requesting_origin,
        bool user_gesture,
        base::OnceCallback<void(blink::mojom::PermissionStatus)> callback) override;

    blink::mojom::PermissionStatus GetPermissionStatus(
        content::PermissionType permission,
        const GURL& requesting_origin,
        const GURL& embedding_origin) override;

    blink::mojom::PermissionStatus GetPermissionStatusForFrame(
        content::PermissionType permission,
        content::RenderFrameHost *render_frame_host,
        const GURL& requesting_origin) override;

    void ResetPermission(
        content::PermissionType permission,
        const GURL& requesting_origin,
        const GURL& embedding_origin) override;

    int RequestPermissions(
        const std::vector<content::PermissionType>& permission,
        content::RenderFrameHost* render_frame_host,
        const GURL& requesting_origin,
        bool user_gesture,
        base::OnceCallback<void(
            const std::vector<blink::mojom::PermissionStatus>&)> callback) override;

    content::PermissionControllerDelegate::SubscriptionId SubscribePermissionStatusChange(
        content::PermissionType permission,
        content::RenderFrameHost* render_frame_host,
        const GURL& requesting_origin,
        const base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback) override;

    void UnsubscribePermissionStatusChange(content::PermissionControllerDelegate::SubscriptionId subscription_id) override;

private:
    QHash<QPair<QUrl, ProfileAdapter::PermissionType>, bool> m_permissions;
    struct Request {
        int id;
        ProfileAdapter::PermissionType type;
        QUrl origin;
        base::OnceCallback<void(blink::mojom::PermissionStatus)> callback;
    };
    struct MultiRequest {
        int id;
        std::vector<content::PermissionType> types;
        QUrl origin;
        base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)> callback;
    };
    struct Subscription {
        ProfileAdapter::PermissionType type;
        QUrl origin;
        base::RepeatingCallback<void(blink::mojom::PermissionStatus)> callback;
    };
    std::vector<Request> m_requests;
    std::vector<MultiRequest> m_multiRequests;
    std::map<content::PermissionControllerDelegate::SubscriptionId, Subscription> m_subscribers;
    content::PermissionControllerDelegate::SubscriptionId::Generator subscription_id_generator_;
    int m_requestIdCount;

};

} // namespace QtWebEngineCore

#endif // PERMISSION_MANAGER_QT_H
