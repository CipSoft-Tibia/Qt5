// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "evaluateblendclipanimatorjob_p.h"
#include <Qt3DCore/private/qaspectmanager_p.h>
#include <Qt3DCore/private/qskeleton_p.h>
#include <Qt3DAnimation/qblendedclipanimator.h>
#include <Qt3DAnimation/private/handler_p.h>
#include <Qt3DAnimation/private/managers_p.h>
#include <Qt3DAnimation/private/animationlogging_p.h>
#include <Qt3DAnimation/private/animationutils_p.h>
#include <Qt3DAnimation/private/clipblendvalue_p.h>
#include <Qt3DAnimation/private/lerpclipblend_p.h>
#include <Qt3DAnimation/private/clipblendnodevisitor_p.h>
#include <Qt3DAnimation/private/job_common_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {
namespace Animation {

EvaluateBlendClipAnimatorJob::EvaluateBlendClipAnimatorJob()
    : AbstractEvaluateClipAnimatorJob()
    , m_handler(nullptr)
{
    SET_JOB_RUN_STAT_TYPE(this, JobTypes::EvaluateBlendClipAnimator, 0)
}

void EvaluateBlendClipAnimatorJob::run()
{
    // Find the set of clips that need to be evaluated by querying each node
    // in the blend tree.
    // TODO: We should be able to cache this for each blend animator and only
    // update when a node indicates its dependencies have changed as a result
    // of blend factors changing

    BlendedClipAnimator *blendedClipAnimator = m_handler->blendedClipAnimatorManager()->data(m_blendClipAnimatorHandle);
    Q_ASSERT(blendedClipAnimator);
    const bool running = blendedClipAnimator->isRunning();
    const bool seeking = blendedClipAnimator->isSeeking();
    if (!running && !seeking) {
        m_handler->setBlendedClipAnimatorRunning(m_blendClipAnimatorHandle, false);
        return;
    }

    Qt3DCore::QNodeId blendTreeRootId = blendedClipAnimator->blendTreeRootId();
    const QVector<Qt3DCore::QNodeId> valueNodeIdsToEvaluate = gatherValueNodesToEvaluate(m_handler, blendTreeRootId);

    // Calculate the resulting duration of the blend tree based upon its current state
    ClipBlendNodeManager *blendNodeManager = m_handler->clipBlendNodeManager();
    ClipBlendNode *blendTreeRootNode = blendNodeManager->lookupNode(blendTreeRootId);
    Q_ASSERT(blendTreeRootNode);
    const double duration = blendTreeRootNode->duration();

    Clock *clock = m_handler->clockManager()->lookupResource(blendedClipAnimator->clockId());

    qint64 globalTimeNS = m_handler->simulationTime();
    qint64 nsSincePreviousFrame = seeking ? toNsecs(duration * blendedClipAnimator->normalizedLocalTime())
                                          : blendedClipAnimator->nsSincePreviousFrame(globalTimeNS);

    // Calculate the phase given the blend tree duration and global time
    AnimatorEvaluationData animatorData = evaluationDataForAnimator(blendedClipAnimator, clock, nsSincePreviousFrame);
    const double phase = phaseFromElapsedTime(animatorData.currentTime, animatorData.elapsedTime,
                                              animatorData.playbackRate,
                                              duration,
                                              animatorData.loopCount,
                                              animatorData.currentLoop);

    // Iterate over the value nodes of the blend tree, evaluate the
    // contained animation clips at the current phase and store the results
    // in the animator indexed by node.
    AnimationClipLoaderManager *clipLoaderManager = m_handler->animationClipLoaderManager();
    for (const auto &valueNodeId : valueNodeIdsToEvaluate) {
        ClipBlendValue *valueNode = static_cast<ClipBlendValue *>(blendNodeManager->lookupNode(valueNodeId));
        Q_ASSERT(valueNode);
        AnimationClip *clip = clipLoaderManager->lookupResource(valueNode->clipId());
        Q_ASSERT(clip);

        ClipResults rawClipResults = evaluateClipAtPhase(clip, float(phase));

        // Reformat the clip results into the layout used by this animator/blend tree
        const ClipFormat format = valueNode->clipFormat(blendedClipAnimator->peerId());
        ClipResults formattedClipResults = formatClipResults(rawClipResults, format.sourceClipIndices);
        applyComponentDefaultValues(format.defaultComponentValues, formattedClipResults);
        valueNode->setClipResults(blendedClipAnimator->peerId(), formattedClipResults);
    }

    // Evaluate the blend tree
    ClipResults blendedResults = evaluateBlendTree(m_handler, blendedClipAnimator, blendTreeRootId);

    const double localTime = phase * duration;
    blendedClipAnimator->setLastGlobalTimeNS(globalTimeNS);
    blendedClipAnimator->setLastLocalTime(localTime);
    blendedClipAnimator->setLastNormalizedLocalTime(float(phase));
    blendedClipAnimator->setCurrentLoop(animatorData.currentLoop);

    // Prepare the change record
    const bool finalFrame = isFinalFrame(localTime, duration, animatorData.currentLoop, animatorData.loopCount, animatorData.playbackRate);
    const QVector<MappingData> mappingData = blendedClipAnimator->mappingData();
    auto record = prepareAnimationRecord(blendedClipAnimator->peerId(),
                                         mappingData,
                                         blendedResults,
                                         finalFrame,
                                         float(phase));

    // Trigger callbacks either on this thread or by notifying the gui thread.
    auto callbacks = prepareCallbacks(mappingData, blendedResults);

    // Update the normalized time on the backend node so that
    // frontend <-> backend sync will not mark things dirty
    // unless the frontend normalized time really is different
    blendedClipAnimator->setNormalizedLocalTime(record.normalizedTime, false);

    setPostFrameData(record, callbacks);
}

} // Animation
} // Qt3DAnimation

QT_END_NAMESPACE
