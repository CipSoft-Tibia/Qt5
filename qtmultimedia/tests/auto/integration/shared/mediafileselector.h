// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MEDIAFILESELECTOR_H
#define MEDIAFILESELECTOR_H

#include <QUrl>
#include <qmediaplayer.h>
#include <qaudiooutput.h>
#include <qvideosink.h>
#include <qsignalspy.h>
#include <qfileinfo.h>
#include <qtest.h>
#include <private/qmultimediautils_p.h>

#include <unordered_map>

QT_BEGIN_NAMESPACE

using MaybeUrl = QMaybe<QUrl, QString>;

#define CHECK_SELECTED_URL(maybeUrl)                                                             \
  if (!maybeUrl)                                                                                 \
  QSKIP((QLatin1String("\nUnable to select none of the media candidates:\n") + maybeUrl.error()) \
                .toLocal8Bit()                                                                   \
                .data())

class MediaFileSelector
{
public:
    quint32 failedSelectionsCount() const { return m_failedSelectionsCount; }

    QString dumpErrors() const
    {
        QStringList failedMedias;
        for (const auto &mediaToError : m_mediaToErrors)
            if (!mediaToError.second.isEmpty())
                failedMedias.emplace_back(mediaToError.first);

        failedMedias.sort();
        return dumpErrors(failedMedias);
    }

    template <typename... Media>
    MaybeUrl select(Media... media)
    {
        return select({ std::move(nativeFileName(media))... });
    }

    MaybeUrl select(const QStringList &candidates)
    {
        QUrl foundUrl;
        for (const auto &media : candidates) {
            auto emplaceRes = m_mediaToErrors.try_emplace(media, QString());
            if (emplaceRes.second) {
                auto maybeUrl = selectMediaFile(media);
                if (!maybeUrl) {
                    Q_ASSERT(!maybeUrl.error().isEmpty());
                    emplaceRes.first->second = maybeUrl.error();
                }
            }

            if (foundUrl.isEmpty() && emplaceRes.first->second.isEmpty())
                foundUrl = media;
        }

        if (!foundUrl.isEmpty())
            return foundUrl;

        ++m_failedSelectionsCount;
        return { QUnexpect{}, dumpErrors(candidates) };
    }

private:
    QString dumpErrors(const QStringList &medias) const
    {
        using namespace Qt::StringLiterals;
        QString result;

        for (const auto &media : medias) {
            auto it = m_mediaToErrors.find(media);
            if (it != m_mediaToErrors.end() && !it->second.isEmpty())
                result.append("\t"_L1)
                        .append(it->first)
                        .append(": "_L1)
                        .append(it->second)
                        .append("\n"_L1);
        }

        return result;
    }

    static MaybeUrl selectMediaFile(QString media)
    {
        using namespace Qt::StringLiterals;

        QAudioOutput audioOutput;
        QVideoSink videoOutput;
        QMediaPlayer player;
        player.setAudioOutput(&audioOutput);
        player.setVideoOutput(&videoOutput);

        player.setSource(media);
        player.play();

        const auto waitingFinished = QTest::qWaitFor([&]() {
            const auto status = player.mediaStatus();
            return status == QMediaPlayer::BufferedMedia || status == QMediaPlayer::EndOfMedia
                    || status == QMediaPlayer::InvalidMedia
                    || player.error() != QMediaPlayer::NoError;
        });

        auto enumValueToString = [](auto enumValue) {
            return QString(QMetaEnum::fromType<decltype(enumValue)>().valueToKey(enumValue));
        };

        if (!waitingFinished)
            return { QUnexpect{},
                     "The media got stuck in the status "_L1
                             + enumValueToString(player.mediaStatus()) };

        if (player.mediaStatus() == QMediaPlayer::InvalidMedia)
            return { QUnexpect{},
                     "Unable to load the media. Error ["_L1 + enumValueToString(player.error())
                             + " "_L1 + player.errorString() + "]"_L1 };

        if (player.error() != QMediaPlayer::NoError)
            return { QUnexpect{},
                     "Unable to start playing the media, codecs issues. Error ["_L1
                             + enumValueToString(player.error()) + " "_L1 + player.errorString()
                             + "]"_L1 };

        return QUrl(media);
    }

    QString nativeFileName(const QString &media)
    {
#ifdef Q_OS_ANDROID
        auto it = m_nativeFiles.find(media);
        if (it != m_nativeFiles.end())
            return it->second->fileName();

        QFile file(media);
        if (file.open(QIODevice::ReadOnly)) {
            m_nativeFiles.insert({ media,  std::unique_ptr<QTemporaryFile>(QTemporaryFile::createNativeFile(file))});
            return m_nativeFiles[media]->fileName();
        }
        qWarning() << "Failed to create temporary file";
#endif // Q_OS_ANDROID

        return media;
    }

private:
#ifdef Q_OS_ANDROID
    std::unordered_map<QString, std::unique_ptr<QTemporaryFile>> m_nativeFiles;
#endif
    std::unordered_map<QString, QString> m_mediaToErrors;
    quint32 m_failedSelectionsCount = 0;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(MaybeUrl)

#endif

