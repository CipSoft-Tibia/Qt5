// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "evaluateclipanimatorjob_p.h"
#include <Qt3DCore/private/qaspectjob_p.h>
#include <Qt3DCore/private/qaspectmanager_p.h>
#include <Qt3DAnimation/qclipanimator.h>
#include <Qt3DAnimation/private/handler_p.h>
#include <Qt3DAnimation/private/managers_p.h>
#include <Qt3DAnimation/private/animationlogging_p.h>
#include <Qt3DAnimation/private/animationutils_p.h>
#include <Qt3DAnimation/private/job_common_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {
namespace Animation {


EvaluateClipAnimatorJob::EvaluateClipAnimatorJob()
    : AbstractEvaluateClipAnimatorJob()
    , m_handler(nullptr)
{
    SET_JOB_RUN_STAT_TYPE(this, JobTypes::EvaluateClipAnimator, 0)
}

void EvaluateClipAnimatorJob::run()
{
    Q_ASSERT(m_handler);

    ClipAnimator *clipAnimator = m_handler->clipAnimatorManager()->data(m_clipAnimatorHandle);
    Q_ASSERT(clipAnimator);
    const bool running = clipAnimator->isRunning();
    const bool seeking = clipAnimator->isSeeking();
    if (!running && !seeking) {
        m_handler->setClipAnimatorRunning(m_clipAnimatorHandle, false);
        return;
    }

    const qint64 globalTimeNS = m_handler->simulationTime();

    Clock *clock = m_handler->clockManager()->lookupResource(clipAnimator->clockId());

    // Evaluate the fcurves
    AnimationClip *clip = m_handler->animationClipLoaderManager()->lookupResource(clipAnimator->clipId());
    Q_ASSERT(clip);

    const qint64 nsSincePreviousFrame = seeking ? toNsecs(clip->duration() * clipAnimator->normalizedLocalTime())
                                                : clipAnimator->nsSincePreviousFrame(globalTimeNS);

    // Prepare for evaluation (convert global time to local time ....)
    const AnimatorEvaluationData animatorEvaluationData = evaluationDataForAnimator(clipAnimator,
                                                                                    clock,
                                                                                    nsSincePreviousFrame);

    const ClipEvaluationData preEvaluationDataForClip = evaluationDataForClip(clip, animatorEvaluationData);
    const ClipResults rawClipResults = evaluateClipAtPhase(clip, preEvaluationDataForClip.normalizedLocalTime);

    // Reformat the clip results into the layout used by this animator/blend tree
    const ClipFormat clipFormat = clipAnimator->clipFormat();
    ClipResults formattedClipResults = formatClipResults(rawClipResults, clipFormat.sourceClipIndices);

    if (preEvaluationDataForClip.isFinalFrame)
        clipAnimator->setRunning(false);

    clipAnimator->setCurrentLoop(preEvaluationDataForClip.currentLoop);
    clipAnimator->setLastGlobalTimeNS(globalTimeNS);
    clipAnimator->setLastLocalTime(preEvaluationDataForClip.localTime);
    clipAnimator->setLastNormalizedLocalTime(preEvaluationDataForClip.normalizedLocalTime);

    // Prepare property changes (if finalFrame it also prepares the change for the running property for the frontend)
    auto record = prepareAnimationRecord(clipAnimator->peerId(),
                                         clipAnimator->mappingData(),
                                         formattedClipResults,
                                         preEvaluationDataForClip.isFinalFrame,
                                         preEvaluationDataForClip.normalizedLocalTime);

    // Trigger callbacks either on this thread or by notifying the gui thread.
    auto callbacks = prepareCallbacks(clipAnimator->mappingData(), formattedClipResults);

    // Update the normalized time on the backend node so that
    // frontend <-> backend sync will not mark things dirty
    // unless the frontend normalized time really is different
    clipAnimator->setNormalizedLocalTime(record.normalizedTime, false);

    setPostFrameData(record, callbacks);
}

} // namespace Animation
} // namespace Qt3DAnimation

QT_END_NAMESPACE
