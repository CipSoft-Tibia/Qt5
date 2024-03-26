// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickprofileradapter.h"

#include <QCoreApplication>
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qquickprofiler_p.h>

QT_BEGIN_NAMESPACE

using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

QQuickProfilerAdapter::QQuickProfilerAdapter(QObject *parent) :
    QQmlAbstractProfilerAdapter(parent), next(0)
{
    QQuickProfiler::initialize(this);

    // We can always do DirectConnection here as all methods are protected by mutexes
    connect(this, &QQmlAbstractProfilerAdapter::profilingEnabled,
            QQuickProfiler::s_instance, &QQuickProfiler::startProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingEnabledWhileWaiting,
            QQuickProfiler::s_instance, &QQuickProfiler::startProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::referenceTimeKnown,
            QQuickProfiler::s_instance, &QQuickProfiler::setTimer, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingDisabled,
            QQuickProfiler::s_instance, &QQuickProfiler::stopProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingDisabledWhileWaiting,
            QQuickProfiler::s_instance, &QQuickProfiler::stopProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::dataRequested,
            QQuickProfiler::s_instance, &QQuickProfiler::reportDataImpl, Qt::DirectConnection);
    connect(QQuickProfiler::s_instance, &QQuickProfiler::dataReady,
            this, &QQuickProfilerAdapter::receiveData, Qt::DirectConnection);
}

QQuickProfilerAdapter::~QQuickProfilerAdapter()
{
    if (service)
        service->removeGlobalProfiler(this);
}

// convert to QByteArrays that can be sent to the debug client
static void qQuickProfilerDataToByteArrays(const QQuickProfilerData &data,
                                           QList<QByteArray> &messages)
{
    QQmlDebugPacket ds;
    Q_ASSERT_X(((data.messageType | data.detailType) & (1 << 31)) == 0, Q_FUNC_INFO,
               "You can use at most 31 message types and 31 detail types.");
    for (uint decodedMessageType = 0; (data.messageType >> decodedMessageType) != 0;
         ++decodedMessageType) {
        if ((data.messageType & (1 << decodedMessageType)) == 0)
            continue;

        for (uint decodedDetailType = 0; (data.detailType >> decodedDetailType) != 0;
             ++decodedDetailType) {
            if ((data.detailType & (1 << decodedDetailType)) == 0)
                continue;

            ds << data.time << decodedMessageType << decodedDetailType;

            switch (decodedMessageType) {
            case QQuickProfiler::Event:
                switch (decodedDetailType) {
                case QQuickProfiler::AnimationFrame:
                    ds << data.framerate << data.count << data.threadId;
                    break;
                case QQuickProfiler::Key:
                case QQuickProfiler::Mouse:
                    ds << data.inputType << data.inputA << data.inputB;
                    break;
                }
                break;
            case QQuickProfiler::PixmapCacheEvent:
                ds << data.detailUrl.toString();
                switch (decodedDetailType) {
                    case QQuickProfiler::PixmapSizeKnown: ds << data.x << data.y; break;
                    case QQuickProfiler::PixmapReferenceCountChanged: ds << data.count; break;
                    case QQuickProfiler::PixmapCacheCountChanged: ds << data.count; break;
                    default: break;
                }
                break;
            case QQuickProfiler::SceneGraphFrame:
                switch (decodedDetailType) {
                    // RendererFrame: preprocessTime, updateTime, bindingTime, renderTime
                    case QQuickProfiler::SceneGraphRendererFrame: ds << data.subtime_1 << data.subtime_2 << data.subtime_3 << data.subtime_4; break;
                    // AdaptationLayerFrame: glyphCount (which is an integer), glyphRenderTime, glyphStoreTime
                    case QQuickProfiler::SceneGraphAdaptationLayerFrame: ds << data.subtime_3 << data.subtime_1 << data.subtime_2; break;
                    // ContextFrame: compiling material time
                    case QQuickProfiler::SceneGraphContextFrame: ds << data.subtime_1; break;
                    // RenderLoop: syncTime, renderTime, swapTime
                    case QQuickProfiler::SceneGraphRenderLoopFrame: ds << data.subtime_1 << data.subtime_2 << data.subtime_3; break;
                    // TexturePrepare: bind, convert, swizzle, upload, mipmap
                    case QQuickProfiler::SceneGraphTexturePrepare: ds << data.subtime_1 << data.subtime_2 << data.subtime_3 << data.subtime_4 << data.subtime_5; break;
                    // TextureDeletion: deletionTime
                    case QQuickProfiler::SceneGraphTextureDeletion: ds << data.subtime_1; break;
                    // PolishAndSync: polishTime, waitTime, syncTime, animationsTime,
                    case QQuickProfiler::SceneGraphPolishAndSync: ds << data.subtime_1 << data.subtime_2 << data.subtime_3 << data.subtime_4; break;
                    // WindowsRenderLoop: GL time, make current time, SceneGraph time
                    case QQuickProfiler::SceneGraphWindowsRenderShow: ds << data.subtime_1 << data.subtime_2 << data.subtime_3; break;
                    // WindowsAnimations: update time
                    case QQuickProfiler::SceneGraphWindowsAnimations: ds << data.subtime_1; break;
                    // non-threaded rendering: polish time
                    case QQuickProfiler::SceneGraphPolishFrame: ds << data.subtime_1; break;
                    default:break;
                }
                break;
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message type.");
                break;
            }
            messages.append(ds.squeezedData());
            ds.clear();
        }
    }
}

qint64 QQuickProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (next < m_data.size()) {
        if (m_data[next].time <= until && messages.size() <= s_numMessagesPerBatch)
            qQuickProfilerDataToByteArrays(m_data[next++], messages);
        else
            return m_data[next].time;
    }
    m_data.clear();
    next = 0;
    return -1;
}

void QQuickProfilerAdapter::receiveData(const QVector<QQuickProfilerData> &new_data)
{
    if (m_data.isEmpty())
        m_data = new_data;
    else
        m_data.append(new_data);
    service->dataReady(this);
}

QT_END_NAMESPACE

#include "moc_qquickprofileradapter.cpp"
