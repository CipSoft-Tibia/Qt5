// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qphysicsworld_p.h"
#include "qphysicscommands_p.h"
#include "qphysicsutils_p.h"
#include "qdynamicrigidbody_p.h"
#include "PxPhysicsAPI.h"

QT_BEGIN_NAMESPACE

static bool isKinematicBody(physx::PxRigidBody &body)
{
    return static_cast<bool>(body.getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC);
}

QPhysicsCommandApplyCentralForce::QPhysicsCommandApplyCentralForce(const QVector3D &inForce)
    : QPhysicsCommand(), force(inForce)
{
}

void QPhysicsCommandApplyCentralForce::execute(const QDynamicRigidBody &rigidBody,
                                               physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    if (isKinematicBody(body))
        return;
    body.addForce(QPhysicsUtils::toPhysXType(force));
}

QPhysicsCommandApplyForce::QPhysicsCommandApplyForce(const QVector3D &inForce,
                                                     const QVector3D &inPosition)
    : QPhysicsCommand(), force(inForce), position(inPosition)
{
}

void QPhysicsCommandApplyForce::execute(const QDynamicRigidBody &rigidBody,
                                        physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    if (isKinematicBody(body))
        return;
    physx::PxRigidBodyExt::addForceAtPos(body, QPhysicsUtils::toPhysXType(force),
                                         QPhysicsUtils::toPhysXType(position));
}

QPhysicsCommandApplyTorque::QPhysicsCommandApplyTorque(const QVector3D &inTorque)
    : QPhysicsCommand(), torque(inTorque)
{
}

void QPhysicsCommandApplyTorque::execute(const QDynamicRigidBody &rigidBody,
                                         physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    if (isKinematicBody(body))
        return;
    body.addTorque(QPhysicsUtils::toPhysXType(torque));
}

QPhysicsCommandApplyCentralImpulse::QPhysicsCommandApplyCentralImpulse(const QVector3D &inImpulse)
    : QPhysicsCommand(), impulse(inImpulse)
{
}

void QPhysicsCommandApplyCentralImpulse::execute(const QDynamicRigidBody &rigidBody,
                                                 physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    if (isKinematicBody(body))
        return;
    body.addForce(QPhysicsUtils::toPhysXType(impulse), physx::PxForceMode::eIMPULSE);
}

QPhysicsCommandApplyImpulse::QPhysicsCommandApplyImpulse(const QVector3D &inImpulse,
                                                         const QVector3D &inPosition)
    : QPhysicsCommand(), impulse(inImpulse), position(inPosition)
{
}

void QPhysicsCommandApplyImpulse::execute(const QDynamicRigidBody &rigidBody,
                                          physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    if (isKinematicBody(body))
        return;
    physx::PxRigidBodyExt::addForceAtPos(body, QPhysicsUtils::toPhysXType(impulse),
                                         QPhysicsUtils::toPhysXType(position),
                                         physx::PxForceMode::eIMPULSE);
}

QPhysicsCommandApplyTorqueImpulse::QPhysicsCommandApplyTorqueImpulse(const QVector3D &inImpulse)
    : QPhysicsCommand(), impulse(inImpulse)
{
}

void QPhysicsCommandApplyTorqueImpulse::execute(const QDynamicRigidBody &rigidBody,
                                                physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    if (isKinematicBody(body))
        return;

    body.addTorque(QPhysicsUtils::toPhysXType(impulse), physx::PxForceMode::eIMPULSE);
}

QPhysicsCommandSetAngularVelocity::QPhysicsCommandSetAngularVelocity(
        const QVector3D &inAngularVelocity)
    : QPhysicsCommand(), angularVelocity(inAngularVelocity)
{
}

void QPhysicsCommandSetAngularVelocity::execute(const QDynamicRigidBody &rigidBody,
                                                physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    body.setAngularVelocity(QPhysicsUtils::toPhysXType(angularVelocity));
}

QPhysicsCommandSetLinearVelocity::QPhysicsCommandSetLinearVelocity(
        const QVector3D &inLinearVelocity)
    : QPhysicsCommand(), linearVelocity(inLinearVelocity)
{
}

void QPhysicsCommandSetLinearVelocity::execute(const QDynamicRigidBody &rigidBody,
                                               physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    body.setLinearVelocity(QPhysicsUtils::toPhysXType(linearVelocity));
}

QPhysicsCommandSetMass::QPhysicsCommandSetMass(float inMass) : QPhysicsCommand(), mass(inMass) { }

void QPhysicsCommandSetMass::execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body)
{
    if (rigidBody.hasStaticShapes()) {
        qWarning() << "Cannot set mass or density on a body containing trimesh/heightfield/plane, "
                      "ignoring.";
        return;
    }

    physx::PxRigidBodyExt::setMassAndUpdateInertia(body, mass);
}

