// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGPLAYBACKENGINEDEFS_P_H
#define QFFMPEGPLAYBACKENGINEDEFS_P_H

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
#include "qobject.h"
#include "qpointer.h"

#include <memory>
#include <array>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
class PlaybackEngine;
}

namespace QFFmpeg {

using StreamIndexes = std::array<int, 3>;

class PlaybackEngineObjectsController;
class PlaybackEngineObject;
class Demuxer;
class StreamDecoder;
class Renderer;
class SubtitleRenderer;
class AudioRenderer;
class VideoRenderer;

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGPLAYBACKENGINEDEFS_P_H
