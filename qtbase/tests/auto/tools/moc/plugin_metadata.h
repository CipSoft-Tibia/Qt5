// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TESTPLUGINMETADATA
#define TESTPLUGINMETADATA

#include <QtPlugin>

class TestPluginMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "test.meta.tags")
};

#endif
