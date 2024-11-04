// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_gstreamer_backend.h"

#include <QtTest/QtTest>
#include <QtMultimedia/qmediaformat.h>

#include <QtQGstreamerMediaPluginImpl/private/qgst_handle_types_p.h>
#include <QtQGstreamerMediaPluginImpl/private/qgst_p.h>
#include <QtQGstreamerMediaPluginImpl/private/qgst_debug_p.h>
#include <QtQGstreamerMediaPluginImpl/private/qgstpipeline_p.h>
#include <QtQGstreamerMediaPluginImpl/private/qgstreamermetadata_p.h>

#include <set>

QT_USE_NAMESPACE

// NOLINTBEGIN(readability-convert-member-functions-to-static)

using namespace Qt::Literals;

namespace {

template <typename... Pairs>
QMediaMetaData makeQMediaMetaData(Pairs &&...pairs)
{
    QMediaMetaData metadata;

    auto addKeyValuePair = [&](auto &&pair) {
        metadata.insert(pair.first, pair.second);
        return;
    };

    (addKeyValuePair(pairs), ...);

    return metadata;
}

QGString makeQGString(std::string_view str)
{
    char *s = (char *)g_malloc(str.size() + 1);
    snprintf(s, str.size() + 1, "%s", str.data());
    return QGString{ s };
};

} // namespace

QGstTagListHandle tst_GStreamer::parseTagList(const char *str)
{
    QGstTagListHandle tagList{
        gst_tag_list_new_from_string(str),
        QGstTagListHandle::NeedsRef,
    };
    return tagList;
}

QGstTagListHandle tst_GStreamer::parseTagList(const QByteArray &ba)
{
    return parseTagList(ba.constData());
}

void tst_GStreamer::QGString_conversions()
{
    QGString str = makeQGString("yada");

    QCOMPARE(str.toQString(), u"yada"_s);
    QCOMPARE(str.asStringView(), "yada"_L1);
    QCOMPARE(str.asByteArrayView(), "yada"_ba);
}

void tst_GStreamer::QGString_transparentCompare()
{
    QGString str = makeQGString("yada");

    std::set<QByteArray, std::less<>> set;
    set.emplace(str);

    QVERIFY(set.find(str) != set.end());
}

void tst_GStreamer::qGstCasts_withElement()
{
    QGstElement element = QGstElement::createFromFactory("identity", "myPipeline");
    QVERIFY(element);

    QVERIFY(!qIsGstObjectOfType<GstPipeline>(element.element()));
    QVERIFY(!qIsGstObjectOfType<GstBin>(element.element()));
}

void tst_GStreamer::qGstCasts_withBin()
{
    QGstBin bin = QGstBin::create("bin");
    QVERIFY(bin);

    QVERIFY(!qIsGstObjectOfType<GstPipeline>(bin.element()));
    QVERIFY(qIsGstObjectOfType<GstBin>(bin.element()));
}

void tst_GStreamer::qGstCasts_withPipeline()
{
    QGstPipeline pipeline = QGstPipeline::create("myPipeline");

    QGstElement element{
        qGstSafeCast<GstElement>(pipeline.pipeline()),
        QGstElement::NeedsRef,
    };

    QVERIFY(element);
    QVERIFY(qIsGstObjectOfType<GstPipeline>(element.element()));
    QVERIFY(qIsGstObjectOfType<GstBin>(element.element()));
}

void tst_GStreamer::metadata_taglistToMetaData()
{
    QGstTagListHandle tagList = parseTagList(R"(taglist, title="My Video", comment="yada")");

    QMediaMetaData parsed = taglistToMetaData(tagList);

    QCOMPARE(parsed.stringValue(QMediaMetaData::Title), u"My Video"_s);
    QCOMPARE(parsed.stringValue(QMediaMetaData::Comment), u"yada"_s);
}

