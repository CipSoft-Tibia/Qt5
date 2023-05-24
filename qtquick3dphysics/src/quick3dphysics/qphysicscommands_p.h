// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QPHYSICSCOMMANDS_H
#define QPHYSICSCOMMANDS_H

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

#include <QtQuick3DPhysics/qtquick3dphysicsglobal.h>

#include <QtCore/QList>
#include <QtGui/QVector3D>
#include <QQuaternion>

namespace physx {
class PxRigidBody;
}

QT_BEGIN_NAMESPACE

class QDynamicRigidBody;

class QPhysicsCommand
{
public:
    virtual ~QPhysicsCommand() = default;
    virtual void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) = 0;
};

class QPhysicsCommandApplyCentralForce : public QPhysicsCommand
{
public:
    QPhysicsCommandApplyCentralForce(const QVector3D &inForce);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D force;
};

class QPhysicsCommandApplyForce : public QPhysicsCommand
{
public:
    QPhysicsCommandApplyForce(const QVector3D &inForce, const QVector3D &inPosition);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D force;
    QVector3D position;
};

class QPhysicsCommandApplyTorque : public QPhysicsCommand
{
public:
    QPhysicsCommandApplyTorque(const QVector3D &inTorque);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D torque;
};

class QPhysicsCommandApplyCentralImpulse : public QPhysicsCommand
{
public:
    QPhysicsCommandApplyCentralImpulse(const QVector3D &inImpulse);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D impulse;
};

class QPhysicsCommandApplyImpulse : public QPhysicsCommand
{
public:
    QPhysicsCommandApplyImpulse(const QVector3D &inImpulse, const QVector3D &inPosition);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D impulse;
    QVector3D position;
};

class QPhysicsCommandApplyTorqueImpulse : public QPhysicsCommand
{
public:
    QPhysicsCommandApplyTorqueImpulse(const QVector3D &inImpulse);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D impulse;
};

class QPhysicsCommandSetAngularVelocity : public QPhysicsCommand
{
public:
    QPhysicsCommandSetAngularVelocity(const QVector3D &inAngularVelocity);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D angularVelocity;
};

class QPhysicsCommandSetLinearVelocity : public QPhysicsCommand
{
public:
    QPhysicsCommandSetLinearVelocity(const QVector3D &inLinearVelocity);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D linearVelocity;
};

class QPhysicsCommandSetMass : public QPhysicsCommand
{
public:
    QPhysicsCommandSetMass(float inMass);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    float mass;
};

class QPhysicsCommandSetMassAndInertiaTensor : public QPhysicsCommand
{
public:
    QPhysicsCommandSetMassAndInertiaTensor(float inMass, const QVector3D &inInertia);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    float mass;
    QVector3D inertia;
};

class QPhysicsCommandSetMassAndInertiaMatrix : public QPhysicsCommand
{
public:
    QPhysicsCommandSetMassAndInertiaMatrix(float inMass, const QMatrix3x3 &inInertia);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    float mass;
    QMatrix3x3 inertia;
};

class QPhysicsCommandSetDensity : public QPhysicsCommand
{
public:
    QPhysicsCommandSetDensity(float inDensity);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    float density;
};

class QPhysicsCommandSetIsKinematic : public QPhysicsCommand
{
public:
    QPhysicsCommandSetIsKinematic(bool inIsKinematic);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    bool isKinematic;
};

class QPhysicsCommandSetGravityEnabled : public QPhysicsCommand
{
public:
    QPhysicsCommandSetGravityEnabled(bool inGravityEnabled);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    bool gravityEnabled;
};

class QPhysicsCommandReset : public QPhysicsCommand
{
public:
    QPhysicsCommandReset(QVector3D inPosition, QVector3D inEulerRotation);
    void execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body) override;

private:
    QVector3D position;
    QVector3D eulerRotation;
};

QT_END_NAMESPACE

#endif // QPHYSICSCOMMANDS_H
