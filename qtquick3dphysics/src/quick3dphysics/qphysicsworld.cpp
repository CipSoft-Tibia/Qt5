// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qphysicsworld_p.h"

#include "physxnode/qabstractphysxnode_p.h"
#include "physxnode/qphysxworld_p.h"
#include "qabstractphysicsnode_p.h"
#include "qdebugdrawhelper_p.h"
#include "qphysicsutils_p.h"
#include "qstaticphysxobjects_p.h"
#include "qboxshape_p.h"
#include "qsphereshape_p.h"
#include "qconvexmeshshape_p.h"
#include "qtrianglemeshshape_p.h"
#include "qcharactercontroller_p.h"
#include "qcapsuleshape_p.h"
#include "qplaneshape_p.h"
#include "qheightfieldshape_p.h"

#include "PxPhysicsAPI.h"
#include "cooking/PxCooking.h"

#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtEnvironmentVariables>

#define PHYSX_ENABLE_PVD 0

QT_BEGIN_NAMESPACE

/*!
    \qmltype PhysicsWorld
    \inqmlmodule QtQuick3D.Physics
    \since 6.4
    \brief Controls the physics simulation.

    The PhysicsWorld type controls the physics simulation. This node is used to create an instance of the physics world as well
    as define its properties. There can only be one physics world. All collision nodes in the qml
    will get added automatically to the physics world.
*/

/*!
    \qmlproperty vector3d PhysicsWorld::gravity
    This property defines the gravity vector of the physics world.
    The default value is \c (0, -981, 0). Set the value to \c{Qt.vector3d(0, -9.81, 0)} if your
    unit of measurement is meters and you are simulating Earth gravity.
*/

/*!
    \qmlproperty bool PhysicsWorld::running
    This property starts or stops the physical simulation. The default value is \c true.
*/

/*!
    \qmlproperty bool PhysicsWorld::forceDebugDraw
    This property enables debug drawing of all active shapes in the physics world. The default value
    is \c false.
*/

/*!
    \qmlproperty bool PhysicsWorld::enableCCD
    This property enables continuous collision detection. This will reduce the risk of bodies going
    through other bodies at high velocities (also known as tunnelling). The default value is \c
    false.
*/

/*!
    \qmlproperty float PhysicsWorld::typicalLength
    This property defines the approximate size of objects in the simulation. This is used to
    estimate certain length-related tolerances. Objects much smaller or much larger than this
    size may not behave properly. The default value is \c 100.

    Range: \c{[0, inf]}
*/

/*!
    \qmlproperty float PhysicsWorld::typicalSpeed
    This property defines the typical magnitude of velocities of objects in simulation. This is used
    to estimate whether a contact should be treated as bouncing or resting based on its impact
    velocity, and a kinetic energy threshold below which the simulation may put objects to sleep.

    For normal physical environments, a good choice is the approximate speed of an object falling
    under gravity for one second. The default value is \c 1000.

    Range: \c{[0, inf]}
*/

/*!
    \qmlproperty float PhysicsWorld::defaultDensity
    This property defines the default density of dynamic objects, measured in kilograms per cubic
    unit. This is equal to the weight of a cube with side \c 1.

    The default value is \c 0.001, corresponding to 1 g/cm³: the density of water. If your unit of
    measurement is meters, a good value would be \c 1000. Note that only positive values are
    allowed.

    Range: \c{(0, inf]}
*/

/*!
    \qmlproperty Node PhysicsWorld::viewport
    This property defines the viewport where debug components will be drawn if \l{forceDebugDraw}
    is enabled. If unset the \l{scene} node will be used.

    \sa forceDebugDraw, scene
*/

/*!
    \qmlproperty float PhysicsWorld::minimumTimestep
    This property defines the minimum simulation timestep in milliseconds. The default value is
    \c 16.667 which corresponds to \c 60 frames per second.

    Range: \c{[0, maximumTimestep]}
*/

/*!
    \qmlproperty float PhysicsWorld::maximumTimestep
    This property defines the maximum simulation timestep in milliseconds. The default value is
    \c 33.333 which corresponds to \c 30 frames per second.

    Range: \c{[0, inf]}
*/

/*!
    \qmlproperty Node PhysicsWorld::scene

    This property defines the top-most Node that contains all the nodes of the physical
    simulation. All physics objects that are an ancestor of this node will be seen as part of this
    PhysicsWorld.

    \note Using the same scene node for several PhysicsWorld is unsupported.
*/

/*!
    \qmlsignal PhysicsWorld::frameDone(float timestep)
    \since 6.5

    This signal is emitted when the physical simulation is done simulating a frame. The \a timestep
    parameter is how long in milliseconds the timestep was in the simulation.
*/

Q_LOGGING_CATEGORY(lcQuick3dPhysics, "qt.quick3d.physics");

/////////////////////////////////////////////////////////////////////////////

class SimulationWorker : public QObject
{
    Q_OBJECT
public:
    SimulationWorker(QPhysXWorld *physx) : m_physx(physx) { }

public slots:
    void simulateFrame(float minTimestep, float maxTimestep)
    {
        if (!m_physx->isRunning) {
            m_timer.start();
            m_physx->isRunning = true;
        }

        // Assuming: 0 <= minTimestep <= maxTimestep

        constexpr auto MILLIONTH = 0.000001;

        // If not enough time has elapsed we sleep until it has
        auto deltaMS = m_timer.nsecsElapsed() * MILLIONTH;
        while (deltaMS < minTimestep) {
            auto sleepUSecs = (minTimestep - deltaMS) * 1000.f;
            QThread::usleep(sleepUSecs);
            deltaMS = m_timer.nsecsElapsed() * MILLIONTH;
        }
        m_timer.restart();

        auto deltaSecs = qMin(float(deltaMS), maxTimestep) * 0.001f;
        m_physx->scene->simulate(deltaSecs);
        m_physx->scene->fetchResults(true);

        emit frameDone(deltaSecs);
    }

    void simulateFrameDesignStudio(float minTimestep, float maxTimestep)
    {
        Q_UNUSED(minTimestep);
        Q_UNUSED(maxTimestep);
        auto sleepUSecs = 16 * 1000.f; // 16 ms
        QThread::usleep(sleepUSecs);
        emit frameDoneDesignStudio();
    }

signals:
    void frameDone(float deltaTime);
    void frameDoneDesignStudio();

private:
    QPhysXWorld *m_physx = nullptr;
    QElapsedTimer m_timer;
};

/////////////////////////////////////////////////////////////////////////////

void QPhysicsWorld::DebugModelHolder::releaseMeshPointer()
{
    if (auto base = static_cast<physx::PxBase *>(ptr); base)
        base->release();
    ptr = nullptr;
}

const QVector3D &QPhysicsWorld::DebugModelHolder::halfExtents() const
{
    return data;
}
void QPhysicsWorld::DebugModelHolder::setHalfExtents(const QVector3D &halfExtents)
{
    data = halfExtents;
}
float QPhysicsWorld::DebugModelHolder::radius() const
{
    return data.x();
}
void QPhysicsWorld::DebugModelHolder::setRadius(float radius)
{
    data.setX(radius);
}
float QPhysicsWorld::DebugModelHolder::heightScale() const
{
    return data.x();
}
void QPhysicsWorld::DebugModelHolder::setHeightScale(float heightScale)
{
    data.setX(heightScale);
}
float QPhysicsWorld::DebugModelHolder::halfHeight() const
{
    return data.y();
}
void QPhysicsWorld::DebugModelHolder::setHalfHeight(float halfHeight)
{
    data.setY(halfHeight);
}
float QPhysicsWorld::DebugModelHolder::rowScale() const
{
    return data.y();
}
void QPhysicsWorld::DebugModelHolder::setRowScale(float rowScale)
{
    data.setY(rowScale);
}
float QPhysicsWorld::DebugModelHolder::columnScale() const
{
    return data.z();
}
void QPhysicsWorld::DebugModelHolder::setColumnScale(float columnScale)
{
    data.setZ(columnScale);
}
physx::PxConvexMesh *QPhysicsWorld::DebugModelHolder::getConvexMesh()
{
    return static_cast<physx::PxConvexMesh *>(ptr);
}
void QPhysicsWorld::DebugModelHolder::setConvexMesh(physx::PxConvexMesh *mesh)
{
    ptr = static_cast<void *>(mesh);
}
physx::PxTriangleMesh *QPhysicsWorld::DebugModelHolder::getTriangleMesh()
{
    return static_cast<physx::PxTriangleMesh *>(ptr);
}
void QPhysicsWorld::DebugModelHolder::setTriangleMesh(physx::PxTriangleMesh *mesh)
{
    ptr = static_cast<void *>(mesh);
}
physx::PxHeightField *QPhysicsWorld::DebugModelHolder::getHeightField()
{
    return static_cast<physx::PxHeightField *>(ptr);
}
void QPhysicsWorld::DebugModelHolder::setHeightField(physx::PxHeightField *hf)
{
    ptr = static_cast<physx::PxHeightField *>(hf);
}

/////////////////////////////////////////////////////////////////////////////

struct QWorldManager
{
    QVector<QPhysicsWorld *> worlds;
    QVector<QAbstractPhysicsNode *> orphanNodes;
};

static QWorldManager worldManager = QWorldManager {};

void QPhysicsWorld::registerNode(QAbstractPhysicsNode *physicsNode)
{
    auto world = getWorld(physicsNode);
    if (world) {
        world->m_newPhysicsNodes.push_back(physicsNode);
    } else {
        worldManager.orphanNodes.push_back(physicsNode);
    }
}

void QPhysicsWorld::deregisterNode(QAbstractPhysicsNode *physicsNode)
{
    for (auto world : worldManager.worlds) {
        world->m_newPhysicsNodes.removeAll(physicsNode);
        QMutexLocker locker(&world->m_removedPhysicsNodesMutex);
        if (physicsNode->m_backendObject) {
            Q_ASSERT(physicsNode->m_backendObject->frontendNode == physicsNode);
            physicsNode->m_backendObject->frontendNode = nullptr;
            physicsNode->m_backendObject->isRemoved = true;
            physicsNode->m_backendObject = nullptr;
        }
        world->m_removedPhysicsNodes.insert(physicsNode);
    }
    worldManager.orphanNodes.removeAll(physicsNode);
}

void QPhysicsWorld::registerContact(QAbstractPhysicsNode *sender, QAbstractPhysicsNode *receiver,
                                    const QVector<QVector3D> &positions,
                                    const QVector<QVector3D> &impulses,
                                    const QVector<QVector3D> &normals)
{
    // Since collision callbacks happen in the physx simulation thread we need
    // to store these callbacks. Otherwise, if an object is deleted in the same
    // frame a 'onBodyContact' signal is enqueued and a crash will happen.
    // Therefore we save these contact callbacks and run them at the end of the
    // physics frame when we know if the objects are deleted or not.

    BodyContact contact;
    contact.sender = sender;
    contact.receiver = receiver;
    contact.positions = positions;
    contact.impulses = impulses;
    contact.normals = normals;

    m_registeredContacts.push_back(contact);
}

QPhysicsWorld::QPhysicsWorld(QObject *parent) : QObject(parent)
{
    m_inDesignStudio = !qEnvironmentVariableIsEmpty("QML_PUPPET_MODE");
    m_physx = new QPhysXWorld;
    m_physx->createWorld();

    worldManager.worlds.push_back(this);
    matchOrphanNodes();
}

QPhysicsWorld::~QPhysicsWorld()
{
    m_workerThread.quit();
    m_workerThread.wait();
    for (auto body : m_physXBodies) {
        body->cleanup(m_physx);
        delete body;
    }
    m_physx->deleteWorld();
    delete m_physx;
    worldManager.worlds.removeAll(this);
}

void QPhysicsWorld::classBegin() {}

void QPhysicsWorld::componentComplete()
{
    if ((!m_running && !m_inDesignStudio) || m_physicsInitialized)
        return;
    initPhysics();
    emit simulateFrame(m_minTimestep, m_maxTimestep);
}

QVector3D QPhysicsWorld::gravity() const
{
    return m_gravity;
}

bool QPhysicsWorld::running() const
{
    return m_running;
}

bool QPhysicsWorld::forceDebugDraw() const
{
    return m_forceDebugDraw;
}

bool QPhysicsWorld::enableCCD() const
{
    return m_enableCCD;
}

float QPhysicsWorld::typicalLength() const
{
    return m_typicalLength;
}

float QPhysicsWorld::typicalSpeed() const
{
    return m_typicalSpeed;
}

bool QPhysicsWorld::isNodeRemoved(QAbstractPhysicsNode *object)
{
    return m_removedPhysicsNodes.contains(object);
}

void QPhysicsWorld::setGravity(QVector3D gravity)
{
    if (m_gravity == gravity)
        return;

    m_gravity = gravity;
    if (m_physx->scene) {
        m_physx->scene->setGravity(QPhysicsUtils::toPhysXType(m_gravity));
    }
    emit gravityChanged(m_gravity);
}

void QPhysicsWorld::setRunning(bool running)
{
    if (m_running == running)
        return;

    m_running = running;
    if (!m_inDesignStudio) {
        if (m_running && !m_physicsInitialized)
            initPhysics();
        if (m_running)
            emit simulateFrame(m_minTimestep, m_maxTimestep);
    }
    emit runningChanged(m_running);
}

void QPhysicsWorld::setForceDebugDraw(bool forceDebugDraw)
{
    if (m_forceDebugDraw == forceDebugDraw)
        return;

    m_forceDebugDraw = forceDebugDraw;
    if (!m_forceDebugDraw)
        disableDebugDraw();
    else
        updateDebugDraw();
    emit forceDebugDrawChanged(m_forceDebugDraw);
}

QQuick3DNode *QPhysicsWorld::viewport() const
{
    return m_viewport;
}

void QPhysicsWorld::setHasIndividualDebugDraw()
{
    m_hasIndividualDebugDraw = true;
}

void QPhysicsWorld::setViewport(QQuick3DNode *viewport)
{
    if (m_viewport == viewport)
        return;

    m_viewport = viewport;

    // TODO: test this
    for (auto material : m_debugMaterials)
        delete material;
    m_debugMaterials.clear();

    for (auto &holder : m_collisionShapeDebugModels) {
        holder.releaseMeshPointer();
        delete holder.model;
    }
    m_collisionShapeDebugModels.clear();

    emit viewportChanged(m_viewport);
}

void QPhysicsWorld::setMinimumTimestep(float minTimestep)
{
    if (qFuzzyCompare(m_minTimestep, minTimestep))
        return;

    if (minTimestep > m_maxTimestep) {
        qWarning("Minimum timestep greater than maximum timestep, value clamped");
        minTimestep = qMin(minTimestep, m_maxTimestep);
    }

    if (minTimestep < 0.f) {
        qWarning("Minimum timestep less than zero, value clamped");
        minTimestep = qMax(minTimestep, 0.f);
    }

    if (qFuzzyCompare(m_minTimestep, minTimestep))
        return;

    m_minTimestep = minTimestep;
    emit minimumTimestepChanged(m_minTimestep);
}

void QPhysicsWorld::setMaximumTimestep(float maxTimestep)
{
    if (qFuzzyCompare(m_maxTimestep, maxTimestep))
        return;

    if (maxTimestep < 0.f) {
        qWarning("Maximum timestep less than zero, value clamped");
        maxTimestep = qMax(maxTimestep, 0.f);
    }

    if (qFuzzyCompare(m_maxTimestep, maxTimestep))
        return;

    m_maxTimestep = maxTimestep;
    emit maximumTimestepChanged(maxTimestep);
}

void QPhysicsWorld::setupDebugMaterials(QQuick3DNode *sceneNode)
{
    if (!m_debugMaterials.isEmpty())
        return;

    const int lineWidth = m_inDesignStudio ? 1 : 3;

    // These colors match the indices of DebugDrawBodyType enum
    for (auto color : { QColorConstants::Svg::chartreuse, QColorConstants::Svg::cyan,
                        QColorConstants::Svg::lightsalmon, QColorConstants::Svg::red,
                        QColorConstants::Svg::black }) {
        auto debugMaterial = new QQuick3DDefaultMaterial();
        debugMaterial->setLineWidth(lineWidth);
        debugMaterial->setParentItem(sceneNode);
        debugMaterial->setParent(sceneNode);
        debugMaterial->setDiffuseColor(color);
        debugMaterial->setLighting(QQuick3DDefaultMaterial::NoLighting);
        debugMaterial->setCullMode(QQuick3DMaterial::NoCulling);
        m_debugMaterials.push_back(debugMaterial);
    }
}

