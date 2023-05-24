// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcacheutils_p.h"
#include "qmeshshape_p.h"

#include <QFile>
#include <QFileInfo>
#include <QtQuick3D/QQuick3DGeometry>
#include <extensions/PxExtensionsAPI.h>

#include "foundation/PxVec3.h"
#include "cooking/PxConvexMeshDesc.h"
#include "extensions/PxDefaultStreams.h"

#include <QtQml/qqml.h>
#include <QtQml/QQmlFile>
#include <QtQml/qqmlcontext.h>

#include <QtQuick3DUtils/private/qssgmesh_p.h>
#include "qmeshshape_p.h"
#include "qphysicsworld_p.h"
#include "qphysicsmeshutils_p_p.h"

QT_BEGIN_NAMESPACE

physx::PxConvexMesh *QQuick3DPhysicsMesh::convexMesh()
{
    if (m_convexMesh != nullptr)
        return m_convexMesh;

    physx::PxPhysics *thePhysics = QPhysicsWorld::getPhysics();
    if (thePhysics == nullptr)
        return nullptr;

    m_convexMesh = QCacheUtils::readCachedConvexMesh(m_meshPath, *thePhysics);
    if (m_convexMesh != nullptr)
        return m_convexMesh;

    m_convexMesh = QCacheUtils::readCookedConvexMesh(m_meshPath, *thePhysics);
    if (m_convexMesh != nullptr)
        return m_convexMesh;

    loadSsgMesh();

    if (!m_ssgMesh.isValid())
        return nullptr;

    physx::PxDefaultMemoryOutputStream buf;
    physx::PxConvexMeshCookingResult::Enum result;
    int vStride = m_ssgMesh.vertexBuffer().stride;
    int vCount = m_ssgMesh.vertexBuffer().data.size() / vStride;
    const auto *vd = m_ssgMesh.vertexBuffer().data.constData();

    qCDebug(lcQuick3dPhysics) << "prepare cooking" << vCount << "verts";

    QVector<physx::PxVec3> verts;

    for (int i = 0; i < vCount; ++i) {
        auto *vp = reinterpret_cast<const QVector3D *>(vd + vStride * i + m_posOffset);
        verts << physx::PxVec3 { vp->x(), vp->y(), vp->z() };
    }

    const auto *convexVerts = verts.constData();

    physx::PxConvexMeshDesc convexDesc;
    convexDesc.points.count = vCount;
    convexDesc.points.stride = sizeof(physx::PxVec3);
    convexDesc.points.data = convexVerts;
    convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

    const auto cooking = QPhysicsWorld::getCooking();
    if (cooking && cooking->cookConvexMesh(convexDesc, buf, &result)) {
        auto size = buf.getSize();
        auto *data = buf.getData();
        physx::PxDefaultMemoryInputData input(data, size);
        m_convexMesh = thePhysics->createConvexMesh(input);
        qCDebug(lcQuick3dPhysics) << "Created convex mesh" << m_convexMesh << "for mesh" << this;
        QCacheUtils::writeCachedConvexMesh(m_meshPath, buf);
    } else {
        qCWarning(lcQuick3dPhysics) << "Could not create convex mesh from" << m_meshPath;
    }

    return m_convexMesh;
}

