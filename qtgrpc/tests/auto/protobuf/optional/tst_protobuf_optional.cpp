// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QObject>

class QtProtobufOptionalTest : public QObject
{
    Q_OBJECT
};

QTEST_MAIN(QtProtobufOptionalTest)

#include "tst_protobuf_optional.moc"
