// Copyright (C) 2020 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DCORE_QGEOMETRYVIEW_H
#define QT3DCORE_QGEOMETRYVIEW_H

#include <Qt3DCore/qnode.h>
#include <Qt3DCore/qgeometry.h>
#include <Qt3DCore/qt3dcore_global.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

class QGeometryViewPrivate;
class QGeometryFactory;

typedef QSharedPointer<QGeometryFactory> QGeometryFactoryPtr;

class Q_3DCORESHARED_EXPORT QGeometryView : public Qt3DCore::QNode
{
    Q_OBJECT
    Q_PROPERTY(int instanceCount READ instanceCount WRITE setInstanceCount NOTIFY instanceCountChanged)
    Q_PROPERTY(int vertexCount READ vertexCount WRITE setVertexCount NOTIFY vertexCountChanged)
    Q_PROPERTY(int indexOffset READ indexOffset WRITE setIndexOffset NOTIFY indexOffsetChanged)
    Q_PROPERTY(int firstInstance READ firstInstance WRITE setFirstInstance NOTIFY firstInstanceChanged)
    Q_PROPERTY(int firstVertex READ firstVertex WRITE setFirstVertex NOTIFY firstVertexChanged)
    Q_PROPERTY(int indexBufferByteOffset READ indexBufferByteOffset WRITE setIndexBufferByteOffset NOTIFY indexBufferByteOffsetChanged)
    Q_PROPERTY(int restartIndexValue READ restartIndexValue WRITE setRestartIndexValue NOTIFY restartIndexValueChanged)
    Q_PROPERTY(int verticesPerPatch READ verticesPerPatch WRITE setVerticesPerPatch NOTIFY verticesPerPatchChanged)
    Q_PROPERTY(bool primitiveRestartEnabled READ primitiveRestartEnabled WRITE setPrimitiveRestartEnabled NOTIFY primitiveRestartEnabledChanged)
    Q_PROPERTY(Qt3DCore::QGeometry* geometry READ geometry WRITE setGeometry NOTIFY geometryChanged)
    Q_PROPERTY(PrimitiveType primitiveType READ primitiveType WRITE setPrimitiveType NOTIFY primitiveTypeChanged)

public:
    explicit QGeometryView(Qt3DCore::QNode *parent = nullptr);
    ~QGeometryView();

    enum PrimitiveType {
        Points = 0x0000,
        Lines = 0x0001,
        LineLoop = 0x0002,
        LineStrip = 0x0003,
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        TriangleFan = 0x0006,
        LinesAdjacency = 0x000A,
        TrianglesAdjacency = 0x000C,
        LineStripAdjacency = 0x000B,
        TriangleStripAdjacency = 0x000D,
        Patches = 0x000E
    };
    Q_ENUM(PrimitiveType) // LCOV_EXCL_LINE

    // how to figure out index count and all the fancy stuff that QMeshData provides for us?
    // also how to figure out which attribute(s?) hold the indices?

    int instanceCount() const;
    int vertexCount() const;
    int indexOffset() const;
    int firstInstance() const;
    int firstVertex() const;
    int indexBufferByteOffset() const;
    int restartIndexValue() const;
    int verticesPerPatch() const;
    bool primitiveRestartEnabled() const;
    QGeometry *geometry() const;
    PrimitiveType primitiveType() const;

    QGeometryFactoryPtr geometryFactory() const;
    void setGeometryFactory(const QGeometryFactoryPtr &factory);

public Q_SLOTS:
    void setInstanceCount(int instanceCount);
    void setVertexCount(int vertexCount);
    void setIndexOffset(int indexOffset);
    void setFirstInstance(int firstInstance);
    void setFirstVertex(int firstVertex);
    void setIndexBufferByteOffset(int offset);
    void setRestartIndexValue(int index);
    void setVerticesPerPatch(int verticesPerPatch);
    void setPrimitiveRestartEnabled(bool enabled);
    void setGeometry(QGeometry *geometry);
    void setPrimitiveType(PrimitiveType primitiveType);

Q_SIGNALS:
    void instanceCountChanged(int instanceCount);
    void vertexCountChanged(int vertexCount);
    void indexOffsetChanged(int indexOffset);
    void firstInstanceChanged(int firstInstance);
    void firstVertexChanged(int firstVertex);
    void indexBufferByteOffsetChanged(int offset);
    void restartIndexValueChanged(int restartIndexValue);
    void verticesPerPatchChanged(int verticesPerPatch);
    void primitiveRestartEnabledChanged(bool primitiveRestartEnabled);
    void geometryChanged(QGeometry *geometry);
    void primitiveTypeChanged(PrimitiveType primitiveType);

protected:
    explicit QGeometryView(QGeometryViewPrivate &dd, Qt3DCore::QNode *parent = nullptr);

private:
    Q_DECLARE_PRIVATE(QGeometryView)
};

} // namespace Qt3DCore

QT_END_NAMESPACE

#endif // QT3DCORE_QGEOMETRYVIEW_H
