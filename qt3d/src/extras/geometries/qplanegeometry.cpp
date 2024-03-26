// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplanegeometry.h"
#include "qplanegeometry_p.h"

#include <Qt3DCore/qattribute.h>
#include <Qt3DCore/qbuffer.h>

#include <limits>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace  Qt3DExtras {

namespace {

QByteArray createPlaneVertexData(float w, float h, const QSize &resolution, bool mirrored)
{
    Q_ASSERT(w > 0.0f);
    Q_ASSERT(h > 0.0f);
    Q_ASSERT(resolution.width() >= 2);
    Q_ASSERT(resolution.height() >= 2);

    const int nVerts = resolution.width() * resolution.height();

    // Populate a buffer with the interleaved per-vertex data with
    // vec3 pos, vec2 texCoord, vec3 normal, vec4 tangent
    const quint32 elementSize = 3 + 2 + 3 + 4;
    const quint32 stride = elementSize * sizeof(float);
    QByteArray bufferBytes;
    bufferBytes.resize(stride * nVerts);
    float* fptr = reinterpret_cast<float*>(bufferBytes.data());

    const float x0 = -w / 2.0f;
    const float z0 = -h / 2.0f;
    const float dx = w / (resolution.width() - 1);
    const float dz = h / (resolution.height() - 1);
    const float du = 1.0 / (resolution.width() - 1);
    const float dv = 1.0 / (resolution.height() - 1);

    // Iterate over z
    for (int j = 0; j < resolution.height(); ++j) {
        const float z = z0 + static_cast<float>(j) * dz;
        const float v = static_cast<float>(j) * dv;

        // Iterate over x
        for (int i = 0; i < resolution.width(); ++i) {
            const float x = x0 + static_cast<float>(i) * dx;
            const float u = static_cast<float>(i) * du;

            // position
            *fptr++ = x;
            *fptr++ = 0.0;
            *fptr++ = z;

            // texture coordinates
            *fptr++ = u;
            *fptr++ = mirrored ? 1.0f - v : v;

            // normal
            *fptr++ = 0.0f;
            *fptr++ = 1.0f;
            *fptr++ = 0.0f;

            // tangent
            *fptr++ = 1.0f;
            *fptr++ = 0.0f;
            *fptr++ = 0.0f;
            *fptr++ = 1.0f;
        }
    }

    return bufferBytes;
}

QByteArray createPlaneIndexData(const QSize &resolution)
{
    // Create the index data. 2 triangles per rectangular face
    const int faces = 2 * (resolution.width() - 1) * (resolution.height() - 1);
    const qsizetype indices = 3 * faces;
    Q_ASSERT(indices < std::numeric_limits<quint16>::max());
    QByteArray indexBytes;
    indexBytes.resize(indices * sizeof(quint16));
    quint16* indexPtr = reinterpret_cast<quint16*>(indexBytes.data());

    // Iterate over z
    for (int j = 0; j < resolution.height() - 1; ++j) {
        const int rowStartIndex = j * resolution.width();
        const int nextRowStartIndex = (j + 1) * resolution.width();

        // Iterate over x
        for (int i = 0; i < resolution.width() - 1; ++i) {
            // Split quad into two triangles
            *indexPtr++ = rowStartIndex + i;
            *indexPtr++ = nextRowStartIndex + i;
            *indexPtr++ = rowStartIndex + i + 1;

            *indexPtr++ = nextRowStartIndex + i;
            *indexPtr++ = nextRowStartIndex + i + 1;
            *indexPtr++ = rowStartIndex + i + 1;
        }
    }

    return indexBytes;
}

} // anonymous

/*!
 * \qmltype PlaneGeometry
 * \instantiates Qt3DExtras::QPlaneGeometry
 * \inqmlmodule Qt3D.Extras
 * \brief PlaneGeometry allows creation of a plane in 3D space.
 *
 * The PlaneGeometry type is most commonly used internally by the PlaneMesh type
 * but can also be used in custom GeometryRenderer types.
 */

/*!
 * \qmlproperty real PlaneGeometry::width
 *
 * Holds the plane width.
 */

/*!
 * \qmlproperty real PlaneGeometry::height
 *
 * Holds the plane height.
 */

/*!
 * \qmlproperty size PlaneGeometry::resolution
 *
 * Holds the plane resolution.
 */

/*!
 * \qmlproperty bool PlaneGeometry::mirrored
 * \since 5.9
 *
 * Controls if the UV coordinates of the plane should be flipped vertically.
 */

/*!
 * \qmlproperty Attribute PlaneGeometry::positionAttribute
 *
 * Holds the geometry position attribute.
 */

/*!
 * \qmlproperty Attribute PlaneGeometry::normalAttribute
 *
 * Holds the geometry normal attribute.
 */

/*!
 * \qmlproperty Attribute PlaneGeometry::texCoordAttribute
 *
 * Holds the geometry texture coordinate attribute.
 */

/*!
 * \qmlproperty Attribute PlaneGeometry::tangentAttribute
 *
 * Holds the geometry tangent attribute.
 */

