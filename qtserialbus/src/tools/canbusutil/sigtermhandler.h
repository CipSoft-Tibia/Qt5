// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef SIGTERMHANDLER_H
#define SIGTERMHANDLER_H

#include <QObject>

class SigTermHandler : public QObject
{
    Q_OBJECT
public:
    static SigTermHandler *instance();
    static void handle(int s);

    virtual ~SigTermHandler();
signals:
    void sigTermSignal();

private:
    explicit SigTermHandler(QObject *parent = nullptr);
};

#endif // SIGTERMHANDLER_H
