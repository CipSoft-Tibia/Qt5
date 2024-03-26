// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef OPCUAOPERANDBASE_P_H
#define OPCUAOPERANDBASE_P_H

#include <QObject>
#include <QVariant>
#include <private/qglobal_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QOpcUaClient;

class OpcUaOperandBase : public QObject {
    Q_OBJECT

public:
    explicit OpcUaOperandBase(QObject *parent = nullptr);
    ~OpcUaOperandBase();

    virtual QVariant toCppVariant(QOpcUaClient *client) const;
};

QT_END_NAMESPACE

#endif // OPCUAOPERANDBASE_P_H