physx::PxTriangleMesh *QQuick3DPhysicsMesh::triangleMesh()
{
    if (m_triangleMesh != nullptr)
        return m_triangleMesh;

    physx::PxPhysics *thePhysics = QPhysicsWorld::getPhysics();
    if (thePhysics == nullptr)
        return nullptr;

    m_triangleMesh = QCacheUtils::readCachedTriangleMesh(m_meshPath, *thePhysics);
    if (m_triangleMesh != nullptr)
        return m_triangleMesh;

    m_triangleMesh = QCacheUtils::readCookedTriangleMesh(m_meshPath, *thePhysics);
    if (m_triangleMesh != nullptr)
        return m_triangleMesh;

    loadSsgMesh();
    if (!m_ssgMesh.isValid())
        return nullptr;

    physx::PxDefaultMemoryOutputStream buf;
    physx::PxTriangleMeshCookingResult::Enum result;
    const int vStride = m_ssgMesh.vertexBuffer().stride;
    const int vCount = m_ssgMesh.vertexBuffer().data.size() / vStride;
    const auto *vd = m_ssgMesh.vertexBuffer().data.constData();

    const int iStride =
            m_ssgMesh.indexBuffer().componentType == QSSGMesh::Mesh::ComponentType::UnsignedInt16
            ? 2
            : 4;
    const int iCount = m_ssgMesh.indexBuffer().data.size() / iStride;

    qCDebug(lcQuick3dPhysics) << "prepare cooking" << vCount << "verts" << iCount << "idxs";

    physx::PxTriangleMeshDesc triangleDesc;
    triangleDesc.points.count = vCount;
    triangleDesc.points.stride = vStride;
    triangleDesc.points.data = vd + m_posOffset;

    triangleDesc.flags = {}; //??? physx::PxMeshFlag::eFLIPNORMALS or
                             // physx::PxMeshFlag::e16_BIT_INDICES
    triangleDesc.triangles.count = iCount / 3;
    triangleDesc.triangles.stride = iStride * 3;
    triangleDesc.triangles.data = m_ssgMesh.indexBuffer().data.constData();

    const auto cooking = QPhysicsWorld::getCooking();
    if (cooking && cooking->cookTriangleMesh(triangleDesc, buf, &result)) {
        auto size = buf.getSize();
        auto *data = buf.getData();
        physx::PxDefaultMemoryInputData input(data, size);
        m_triangleMesh = thePhysics->createTriangleMesh(input);
        qCDebug(lcQuick3dPhysics) << "Created triangle mesh" << m_triangleMesh << "for mesh"
                                  << this;
        QCacheUtils::writeCachedTriangleMesh(m_meshPath, buf);
    } else {
        qCWarning(lcQuick3dPhysics) << "Could not create triangle mesh from" << m_meshPath;
    }

    return m_triangleMesh;
}

void QQuick3DPhysicsMesh::loadSsgMesh()
{
    if (m_ssgMesh.isValid())
        return;

    static const char *compTypes[] = { "Null",  "UnsignedInt8",  "Int8",    "UnsignedInt16",
                                       "Int16", "UnsignedInt32", "Int32",   "UnsignedInt64",
                                       "Int64", "Float16",       "Float32", "Float64" };

    QFileInfo fileInfo = QFileInfo(m_meshPath);
    if (fileInfo.exists()) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QFile::ReadOnly))
            m_ssgMesh = QSSGMesh::Mesh::loadMesh(&file);
    }
    qCDebug(lcQuick3dPhysics) << "Loaded SSG mesh from" << m_meshPath << m_ssgMesh.isValid()
                              << "draw" << int(m_ssgMesh.drawMode()) << "wind"
                              << int(m_ssgMesh.winding()) << "subs" << m_ssgMesh.subsets().count()
                              << "attrs" << m_ssgMesh.vertexBuffer().entries.count()
                              << m_ssgMesh.vertexBuffer().data.size() << "stride"
                              << m_ssgMesh.vertexBuffer().stride << "verts"
                              << m_ssgMesh.vertexBuffer().data.size()
                    / m_ssgMesh.vertexBuffer().stride;

    for (auto &v : m_ssgMesh.vertexBuffer().entries) {
        qCDebug(lcQuick3dPhysics) << "  attr" << v.name << compTypes[int(v.componentType)] << "cc"
                                  << v.componentCount << "offs" << v.offset;
        Q_ASSERT(v.componentType == QSSGMesh::Mesh::ComponentType::Float32);
        if (v.name == "attr_pos")
            m_posOffset = v.offset;
    }

    if (m_ssgMesh.isValid()) {
        auto sub = m_ssgMesh.subsets().constFirst();
        qCDebug(lcQuick3dPhysics) << "..." << sub.name << "count" << sub.count << "bounds"
                                  << sub.bounds.min << sub.bounds.max << "offset" << sub.offset;
    }

