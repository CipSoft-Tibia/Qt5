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

#include "qgstreameraudiodecoderserviceplugin.h"

#include "qgstreameraudiodecoderservice.h"
#include <private/qgstutils_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/QDir>
#include <QtCore/QDebug>

// #define QT_SUPPORTEDMIMETYPES_DEBUG

QMediaService* QGstreamerAudioDecoderServicePlugin::create(const QString &key)
{
    QGstUtils::initializeGst();

    if (key == QLatin1String(Q_MEDIASERVICE_AUDIODECODER))
        return new QGstreamerAudioDecoderService;

    qWarning() << "Gstreamer audio decoder service plugin: unsupported key:" << key;
    return 0;
}

void QGstreamerAudioDecoderServicePlugin::release(QMediaService *service)
{
    delete service;
}

QMultimedia::SupportEstimate QGstreamerAudioDecoderServicePlugin::hasSupport(const QString &mimeType,
                                                                     const QStringList &codecs) const
{
    if (m_supportedMimeTypeSet.isEmpty())
        updateSupportedMimeTypes();

    return QGstUtils::hasSupport(mimeType, codecs, m_supportedMimeTypeSet);
}

static bool isDecoderOrDemuxer(GstElementFactory *factory)
{
#if GST_CHECK_VERSION(0, 10, 31)
    return gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DEMUXER)
            || gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER
                                                       | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO);
#else
    return (factory
            && (qstrcmp(factory->details.klass, "Codec/Decoder/Audio") == 0
                || qstrcmp(factory->details.klass, "Codec/Demux") == 0));
#endif
}

void QGstreamerAudioDecoderServicePlugin::updateSupportedMimeTypes() const
{
    m_supportedMimeTypeSet = QGstUtils::supportedMimeTypes(isDecoderOrDemuxer);
}

QStringList QGstreamerAudioDecoderServicePlugin::supportedMimeTypes() const
{
    return QStringList();
}

