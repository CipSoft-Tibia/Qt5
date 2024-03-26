// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "animationclip_p.h"
#include <Qt3DAnimation/qanimationclip.h>
#include <Qt3DAnimation/qanimationcliploader.h>
#include <Qt3DAnimation/private/qanimationclip_p.h>
#include <Qt3DAnimation/private/qanimationcliploader_p.h>
#include <Qt3DAnimation/private/animationlogging_p.h>
#include <Qt3DAnimation/private/managers_p.h>
#include <Qt3DAnimation/private/gltfimporter_p.h>
#include <Qt3DCore/private/qurlhelper_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qurlquery.h>

QT_BEGIN_NAMESPACE

#define ANIMATION_INDEX_KEY     QLatin1String("animationIndex")
#define ANIMATION_NAME_KEY      QLatin1String("animationName")

namespace Qt3DAnimation {
namespace Animation {

AnimationClip::AnimationClip()
    : BackendNode(ReadWrite)
    , m_source()
    , m_status(QAnimationClipLoader::NotReady)
    , m_clipData()
    , m_dataType(Unknown)
    , m_name()
    , m_channels()
    , m_duration(0.0f)
    , m_channelComponentCount(0)
{
}

void AnimationClip::cleanup()
{
    setEnabled(false);
    m_handler = nullptr;
    m_source.clear();
    m_clipData.clearChannels();
    m_status = QAnimationClipLoader::NotReady;
    m_dataType = Unknown;
    m_channels.clear();
    m_duration = 0.0f;
    m_channelComponentCount = 0;

    clearData();
}

void AnimationClip::setStatus(QAnimationClipLoader::Status status)
{
    if (status != m_status) {
        m_status = status;
    }
}

void AnimationClip::syncFromFrontEnd(const Qt3DCore::QNode *frontEnd, bool firstTime)
{
    BackendNode::syncFromFrontEnd(frontEnd, firstTime);
    const QAbstractAnimationClip *node = qobject_cast<const QAbstractAnimationClip *>(frontEnd);
    if (!node)
        return;

    const QAnimationClip *clipNode = qobject_cast<const QAnimationClip *>(frontEnd);
    if (clipNode) {
        if (firstTime)
            m_dataType = Data;
        Q_ASSERT(m_dataType == Data);
        if (m_clipData != clipNode->clipData()) {
            m_clipData = clipNode->clipData();
            if (m_clipData.isValid())
                setDirty(Handler::AnimationClipDirty);
        }
    }

    const QAnimationClipLoader *loaderNode = qobject_cast<const QAnimationClipLoader *>(frontEnd);
    if (loaderNode) {
        if (firstTime)
            m_dataType = File;
        Q_ASSERT(m_dataType == File);
        if (m_source != loaderNode->source()) {
            m_source = loaderNode->source();
            if (!m_source.isEmpty())
                setDirty(Handler::AnimationClipDirty);
        }
    }
}

/*!
    \internal
    Called by LoadAnimationClipJob on the threadpool
 */
void AnimationClip::loadAnimation()
{
    qCDebug(Jobs) << Q_FUNC_INFO << m_source;
    clearData();

    // Load the data
    switch (m_dataType) {
    case File:
        loadAnimationFromUrl();
        break;

    case Data:
        loadAnimationFromData();
        break;

    default:
        Q_UNREACHABLE();
    }

    // Update the duration
    const float t = findDuration();
    setDuration(t);

    m_channelComponentCount = findChannelComponentCount();

    // If using a loader inform the frontend of the status change
    if (m_source.isEmpty()) {
        if (qFuzzyIsNull(t) || m_channelComponentCount == 0)
            setStatus(QAnimationClipLoader::Error);
        else
            setStatus(QAnimationClipLoader::Ready);
    }

    // notify all ClipAnimators and BlendedClipAnimators that depend on this clip,
    // that the clip has changed and that they are now dirty
    {
        QMutexLocker lock(&m_mutex);
        for (const Qt3DCore::QNodeId &id : std::as_const(m_dependingAnimators)) {
            ClipAnimator *animator = m_handler->clipAnimatorManager()->lookupResource(id);
            if (animator)
                animator->animationClipMarkedDirty();
        }
        for (const Qt3DCore::QNodeId &id : std::as_const(m_dependingBlendedAnimators)) {
            BlendedClipAnimator *animator = m_handler->blendedClipAnimatorManager()->lookupResource(id);
            if (animator)
                animator->animationClipMarkedDirty();
        }
        m_dependingAnimators.clear();
        m_dependingBlendedAnimators.clear();
    }

    qCDebug(Jobs) << "Loaded animation data:" << *this;
}

void AnimationClip::loadAnimationFromUrl()
{
    // TODO: Handle remote files
    QString filePath = Qt3DCore::QUrlHelper::urlToLocalFileOrQrc(m_source);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not find animation clip:" << filePath;
        setStatus(QAnimationClipLoader::Error);
        return;
    }

    // Extract the animationName or animationIndex from the url query parameters.
    // If both present, animationIndex wins.
    int animationIndex = -1;
    QString animationName;
    if (m_source.hasQuery()) {
        QUrlQuery query(m_source);
        if (query.hasQueryItem(ANIMATION_INDEX_KEY)) {
            bool ok = false;
            int i = query.queryItemValue(ANIMATION_INDEX_KEY).toInt(&ok);
            if (ok)
                animationIndex = i;
        }

        if (animationIndex == -1 && query.hasQueryItem(ANIMATION_NAME_KEY)) {
            animationName = query.queryItemValue(ANIMATION_NAME_KEY);
        }

        qCDebug(Jobs) << "animationIndex =" << animationIndex;
        qCDebug(Jobs) << "animationName =" << animationName;
    }