void QPhysicsWorld::updateDebugDraw()
{
    if (!(m_forceDebugDraw || m_hasIndividualDebugDraw)) {
        // Nothing to draw, trash all previous models (if any) and return
        for (auto &holder : m_collisionShapeDebugModels) {
            holder.releaseMeshPointer();
            delete holder.model;
        }
        m_collisionShapeDebugModels.clear();
        return;
    }

    // Use scene node if no viewport has been specified
    auto sceneNode = m_viewport ? m_viewport : m_scene;

    if (sceneNode == nullptr)
        return;

    setupDebugMaterials(sceneNode);
    m_hasIndividualDebugDraw = false;

    // Store the collision shapes we have now so we can clear out the removed ones
    QSet<QPair<QAbstractCollisionShape *, QAbstractPhysXNode *>> currentCollisionShapes;
    currentCollisionShapes.reserve(m_collisionShapeDebugModels.size());

    for (QAbstractPhysXNode *node : m_physXBodies) {
        if (!node->debugGeometryCapability())
            continue;

        const auto &collisionShapes = node->frontendNode->getCollisionShapesList();
        const int materialIdx = static_cast<int>(node->getDebugDrawBodyType());
        const int length = collisionShapes.length();
        if (node->shapes.length() < length)
            continue; // CharacterController has shapes, but not PhysX shapes
        for (int idx = 0; idx < length; idx++) {
            const auto collisionShape = collisionShapes[idx];

            if (!m_forceDebugDraw && !collisionShape->enableDebugDraw())
                continue;

            const auto physXShape = node->shapes[idx];
            DebugModelHolder &holder =
                m_collisionShapeDebugModels[std::make_pair(collisionShape, node)];
            auto &model = holder.model;

            currentCollisionShapes.insert(std::make_pair(collisionShape, node));

            m_hasIndividualDebugDraw =
                    m_hasIndividualDebugDraw || collisionShape->enableDebugDraw();

            auto localPose = physXShape->getLocalPose();

            // Create/Update debug view infrastructure
            if (!model) {
                model = new QQuick3DModel();
                model->setParentItem(sceneNode);
                model->setParent(sceneNode);
                model->setCastsShadows(false);
                model->setReceivesShadows(false);
                model->setCastsReflections(false);
            }

            { // update or set material
                auto material = m_debugMaterials[materialIdx];
                QQmlListReference materialsRef(model, "materials");
                if (materialsRef.count() == 0 || materialsRef.at(0) != material) {
                    materialsRef.clear();
                    materialsRef.append(material);
                }
            }

            switch (physXShape->getGeometryType()) {
            case physx::PxGeometryType::eBOX: {
                physx::PxBoxGeometry boxGeometry;
                physXShape->getBoxGeometry(boxGeometry);
                const auto &halfExtentsOld = holder.halfExtents();
                const auto halfExtents = QPhysicsUtils::toQtType(boxGeometry.halfExtents);
                if (!qFuzzyCompare(halfExtentsOld, halfExtents)) {
                    auto geom = QDebugDrawHelper::generateBoxGeometry(halfExtents);
                    geom->setParent(model);
                    model->setGeometry(geom);
                    holder.setHalfExtents(halfExtents);
                }

            }
                break;

            case physx::PxGeometryType::eSPHERE: {
                physx::PxSphereGeometry sphereGeometry;
                physXShape->getSphereGeometry(sphereGeometry);
                const float radius = holder.radius();
                if (!qFuzzyCompare(sphereGeometry.radius, radius)) {
                    auto geom = QDebugDrawHelper::generateSphereGeometry(sphereGeometry.radius);
                    geom->setParent(model);
                    model->setGeometry(geom);
                    holder.setRadius(sphereGeometry.radius);
                }
            }
                break;

            case physx::PxGeometryType::eCAPSULE: {
                physx::PxCapsuleGeometry capsuleGeometry;
                physXShape->getCapsuleGeometry(capsuleGeometry);
                const float radius = holder.radius();
                const float halfHeight = holder.halfHeight();

                if (!qFuzzyCompare(capsuleGeometry.radius, radius)
                    || !qFuzzyCompare(capsuleGeometry.halfHeight, halfHeight)) {
                    auto geom = QDebugDrawHelper::generateCapsuleGeometry(
                            capsuleGeometry.radius, capsuleGeometry.halfHeight);
                    geom->setParent(model);
                    model->setGeometry(geom);
                    holder.setRadius(capsuleGeometry.radius);
                    holder.setHalfHeight(capsuleGeometry.halfHeight);
                }
            }
                break;

            case physx::PxGeometryType::ePLANE:{
                physx::PxPlaneGeometry planeGeometry;
                physXShape->getPlaneGeometry(planeGeometry);
                // Special rotation
                const QQuaternion rotation =
                        QPhysicsUtils::kMinus90YawRotation * QPhysicsUtils::toQtType(localPose.q);
                localPose = physx::PxTransform(localPose.p, QPhysicsUtils::toPhysXType(rotation));

                if (model->geometry() == nullptr) {
                    auto geom = QDebugDrawHelper::generatePlaneGeometry();
                    geom->setParent(model);
                    model->setGeometry(geom);
                }
            }
                break;

            // For heightfield, convex mesh and triangle mesh we increase its reference count
            // to make sure it does not get dereferenced and deleted so that the new mesh will
            // have another memory address so we know when it has changed.
            case physx::PxGeometryType::eHEIGHTFIELD: {
                physx::PxHeightFieldGeometry heightFieldGeometry;
                bool success = physXShape->getHeightFieldGeometry(heightFieldGeometry);
                Q_ASSERT(success);
                const float heightScale = holder.heightScale();
                const float rowScale = holder.rowScale();
                const float columnScale = holder.columnScale();

                if (auto heightField = holder.getHeightField();
                    heightField && heightField != heightFieldGeometry.heightField) {
                    heightField->release();
                    holder.setHeightField(nullptr);
                }

                if (!qFuzzyCompare(heightFieldGeometry.heightScale, heightScale)
                    || !qFuzzyCompare(heightFieldGeometry.rowScale, rowScale)
                    || !qFuzzyCompare(heightFieldGeometry.columnScale, columnScale)
                    || !holder.getHeightField()) {
                    if (!holder.getHeightField()) {
                        heightFieldGeometry.heightField->acquireReference();
                        holder.setHeightField(heightFieldGeometry.heightField);
                    }
                    auto geom = QDebugDrawHelper::generateHeightFieldGeometry(
                            heightFieldGeometry.heightField, heightFieldGeometry.heightScale,
                            heightFieldGeometry.rowScale, heightFieldGeometry.columnScale);
                    geom->setParent(model);
                    model->setGeometry(geom);
                    holder.setHeightScale(heightFieldGeometry.heightScale);
                    holder.setRowScale(heightFieldGeometry.rowScale);
                    holder.setColumnScale(heightFieldGeometry.columnScale);
                }
            }
                break;

            case physx::PxGeometryType::eCONVEXMESH: {
                physx::PxConvexMeshGeometry convexMeshGeometry;
                const bool success = physXShape->getConvexMeshGeometry(convexMeshGeometry);
                Q_ASSERT(success);
                const auto rotation = convexMeshGeometry.scale.rotation * localPose.q;
                localPose = physx::PxTransform(localPose.p, rotation);
                model->setScale(QPhysicsUtils::toQtType(convexMeshGeometry.scale.scale));

                if (auto convexMesh = holder.getConvexMesh();
                    convexMesh && convexMesh != convexMeshGeometry.convexMesh) {
                    convexMesh->release();
                    holder.setConvexMesh(nullptr);
                }

                if (!model->geometry() || !holder.getConvexMesh()) {
                    if (!holder.getConvexMesh()) {
                        convexMeshGeometry.convexMesh->acquireReference();
                        holder.setConvexMesh(convexMeshGeometry.convexMesh);
                    }
                    auto geom = QDebugDrawHelper::generateConvexMeshGeometry(
                            convexMeshGeometry.convexMesh);
                    geom->setParent(model);
                    model->setGeometry(geom);
                }
            }
                break;

            case physx::PxGeometryType::eTRIANGLEMESH: {
                physx::PxTriangleMeshGeometry triangleMeshGeometry;
                const bool success = physXShape->getTriangleMeshGeometry(triangleMeshGeometry);
                Q_ASSERT(success);
                const auto rotation = triangleMeshGeometry.scale.rotation * localPose.q;
                localPose = physx::PxTransform(localPose.p, rotation);
                model->setScale(QPhysicsUtils::toQtType(triangleMeshGeometry.scale.scale));

                if (auto triangleMesh = holder.getTriangleMesh();
                    triangleMesh && triangleMesh != triangleMeshGeometry.triangleMesh) {
                    triangleMesh->release();
                    holder.setTriangleMesh(nullptr);
                }

                if (!model->geometry() || !holder.getTriangleMesh()) {
                    if (!holder.getTriangleMesh()) {
                        triangleMeshGeometry.triangleMesh->acquireReference();
                        holder.setTriangleMesh(triangleMeshGeometry.triangleMesh);
                    }
                    auto geom = QDebugDrawHelper::generateTriangleMeshGeometry(
                            triangleMeshGeometry.triangleMesh);
                    geom->setParent(model);
                    model->setGeometry(geom);
                }
            }
                break;

            case physx::PxGeometryType::eINVALID:
            case physx::PxGeometryType::eGEOMETRY_COUNT:
                // should not happen
                Q_UNREACHABLE();
            }

            model->setVisible(true);

            auto globalPose = node->getGlobalPose();
            auto finalPose = globalPose.transform(localPose);

            model->setRotation(QPhysicsUtils::toQtType(finalPose.q));
            model->setPosition(QPhysicsUtils::toQtType(finalPose.p));
        }
    }

    // Remove old collision shapes
    m_collisionShapeDebugModels.removeIf(
            [&](QHash<QPair<QAbstractCollisionShape *, QAbstractPhysXNode *>,
                      DebugModelHolder>::iterator it) {
                if (!currentCollisionShapes.contains(it.key())) {
                    auto holder = it.value();
                    holder.releaseMeshPointer();
                    if (holder.model)
                        delete holder.model;
                    return true;
                }
                return false;
            });
}

