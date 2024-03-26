// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTPHYSICSBODY_H
#define QABSTRACTPHYSICSBODY_H

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

#include "qabstractphysicsnode_p.h"
#include <QtQuick3DPhysics/private/qphysicsmaterial_p.h>
#include <QtQml/QQmlEngine>

QT_BEGIN_NAMESPACE

class QAbstractPhysicsBody : public QAbstractPhysicsNode
{
    Q_OBJECT
    Q_PROPERTY(QPhysicsMaterial *physicsMaterial READ physicsMaterial WRITE setPhysicsMaterial
                       NOTIFY physicsMaterialChanged)
    QML_NAMED_ELEMENT(PhysicsBody)
    QML_UNCREATABLE("abstract interface")

public:
    QAbstractPhysicsBody();
    QPhysicsMaterial *physicsMaterial() const;
    void setPhysicsMaterial(QPhysicsMaterial *newPhysicsMaterial);
Q_SIGNALS:
    void physicsMaterialChanged();

private:
    QPhysicsMaterial *m_physicsMaterial = nullptr;
};

QT_END_NAMESPACE

#endif // QABSTRACTPHYSICSBODY_H
