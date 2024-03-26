// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGSUBTITLERENDERER_P_H
#define QFFMPEGSUBTITLERENDERER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "playbackengine/qffmpegrenderer_p.h"

QT_BEGIN_NAMESPACE

class QVideoSink;

namespace QFFmpeg {

class SubtitleRenderer : public Renderer
{
    Q_OBJECT
public:
    SubtitleRenderer(const TimeController &tc, QVideoSink *sink);

    ~SubtitleRenderer() override;

    void setOutput(QVideoSink *sink, bool cleanPrevSink = false);

protected:
    RenderingResult renderInternal(Frame frame) override;

private:
    QPointer<QVideoSink> m_sink;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGSUBTITLERENDERER_P_H