static void collectPhysicsNodes(QQuick3DObject *node, QList<QAbstractPhysicsNode *> &nodes)
{
    if (auto shape = qobject_cast<QAbstractPhysicsNode *>(node)) {
        nodes.push_back(shape);
        return;
    }

    for (QQuick3DObject *child : node->childItems())
        collectPhysicsNodes(child, nodes);
}

void QPhysicsWorld::updateDebugDrawDesignStudio()
{
    // Use scene node if no viewport has been specified
    auto sceneNode = m_viewport ? m_viewport : m_scene;

    if (sceneNode == nullptr)
        return;

    setupDebugMaterials(sceneNode);

    // Store the collision shapes we have now so we can clear out the removed ones
    QSet<QPair<QAbstractCollisionShape *, QAbstractPhysicsNode *>> currentCollisionShapes;
    currentCollisionShapes.reserve(m_collisionShapeDebugModels.size());

    QList<QAbstractPhysicsNode *> activePhysicsNodes;
    activePhysicsNodes.reserve(m_collisionShapeDebugModels.size());
    collectPhysicsNodes(m_scene, activePhysicsNodes);

    for (QAbstractPhysicsNode *node : activePhysicsNodes) {

        const auto &collisionShapes = node->getCollisionShapesList();
        const int materialIdx = 0; // Just take first material
        const int length = collisionShapes.length();

        const bool isCharacterController = qobject_cast<QCharacterController *>(node) != nullptr;

        for (int idx = 0; idx < length; idx++) {
            QAbstractCollisionShape *collisionShape = collisionShapes[idx];
            DebugModelHolder &holder =
                    m_DesignStudioDebugModels[std::make_pair(collisionShape, node)];
            auto &model = holder.model;

            currentCollisionShapes.insert(std::make_pair(collisionShape, node));

            m_hasIndividualDebugDraw =
                    m_hasIndividualDebugDraw || collisionShape->enableDebugDraw();

            // Create/Update debug view infrastructure
            {
                // Hack: we have to delete the model every frame so it shows up in QDS
                // whenever the code is updated, not sure why ¯\_(?)_/¯
                delete model;
                model = new QQuick3DModel();
                model->setParentItem(sceneNode);
                model->setParent(sceneNode);
                model->setCastsShadows(false);
                model->setReceivesShadows(false);
                model->setCastsReflections(false);
            }

            const bool hasGeometry = holder.geometry != nullptr;
            QVector3D scenePosition = collisionShape->scenePosition();
            QQuaternion sceneRotation = collisionShape->sceneRotation();
            QQuick3DGeometry *newGeometry = nullptr;

            if (isCharacterController)
                sceneRotation = sceneRotation * QQuaternion::fromEulerAngles(QVector3D(0, 0, 90));

            { // update or set material
                auto material = m_debugMaterials[materialIdx];
                QQmlListReference materialsRef(model, "materials");
                if (materialsRef.count() == 0 || materialsRef.at(0) != material) {
                    materialsRef.clear();
                    materialsRef.append(material);
                }
            }

            if (auto shape = qobject_cast<QBoxShape *>(collisionShape)) {
                const auto &halfExtentsOld = holder.halfExtents();
                const auto halfExtents = shape->sceneScale() * shape->extents() * 0.5f;
                if (!qFuzzyCompare(halfExtentsOld, halfExtents) || !hasGeometry) {
                    newGeometry = QDebugDrawHelper::generateBoxGeometry(halfExtents);
                    holder.setHalfExtents(halfExtents);
                }
            } else if (auto shape = qobject_cast<QSphereShape *>(collisionShape)) {
                const float radiusOld = holder.radius();
                const float radius = shape->sceneScale().x() * shape->diameter() * 0.5f;
                if (!qFuzzyCompare(radiusOld, radius) || !hasGeometry) {
                    newGeometry = QDebugDrawHelper::generateSphereGeometry(radius);
                    holder.setRadius(radius);
                }
            } else if (auto shape = qobject_cast<QCapsuleShape *>(collisionShape)) {
                const float radiusOld = holder.radius();
                const float halfHeightOld = holder.halfHeight();
                const float radius = shape->sceneScale().y() * shape->diameter() * 0.5f;
                const float halfHeight = shape->sceneScale().x() * shape->height() * 0.5f;

                if ((!qFuzzyCompare(radiusOld, radius) || !qFuzzyCompare(halfHeightOld, halfHeight))
                    || !hasGeometry) {
                    newGeometry = QDebugDrawHelper::generateCapsuleGeometry(radius, halfHeight);
                    holder.setRadius(radius);
                    holder.setHalfHeight(halfHeight);
                }
            } else if (qobject_cast<QPlaneShape *>(collisionShape)) {
                if (!hasGeometry)
                    newGeometry = QDebugDrawHelper::generatePlaneGeometry();
            } else if (auto shape = qobject_cast<QHeightFieldShape *>(collisionShape)) {
                physx::PxHeightFieldGeometry *heightFieldGeometry =
                        static_cast<physx::PxHeightFieldGeometry *>(shape->getPhysXGeometry());
                const float heightScale = holder.heightScale();
                const float rowScale = holder.rowScale();
                const float columnScale = holder.columnScale();
                scenePosition += shape->hfOffset();
                if (!heightFieldGeometry) {
                    qWarning() << "Could not get height field";
                } else if (!qFuzzyCompare(heightFieldGeometry->heightScale, heightScale)
                           || !qFuzzyCompare(heightFieldGeometry->rowScale, rowScale)
                           || !qFuzzyCompare(heightFieldGeometry->columnScale, columnScale)
                           || !hasGeometry) {
                    newGeometry = QDebugDrawHelper::generateHeightFieldGeometry(
                            heightFieldGeometry->heightField, heightFieldGeometry->heightScale,
                            heightFieldGeometry->rowScale, heightFieldGeometry->columnScale);
                    holder.setHeightScale(heightFieldGeometry->heightScale);
                    holder.setRowScale(heightFieldGeometry->rowScale);
                    holder.setColumnScale(heightFieldGeometry->columnScale);
                }
            } else if (auto shape = qobject_cast<QConvexMeshShape *>(collisionShape)) {
                auto convexMeshGeometry =
                        static_cast<physx::PxConvexMeshGeometry *>(shape->getPhysXGeometry());
                if (!convexMeshGeometry) {
                    qWarning() << "Could not get convex mesh";
                } else {
                    model->setScale(QPhysicsUtils::toQtType(convexMeshGeometry->scale.scale));

                    if (!hasGeometry) {
                        newGeometry = QDebugDrawHelper::generateConvexMeshGeometry(
                                convexMeshGeometry->convexMesh);
                    }
                }
            } else if (auto shape = qobject_cast<QTriangleMeshShape *>(collisionShape)) {
                physx::PxTriangleMeshGeometry *triangleMeshGeometry =
                        static_cast<physx::PxTriangleMeshGeometry *>(shape->getPhysXGeometry());
                if (!triangleMeshGeometry) {
                    qWarning() << "Could not get triangle mesh";
                } else {
                    model->setScale(QPhysicsUtils::toQtType(triangleMeshGeometry->scale.scale));

                    if (!hasGeometry) {
                        newGeometry = QDebugDrawHelper::generateTriangleMeshGeometry(
                                triangleMeshGeometry->triangleMesh);
                    }
                }
            }

            if (newGeometry) {
                delete holder.geometry;
                holder.geometry = newGeometry;
            }

            model->setGeometry(holder.geometry);
            model->setVisible(true);

            model->setRotation(sceneRotation);
            model->setPosition(scenePosition);
        }
    }

    // Remove old debug models
    m_DesignStudioDebugModels.removeIf(
            [&](QHash<QPair<QAbstractCollisionShape *, QAbstractPhysicsNode *>,
                      DebugModelHolder>::iterator it) {
                if (!currentCollisionShapes.contains(it.key())) {
                    auto holder = it.value();
                    holder.releaseMeshPointer();
                    if (holder.model) {
                        delete holder.geometry;
                        delete holder.model;
                    }
                    return true;
                }
                return false;
            });
}

