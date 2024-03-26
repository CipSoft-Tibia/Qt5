// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TEST_SENSOR_P_H
#define TEST_SENSOR_P_H

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

class TestSensorReadingPrivate
{
public:
    TestSensorReadingPrivate()
        : test(0)
    {
    }

    int test;
};

#endif