/*!
 * \qmlproperty Attribute PlaneGeometry::indexAttribute
 *
 * Holds the geometry index attribute.
 */

/*!
 * \class Qt3DExtras::QPlaneGeometry
 * \ingroup qt3d-extras-geometries
 * \inheaderfile Qt3DExtras/QPlaneGeometry
 * \inmodule Qt3DExtras
 * \brief The QPlaneGeometry class allows creation of a plane in 3D space.
 * \since 5.7
 * \ingroup geometries
 *
 * The QPlaneGeometry class is most commonly used internally by the QPlaneMesh
 * but can also be used in custom Qt3DRender::QGeometryRenderer subclasses.
 */

/*!
 * \fn Qt3DExtras::QPlaneGeometry::QPlaneGeometry(QNode *parent)
 *
 * Constructs a new QPlaneGeometry with \a parent.
 */
QPlaneGeometry::QPlaneGeometry(QPlaneGeometry::QNode *parent)
    : QGeometry(*new QPlaneGeometryPrivate(), parent)
{
    Q_D(QPlaneGeometry);
    d->init();
}

/*!
 * \internal
 */
QPlaneGeometry::QPlaneGeometry(QPlaneGeometryPrivate &dd, QNode *parent)
    : QGeometry(dd, parent)
{
    Q_D(QPlaneGeometry);
    d->init();
}

/*!
 * \internal
 */
QPlaneGeometry::~QPlaneGeometry()
{
}

/*!
 * Updates vertices based on mesh resolution, width, and height properties.
 */
void QPlaneGeometry::updateVertices()
{
    Q_D(QPlaneGeometry);
    const int nVerts = d->m_meshResolution.width() * d->m_meshResolution.height();

    d->m_positionAttribute->setCount(nVerts);
    d->m_normalAttribute->setCount(nVerts);
    d->m_texCoordAttribute->setCount(nVerts);
    d->m_tangentAttribute->setCount(nVerts);
    d->m_vertexBuffer->setData(d->generateVertexData());
}

/*!
 * Updates indices based on mesh resolution.
 */
void QPlaneGeometry::updateIndices()
{
    Q_D(QPlaneGeometry);
    const int faces = 2 * (d->m_meshResolution.width() - 1) * (d->m_meshResolution.height() - 1);
    // Each primitive has 3 vertices
    d->m_indexAttribute->setCount(faces * 3);
    d->m_indexBuffer->setData(d->generateIndexData());

}

void QPlaneGeometry::setResolution(const QSize &resolution)
{
    Q_D(QPlaneGeometry);
    if (d->m_meshResolution == resolution)
        return;
    d->m_meshResolution = resolution;
    updateVertices();
    updateIndices();
    emit resolutionChanged(resolution);
}

void QPlaneGeometry::setWidth(float width)
{
    Q_D(QPlaneGeometry);
    if (width == d->m_width)
        return;
    d->m_width = width;
    updateVertices();
    emit widthChanged(width);
}

void QPlaneGeometry::setHeight(float height)
{
    Q_D(QPlaneGeometry);
    if (height == d->m_height)
        return;
    d->m_height = height;
    updateVertices();
    emit heightChanged(height);
}

void QPlaneGeometry::setMirrored(bool mirrored)
{
    Q_D(QPlaneGeometry);
    if (mirrored == d->m_mirrored)
        return;
    d->m_mirrored = mirrored;
    updateVertices();
    emit mirroredChanged(mirrored);
}

/*!
 * \property QPlaneGeometry::resolution
 *
 * Holds the plane resolution.
 */
QSize QPlaneGeometry::resolution() const
{
    Q_D(const QPlaneGeometry);
    return d->m_meshResolution;
}

/*!
 * \property QPlaneGeometry::width
 *
 * Holds the plane width.
 */
float QPlaneGeometry::width() const
{
    Q_D(const QPlaneGeometry);
    return d->m_width;
}

/*!
 * \property QPlaneGeometry::height
 *
 * Holds the plane height.
 */
float QPlaneGeometry::height() const
{
    Q_D(const QPlaneGeometry);
    return d->m_height;
}

/*!
 * \property QPlaneGeometry::mirrored
 * \since 5.9
 *
 * Controls if the UV coordinates of the plane should be flipped vertically.
 */
bool QPlaneGeometry::mirrored() const
{
    Q_D(const QPlaneGeometry);
    return d->m_mirrored;
}

/*!
 * \property QPlaneGeometry::positionAttribute
 *
 * Holds the geometry position attribute.
 */
QAttribute *QPlaneGeometry::positionAttribute() const
{
    Q_D(const QPlaneGeometry);
    return d->m_positionAttribute;
}

/*!
 * \property QPlaneGeometry::normalAttribute
 *
 * Holds the geometry normal attribute.
 */
QAttribute *QPlaneGeometry::normalAttribute() const
{
    Q_D(const QPlaneGeometry);
    return d->m_normalAttribute;
}

