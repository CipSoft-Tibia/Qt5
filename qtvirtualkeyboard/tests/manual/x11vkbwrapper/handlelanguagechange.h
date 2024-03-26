// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef HANDLELANGUAGECHANGE_H
#define HANDLELANGUAGECHANGE_H

#include <QObject>

class HandleLanguageChange: public QObject
{
    Q_OBJECT

public:
    HandleLanguageChange(QObject *parent = nullptr);

public slots:
    void languageChanged(const QString &language) const;

};

#endif // HANDLELANGUAGECHANGE_H
