// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

class UnseenClass
{
public:
    UnseenClass();
};

class SeenClass : public UnseenClass
{
public:
    SeenClass();
};
