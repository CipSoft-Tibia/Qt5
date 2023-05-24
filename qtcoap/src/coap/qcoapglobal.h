// Copyright (C) 2017 Witekio.
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QCOAPGLOBAL_H
#define QCOAPGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCoap/qtcoapexports.h>

QT_BEGIN_NAMESPACE

typedef QByteArray QCoapToken;
typedef quint16 QCoapMessageId;

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCoapToken)
Q_DECLARE_METATYPE(QCoapMessageId)

#endif // QCOAPGLOBAL_H
