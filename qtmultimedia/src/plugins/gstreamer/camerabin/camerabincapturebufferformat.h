/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef CAMERABINCAPTUREBUFFERFORMAT_H
#define CAMERABINCAPTUREBUFFERFORMAT_H

#include <qcamera.h>
#include <qcameracapturebufferformatcontrol.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

QT_USE_NAMESPACE

class CameraBinCaptureBufferFormat : public QCameraCaptureBufferFormatControl
{
    Q_OBJECT
public:
    CameraBinCaptureBufferFormat(CameraBinSession *session);
    virtual ~CameraBinCaptureBufferFormat();

    QList<QVideoFrame::PixelFormat> supportedBufferFormats() const override;

    QVideoFrame::PixelFormat bufferFormat() const override;
    void setBufferFormat(QVideoFrame::PixelFormat format) override;

private:
    CameraBinSession *m_session;
    QVideoFrame::PixelFormat m_format;
};

QT_END_NAMESPACE

#endif
