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

#ifndef AVFCAMERAZOOMCONTROL_H
#define AVFCAMERAZOOMCONTROL_H

#include <QtMultimedia/qcamerazoomcontrol.h>
#include <QtMultimedia/qcamera.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;
class AVFCameraSession;
class AVFCameraControl;

class AVFCameraZoomControl : public QCameraZoomControl
{
    Q_OBJECT
public:
    AVFCameraZoomControl(AVFCameraService *service);

    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;

    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;

    void zoomTo(qreal optical, qreal digital) override;

private Q_SLOTS:
    void cameraStateChanged();

private:
    void zoomToRequestedDigital();

    AVFCameraSession *m_session;

    CGFloat m_maxZoomFactor;
    CGFloat m_zoomFactor;
    CGFloat m_requestedZoomFactor;
};

QT_END_NAMESPACE

#endif
