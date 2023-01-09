/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of plugins of the Qt Toolkit.
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

#ifndef QWAYLANDVULKANINSTANCE_P_H
#define QWAYLANDVULKANINSTANCE_P_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_WAYLAND_KHR)
#error "vulkan.h included without Wayland WSI"
#endif

#define VK_USE_PLATFORM_WAYLAND_KHR

#include <QtVulkanSupport/private/qbasicvulkanplatforminstance_p.h>
#include <QLibrary>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandWindow;

class QWaylandVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    explicit QWaylandVulkanInstance(QVulkanInstance *instance);
    ~QWaylandVulkanInstance() override;

    void createOrAdoptInstance() override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;
    void presentAboutToBeQueued(QWindow *window) override;

    VkSurfaceKHR createSurface(QWaylandWindow *window);

private:
    QVulkanInstance *m_instance = nullptr;
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR m_getPhysDevPresSupport = nullptr;
    PFN_vkCreateWaylandSurfaceKHR m_createSurface = nullptr;
};

} // namespace QtWaylandClient

QT_END_NAMESPACE

#endif // QWAYLANDVULKANINSTANCE_P_H
