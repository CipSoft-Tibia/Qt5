// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "playbackengine/qffmpegcodec_p.h"
#include "qloggingcategory.h"

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcPlaybackEngineCodec, "qt.multimedia.playbackengine.codec");

namespace QFFmpeg {

Codec::Data::Data(AVCodecContextUPtr context, AVStream *stream, AVFormatContext *formatContext,
                  std::unique_ptr<QFFmpeg::HWAccel> hwAccel)
    : context(std::move(context)), stream(stream), hwAccel(std::move(hwAccel))
{
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        pixelAspectRatio = av_guess_sample_aspect_ratio(formatContext, stream, nullptr);
}

QMaybe<Codec> Codec::create(AVStream *stream, AVFormatContext *formatContext)
{
    if (!stream)
        return { "Invalid stream" };

    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        auto hwCodec = create(stream, formatContext, Hw);
        if (hwCodec)
            return hwCodec;

        qCInfo(qLcPlaybackEngineCodec) << hwCodec.error();
    }

    auto codec = create(stream, formatContext, Sw);
    if (!codec)
        qCWarning(qLcPlaybackEngineCodec) << codec.error();

    return codec;
}

AVRational Codec::pixelAspectRatio(AVFrame *frame) const
{
    // does the same as av_guess_sample_aspect_ratio, but more efficient
    return d->pixelAspectRatio.num && d->pixelAspectRatio.den ? d->pixelAspectRatio
                                                              : frame->sample_aspect_ratio;
}

QMaybe<Codec> Codec::create(AVStream *stream, AVFormatContext *formatContext,
                            VideoCodecCreationPolicy videoCodecPolicy)
{
    Q_ASSERT(stream);

    if (videoCodecPolicy == Hw && stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
        Q_ASSERT(!"Codec::create has been called with Hw policy on a non-video stream");

    const AVCodec *decoder = nullptr;
    std::unique_ptr<QFFmpeg::HWAccel> hwAccel;

    if (videoCodecPolicy == Hw)
        std::tie(decoder, hwAccel) = HWAccel::findDecoderWithHwAccel(stream->codecpar->codec_id);
    else
        decoder = QFFmpeg::findAVDecoder(stream->codecpar->codec_id);

    if (!decoder)
        return { QString("No %1 decoder found").arg(videoCodecPolicy == Hw ? "HW" : "SW") };

    qCDebug(qLcPlaybackEngineCodec) << "found decoder" << decoder->name << "for id" << decoder->id;

    AVCodecContextUPtr context(avcodec_alloc_context3(decoder));
    if (!context)
        return { "Failed to allocate a FFmpeg codec context" };

    // Use HW decoding even if the codec level doesn't match the reported capabilities
    // of the hardware. FFmpeg documentation recommendeds setting this flag by default.
    context->hwaccel_flags |= AV_HWACCEL_FLAG_IGNORE_LEVEL;

    static const bool allowProfileMismatch = static_cast<bool>(
            qEnvironmentVariableIntValue("QT_FFMPEG_HW_ALLOW_PROFILE_MISMATCH"));
    if (allowProfileMismatch) {
        // Use HW decoding even if the codec profile doesn't match the reported capabilities
        // of the hardware.
        context->hwaccel_flags |= AV_HWACCEL_FLAG_ALLOW_PROFILE_MISMATCH;
    }

    if (hwAccel)
        context->hw_device_ctx = av_buffer_ref(hwAccel->hwDeviceContextAsBuffer());

    if (context->codec_type != AVMEDIA_TYPE_AUDIO && context->codec_type != AVMEDIA_TYPE_VIDEO
        && context->codec_type != AVMEDIA_TYPE_SUBTITLE) {
        return { "Unknown codec type" };
    }

    int ret = avcodec_parameters_to_context(context.get(), stream->codecpar);
    if (ret < 0)
        return QStringLiteral("Failed to set FFmpeg codec parameters: %1").arg(err2str(ret));

    // ### This still gives errors about wrong HW formats (as we accept all of them)
    // But it would be good to get so we can filter out pixel format we don't support natively
    context->get_format = QFFmpeg::getFormat;

    /* Init the decoder, with reference counting and threading */
    AVDictionaryHolder opts;
    av_dict_set(opts, "refcounted_frames", "1", 0);
    av_dict_set(opts, "threads", "auto", 0);
    applyExperimentalCodecOptions(decoder, opts);

    ret = avcodec_open2(context.get(), decoder, opts);

    if (ret < 0)
        return QStringLiteral("Failed to open FFmpeg codec context: %1").arg(err2str(ret));

    return Codec(new Data(std::move(context), stream, formatContext, std::move(hwAccel)));
}

QT_END_NAMESPACE

} // namespace QFFmpeg