void tst_GStreamer::metadata_taglistToMetaData_extractsOrientation()
{
    QFETCH(QByteArray, taglist);
    QFETCH(QtVideo::Rotation, rotation);

    QGstTagListHandle tagList = parseTagList(taglist);
    QMediaMetaData parsed = taglistToMetaData(tagList);
    QCOMPARE(parsed[QMediaMetaData::Orientation].value<QtVideo::Rotation>(), rotation);
}

void tst_GStreamer::metadata_taglistToMetaData_extractsOrientation_data()
{
    QTest::addColumn<QByteArray>("taglist");
    QTest::addColumn<QtVideo::Rotation>("rotation");

    QTest::newRow("no rotation") << R"(taglist, title="My Video", comment="yada")"_ba
                                 << QtVideo::Rotation::None;
    QTest::newRow("90 degree")
            << R"(taglist, title="My Video", comment="yada", image-orientation=(string)rotate-90)"_ba
            << QtVideo::Rotation::Clockwise90;
    QTest::newRow("180 degree")
            << R"(taglist, title="My Video", comment="yada", image-orientation=(string)rotate-180)"_ba
            << QtVideo::Rotation::Clockwise180;
    QTest::newRow("270 degree")
            << R"(taglist, title="My Video", comment="yada", image-orientation=(string)rotate-270)"_ba
            << QtVideo::Rotation::Clockwise270;
}

void tst_GStreamer::metadata_taglistToMetaData_extractsDuration()
{
    QGstTagListHandle tagList = parseTagList(
            R"__(taglist, video-codec=(string)"On2\ VP9",  container-specific-track-id=(string)1, extended-comment=(string){ "ALPHA_MODE\=1", "HANDLER_NAME\=Apple\ Video\ Media\ Handler", "VENDOR_ID\=appl", "TIMECODE\=00:00:00:00", "DURATION\=00:00:00.400000000" }, encoder=(string)"Lavc59.37.100\ libvpx-vp9")__");

    QMediaMetaData parsed = taglistToMetaData(tagList);
    QCOMPARE(parsed[QMediaMetaData::Duration].value<int>(), 400);
}

void tst_GStreamer::metadata_taglistToMetaData_extractsLanguage()
{
    QFETCH(QByteArray, tagListString);
    QFETCH(QLocale::Language, language);

    QGstTagListHandle tagList = parseTagList(tagListString);
    QVERIFY(tagList);

    QMediaMetaData parsed = taglistToMetaData(tagList);
    QCOMPARE(parsed[QMediaMetaData::Language].value<QLocale::Language>(), language);
}

void tst_GStreamer::metadata_taglistToMetaData_extractsLanguage_data()
{
    QTest::addColumn<QByteArray>("tagListString");
    QTest::addColumn<QLocale::Language>("language");

    QTest::newRow("english, en")
            << R"__(taglist, container-format=(string)Matroska, audio-codec=(string)"MPEG-4\ AAC", language-code=(string)en, container-specific-track-id=(string)5, encoder=(string)Lavf60.16.100, extended-comment=(string)"DURATION\=00:00:05.055000000")__"_ba
            << QLocale::Language::English;
    QTest::newRow("spanish, es")
            << R"__(taglist, container-format=(string)Matroska, audio-codec=(string)"MPEG-4\ AAC", language-code=(string)es, container-specific-track-id=(string)5, encoder=(string)Lavf60.16.100, extended-comment=(string)"DURATION\=00:00:05.055000000")__"_ba
            << QLocale::Language::Spanish;
    QTest::newRow("english, eng")
            << R"__(taglist, container-format=(string)Matroska, audio-codec=(string)"MPEG-4\ AAC", language-code=(string)eng, container-specific-track-id=(string)5, encoder=(string)Lavf60.16.100, extended-comment=(string)"DURATION\=00:00:05.055000000")__"_ba
            << QLocale::Language::English;
    QTest::newRow("spanish, spa")
            << R"__(taglist, container-format=(string)Matroska, audio-codec=(string)"MPEG-4\ AAC", language-code=(string)spa, container-specific-track-id=(string)5, encoder=(string)Lavf60.16.100, extended-comment=(string)"DURATION\=00:00:05.055000000")__"_ba
            << QLocale::Language::Spanish;
}

