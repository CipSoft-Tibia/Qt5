// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QPHYSICSMESHUTILS_P_P_H
#define QPHYSICSMESHUTILS_P_P_H

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
#include <QtGui/QVector3D>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

namespace physx {
class PxBoxGeometry;
class PxConvexMesh;
class PxConvexMeshGeometry;
class PxTriangleMesh;
}

QT_BEGIN_NAMESPACE

class QQuick3DGeometry;

class QQuick3DPhysicsMesh
{
public:
    QQuick3DPhysicsMesh(const QString &qmlSource) : m_meshPath(qmlSource) { }
    ~QQuick3DPhysicsMesh() { }

    QList<QVector3D> positions();

    QPair<QVector3D, QVector3D> bounds()
    {
        loadSsgMesh();
        if (m_ssgMesh.isValid()) {
            auto b = m_ssgMesh.subsets().constFirst().bounds;
            return { b.min, b.max };
        }
        return {};
    }

    void ref() { ++refCount; }
    int deref() { return --refCount; }

    physx::PxConvexMesh *convexMesh();
    physx::PxTriangleMesh *triangleMesh();

    enum MeshType { Convex, Triangle };

private:
    void loadSsgMesh();

    QString m_meshPath;
    QSSGMesh::Mesh m_ssgMesh;
    int m_posOffset = 0;

    physx::PxConvexMesh *m_convexMesh = nullptr;
    physx::PxTriangleMesh *m_triangleMesh = nullptr;
    int refCount = 0;
};

class QQuick3DPhysicsMeshManager
{
public:
    static QQuick3DPhysicsMesh *getMesh(const QUrl &source, const QObject *contextObject);
    static void releaseMesh(QQuick3DPhysicsMesh *mesh);

private:
    static QHash<QString, QQuick3DPhysicsMesh *> meshHash;
};

QT_END_NAMESPACE

#endif // QPHYSICSMESHUTILS_P_P_H