    // TODO: Convert to plugins
    // Load glTF or "native"
    if (filePath.endsWith(QLatin1String("gltf"))) {
        qCDebug(Jobs) << "Loading glTF animation from" << filePath;
        GLTFImporter gltf;
        gltf.load(&file);
        auto nameAndChannels = gltf.createAnimationData(animationIndex, animationName);
        m_name = nameAndChannels.name;
        m_channels = nameAndChannels.channels;
    } else if (filePath.endsWith(QLatin1String("json"))) {
        // Native format
        QByteArray animationData = file.readAll();
        QJsonDocument document = QJsonDocument::fromJson(animationData);
        QJsonObject rootObject = document.object();

        // TODO: Allow loading of a named animation from a file containing many
        const QJsonArray animationsArray = rootObject[QLatin1String("animations")].toArray();
        qCDebug(Jobs) << "Found" << animationsArray.size() << "animations:";
        for (qsizetype i = 0; i < animationsArray.size(); ++i) {
            QJsonObject animation = animationsArray.at(i).toObject();
            qCDebug(Jobs) << "Animation Name:" << animation[QLatin1String("animationName")].toString();
        }

        // Find which animation clip to load from the file.
        // Give priority to animationIndex over animationName
        if (animationIndex >= animationsArray.size()) {
            qCWarning(Jobs) << "Invalid animation index. Skipping.";
            return;
        }

        if (animationsArray.size() == 1) {
            animationIndex = 0;
        } else if (animationIndex < 0 && !animationName.isEmpty()) {
            // Can we find an animation of the correct name?
            bool foundAnimation = false;
            for (qsizetype i = 0; i < animationsArray.size(); ++i) {
                if (animationsArray.at(i)[ANIMATION_NAME_KEY].toString() == animationName) {
                    animationIndex = i;
                    foundAnimation = true;
                    break;
                }
            }

            if (!foundAnimation) {
                qCWarning(Jobs) << "Invalid animation name. Skipping.";
                return;
            }
        }

        if (animationIndex < 0 || animationIndex >= animationsArray.size()) {
            qCWarning(Jobs) << "Failed to find animation. Skipping.";
            return;
        }

        QJsonObject animation = animationsArray.at(animationIndex).toObject();
        m_name = animation[QLatin1String("animationName")].toString();

        QJsonArray channelsArray = animation[QLatin1String("channels")].toArray();
        const qsizetype channelCount = channelsArray.size();
        m_channels.resize(channelCount);
        for (qsizetype i = 0; i < channelCount; ++i) {
            const QJsonObject group = channelsArray.at(i).toObject();
            m_channels[i].read(group);
        }
    } else {
        qWarning() << "Unknown animation clip type. Please use json or glTF 2.0";
        setStatus(QAnimationClipLoader::Error);
    }
}

void AnimationClip::loadAnimationFromData()
{
    // Reformat data from QAnimationClipData to backend format
    m_channels.resize(m_clipData.channelCount());
    qsizetype i = 0;
    for (const auto &frontendChannel : std::as_const(m_clipData))
        m_channels[i++].setFromQChannel(frontendChannel);
}

void AnimationClip::addDependingClipAnimator(const Qt3DCore::QNodeId &id)
{
    QMutexLocker lock(&m_mutex);
    m_dependingAnimators.push_back(id);
}

void AnimationClip::addDependingBlendedClipAnimator(const Qt3DCore::QNodeId &id)
{
    QMutexLocker lock(&m_mutex);
    m_dependingBlendedAnimators.push_back(id);
}

void AnimationClip::setDuration(float duration)
{
    if (qFuzzyCompare(duration, m_duration))
        return;

    m_duration = duration;
}

qsizetype AnimationClip::channelIndex(const QString &channelName, qsizetype jointIndex) const
{
    const qsizetype channelCount = m_channels.size();
    for (qsizetype i = 0; i < channelCount; ++i) {
        if (m_channels[i].name == channelName
            && (jointIndex == -1 || m_channels[i].jointIndex == jointIndex)) {
            return i;
        }
    }
    return -1;
}

/*!
    \internal

    Given the index of a channel, \a channelIndex, calculates
    the base index of the first channelComponent in this group. For example, if
    there are two channel groups each with 3 channels and you request
    the channelBaseIndex(1), the return value will be 3. Indices 0-2 are
    for the first group, so the first channel of the second group occurs
    at index 3.
 */
qsizetype AnimationClip::channelComponentBaseIndex(qsizetype channelIndex) const
{
    qsizetype index = 0;
    for (qsizetype i = 0; i < channelIndex; ++i)
        index += m_channels[i].channelComponents.size();
    return index;
}

void AnimationClip::clearData()
{
    m_name.clear();
    m_channels.clear();
}

float AnimationClip::findDuration()
{
    // Iterate over the contained fcurves and find the longest one
    float tMax = 0.f;
    for (const Channel &channel : std::as_const(m_channels)) {
        for (const ChannelComponent &channelComponent : std::as_const(channel.channelComponents)) {
            const float t = channelComponent.fcurve.endTime();
            if (t > tMax)
                tMax = t;
        }
    }
    return tMax;
}

qsizetype AnimationClip::findChannelComponentCount()
{
    qsizetype channelCount = 0;
    for (const Channel &channel : std::as_const(m_channels))
        channelCount += channel.channelComponents.size();
    return channelCount;
}

} // namespace Animation
} // namespace Qt3DAnimation

QT_END_NAMESPACE