void QPhysicsWorld::disableDebugDraw()
{
    m_hasIndividualDebugDraw = false;

    for (QAbstractPhysXNode *body : m_physXBodies) {
        const auto &collisionShapes = body->frontendNode->getCollisionShapesList();
        const int length = collisionShapes.length();
        for (int idx = 0; idx < length; idx++) {
            const auto collisionShape = collisionShapes[idx];
            if (collisionShape->enableDebugDraw()) {
                m_hasIndividualDebugDraw = true;
                return;
            }
        }
    }
}

void QPhysicsWorld::setEnableCCD(bool enableCCD)
{
    if (m_enableCCD == enableCCD)
        return;

    if (m_physicsInitialized) {
        qWarning()
                << "Warning: Changing 'enableCCD' after physics is initialized will have no effect";
        return;
    }

    m_enableCCD = enableCCD;
    emit enableCCDChanged(m_enableCCD);
}

void QPhysicsWorld::setTypicalLength(float typicalLength)
{
    if (qFuzzyCompare(typicalLength, m_typicalLength))
        return;

    if (typicalLength <= 0.f) {
        qWarning() << "Warning: 'typicalLength' value less than zero, ignored";
        return;
    }

    if (m_physicsInitialized) {
        qWarning() << "Warning: Changing 'typicalLength' after physics is initialized will have "
                      "no effect";
        return;
    }

    m_typicalLength = typicalLength;

    emit typicalLengthChanged(typicalLength);
}

