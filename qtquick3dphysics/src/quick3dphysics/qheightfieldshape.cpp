// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcacheutils_p.h"
#include "qheightfieldshape_p.h"

#include <QFileInfo>
#include <QImage>
#include <QQmlContext>
#include <QQmlFile>
#include <QtQuick3D/QQuick3DGeometry>
#include <extensions/PxExtensionsAPI.h>

//########################################################################################
// NOTE:
// Triangle mesh, heightfield or plane geometry shapes configured as eSIMULATION_SHAPE are
// not supported for non-kinematic PxRigidDynamic instances.
//########################################################################################

#include "foundation/PxVec3.h"
//#include "cooking/PxTriangleMeshDesc.h"
#include "extensions/PxDefaultStreams.h"
#include "geometry/PxHeightField.h"
#include "geometry/PxHeightFieldDesc.h"

#include "qphysicsworld_p.h"

QT_BEGIN_NAMESPACE

// TODO: Unify with QQuick3DPhysicsMeshManager??? It's the same basic logic,
// but we're using images instead of meshes.

class QQuick3DPhysicsHeightField
{
public:
    QQuick3DPhysicsHeightField(const QString &qmlSource);
    ~QQuick3DPhysicsHeightField();

    void ref() { ++refCount; }
    int deref() { return --refCount; }
    physx::PxHeightFieldSample *getSamples();
    physx::PxHeightField *heightField();

    int rows() const;
    int columns() const;

private:
    QString m_sourcePath;
    physx::PxHeightFieldSample *m_samples = nullptr;
    physx::PxHeightField *m_heightField = nullptr;
    int m_rows = 0;
    int m_columns = 0;
    int refCount = 0;
};

class QQuick3DPhysicsHeightFieldManager
{
public:
    static QQuick3DPhysicsHeightField *getHeightField(const QUrl &source,
                                                      const QObject *contextObject);
    static void releaseHeightField(QQuick3DPhysicsHeightField *heightField);

private:
    static QHash<QString, QQuick3DPhysicsHeightField *> heightFieldHash;
};

QHash<QString, QQuick3DPhysicsHeightField *> QQuick3DPhysicsHeightFieldManager::heightFieldHash;

QQuick3DPhysicsHeightField *
QQuick3DPhysicsHeightFieldManager::getHeightField(const QUrl &source, const QObject *contextObject)
{
    const QQmlContext *context = qmlContext(contextObject);

    const auto resolvedUrl = context ? context->resolvedUrl(source) : source;
    const auto qmlSource = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    auto *heightField = heightFieldHash.value(qmlSource);
    if (!heightField) {
        heightField = new QQuick3DPhysicsHeightField(qmlSource);
        heightFieldHash[qmlSource] = heightField;
    }
    heightField->ref();
    return heightField;
}

void QQuick3DPhysicsHeightFieldManager::releaseHeightField(QQuick3DPhysicsHeightField *heightField)
{
    if (heightField->deref() == 0) {
        qCDebug(lcQuick3dPhysics()) << "deleting height field" << heightField;
        erase_if(heightFieldHash,
                 [heightField](std::pair<const QString &, QQuick3DPhysicsHeightField *&> h) {
                     return h.second == heightField;
                 });
        delete heightField;
    }
}

QQuick3DPhysicsHeightField::QQuick3DPhysicsHeightField(const QString &qmlSource)
    : m_sourcePath(qmlSource)
{
}

QQuick3DPhysicsHeightField::~QQuick3DPhysicsHeightField()
{
    free(m_samples);
}

