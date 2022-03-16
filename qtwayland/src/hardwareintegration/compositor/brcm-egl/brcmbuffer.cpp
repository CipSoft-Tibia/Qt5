/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
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

#include "brcmbuffer.h"

#include <EGL/eglext.h>

#include <EGL/eglext_brcm.h>

QT_BEGIN_NAMESPACE

BrcmBuffer::BrcmBuffer(struct ::wl_client *client, uint32_t id, const QSize &size, EGLint *data, size_t count)
    : QtWaylandServer::wl_buffer(client, id, 1)
    , m_handle(count)
    , m_size(size)
{
    for (size_t i = 0; i < count; ++i)
        m_handle[i] = data[i];
}

BrcmBuffer::~BrcmBuffer()
{
    static PFNEGLDESTROYGLOBALIMAGEBRCMPROC eglDestroyGlobalImage =
        (PFNEGLDESTROYGLOBALIMAGEBRCMPROC) eglGetProcAddress("eglDestroyGlobalImageBRCM");
    eglDestroyGlobalImage(handle());
}

void BrcmBuffer::buffer_destroy_resource(Resource *)
{
    delete this;
}

void BrcmBuffer::buffer_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

QT_END_NAMESPACE