void QPhysicsWorld::setTypicalSpeed(float typicalSpeed)
{
    if (qFuzzyCompare(typicalSpeed, m_typicalSpeed))
        return;

    if (m_physicsInitialized) {
        qWarning() << "Warning: Changing 'typicalSpeed' after physics is initialized will have "
                      "no effect";
        return;
    }

    m_typicalSpeed = typicalSpeed;

    emit typicalSpeedChanged(typicalSpeed);
}

float QPhysicsWorld::defaultDensity() const
{
    return m_defaultDensity;
}

float QPhysicsWorld::minimumTimestep() const
{
    return m_minTimestep;
}

float QPhysicsWorld::maximumTimestep() const
{
    return m_maxTimestep;
}

void QPhysicsWorld::setDefaultDensity(float defaultDensity)
{
    if (qFuzzyCompare(m_defaultDensity, defaultDensity))
        return;
    m_defaultDensity = defaultDensity;

    // Go through all dynamic rigid bodies and update the default density
    for (QAbstractPhysXNode *body : m_physXBodies)
        body->updateDefaultDensity(m_defaultDensity);

    emit defaultDensityChanged(defaultDensity);
}

// Remove physics world items that no longer exist

void QPhysicsWorld::cleanupRemovedNodes()
{
    m_physXBodies.removeIf([this](QAbstractPhysXNode *body) {
                               return body->cleanupIfRemoved(m_physx);
                           });
    // We don't need to lock the mutex here since the simulation
    // worker is waiting
    m_removedPhysicsNodes.clear();
}

void QPhysicsWorld::initPhysics()
{
    Q_ASSERT(!m_physicsInitialized);

    m_physx->createScene(m_typicalLength, m_typicalSpeed, m_gravity, m_enableCCD, this);

    // Setup worker thread
    SimulationWorker *worker = new SimulationWorker(m_physx);
    worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, worker, &QObject::deleteLater);
    if (m_inDesignStudio) {
        connect(this, &QPhysicsWorld::simulateFrame, worker,
                &SimulationWorker::simulateFrameDesignStudio);
        connect(worker, &SimulationWorker::frameDoneDesignStudio, this,
                &QPhysicsWorld::frameFinishedDesignStudio);
    } else {
        connect(this, &QPhysicsWorld::simulateFrame, worker, &SimulationWorker::simulateFrame);
        connect(worker, &SimulationWorker::frameDone, this, &QPhysicsWorld::frameFinished);
    }
    m_workerThread.start();

    m_physicsInitialized = true;
}

