// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only



#ifndef QVOICE_P_H
#define QVOICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qvoice.h>

#include <QString>
#include <QLocale>
#include <QVariant>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QVoicePrivate : public QSharedData
{
public:
    QVoicePrivate() = default;
    QVoicePrivate(const QVoicePrivate &other);
    QVoicePrivate(const QString &n, const QLocale &l, QVoice::Gender g,
                  QVoice::Age a, const QVariant &d);
    ~QVoicePrivate() = default;

    QString name;
    QLocale locale;
    QVoice::Gender gender = QVoice::Unknown;
    QVoice::Age age = QVoice::Other;
    // Various data depending on the platform:
    // On OS X the VoiceIdentifier is stored.
    // On unix the synthesizer (output module) is stored.
    QVariant data;
};

QVoicePrivate::QVoicePrivate(const QVoicePrivate &other)
    : QSharedData(other), name(other.name), locale(other.locale)
    , gender(other.gender), age(other.age), data(other.data)
{
}

QVoicePrivate::QVoicePrivate(const QString &n, const QLocale &l, QVoice::Gender g,
                             QVoice::Age a, const QVariant &d)
    :name(n), locale(l), gender(g), age(a), data(d)
{
}

QT_END_NAMESPACE

#endif