void tst_GStreamer::metadata_capsToMetaData()
{
    QFETCH(QByteArray, capsString);
    QFETCH(QMediaMetaData, expectedMetadata);

    QGstCaps caps{
        gst_caps_from_string(capsString.constData()),
        QGstCaps::HasRef,
    };

    QMediaMetaData md = capsToMetaData(caps);

    QCOMPARE(md, expectedMetadata);
}

void tst_GStreamer::metadata_capsToMetaData_data()
{
    using Key = QMediaMetaData::Key;
    using KVPair = std::pair<QMediaMetaData::Key, QVariant>;

    auto makeKVPair = [](Key key, auto value) {
        return KVPair{
            key,
            QVariant::fromValue(value),
        };
    };

    QTest::addColumn<QByteArray>("capsString");
    QTest::addColumn<QMediaMetaData>("expectedMetadata");

    QTest::newRow("container") << R"(video/quicktime, variant=(string)iso)"_ba
                               << makeQMediaMetaData(makeKVPair(Key::FileFormat,
                                                                QMediaFormat::FileFormat::MPEG4));

    QTest::newRow("video")
            << R"(video/x-h264, stream-format=(string)avc, alignment=(string)au, level=(string)3.1, profile=(string)main, codec_data=(buffer)014d401fffe10017674d401fda014016ec0440000003004000000c83c60ca801000468ef3c80, width=(int)1280, height=(int)720, framerate=(fraction)25/1, pixel-aspect-ratio=(fraction)1/1)"_ba
            << makeQMediaMetaData(makeKVPair(Key::VideoCodec, QMediaFormat::VideoCodec::H264),
                                  makeKVPair(Key::VideoFrameRate, 25),
                                  makeKVPair(Key::Resolution, QSize(1280, 720)));

    QTest::newRow("audio")
            << R"(audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, level=(string)4, base-profile=(string)lc, profile=(string)lc, codec_data=(buffer)11b0, rate=(int)48000, channels=(int)6)"_ba
            << makeQMediaMetaData(makeKVPair(Key::AudioCodec, QMediaFormat::AudioCodec::AAC));
}

void tst_GStreamer::parseRotationTag_returnsCorrectResults()
{
    QCOMPARE_EQ(parseRotationTag("rotate-0"), (RotationResult{ QtVideo::Rotation::None, false }));
    QCOMPARE_EQ(parseRotationTag("rotate-90"),
                (RotationResult{ QtVideo::Rotation::Clockwise90, false }));
    QCOMPARE_EQ(parseRotationTag("rotate-180"),
                (RotationResult{ QtVideo::Rotation::Clockwise180, false }));
    QCOMPARE_EQ(parseRotationTag("rotate-270"),
                (RotationResult{ QtVideo::Rotation::Clockwise270, false }));

    QCOMPARE_EQ(parseRotationTag("flip-rotate-0"),
                (RotationResult{ QtVideo::Rotation::Clockwise180, true }));
    QCOMPARE_EQ(parseRotationTag("flip-rotate-90"),
                (RotationResult{ QtVideo::Rotation::Clockwise270, true }));
    QCOMPARE_EQ(parseRotationTag("flip-rotate-180"),
                (RotationResult{ QtVideo::Rotation::None, true }));
    QCOMPARE_EQ(parseRotationTag("flip-rotate-270"),
                (RotationResult{ QtVideo::Rotation::Clockwise90, true }));
}

void tst_GStreamer::QGstBin_createFromPipelineDescription()
{
    QGstBin bin = QGstBin::createFromPipelineDescription("identity name=foo ! identity name=bar");

    QVERIFY(bin);
    QVERIFY(bin.findByName("foo"));
    QCOMPARE_EQ(bin.findByName("foo").getParent(), bin);
    QVERIFY(bin.findByName("bar"));
    QVERIFY(!bin.findByName("baz"));
    bin.dumpGraph("QGstBin_createFromPipelineDescription");
}

