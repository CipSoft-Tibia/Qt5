// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "uniform_p.h"
#include "qabstracttexture.h"
#include <QVariant>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {

namespace {

const int qNodeIdTypeId = qMetaTypeId<Qt3DCore::QNodeId>();
const int qVector3DTypeId = qMetaTypeId<Vector3D>();
const int qVector4DTypeId = qMetaTypeId<Vector4D>();
const int qMatrix4x4TypeId = qMetaTypeId<Matrix4x4>();

// glUniform*fv/glUniform*iv/glUniform*uiv -> only handles sizeof(float)/sizeof(int)
int byteSizeForMetaType(int type)
{
    if (type == qMatrix4x4TypeId)
        return sizeof(Matrix4x4);
    if (type == qVector3DTypeId)
        return sizeof(Vector3D);
    if (type == qVector4DTypeId)
        return sizeof(Vector4D);
    if (type == qNodeIdTypeId)
        return sizeof(Qt3DCore::QNodeId);

    switch (type) {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
    case QMetaType::LongLong:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Char:
    case QMetaType::UChar:
        return sizeof(int);

    case QMetaType::Float:
    case QMetaType::Double: // Assumes conversion to float
        return sizeof(float);

    case QMetaType::QPoint:
    case QMetaType::QSize:
        return 2 * sizeof(int);

    case QMetaType::QRect:
        return 4 * sizeof(int);

    case QMetaType::QPointF:
    case QMetaType::QSizeF:
    case QMetaType::QVector2D:
        return 2 * sizeof(float);

    case QMetaType::QVector3D:
        return 3 * sizeof(float);

    case QMetaType::QRectF:
    case QMetaType::QVector4D:
    case QMetaType::QColor:
        return 4 * sizeof(float);
    case QMetaType::QMatrix4x4:
        return 16 * sizeof(float);

    default:
        Q_UNREACHABLE_RETURN(-1);
    }
}

} // anonymous

UniformValue UniformValue::fromVariant(const QVariant &variant)
{
    // Texture/Buffer case
    const int type = variant.userType();

    if (type == qNodeIdTypeId)
        return UniformValue(variant.value<Qt3DCore::QNodeId>());

    if (type == qMatrix4x4TypeId)
        return UniformValue(variant.value<Matrix4x4>());

    if (type == qVector3DTypeId)
        return UniformValue(variant.value<Vector3D>());

    if (type == qVector4DTypeId)
        return UniformValue(variant.value<Vector4D>());

    UniformValue v;
    switch (type) {
    case QMetaType::Bool:
        v.data<bool>()[0] = variant.toBool();
        break;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
    case QMetaType::UShort:
    case QMetaType::Char:
    case QMetaType::UChar:
        v.data<int>()[0] = variant.toInt();
        v.m_storedType = Int;
        break;
    case QMetaType::Float:
    case QMetaType::Double: // Convert double to floats
        v.m_data[0] = variant.toFloat();
        break;
    case QMetaType::QPoint: {
        const QPoint p = variant.toPoint();
        v.data<int>()[0] = p.x();
        v.data<int>()[1] = p.y();
        break;
    }
    case QMetaType::QSize: {
        const QSize s = variant.toSize();
        v.data<int>()[0] = s.width();
        v.data<int>()[1] = s.height();
        break;
    }
    case QMetaType::QRect: {
        const QRect r = variant.toRect();
        v.data<int>()[0] = r.x();
        v.data<int>()[1] = r.y();
        v.data<int>()[2] = r.width();
        v.data<int>()[3] = r.height();
        break;
    }
    case QMetaType::QSizeF: {
        const QSizeF s = variant.toSize();
        v.m_data[0] = s.width();
        v.m_data[1] = s.height();
        break;
    }
    case QMetaType::QPointF: {
        const QPointF p = variant.toPointF();
        v.m_data[0] = p.x();
        v.m_data[1] = p.y();
        break;
    }
    case QMetaType::QRectF: {
        const QRectF r = variant.toRect();
        v.m_data[0] = r.x();
        v.m_data[1] = r.y();
        v.m_data[2] = r.width();
        v.m_data[3] = r.height();
        break;
    }
    case QMetaType::QVector2D: {
        const QVector2D vec2 = variant.value<QVector2D>();
        v.m_data[0] = vec2.x();
        v.m_data[1] = vec2.y();
        break;
    }
    case QMetaType::QVector3D: {
        const QVector3D vec3 = variant.value<QVector3D>();
        v.m_data[0] = vec3.x();
        v.m_data[1] = vec3.y();
        v.m_data[2] = vec3.z();
        break;
    }
    case QMetaType::QVector4D: {
        const QVector4D vec4 = variant.value<QVector4D>();
        v.m_data[0] = vec4.x();
        v.m_data[1] = vec4.y();
        v.m_data[2] = vec4.z();
        v.m_data[3] = vec4.w();
        break;
    }
    case QMetaType::QColor: {
        const QColor col = variant.value<QColor>();
        v.m_data[0] = col.redF();
        v.m_data[1] = col.greenF();
        v.m_data[2] = col.blueF();
        v.m_data[3] = col.alphaF();
        break;
    }
    case QMetaType::QMatrix4x4: {
        const QMatrix4x4 mat44 = variant.value<QMatrix4x4>();
        // Use constData because we want column-major layout
        v.m_data.resize(16);
        memcpy(v.data<float>(), mat44.constData(), 16 * sizeof(float));
        break;
    }
    case QMetaType::QVariantList: {
        const QVariantList variants = variant.toList();
        if (variants.size() < 1)
            break;

        const int listEntryType = variants.first().userType();

        // array of textures
        if (listEntryType == qNodeIdTypeId)
            v.m_valueType = NodeId;

        v.m_elementByteSize = byteSizeForMetaType(listEntryType);
        const int stride =  v.m_elementByteSize / sizeof(float);

        // Resize v.m_data
        v.m_data.resize(stride * variants.size());

        int idx = 0;
        for (const QVariant &variant : variants) {
            Q_ASSERT_X(variant.userType() == listEntryType,
                       Q_FUNC_INFO,
                       "Uniform array doesn't contain elements of the same type");
            UniformValue vi = UniformValue::fromVariant(variant);
            memcpy(v.data<float>() + idx, vi.data<float>(), stride * sizeof(float));
            idx += stride;
        }
        break;
    }

    default: {
        if (variant.userType() == qMetaTypeId<QMatrix3x3>()) {
            const QMatrix3x3 mat33 = variant.value<QMatrix3x3>();
            // Use constData because we want column-major layout
            v.m_data.resize(9);
            memcpy(v.data<float>(), mat33.constData(), 9 * sizeof(float));
            break;
        }
        if (variant.userType() == qMetaTypeId<Qt3DRender::QAbstractTexture *>()) {
            // silently ignore null texture pointers as they are common while textures are loading
            if (variant.value<Qt3DRender::QAbstractTexture *>() == nullptr)
                break;
        }
        qWarning() << "Unknown uniform type or value:" << variant << "Please check your QParameters";
    }
    }
    return v;
}

template<>
void UniformValue::setData<QMatrix4x4>(const QVector<QMatrix4x4> &v)
{
    m_data.resize(16 * v.size());
    m_valueType = ScalarValue;
    int offset = 0;
    const int byteSize = 16 * sizeof(float);
    float *data = m_data.data();
    for (const auto &m : v) {
        memcpy(data + offset, m.constData(), byteSize);
        offset += 16;
    }
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
