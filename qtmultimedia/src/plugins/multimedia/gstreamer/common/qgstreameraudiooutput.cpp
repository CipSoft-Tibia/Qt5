// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qgstreameraudiooutput_p.h>
#include <qgstreameraudiodevice_p.h>
#include <qaudiodevice.h>
#include <qaudiooutput.h>

#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>

static Q_LOGGING_CATEGORY(qLcMediaAudioOutput, "qt.multimedia.audiooutput")

QT_BEGIN_NAMESPACE

QMaybe<QPlatformAudioOutput *> QGstreamerAudioOutput::create(QAudioOutput *parent)
{
    QGstElement audioconvert = QGstElement::createFromFactory("audioconvert", "audioConvert");
    if (!audioconvert)
        return errorMessageCannotFindElement("audioconvert");

    QGstElement audioresample = QGstElement::createFromFactory("audioresample", "audioResample");
    if (!audioresample)
        return errorMessageCannotFindElement("audioresample");

    QGstElement volume = QGstElement::createFromFactory("volume", "volume");
    if (!volume)
        return errorMessageCannotFindElement("volume");

    QGstElement autoaudiosink = QGstElement::createFromFactory("autoaudiosink", "autoAudioSink");
    if (!autoaudiosink)
        return errorMessageCannotFindElement("autoaudiosink");

    return new QGstreamerAudioOutput(audioconvert, audioresample, volume, autoaudiosink, parent);
}

QGstreamerAudioOutput::QGstreamerAudioOutput(QGstElement audioconvert, QGstElement audioresample,
                                             QGstElement volume, QGstElement autoaudiosink,
                                             QAudioOutput *parent)
    : QObject(parent),
      QPlatformAudioOutput(parent),
      gstAudioOutput(QGstBin::create("audioOutput")),
      audioConvert(std::move(audioconvert)),
      audioResample(std::move(audioresample)),
      audioVolume(std::move(volume)),
      audioSink(std::move(autoaudiosink))
{
    audioQueue = QGstElement::createFromFactory("queue", "audioQueue");
    gstAudioOutput.add(audioQueue, audioConvert, audioResample, audioVolume, audioSink);
    qLinkGstElements(audioQueue, audioConvert, audioResample, audioVolume, audioSink);

    gstAudioOutput.addGhostPad(audioQueue, "sink");
}

QGstreamerAudioOutput::~QGstreamerAudioOutput()
{
    gstAudioOutput.setStateSync(GST_STATE_NULL);
}

void QGstreamerAudioOutput::setVolume(float vol)
{
    audioVolume.set("volume", vol);
}

void QGstreamerAudioOutput::setMuted(bool muted)
{
    audioVolume.set("mute", muted);
}

void QGstreamerAudioOutput::setPipeline(const QGstPipeline &pipeline)
{
    gstPipeline = pipeline;
}

void QGstreamerAudioOutput::setAudioDevice(const QAudioDevice &info)
{
    if (info == m_audioOutput)
        return;
    qCDebug(qLcMediaAudioOutput) << "setAudioOutput" << info.description() << info.isNull();
    m_audioOutput = info;

    QGstElement newSink;
    if constexpr (QT_CONFIG(pulseaudio)) {
        auto id = m_audioOutput.id();
        newSink = QGstElement::createFromFactory("pulsesink", "audiosink");
        if (!newSink.isNull())
            newSink.set("device", id.constData());
        else
            qCWarning(qLcMediaAudioOutput) << "Invalid audio device";
    } else {
        auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioOutput.handle());
        if (deviceInfo && deviceInfo->gstDevice) {
            newSink = QGstElement{
                gst_device_create_element(deviceInfo->gstDevice.get(), "audiosink"),
                QGstElement::HasRef,
            };
        } else {
            qCWarning(qLcMediaAudioOutput) << "Invalid audio device";
        }
    }

    if (newSink.isNull()) {
        qCWarning(qLcMediaAudioOutput) << "Failed to create a gst element for the audio device, using a default audio sink";
        newSink = QGstElement::createFromFactory("autoaudiosink", "audiosink");
    }

    audioVolume.staticPad("src").doInIdleProbe([&]() {
        qUnlinkGstElements(audioVolume, audioSink);
        gstAudioOutput.remove(audioSink);
        gstAudioOutput.add(newSink);
        newSink.syncStateWithParent();
        qLinkGstElements(audioVolume, newSink);
    });

    audioSink.setStateSync(GST_STATE_NULL);
    audioSink = newSink;
}

QT_END_NAMESPACE

#include "moc_qgstreameraudiooutput_p.cpp"