void tst_GStreamer::QGstElement_createFromPipelineDescription()
{
    using namespace std::string_view_literals;
    QGstElement element = QGstElement::createFromPipelineDescription("identity name=foo");
    QCOMPARE_EQ(element.name().constData(), "foo"sv);
    QCOMPARE_EQ(element.typeName().constData(), "GstIdentity"sv);
}

void tst_GStreamer::QGstElement_createFromPipelineDescription_multipleElementsCreatesBin()
{
    using namespace std::string_view_literals;
    QGstElement element =
            QGstElement::createFromPipelineDescription("identity name=foo ! identity name=bar");

    QVERIFY(element);
    QCOMPARE_EQ(element.typeName().constData(), "GstPipeline"sv);

    QGstBin bin{
        qGstSafeCast<GstBin>(element.element()),
        QGstBin::NeedsRef,
    };

    QVERIFY(bin);
    QVERIFY(bin.findByName("foo"));
    QCOMPARE_EQ(bin.findByName("foo").getParent(), bin);
    QVERIFY(bin.findByName("bar"));
    QVERIFY(!bin.findByName("baz"));

    bin.dumpGraph("QGstElement_createFromPipelineDescription_multipleElements");
}

void tst_GStreamer::QGstPad_inferTypeFromName()
{
    auto makePad = [](const char *name, GstPadDirection direction) {
        return QGstPad{
            gst_pad_new(name, direction),
            QGstPad::NeedsRef,
        };
    };

    QVERIFY(makePad("audio_0", GST_PAD_SRC).inferTrackTypeFromName()
            == QPlatformMediaPlayer::AudioStream);
    QVERIFY(makePad("video_0", GST_PAD_SRC).inferTrackTypeFromName()
            == QPlatformMediaPlayer::VideoStream);
    QVERIFY(makePad("text_0", GST_PAD_SRC).inferTrackTypeFromName()
            == QPlatformMediaPlayer::SubtitleStream);
    QVERIFY(makePad("src_0", GST_PAD_SRC).inferTrackTypeFromName() == std::nullopt);
    QVERIFY(makePad("text", GST_PAD_SRC).inferTrackTypeFromName() == std::nullopt);
}

void tst_GStreamer::qDebug_GstPadDirection()
{
    auto validate = [](GstPadDirection direction, QString expectedString) {
        QString str;
        QDebug dbg(&str);

        dbg << direction;

        QCOMPARE_EQ(str, expectedString);
    };

    validate(GST_PAD_UNKNOWN, u"GST_PAD_UNKNOWN "_s);
    validate(GST_PAD_SRC, u"GST_PAD_SRC "_s);
    validate(GST_PAD_SINK, u"GST_PAD_SINK "_s);
}

void tst_GStreamer::qDebug_GstStreamStatusType()
{
    auto validate = [](GstStreamStatusType type, QString expectedString) {
        QString str;
        QDebug dbg(&str);

        dbg << type;

        QCOMPARE_EQ(str, expectedString);
    };

    validate(GST_STREAM_STATUS_TYPE_CREATE, u"GST_STREAM_STATUS_TYPE_CREATE "_s);
    validate(GST_STREAM_STATUS_TYPE_ENTER, u"GST_STREAM_STATUS_TYPE_ENTER "_s);
    validate(GST_STREAM_STATUS_TYPE_LEAVE, u"GST_STREAM_STATUS_TYPE_LEAVE "_s);
    validate(GST_STREAM_STATUS_TYPE_DESTROY, u"GST_STREAM_STATUS_TYPE_DESTROY "_s);
    validate(GST_STREAM_STATUS_TYPE_START, u"GST_STREAM_STATUS_TYPE_START "_s);
    validate(GST_STREAM_STATUS_TYPE_PAUSE, u"GST_STREAM_STATUS_TYPE_PAUSE "_s);
    validate(GST_STREAM_STATUS_TYPE_STOP, u"GST_STREAM_STATUS_TYPE_STOP "_s);
}

QTEST_GUILESS_MAIN(tst_GStreamer)

#include "moc_tst_gstreamer_backend.cpp"
