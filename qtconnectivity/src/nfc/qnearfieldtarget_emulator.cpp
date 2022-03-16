/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNfc module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnearfieldtarget_emulator_p.h"
#include "qnearfieldtarget_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDirIterator>
#include <QtCore/QMutex>
#include <QtCore/QSettings>

QT_BEGIN_NAMESPACE

static QMutex tagMutex;
static QMap<TagBase *, bool> tagMap;

Q_GLOBAL_STATIC(TagActivator, globalTagActivator);

TagType1::TagType1(TagBase *tag, QObject *parent)
:   QNearFieldTagType1(parent), m_tag(tag)
{
}

TagType1::~TagType1()
{
}

QByteArray TagType1::uid() const
{
    QMutexLocker locker(&tagMutex);

    return m_tag->uid();
}

QNearFieldTarget::AccessMethods TagType1::accessMethods() const
{
    return NdefAccess | TagTypeSpecificAccess;
}

QNearFieldTarget::RequestId TagType1::sendCommand(const QByteArray &command)
{
    QMutexLocker locker(&tagMutex);

    RequestId id(new RequestIdPrivate);

    // tag not in proximity
    if (!tagMap.value(m_tag)) {
        reportError(QNearFieldTarget::TargetOutOfRangeError, id);
        return id;
    }

    quint16 crc = qChecksum(command.constData(), command.length(), Qt::ChecksumItuV41);

    QByteArray response = m_tag->processCommand(command + char(crc & 0xff) + char(crc >> 8));

    if (response.isEmpty()) {
        reportError(QNearFieldTarget::NoResponseError, id);
        return id;
    }

    // check crc
    if (qChecksum(response.constData(), response.length(), Qt::ChecksumItuV41) != 0) {
        reportError(QNearFieldTarget::ChecksumMismatchError, id);
        return id;
    }

    response.chop(2);

    QMetaObject::invokeMethod(this, "handleResponse", Qt::QueuedConnection,
                              Q_ARG(QNearFieldTarget::RequestId, id), Q_ARG(QByteArray, response));

    return id;
}

bool TagType1::waitForRequestCompleted(const RequestId &id, int msecs)
{
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);

    return QNearFieldTagType1::waitForRequestCompleted(id, msecs);
}


TagType2::TagType2(TagBase *tag, QObject *parent)
:   QNearFieldTagType2(parent), m_tag(tag)
{
}

TagType2::~TagType2()
{
}

QByteArray TagType2::uid() const
{
    QMutexLocker locker(&tagMutex);

    return m_tag->uid();
}

QNearFieldTarget::AccessMethods TagType2::accessMethods() const
{
    return NdefAccess | TagTypeSpecificAccess;
}

QNearFieldTarget::RequestId TagType2::sendCommand(const QByteArray &command)
{
    QMutexLocker locker(&tagMutex);

    RequestId id(new RequestIdPrivate);

    // tag not in proximity
    if (!tagMap.value(m_tag)) {
        reportError(QNearFieldTarget::TargetOutOfRangeError, id);
        return id;
    }

    quint16 crc = qChecksum(command.constData(), command.length(), Qt::ChecksumItuV41);

    QByteArray response = m_tag->processCommand(command + char(crc & 0xff) + char(crc >> 8));

    if (response.isEmpty())
        return id;

    if (response.length() > 1) {
        // check crc
        if (qChecksum(response.constData(), response.length(), Qt::ChecksumItuV41) != 0) {
            reportError(QNearFieldTarget::ChecksumMismatchError, id);
            return id;
        }

        response.chop(2);
    }

    QMetaObject::invokeMethod(this, "handleResponse", Qt::QueuedConnection,
                              Q_ARG(QNearFieldTarget::RequestId, id), Q_ARG(QByteArray, response));

    return id;
}

bool TagType2::waitForRequestCompleted(const RequestId &id, int msecs)
{
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);

    return QNearFieldTagType2::waitForRequestCompleted(id, msecs);
}


TagActivator::TagActivator()
:   timerId(-1)
{
    qRegisterMetaType<QNearFieldTarget::Error>();
}

TagActivator::~TagActivator()
{
    QMutexLocker locker(&tagMutex);
    qDeleteAll(tagMap.keys());
    tagMap.clear();
}

void TagActivator::initialize()
{
    QMutexLocker locker(&tagMutex);

    if (!tagMap.isEmpty())
        return;

#ifndef BUILTIN_TESTDATA
    QDirIterator nfcTargets(QDir::currentPath(), QStringList(QStringLiteral("*.nfc")), QDir::Files);
#else
    QDirIterator nfcTargets(":/nfcdata", QStringList(QStringLiteral("*.nfc")), QDir::Files);
#endif
    while (nfcTargets.hasNext()) {
        const QString targetFilename = nfcTargets.next();

        QSettings target(targetFilename, QSettings::IniFormat);

        target.beginGroup(QStringLiteral("Target"));

        const QString tagType = target.value(QStringLiteral("Type")).toString();

        target.endGroup();

        if (tagType == QLatin1String("TagType1")) {
            NfcTagType1 *tag = new NfcTagType1;
            tag->load(&target);

            tagMap.insert(tag, false);
        } else if (tagType == QLatin1String("TagType2")) {
            NfcTagType2 *tag = new NfcTagType2;
            tag->load(&target);

            tagMap.insert(tag, false);
        } else {
            qWarning("Unknown tag type %s\n", qPrintable(tagType));
        }
    }

    m_current = tagMap.end();

    timerId = startTimer(1000);
}

void TagActivator::reset()
{
    QMutexLocker locker(&tagMutex);

    killTimer(timerId);
    timerId = -1;

    qDeleteAll(tagMap.keys());
    tagMap.clear();
}

TagActivator *TagActivator::instance()
{
    return globalTagActivator();
}

void TagActivator::timerEvent(QTimerEvent *e)
{
    Q_UNUSED(e);

    tagMutex.lock();

    if (m_current != tagMap.end()) {
        if (m_current.key()->lastAccessTime() + 1500 > QDateTime::currentMSecsSinceEpoch()) {
            tagMutex.unlock();
            return;
        }

        *m_current = false;

        TagBase *tag = m_current.key();

        tagMutex.unlock();
        emit tagDeactivated(tag);
        tagMutex.lock();
    }

    if (m_current != tagMap.end())
        ++m_current;

    if (m_current == tagMap.end())
        m_current = tagMap.begin();

    if (m_current != tagMap.end()) {
        *m_current = true;

        TagBase *tag = m_current.key();

        tagMutex.unlock();
        emit tagActivated(tag);
        tagMutex.lock();
    }

    tagMutex.unlock();
}

QT_END_NAMESPACE