/*!
 * \property QPlaneGeometry::texCoordAttribute
 *
 * Holds the geometry texture coordinate attribute.
 */
QAttribute *QPlaneGeometry::texCoordAttribute() const
{
    Q_D(const QPlaneGeometry);
    return d->m_texCoordAttribute;
}

/*!
 * \property QPlaneGeometry::tangentAttribute
 *
 * Holds the geometry tangent attribute.
 */
QAttribute *QPlaneGeometry::tangentAttribute() const
{
    Q_D(const QPlaneGeometry);
    return d->m_tangentAttribute;
}

/*!
 * \property QPlaneGeometry::indexAttribute
 *
 * Holds the geometry index attribute.
 */
QAttribute *QPlaneGeometry::indexAttribute() const
{
    Q_D(const QPlaneGeometry);
    return d->m_indexAttribute;
}

QPlaneGeometryPrivate::QPlaneGeometryPrivate()
    : QGeometryPrivate()
    , m_width(1.0f)
    , m_height(1.0f)
    , m_meshResolution(QSize(2, 2))
    , m_mirrored(false)
    , m_positionAttribute(nullptr)
    , m_normalAttribute(nullptr)
    , m_texCoordAttribute(nullptr)
    , m_tangentAttribute(nullptr)
    , m_indexAttribute(nullptr)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
{
}

void QPlaneGeometryPrivate::init()
{
    Q_Q(QPlaneGeometry);
    m_positionAttribute = new QAttribute(q);
    m_normalAttribute = new QAttribute(q);
    m_texCoordAttribute = new QAttribute(q);
    m_tangentAttribute = new QAttribute(q);
    m_indexAttribute = new QAttribute(q);
    m_vertexBuffer = new Qt3DCore::QBuffer(q);
    m_indexBuffer = new Qt3DCore::QBuffer(q);

    const int nVerts = m_meshResolution.width() * m_meshResolution.height();
    const int stride = (3 + 2 + 3 + 4) * sizeof(float);
    const int faces = 2 * (m_meshResolution.width() - 1) * (m_meshResolution.height() - 1);

    m_positionAttribute->setName(QAttribute::defaultPositionAttributeName());
    m_positionAttribute->setVertexBaseType(QAttribute::Float);
    m_positionAttribute->setVertexSize(3);
    m_positionAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_positionAttribute->setBuffer(m_vertexBuffer);
    m_positionAttribute->setByteStride(stride);
    m_positionAttribute->setCount(nVerts);

    m_texCoordAttribute->setName(QAttribute::defaultTextureCoordinateAttributeName());
    m_texCoordAttribute->setVertexBaseType(QAttribute::Float);
    m_texCoordAttribute->setVertexSize(2);
    m_texCoordAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_texCoordAttribute->setBuffer(m_vertexBuffer);
    m_texCoordAttribute->setByteStride(stride);
    m_texCoordAttribute->setByteOffset(3 * sizeof(float));
    m_texCoordAttribute->setCount(nVerts);

    m_normalAttribute->setName(QAttribute::defaultNormalAttributeName());
    m_normalAttribute->setVertexBaseType(QAttribute::Float);
    m_normalAttribute->setVertexSize(3);
    m_normalAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_normalAttribute->setBuffer(m_vertexBuffer);
    m_normalAttribute->setByteStride(stride);
    m_normalAttribute->setByteOffset(5 * sizeof(float));
    m_normalAttribute->setCount(nVerts);

    m_tangentAttribute->setName(QAttribute::defaultTangentAttributeName());
    m_tangentAttribute->setVertexBaseType(QAttribute::Float);
    m_tangentAttribute->setVertexSize(4);
    m_tangentAttribute->setAttributeType(QAttribute::VertexAttribute);
    m_tangentAttribute->setBuffer(m_vertexBuffer);
    m_tangentAttribute->setByteStride(stride);
    m_tangentAttribute->setByteOffset(8 * sizeof(float));
    m_tangentAttribute->setCount(nVerts);

    m_indexAttribute->setAttributeType(QAttribute::IndexAttribute);
    m_indexAttribute->setVertexBaseType(QAttribute::UnsignedShort);
    m_indexAttribute->setBuffer(m_indexBuffer);

    // Each primitive has 3 vertives
    m_indexAttribute->setCount(faces * 3);

    m_vertexBuffer->setData(generateVertexData());
    m_indexBuffer->setData(generateIndexData());

    q->addAttribute(m_positionAttribute);
    q->addAttribute(m_texCoordAttribute);
    q->addAttribute(m_normalAttribute);
    q->addAttribute(m_tangentAttribute);
    q->addAttribute(m_indexAttribute);
}

QByteArray QPlaneGeometryPrivate::generateVertexData() const
{
    return createPlaneVertexData(m_width, m_height, m_meshResolution, m_mirrored);
}

QByteArray QPlaneGeometryPrivate::generateIndexData() const
{
    return createPlaneIndexData(m_meshResolution);
}

} //  Qt3DExtras

QT_END_NAMESPACE

#include "moc_qplanegeometry.cpp"