physx::PxHeightFieldSample *QQuick3DPhysicsHeightField::getSamples()
{
    if (!m_samples && !m_sourcePath.isEmpty()) {
        QImage heightMap(m_sourcePath);

        m_rows = heightMap.height();
        m_columns = heightMap.width();
        int numRows = m_rows;
        int numCols = m_columns;

        auto samples = reinterpret_cast<physx::PxHeightFieldSample *>(
                malloc(sizeof(physx::PxHeightFieldSample) * (numRows * numCols)));
        for (int i = 0; i < numCols; i++)
            for (int j = 0; j < numRows; j++) {
                float f = heightMap.pixelColor(i, j).valueF() - 0.5;
                // qDebug() << i << j << f;
                samples[i * numRows + j] = { qint16(0xffff * f), 0,
                                             0 }; //{qint16(i%3*2 + j), 0, 0};
            }
        m_samples = samples;
    }
    return m_samples;
}

physx::PxHeightField *QQuick3DPhysicsHeightField::heightField()
{
    if (m_heightField)
        return m_heightField;

    physx::PxPhysics *thePhysics = QPhysicsWorld::getPhysics();
    if (thePhysics == nullptr)
        return nullptr;

    m_heightField = QCacheUtils::readCachedHeightField(m_sourcePath, *thePhysics);
    if (m_heightField != nullptr) {
        m_rows = m_heightField->getNbRows();
        m_columns = m_heightField->getNbColumns();
        return m_heightField;
    }

    m_heightField = QCacheUtils::readCookedHeightField(m_sourcePath, *thePhysics);
    if (m_heightField != nullptr) {
        m_rows = m_heightField->getNbRows();
        m_columns = m_heightField->getNbColumns();
        return m_heightField;
    }

    getSamples();
    int numRows = m_rows;
    int numCols = m_columns;
    auto samples = m_samples;

    physx::PxHeightFieldDesc hfDesc;
    hfDesc.format = physx::PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns = numRows;
    hfDesc.nbRows = numCols;
    hfDesc.samples.data = samples;
    hfDesc.samples.stride = sizeof(physx::PxHeightFieldSample);

    physx::PxDefaultMemoryOutputStream buf;

    const auto cooking = QPhysicsWorld::getCooking();
    if (numRows && numCols && cooking && cooking->cookHeightField(hfDesc, buf)) {
        auto size = buf.getSize();
        auto *data = buf.getData();
        physx::PxDefaultMemoryInputData input(data, size);
        m_heightField = thePhysics->createHeightField(input);
        qCDebug(lcQuick3dPhysics) << "created height field" << m_heightField << numCols << numRows
                                  << "from" << m_sourcePath;
        QCacheUtils::writeCachedHeightField(m_sourcePath, buf);
    } else {
        qCWarning(lcQuick3dPhysics) << "Could not create height field from" << m_sourcePath;
    }

    return m_heightField;
}

int QQuick3DPhysicsHeightField::rows() const
{
    return m_rows;
}

int QQuick3DPhysicsHeightField::columns() const
{
    return m_columns;
}

/*!
    \qmltype HeightFieldShape
    \inqmlmodule QtQuick3D.Physics
    \inherits CollisionShape
    \since 6.4
    \brief A collision shape where the elevation is defined by a height map.

    The HeightFieldShape type defines a physical surface where the height is determined by
    the \l {QColor#The HSV Color Model}{value} of the pixels of the \l {source} image. The
    x-axis of the image is mapped to the positive x-axis of the scene, and the y-axis of the
    image is mapped to the negative z-axis of the scene. A typical use case is to represent
    natural terrain.

    Objects that are controlled by the physics simulation cannot use HeightFieldShape: It can only
    be used with \l StaticRigidBody and \l {DynamicRigidBody::isKinematic}{kinematic bodies}.

    \l [QtQuick3D]{HeightFieldGeometry}{QtQuick3D.Helpers.HeightFieldGeometry} is API compatible
    with the HeightFieldShape type, and can be used to show the height field visually. To
    improve performance, use a lower resolution version of the height map for the HeightFieldShape:
    As long as the \l{extents} and the image aspect ratio are the same, the physics body and the
    visual item will overlap.

    \sa {Qt Quick 3D Physics Shapes and Bodies}{Shapes and Bodies overview documentation}
*/