void QPhysicsCommandSetMassAndInertiaTensor::execute(const QDynamicRigidBody &rigidBody,
                                                     physx::PxRigidBody &body)
{
    if (rigidBody.hasStaticShapes()) {
        qWarning() << "Cannot set mass or density on a body containing trimesh/heightfield/plane, "
                      "ignoring.";
        return;
    }

    body.setMass(mass);
    body.setCMassLocalPose(
            physx::PxTransform(QPhysicsUtils::toPhysXType(rigidBody.centerOfMassPosition()),
                               QPhysicsUtils::toPhysXType(rigidBody.centerOfMassRotation())));
    body.setMassSpaceInertiaTensor(QPhysicsUtils::toPhysXType(inertia));
}

QPhysicsCommandSetMassAndInertiaMatrix::QPhysicsCommandSetMassAndInertiaMatrix(
        float inMass, const QMatrix3x3 &inInertia)
    : QPhysicsCommand(), mass(inMass), inertia(inInertia)
{
}

void QPhysicsCommandSetMassAndInertiaMatrix::execute(const QDynamicRigidBody &rigidBody,
                                                     physx::PxRigidBody &body)
{
    if (rigidBody.hasStaticShapes()) {
        qWarning() << "Cannot set mass or density on a body containing trimesh/heightfield/plane, "
                      "ignoring.";
        return;
    }

    physx::PxQuat massFrame;
    physx::PxVec3 diagTensor = physx::PxDiagonalize(QPhysicsUtils::toPhysXType(inertia), massFrame);
    if ((diagTensor.x <= 0.0f) || (diagTensor.y <= 0.0f) || (diagTensor.z <= 0.0f))
        return; // FIXME: print error?

    body.setCMassLocalPose(physx::PxTransform(
            QPhysicsUtils::toPhysXType(rigidBody.centerOfMassPosition()), massFrame));
    body.setMass(mass);
    body.setMassSpaceInertiaTensor(diagTensor);
}

QPhysicsCommandSetDensity::QPhysicsCommandSetDensity(float inDensity)
    : QPhysicsCommand(), density(inDensity)
{
}

void QPhysicsCommandSetDensity::execute(const QDynamicRigidBody &rigidBody,
                                        physx::PxRigidBody &body)
{
    if (rigidBody.hasStaticShapes()) {
        qWarning() << "Cannot set mass or density on a body containing trimesh/heightfield/plane, "
                      "ignoring.";
        return;
    }

    const float clampedDensity = qMax(0.0000001, density);
    if (clampedDensity != density) {
        qWarning() << "Clamping density " << density;
        return;
    }

    physx::PxRigidBodyExt::updateMassAndInertia(body, clampedDensity);
}

QPhysicsCommandSetIsKinematic::QPhysicsCommandSetIsKinematic(bool inIsKinematic)
    : QPhysicsCommand(), isKinematic(inIsKinematic)
{
}

void QPhysicsCommandSetIsKinematic::execute(const QDynamicRigidBody &rigidBody,
                                            physx::PxRigidBody &body)
{
    if (rigidBody.hasStaticShapes() && !isKinematic) {
        qWarning() << "Cannot make a body containing trimesh/heightfield/plane non-kinematic, "
                      "ignoring.";
        return;
    }

    body.setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, isKinematic);
}

QPhysicsCommandSetGravityEnabled::QPhysicsCommandSetGravityEnabled(bool inGravityEnabled)
    : QPhysicsCommand(), gravityEnabled(inGravityEnabled)
{
}

void QPhysicsCommandSetGravityEnabled::execute(const QDynamicRigidBody &rigidBody,
                                               physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    body.setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !gravityEnabled);
}

QPhysicsCommandReset::QPhysicsCommandReset(QVector3D inPosition, QVector3D inEulerRotation)
    : QPhysicsCommand(), position(inPosition), eulerRotation(inEulerRotation)
{
}

void QPhysicsCommandReset::execute(const QDynamicRigidBody &rigidBody, physx::PxRigidBody &body)
{
    Q_UNUSED(rigidBody)
    body.setLinearVelocity(physx::PxVec3(0, 0, 0));
    body.setAngularVelocity(physx::PxVec3(0, 0, 0));

    auto *parentNode = rigidBody.parentNode();
    QVector3D scenePosition = parentNode ? parentNode->mapPositionToScene(position) : position;
    // TODO: rotation also needs to be mapped

    body.setGlobalPose(physx::PxTransform(
            QPhysicsUtils::toPhysXType(scenePosition),
            QPhysicsUtils::toPhysXType(QQuaternion::fromEulerAngles(eulerRotation))));
}

QPhysicsCommandSetMassAndInertiaTensor::QPhysicsCommandSetMassAndInertiaTensor(
        float inMass, const QVector3D &inInertia)
    : QPhysicsCommand(), mass(inMass), inertia(inInertia)
{
}

QT_END_NAMESPACE