void QPhysicsWorld::frameFinished(float deltaTime)
{
    matchOrphanNodes();
    emitContactCallbacks();
    cleanupRemovedNodes();
    for (auto *node : std::as_const(m_newPhysicsNodes)) {
        auto *body = node->createPhysXBackend();
        body->init(this, m_physx);
        m_physXBodies.push_back(body);
    }
    m_newPhysicsNodes.clear();

    QHash<QQuick3DNode *, QMatrix4x4> transformCache;

    // TODO: Use dirty flag/dirty list to avoid redoing things that didn't change
    for (auto *physXBody : std::as_const(m_physXBodies)) {
        physXBody->markDirtyShapes();
        physXBody->rebuildDirtyShapes(this, m_physx);

        // Sync the physics world and the scene
        physXBody->sync(deltaTime, transformCache);
    }

    updateDebugDraw();

    if (m_running)
        emit simulateFrame(m_minTimestep, m_maxTimestep);
    emit frameDone(deltaTime * 1000);
}

void QPhysicsWorld::frameFinishedDesignStudio()
{
    // Note sure if this is needed but do it anyway
    matchOrphanNodes();
    emitContactCallbacks();
    cleanupRemovedNodes();
    // Ignore new physics nodes, we find them from the scene node anyway
    m_newPhysicsNodes.clear();

    updateDebugDrawDesignStudio();

    emit simulateFrame(m_minTimestep, m_maxTimestep);
}

QPhysicsWorld *QPhysicsWorld::getWorld(QQuick3DNode *node)
{
    for (QPhysicsWorld *world : worldManager.worlds) {
        if (!world->m_scene) {
            continue;
        }

        QQuick3DNode *nodeCurr = node;

        // Maybe pointless but check starting node
        if (nodeCurr == world->m_scene)
            return world;

        while (nodeCurr->parentNode()) {
            nodeCurr = nodeCurr->parentNode();
            if (nodeCurr == world->m_scene)
                return world;
        }
    }

    return nullptr;
}

void QPhysicsWorld::matchOrphanNodes()
{
    // FIXME: does this need thread safety?
    if (worldManager.orphanNodes.isEmpty())
        return;

    qsizetype numNodes = worldManager.orphanNodes.length();
    qsizetype idx = 0;

    while (idx < numNodes) {
        auto node = worldManager.orphanNodes[idx];
        auto world = getWorld(node);
        if (world == this) {
            world->m_newPhysicsNodes.push_back(node);
            // swap-erase
            worldManager.orphanNodes.swapItemsAt(idx, numNodes - 1);
            worldManager.orphanNodes.pop_back();
            numNodes--;
        } else {
            idx++;
        }
    }
}

void QPhysicsWorld::findPhysicsNodes()
{
    // This method finds the physics nodes inside the scene pointed to by the
    // scene property. This method is necessary to run whenever the scene
    // property is changed.
    if (m_scene == nullptr)
        return;

    // Recursively go through all children and add all QAbstractPhysicsNode's
    QList<QQuick3DObject *> children = m_scene->childItems();
    while (!children.empty()) {
        auto child = children.takeFirst();
        if (auto converted = qobject_cast<QAbstractPhysicsNode *>(child); converted != nullptr) {
            // This should never happen but check anyway.
            if (converted->m_backendObject != nullptr) {
                qWarning() << "Warning: physics node already associated with a backend node.";
                continue;
            }

            m_newPhysicsNodes.push_back(converted);
            worldManager.orphanNodes.removeAll(converted); // No longer orphan
        }
        children.append(child->childItems());
    }
}

void QPhysicsWorld::emitContactCallbacks()
{
    for (const QPhysicsWorld::BodyContact &contact : m_registeredContacts) {
        if (m_removedPhysicsNodes.contains(contact.sender)
            || m_removedPhysicsNodes.contains(contact.receiver))
            continue;
        contact.receiver->registerContact(contact.sender, contact.positions, contact.impulses,
                                          contact.normals);
    }

    m_registeredContacts.clear();
}

physx::PxPhysics *QPhysicsWorld::getPhysics()
{
    return StaticPhysXObjects::getReference().physics;
}

physx::PxCooking *QPhysicsWorld::getCooking()
{
    return StaticPhysXObjects::getReference().cooking;
}

physx::PxControllerManager *QPhysicsWorld::controllerManager()
{
    if (m_physx->scene && !m_physx->controllerManager) {
        m_physx->controllerManager = PxCreateControllerManager(*m_physx->scene);
        qCDebug(lcQuick3dPhysics) << "Created controller manager" << m_physx->controllerManager;
    }
    return m_physx->controllerManager;
}

QQuick3DNode *QPhysicsWorld::scene() const
{
    return m_scene;
}

void QPhysicsWorld::setScene(QQuick3DNode *newScene)
{
    if (m_scene == newScene)
        return;

    m_scene = newScene;

    // Delete all nodes since they are associated with the previous scene
    for (auto body : m_physXBodies) {
        deregisterNode(body->frontendNode);
    }

    // Check if scene is already used by another world
    bool sceneOK = true;
    for (QPhysicsWorld *world : worldManager.worlds) {
        if (world != this && world->scene() == newScene) {
            sceneOK = false;
            qWarning() << "Warning: scene already associated with physics world";
        }
    }

    if (sceneOK)
        findPhysicsNodes();
    emit sceneChanged();
}

QT_END_NAMESPACE

#include "qphysicsworld.moc"