/*!
    \qmlproperty vector3d HeightFieldShape::extents
    This property defines the extents of the height field. The default value
    is \c{(100, 100, 100)} when the heightMap is square. If the heightMap is
    non-square, the default value is reduced along the x- or z-axis, so the height
    field will keep the aspect ratio of the image.
*/

/*!
    \qmlproperty url HeightFieldShape::source
    This property defines the location of the heightMap file.

    Internally, HeightFieldShape converts the height map image to an optimized data structure. This
    conversion can be done in advance. See the \l{Qt Quick 3D Physics Cooking}{cooking overview
    documentation} for details.

*/

QHeightFieldShape::QHeightFieldShape() = default;

QHeightFieldShape::~QHeightFieldShape()
{
    delete m_heightFieldGeometry;
    if (m_heightField)
        QQuick3DPhysicsHeightFieldManager::releaseHeightField(m_heightField);
}

physx::PxGeometry *QHeightFieldShape::getPhysXGeometry()
{
    if (m_dirtyPhysx || m_scaleDirty || !m_heightFieldGeometry) {
        updatePhysXGeometry();
    }
    return m_heightFieldGeometry;
}

void QHeightFieldShape::updatePhysXGeometry()
{
    delete m_heightFieldGeometry;
    m_heightFieldGeometry = nullptr;
    if (!m_heightField)
        return;

    auto *hf = m_heightField->heightField();
    float rows = m_heightField->rows();
    float cols = m_heightField->columns();
    updateExtents();
    if (hf && cols > 1 && rows > 1) {
        QVector3D scaledExtents = m_extents * sceneScale();
        m_heightFieldGeometry = new physx::PxHeightFieldGeometry(
                hf, physx::PxMeshGeometryFlags(), scaledExtents.y() / 0x10000,
                scaledExtents.x() / (cols - 1), scaledExtents.z() / (rows - 1));
        m_hfOffset = { -scaledExtents.x() / 2, 0, -scaledExtents.z() / 2 };

        qCDebug(lcQuick3dPhysics) << "created height field geom" << m_heightFieldGeometry << "scale"
                                  << scaledExtents << m_heightField->columns()
                                  << m_heightField->rows();
    }
    m_dirtyPhysx = false;
}

void QHeightFieldShape::updateExtents()
{
    if (!m_heightField || m_extentsSetExplicitly)
        return;
    int numRows = m_heightField->rows();
    int numCols = m_heightField->columns();
    auto prevExt = m_extents;
    if (numRows == numCols) {
        m_extents = { 100, 100, 100 };
    } else if (numRows < numCols) {
        float f = float(numRows) / float(numCols);
        m_extents = { 100.f, 100.f, 100.f * f };
    } else {
        float f = float(numCols) / float(numRows);
        m_extents = { 100.f * f, 100.f, 100.f };
    }
    if (m_extents != prevExt) {
        emit extentsChanged();
    }
}

const QUrl &QHeightFieldShape::source() const
{
    return m_heightMapSource;
}

void QHeightFieldShape::setSource(const QUrl &newSource)
{
    if (m_heightMapSource == newSource)
        return;
    m_heightMapSource = newSource;

    m_heightField = QQuick3DPhysicsHeightFieldManager::getHeightField(m_heightMapSource, this);

    m_dirtyPhysx = true;

    emit needsRebuild(this);
    emit sourceChanged();
}

const QVector3D &QHeightFieldShape::extents() const
{
    return m_extents;
}

void QHeightFieldShape::setExtents(const QVector3D &newExtents)
{
    m_extentsSetExplicitly = true;
    if (m_extents == newExtents)
        return;
    m_extents = newExtents;

    m_dirtyPhysx = true;

    emit needsRebuild(this);
    emit extentsChanged();
}

QT_END_NAMESPACE
