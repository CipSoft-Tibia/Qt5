// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BATCHRENDERER_H
#define BATCHRENDERER_H

#include <QHash>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE

class BMBase;
class QImage;
class QVersionNumber;
class LottieAnimation;

class BatchRenderer : public QThread
{
    Q_OBJECT

    struct Entry
    {
        LottieAnimation* animator = nullptr;
        BMBase *bmTreeBlueprint = nullptr;
        int startFrame = 0;
        int endFrame = 0;
        int currentFrame = 0;
        int animDir = 1;
        QHash<int, BMBase*> frameCache;
    };

public:
    ~BatchRenderer() override;

    BatchRenderer(BatchRenderer const &) = delete;
    void operator=(BatchRenderer const&) = delete;

    static BatchRenderer *instance();
    static void deleteInstance();

    BMBase *getFrame(LottieAnimation *animator, int frameNumber);

signals:
    void frameReady(LottieAnimation *animator, int frameNumber);

public slots:
    void registerAnimator(LottieAnimation *animator);
    void deregisterAnimator(LottieAnimation *animator);

    bool gotoFrame(LottieAnimation *animator, int frame);

    void frameRendered(LottieAnimation *animator, int frameNumber);

protected:
    void run() override;

    int parse(BMBase *rootElement, const QByteArray &jsonSource,
              const QVersionNumber &version) const;

    void prerender(Entry *animEntry);

private:
    BatchRenderer();

    void pruneFrameCache(Entry* e);

private:
    static BatchRenderer *m_rendererInstance;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;

    int m_cacheSize = 2;
    QHash<LottieAnimation *, Entry *> m_animData;
    int m_lastRenderedFrame = -1;
};

QT_END_NAMESPACE

#endif // BATCHRENDERER_H