#if 0 // EXTRA_DEBUG

    int iStride = m_ssgMesh.indexBuffer().componentType == QSSGMesh::Mesh::ComponentType::UnsignedInt16 ? 2 : 4;
    int vStride = m_ssgMesh.vertexBuffer().stride;
    qDebug() << "IDX" << compTypes[int(m_ssgMesh.indexBuffer().componentType)] << m_ssgMesh.indexBuffer().data.size() / iStride;
    const auto ib = m_ssgMesh.indexBuffer().data;
    const auto vb = m_ssgMesh.vertexBuffer().data;

    auto getPoint = [&vb, vStride, this](int idx) -> QVector3D {
        auto *vp = vb.constData() + vStride * idx + m_posOffset;
        return *reinterpret_cast<const QVector3D *>(vp);
        return {};
    };

    if (iStride == 2) {

    } else {
        auto *ip = reinterpret_cast<const uint32_t *>(ib.data());
        int n = ib.size() / iStride;
        for (int i = 0; i < qMin(50,n); i += 3) {

            qDebug() << "    " << ip [i] << ip[i+1] << ip[i+2] << " --- "
                     << getPoint(ip[i]) << getPoint(ip[i+1]) << getPoint(ip[i+2]);
        }
    }
#endif
    if (!m_ssgMesh.isValid())
        qCWarning(lcQuick3dPhysics) << "Could not read mesh from" << m_meshPath;
}

QQuick3DPhysicsMesh *QQuick3DPhysicsMeshManager::getMesh(const QUrl &source,
                                                         const QObject *contextObject)
{
    const QQmlContext *context = qmlContext(contextObject);
    const auto resolvedUrl = context ? context->resolvedUrl(source) : source;
    const auto qmlSource = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);
    auto *mesh = meshHash.value(qmlSource);
    if (!mesh) {
        mesh = new QQuick3DPhysicsMesh(qmlSource);
        meshHash[qmlSource] = mesh;
    }
    mesh->ref();
    return mesh;
}

void QQuick3DPhysicsMeshManager::releaseMesh(QQuick3DPhysicsMesh *mesh)
{
    if (mesh->deref() == 0) {
        qCDebug(lcQuick3dPhysics()) << "deleting mesh" << mesh;
        erase_if(meshHash, [mesh](std::pair<const QString &, QQuick3DPhysicsMesh *&> h) {
            return h.second == mesh;
        });
        delete mesh;
    }
}

QHash<QString, QQuick3DPhysicsMesh *> QQuick3DPhysicsMeshManager::meshHash;

QMeshShape::~QMeshShape()
{
    delete m_convexGeometry;
    if (m_mesh)
        QQuick3DPhysicsMeshManager::releaseMesh(m_mesh);
}

physx::PxGeometry *QMeshShape::getPhysXGeometry()
{
    if (m_dirtyPhysx || m_scaleDirty)
        updatePhysXGeometry();
    if (shapeType() == MeshType::CONVEX)
        return m_convexGeometry;
    if (shapeType() == MeshType::TRIANGLE)
        return m_triangleGeometry;

    Q_UNREACHABLE_RETURN(nullptr);
}

void QMeshShape::updatePhysXGeometry()
{
    delete m_convexGeometry;
    delete m_triangleGeometry;
    m_convexGeometry = nullptr;
    m_triangleGeometry = nullptr;

    if (!m_mesh)
        return;

    auto *convexMesh = shapeType() == MeshType::CONVEX ? m_mesh->convexMesh() : nullptr;
    auto *triangleMesh = shapeType() == MeshType::TRIANGLE ? m_mesh->triangleMesh() : nullptr;
    if (!convexMesh && !triangleMesh)
        return;

    auto meshScale = sceneScale();
    physx::PxMeshScale scale(physx::PxVec3(meshScale.x(), meshScale.y(), meshScale.z()),
                             physx::PxQuat(physx::PxIdentity));

    if (convexMesh)
        m_convexGeometry = new physx::PxConvexMeshGeometry(convexMesh, scale);
    if (triangleMesh)
        m_triangleGeometry = new physx::PxTriangleMeshGeometry(triangleMesh, scale);

    m_dirtyPhysx = false;
}

const QUrl &QMeshShape::source() const
{
    return m_meshSource;
}

void QMeshShape::setSource(const QUrl &newSource)
{
    if (m_meshSource == newSource)
        return;
    m_meshSource = newSource;
    m_mesh = QQuick3DPhysicsMeshManager::getMesh(m_meshSource, this);
    updatePhysXGeometry();

    m_dirtyPhysx = true;

    emit needsRebuild(this);
    emit sourceChanged();
}

QT_END_NAMESPACE
