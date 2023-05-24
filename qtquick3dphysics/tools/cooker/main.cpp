// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "cooking/PxCooking.h"

#include <QtQuick3DUtils/private/qssgmesh_p.h>
#include <QtQuick3DPhysics/private/qcacheutils_p.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QImage>

#include "PxPhysicsAPI.h"
#include "cooking/PxCooking.h"

#include <iostream>

bool tryReadMesh(QFile *file, QSSGMesh::Mesh &mesh)
{
    auto device = QSharedPointer<QIODevice>(file);
    const quint32 id = 1;
    mesh = QSSGMesh::Mesh::loadMesh(device.data(), id);
    return mesh.isValid();
}

bool tryReadImage(const char *inputPath, QImage &image)
{
    image = QImage(inputPath);
    return image.format() != QImage::Format_Invalid;
}

bool cookMeshes(const char *inputPath, QSSGMesh::Mesh &mesh)
{
    physx::PxDefaultErrorCallback defaultErrorCallback;
    physx::PxDefaultAllocator defaultAllocatorCallback;
    auto foundation = PxCreateFoundation(PX_PHYSICS_VERSION, defaultAllocatorCallback, defaultErrorCallback);
    auto cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, physx::PxCookingParams(physx::PxTolerancesScale()));

    const int vStride = mesh.vertexBuffer().stride;
    const int vCount = mesh.vertexBuffer().data.size() / vStride;
    const auto *vd = mesh.vertexBuffer().data.constData();

    const int iStride = mesh.indexBuffer().componentType == QSSGMesh::Mesh::ComponentType::UnsignedInt16 ? 2 : 4;
    const int iCount = mesh.indexBuffer().data.size() / iStride;

    int m_posOffset = 0;

    for (auto &v : mesh.vertexBuffer().entries) {
        Q_ASSERT(v.componentType == QSSGMesh::Mesh::ComponentType::Float32);
        if (v.name == "attr_pos")
            m_posOffset = v.offset;
    }

    { // Triangle mesh
        physx::PxTriangleMeshCookingResult::Enum result;
        physx::PxTriangleMeshDesc triangleDesc;
        triangleDesc.points.count = vCount;
        triangleDesc.points.stride = vStride;
        triangleDesc.points.data = vd + m_posOffset;

        triangleDesc.flags = {}; //??? physx::PxMeshFlag::eFLIPNORMALS or
                                 // physx::PxMeshFlag::e16_BIT_INDICES
        triangleDesc.triangles.count = iCount / 3;
        triangleDesc.triangles.stride = iStride * 3;
        triangleDesc.triangles.data = mesh.indexBuffer().data.constData();

        physx::PxDefaultMemoryOutputStream buf;
        if (!cooking->cookTriangleMesh(triangleDesc, buf, &result)) {
            std::cerr << "Could not cook triangle mesh.";
            return false;
        }

        auto size = buf.getSize();
        auto *data = buf.getData();
        physx::PxDefaultMemoryInputData input(data, size);

        QString output = QFileInfo(inputPath).baseName() + QString(".cooked.tri");
        auto outputFile = QFile(output);

        if (!outputFile.open(QIODevice::WriteOnly)) {
            std::cerr << "Could not open " << output.toStdString() << "for writing.";
            return false;
        }

        outputFile.write(reinterpret_cast<char *>(buf.getData()), buf.getSize());
        outputFile.close();

        std::cout << "Success: wrote triangle mesh '" << output.toStdString() << "'" << std::endl;
    }

    { // Convex mesh
        physx::PxConvexMeshCookingResult::Enum result;
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

        physx::PxDefaultMemoryOutputStream buf;
        if (!cooking->cookConvexMesh(convexDesc, buf, &result)) {
            std::cerr << "Could not cook convex mesh.";
            return false;
        }

        auto size = buf.getSize();
        auto *data = buf.getData();
        physx::PxDefaultMemoryInputData input(data, size);

        QString output = QFileInfo(inputPath).baseName() + QString(".cooked.cvx");
        auto outputFile = QFile(output);

        if (!outputFile.open(QIODevice::WriteOnly)) {
            std::cerr << "Could not open " << output.toStdString() << "for writing.";
            return false;
        }

        outputFile.write(reinterpret_cast<char *>(buf.getData()), buf.getSize());
        outputFile.close();

        std::cout << "Success: wrote convex mesh '" << output.toStdString() << "'" << std::endl;
    }

    return true;
}

bool cookHeightfield(const char *inputPath, QImage &heightMap)
{
    physx::PxDefaultErrorCallback defaultErrorCallback;
    physx::PxDefaultAllocator defaultAllocatorCallback;
    auto foundation = PxCreateFoundation(PX_PHYSICS_VERSION, defaultAllocatorCallback, defaultErrorCallback);
    auto cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, physx::PxCookingParams(physx::PxTolerancesScale()));

    int numRows = heightMap.height();
    int numCols = heightMap.width();

    auto samples = reinterpret_cast<physx::PxHeightFieldSample *>(malloc(sizeof(physx::PxHeightFieldSample) * (numRows * numCols)));
    for (int i = 0; i < numCols; i++) {
        for (int j = 0; j < numRows; j++) {
            float f = heightMap.pixelColor(i, j).valueF() - 0.5;
            samples[i * numRows + j] = { qint16(0xffff * f), 0, 0 };
        }
    }

    physx::PxHeightFieldDesc hfDesc;
    hfDesc.format = physx::PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns = numRows;
    hfDesc.nbRows = numCols;
    hfDesc.samples.data = samples;
    hfDesc.samples.stride = sizeof(physx::PxHeightFieldSample);

    physx::PxDefaultMemoryOutputStream buf;
    if (!(numRows && numCols && cooking->cookHeightField(hfDesc, buf))) {
        std::cerr << "Could not create height field from " << inputPath << std::endl;
        return false;
    }

    QString output = QFileInfo(inputPath).baseName() + QString(".cooked.hf");
    auto outputFile = QFile(output);

    if (!outputFile.open(QIODevice::WriteOnly)) {
        std::cerr << "Could not open " << output.toStdString() << "for writing.";
        return false;
    }

    outputFile.write(reinterpret_cast<char *>(buf.getData()), buf.getSize());
    outputFile.close();
    std::cout << "Success: wrote height field '" << output.toStdString() << "'" << std::endl;

    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        qDebug() << "Invalid number of arguments provided. Usage: cooker input.mesh";
        return -1;
    }

    const char *inputPath = argv[1];

    QFile *file = new QFile(inputPath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        std::cerr << "Could not open input file '" << inputPath << "'" << std::endl;
        return -1;
    }

    QImage image;
    QSSGMesh::Mesh mesh;
    if (tryReadImage(inputPath, image)) {
        if (!cookHeightfield(inputPath, image))
            return -1;
    } else if (tryReadMesh(file, mesh)) {
        if (!cookMeshes(inputPath, mesh))
            return -1;
    } else {
        std::cerr << "Failed to read mesh or image from file '" << inputPath << "'" << std::endl;
        return -1;
    }

    return 0;
}
