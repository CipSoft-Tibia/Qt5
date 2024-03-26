// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "gltfgeometryloader.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QVersionNumber>

#include <QOpenGLTexture>

#include <Qt3DRender/private/renderlogging_p.h>
#include <Qt3DCore/QGeometry>
#include <Qt3DCore/private/qloadgltf_p.h>

QT_BEGIN_NAMESPACE

#ifndef qUtf16PrintableImpl // -Impl is a Qt 5.8 feature
#  define qUtf16PrintableImpl(string) \
    static_cast<const wchar_t*>(static_cast<const void*>(string.utf16()))
#endif

using namespace Qt3DCore;

namespace Qt3DRender {

Q_LOGGING_CATEGORY(GLTFGeometryLoaderLog, "Qt3D.GLTFGeometryLoader", QtWarningMsg)

#define KEY_ASSET        QLatin1String("asset")
#define KEY_ACCESSORS    QLatin1String("accessors")
#define KEY_ATTRIBUTES   QLatin1String("attributes")
#define KEY_BUFFER       QLatin1String("buffer")
#define KEY_BUFFERS      QLatin1String("buffers")
#define KEY_BYTE_LENGTH  QLatin1String("byteLength")
#define KEY_BYTE_OFFSET  QLatin1String("byteOffset")
#define KEY_BYTE_STRIDE  QLatin1String("byteStride")
#define KEY_COUNT        QLatin1String("count")
#define KEY_INDICES      QLatin1String("indices")
#define KEY_MATERIAL     QLatin1String("material")
#define KEY_MESHES       QLatin1String("meshes")
#define KEY_NAME         QLatin1String("name")
#define KEY_PRIMITIVES   QLatin1String("primitives")
#define KEY_TARGET       QLatin1String("target")
#define KEY_TYPE         QLatin1String("type")
#define KEY_URI          QLatin1String("uri")
#define KEY_VERSION      QLatin1String("version")

#define KEY_BUFFER_VIEW         QLatin1String("bufferView")
#define KEY_BUFFER_VIEWS        QLatin1String("bufferViews")
#define KEY_COMPONENT_TYPE      QLatin1String("componentType")

GLTFGeometryLoader::GLTFGeometryLoader()
    : m_geometry(nullptr)
{
}

GLTFGeometryLoader::~GLTFGeometryLoader()
{
    cleanup();
}

QGeometry *GLTFGeometryLoader::geometry() const
{
    return m_geometry;
}

bool GLTFGeometryLoader::load(QIODevice *ioDev, const QString &subMesh)
{
    Q_UNUSED(subMesh);

    if (Q_UNLIKELY(!setJSON(qLoadGLTF(ioDev->readAll())))) {
        qCWarning(GLTFGeometryLoaderLog, "not a JSON document");
        return false;
    }

    auto file = qobject_cast<QFile*>(ioDev);
    if (file) {
        QFileInfo finfo(file->fileName());
        setBasePath(finfo.dir().absolutePath());
    }

    m_mesh = subMesh;

    parse();

    return true;
}

GLTFGeometryLoader::BufferData::BufferData()
    : length(0)
    , data(nullptr)
{
}

GLTFGeometryLoader::BufferData::BufferData(const QJsonObject &json)
    : length(json.value(KEY_BYTE_LENGTH).toInt())
    , path(json.value(KEY_URI).toString())
    , data(nullptr)
{
}

GLTFGeometryLoader::AccessorData::AccessorData()
    : bufferViewIndex(0)
    , type(QAttribute::Float)
    , dataSize(0)
    , count(0)
    , offset(0)
    , stride(0)
{

}

GLTFGeometryLoader::AccessorData::AccessorData(const QJsonObject &json)
    : bufferViewName(json.value(KEY_BUFFER_VIEW).toString())
    , bufferViewIndex(json.value(KEY_BUFFER_VIEW).toInt(-1))
    , type(accessorTypeFromJSON(json.value(KEY_COMPONENT_TYPE).toInt()))
    , dataSize(accessorDataSizeFromJson(json.value(KEY_TYPE).toString()))
    , count(json.value(KEY_COUNT).toInt())
    , offset(0)
    , stride(0)
{
    const auto byteOffset = json.value(KEY_BYTE_OFFSET);
    if (!byteOffset.isUndefined())
        offset = byteOffset.toInt();
    const auto byteStride = json.value(KEY_BYTE_STRIDE);
    if (!byteStride.isUndefined())
        stride = byteStride.toInt();
}

void GLTFGeometryLoader::setBasePath(const QString &path)
{
    m_basePath = path;
}

bool GLTFGeometryLoader::setJSON(const QJsonDocument &json )
{
    if (!json.isObject())
        return false;

    m_json = json;

    cleanup();

    return true;
}

QString GLTFGeometryLoader::standardAttributeNameFromSemantic(const QString &semantic)
{
    //Standard Attributes
    if (semantic.startsWith(QLatin1String("POSITION")))
        return QAttribute::defaultPositionAttributeName();
    if (semantic.startsWith(QLatin1String("NORMAL")))
        return QAttribute::defaultNormalAttributeName();
    if (semantic.startsWith(QLatin1String("TEXCOORD")))
        return QAttribute::defaultTextureCoordinateAttributeName();
    if (semantic.startsWith(QLatin1String("COLOR")))
        return QAttribute::defaultColorAttributeName();
    if (semantic.startsWith(QLatin1String("TANGENT")))
        return QAttribute::defaultTangentAttributeName();
    if (semantic.startsWith(QLatin1String("JOINTS")))
        return QAttribute::defaultJointIndicesAttributeName();
    if (semantic.startsWith(QLatin1String("WEIGHTS")))
        return QAttribute::defaultJointWeightsAttributeName();

    return QString();
}

void GLTFGeometryLoader::parse()
{
    // Find the glTF version
    const QJsonObject asset = m_json.object().value(KEY_ASSET).toObject();
    const QString versionString = asset.value(KEY_VERSION).toString();
    const auto version = QVersionNumber::fromString(versionString);
    switch (version.majorVersion()) {
    case 1:
        parseGLTF1();
        break;

    case 2:
        parseGLTF2();
        break;

    default:
        qWarning() << "Unsupported version of glTF" << versionString;
    }
}

void GLTFGeometryLoader::parseGLTF1()
{
    const QJsonObject buffers = m_json.object().value(KEY_BUFFERS).toObject();
    for (auto it = buffers.begin(), end = buffers.end(); it != end; ++it)
        processJSONBuffer(it.key(), it.value().toObject());

    const QJsonObject views = m_json.object().value(KEY_BUFFER_VIEWS).toObject();
    loadBufferData();
    for (auto it = views.begin(), end = views.end(); it != end; ++it)
        processJSONBufferView(it.key(), it.value().toObject());
    unloadBufferData();

    const QJsonObject attrs = m_json.object().value(KEY_ACCESSORS).toObject();
    for (auto it = attrs.begin(), end = attrs.end(); it != end; ++it)
        processJSONAccessor(it.key(), it.value().toObject());

    const QJsonObject meshes = m_json.object().value(KEY_MESHES).toObject();
    for (auto it = meshes.begin(), end = meshes.end(); it != end && !m_geometry; ++it) {
        const QJsonObject &mesh = it.value().toObject();
        if (m_mesh.isEmpty() ||
            m_mesh.compare(mesh.value(KEY_NAME).toString(), Qt::CaseInsensitive) == 0)
            processJSONMesh(it.key(), mesh);
    }
}

void GLTFGeometryLoader::parseGLTF2()
{
    const QJsonArray buffers = m_json.object().value(KEY_BUFFERS).toArray();
    for (auto it = buffers.begin(), end = buffers.end(); it != end; ++it)
        processJSONBufferV2(it->toObject());

    const QJsonArray views = m_json.object().value(KEY_BUFFER_VIEWS).toArray();
    loadBufferDataV2();
    for (auto it = views.begin(), end = views.end(); it != end; ++it)
        processJSONBufferViewV2(it->toObject());
    unloadBufferDataV2();

    const QJsonArray attrs = m_json.object().value(KEY_ACCESSORS).toArray();
    for (auto it = attrs.begin(), end = attrs.end(); it != end; ++it)
        processJSONAccessorV2(it->toObject());

    const QJsonArray meshes = m_json.object().value(KEY_MESHES).toArray();
    for (auto it = meshes.begin(), end = meshes.end(); it != end && !m_geometry; ++it) {
        const QJsonObject &mesh = it->toObject();
        if (m_mesh.isEmpty() || m_mesh.compare(mesh.value(KEY_NAME).toString(), Qt::CaseInsensitive) == 0)
            processJSONMeshV2(mesh);
    }
}

void GLTFGeometryLoader::cleanup()
{
    m_geometry = nullptr;
    m_gltf1.m_accessorDict.clear();
    m_gltf1.m_buffers.clear();
}

void GLTFGeometryLoader::processJSONBuffer(const QString &id, const QJsonObject &json)
{
    // simply cache buffers for lookup by buffer-views
    m_gltf1.m_bufferDatas[id] = BufferData(json);
}

void GLTFGeometryLoader::processJSONBufferV2(const QJsonObject &json)
{
    // simply cache buffers for lookup by buffer-views
    m_gltf2.m_bufferDatas.push_back(BufferData(json));
}

void GLTFGeometryLoader::processJSONBufferView(const QString &id, const QJsonObject &json)
{
    QString bufName = json.value(KEY_BUFFER).toString();
    const auto it = std::as_const(m_gltf1.m_bufferDatas).find(bufName);
    if (Q_UNLIKELY(it == m_gltf1.m_bufferDatas.cend())) {
        qCWarning(GLTFGeometryLoaderLog, "unknown buffer: %ls processing view: %ls",
                  qUtf16PrintableImpl(bufName), qUtf16PrintableImpl(id));
        return;
    }
    const auto &bufferData = *it;

    int target = json.value(KEY_TARGET).toInt();

    switch (target) {
    case GL_ARRAY_BUFFER:
    case GL_ELEMENT_ARRAY_BUFFER:
        break;
    default:
        qCWarning(GLTFGeometryLoaderLog, "buffer %ls unsupported target: %d",
                  qUtf16PrintableImpl(id), target);
        return;
    }

    quint64 offset = 0;
    const auto byteOffset = json.value(KEY_BYTE_OFFSET);
    if (!byteOffset.isUndefined()) {
        offset = byteOffset.toInt();
        qCDebug(GLTFGeometryLoaderLog, "bv: %ls has offset: %lld", qUtf16PrintableImpl(id), offset);
    }

    const quint64 len = json.value(KEY_BYTE_LENGTH).toInt();

    QByteArray bytes = bufferData.data->mid(offset, len);
    if (Q_UNLIKELY(bytes.size() != qsizetype(len))) {
        qCWarning(GLTFGeometryLoaderLog, "failed to read sufficient bytes from: %ls for view %ls",
                  qUtf16PrintableImpl(bufferData.path), qUtf16PrintableImpl(id));
    }

    Qt3DCore::QBuffer *b = new Qt3DCore::QBuffer();
    b->setData(bytes);
    m_gltf1.m_buffers[id] = b;
}

void GLTFGeometryLoader::processJSONBufferViewV2(const QJsonObject &json)
{
    const int bufferIndex = json.value(KEY_BUFFER).toInt();
    if (Q_UNLIKELY(bufferIndex) >= m_gltf2.m_bufferDatas.size()) {
        qCWarning(GLTFGeometryLoaderLog, "unknown buffer: %d processing view", bufferIndex);
        return;
    }
    const auto bufferData = m_gltf2.m_bufferDatas[bufferIndex];

    quint64 offset = 0;
    const auto byteOffset = json.value(KEY_BYTE_OFFSET);
    if (!byteOffset.isUndefined()) {
        offset = byteOffset.toInt();
        qCDebug(GLTFGeometryLoaderLog, "bufferview has offset: %lld", offset);
    }

    const quint64 len = json.value(KEY_BYTE_LENGTH).toInt();
    QByteArray bytes = bufferData.data->mid(offset, len);
    if (Q_UNLIKELY(bytes.size() != qsizetype(len))) {
        qCWarning(GLTFGeometryLoaderLog, "failed to read sufficient bytes from: %ls for view",
                  qUtf16PrintableImpl(bufferData.path));
    }

    auto b = new Qt3DCore::QBuffer;
    b->setData(bytes);
    m_gltf2.m_buffers.push_back(b);
}

void GLTFGeometryLoader::processJSONAccessor(const QString &id, const QJsonObject &json)
{
    m_gltf1.m_accessorDict[id] = AccessorData(json);
}

void GLTFGeometryLoader::processJSONAccessorV2(const QJsonObject &json)
{
    m_gltf2.m_accessors.push_back(AccessorData(json));
}

void GLTFGeometryLoader::processJSONMesh(const QString &id, const QJsonObject &json)
{
    const QJsonArray primitivesArray = json.value(KEY_PRIMITIVES).toArray();
    for (const QJsonValue &primitiveValue : primitivesArray) {
        QJsonObject primitiveObject = primitiveValue.toObject();
        QString material = primitiveObject.value(KEY_MATERIAL).toString();

        if (Q_UNLIKELY(material.isEmpty())) {
            qCWarning(GLTFGeometryLoaderLog, "malformed primitive on %ls, missing material value %ls",
                      qUtf16PrintableImpl(id), qUtf16PrintableImpl(material));
            continue;
        }

        QGeometry *meshGeometry = new QGeometry;

        const QJsonObject attrs = primitiveObject.value(KEY_ATTRIBUTES).toObject();
        for (auto it = attrs.begin(), end = attrs.end(); it != end; ++it) {
            QString k = it.value().toString();
            const auto accessorIt = std::as_const(m_gltf1.m_accessorDict).find(k);
            if (Q_UNLIKELY(accessorIt == m_gltf1.m_accessorDict.cend())) {
                qCWarning(GLTFGeometryLoaderLog, "unknown attribute accessor: %ls on mesh %ls",
                          qUtf16PrintableImpl(k), qUtf16PrintableImpl(id));
                continue;
            }

            const QString attrName = it.key();
            QString attributeName = standardAttributeNameFromSemantic(attrName);
            if (attributeName.isEmpty())
                attributeName = attrName;

            //Get buffer handle for accessor
            Qt3DCore::QBuffer *buffer = m_gltf1.m_buffers.value(accessorIt->bufferViewName, nullptr);
            if (Q_UNLIKELY(!buffer)) {
                qCWarning(GLTFGeometryLoaderLog, "unknown buffer-view: %ls processing accessor: %ls",
                          qUtf16PrintableImpl(accessorIt->bufferViewName), qUtf16PrintableImpl(id));
                continue;
            }

            QAttribute *attribute = new QAttribute(buffer,
                                                   attributeName,
                                                   accessorIt->type,
                                                   accessorIt->dataSize,
                                                   accessorIt->count,
                                                   accessorIt->offset,
                                                   accessorIt->stride);
            attribute->setAttributeType(QAttribute::VertexAttribute);
            meshGeometry->addAttribute(attribute);
        }

        const auto indices = primitiveObject.value(KEY_INDICES);
        if (!indices.isUndefined()) {
            QString k = indices.toString();
            const auto accessorIt = std::as_const(m_gltf1.m_accessorDict).find(k);
            if (Q_UNLIKELY(accessorIt == m_gltf1.m_accessorDict.cend())) {
                qCWarning(GLTFGeometryLoaderLog, "unknown index accessor: %ls on mesh %ls",
                          qUtf16PrintableImpl(k), qUtf16PrintableImpl(id));
            } else {
                //Get buffer handle for accessor
                Qt3DCore::QBuffer *buffer = m_gltf1.m_buffers.value(accessorIt->bufferViewName, nullptr);
                if (Q_UNLIKELY(!buffer)) {
                    qCWarning(GLTFGeometryLoaderLog, "unknown buffer-view: %ls processing accessor: %ls",
                              qUtf16PrintableImpl(accessorIt->bufferViewName), qUtf16PrintableImpl(id));
                    continue;
                }

                QAttribute *attribute = new QAttribute(buffer,
                                                       accessorIt->type,
                                                       accessorIt->dataSize,
                                                       accessorIt->count,
                                                       accessorIt->offset,
                                                       accessorIt->stride);
                attribute->setAttributeType(QAttribute::IndexAttribute);
                meshGeometry->addAttribute(attribute);
            }
        } // of has indices

        m_geometry = meshGeometry;

        break;
    } // of primitives iteration
}

void GLTFGeometryLoader::processJSONMeshV2(const QJsonObject &json)
{
    const QJsonArray primitivesArray = json.value(KEY_PRIMITIVES).toArray();
    for (const QJsonValue &primitiveValue : primitivesArray) {
        QJsonObject primitiveObject = primitiveValue.toObject();

        QGeometry *meshGeometry = new QGeometry;

        const QJsonObject attrs = primitiveObject.value(KEY_ATTRIBUTES).toObject();
        for (auto it = attrs.begin(), end = attrs.end(); it != end; ++it) {
            const int accessorIndex = it.value().toInt();
            if (Q_UNLIKELY(accessorIndex >= m_gltf2.m_accessors.size())) {
                qCWarning(GLTFGeometryLoaderLog, "unknown attribute accessor: %d on mesh %ls",
                          accessorIndex, qUtf16PrintableImpl(json.value(KEY_NAME).toString()));
                continue;
            }
            const auto &accessor = m_gltf2.m_accessors[accessorIndex];

            const QString attrName = it.key();
            QString attributeName = standardAttributeNameFromSemantic(attrName);
            if (attributeName.isEmpty())
                attributeName = attrName;

            // Get buffer handle for accessor
            if (Q_UNLIKELY(accessor.bufferViewIndex >= m_gltf2.m_buffers.size())) {
                qCWarning(GLTFGeometryLoaderLog, "unknown buffer-view: %d processing accessor: %ls",
                          accessor.bufferViewIndex, qUtf16PrintableImpl(json.value(KEY_NAME).toString()));
                continue;
            }
            Qt3DCore::QBuffer *buffer = m_gltf2.m_buffers[accessor.bufferViewIndex];

            QAttribute *attribute = new QAttribute(buffer,
                                                   attributeName,
                                                   accessor.type,
                                                   accessor.dataSize,
                                                   accessor.count,
                                                   accessor.offset,
                                                   accessor.stride);
            attribute->setAttributeType(QAttribute::VertexAttribute);
            meshGeometry->addAttribute(attribute);
        }

        const auto indices = primitiveObject.value(KEY_INDICES);
        if (!indices.isUndefined()) {
            const int accessorIndex = indices.toInt();
            if (Q_UNLIKELY(accessorIndex >= m_gltf2.m_accessors.size())) {
                qCWarning(GLTFGeometryLoaderLog, "unknown index accessor: %d on mesh %ls",
                          accessorIndex, qUtf16PrintableImpl(json.value(KEY_NAME).toString()));
            } else {
                const auto &accessor = m_gltf2.m_accessors[accessorIndex];

                //Get buffer handle for accessor
                if (Q_UNLIKELY(accessor.bufferViewIndex >= m_gltf2.m_buffers.size())) {
                    qCWarning(GLTFGeometryLoaderLog, "unknown buffer-view: %d processing accessor: %ls",
                              accessor.bufferViewIndex, qUtf16PrintableImpl(json.value(KEY_NAME).toString()));
                    continue;
                }
                Qt3DCore::QBuffer *buffer = m_gltf2.m_buffers[accessor.bufferViewIndex];

                QAttribute *attribute = new QAttribute(buffer,
                                                       accessor.type,
                                                       accessor.dataSize,
                                                       accessor.count,
                                                       accessor.offset,
                                                       accessor.stride);
                attribute->setAttributeType(QAttribute::IndexAttribute);
                meshGeometry->addAttribute(attribute);
            }
        } // of has indices

        m_geometry = meshGeometry;

        break;
    } // of primitives iteration
}

void GLTFGeometryLoader::loadBufferData()
{
    for (auto &bufferData : m_gltf1.m_bufferDatas) {
        if (!bufferData.data) {
            bufferData.data = new QByteArray(resolveLocalData(bufferData.path));
        }
    }
}

void GLTFGeometryLoader::unloadBufferData()
{
    for (const auto &bufferData : std::as_const(m_gltf1.m_bufferDatas)) {
        QByteArray *data = bufferData.data;
        delete data;
    }
}

void GLTFGeometryLoader::loadBufferDataV2()
{
    for (auto &bufferData : m_gltf2.m_bufferDatas) {
        if (!bufferData.data)
            bufferData.data = new QByteArray(resolveLocalData(bufferData.path));
    }
}

void GLTFGeometryLoader::unloadBufferDataV2()
{
    for (const auto &bufferData : std::as_const(m_gltf2.m_bufferDatas)) {
        QByteArray *data = bufferData.data;
        delete data;
    }
}

QByteArray GLTFGeometryLoader::resolveLocalData(const QString &path) const
{
    QDir d(m_basePath);
    Q_ASSERT(d.exists());

    QString absPath = d.absoluteFilePath(path);
    QFile f(absPath);
    f.open(QIODevice::ReadOnly);
    return f.readAll();
}

QAttribute::VertexBaseType GLTFGeometryLoader::accessorTypeFromJSON(int componentType)
{
    if (componentType == GL_BYTE)
        return QAttribute::Byte;
    else if (componentType == GL_UNSIGNED_BYTE)
        return QAttribute::UnsignedByte;
    else if (componentType == GL_SHORT)
        return QAttribute::Short;
    else if (componentType == GL_UNSIGNED_SHORT)
        return QAttribute::UnsignedShort;
    else if (componentType == GL_UNSIGNED_INT)
        return QAttribute::UnsignedInt;
    else if (componentType == GL_FLOAT)
        return QAttribute::Float;

    //There shouldn't be an invalid case here
    qCWarning(GLTFGeometryLoaderLog, "unsupported accessor type %d", componentType);
    return QAttribute::Float;
}

uint GLTFGeometryLoader::accessorDataSizeFromJson(const QString &type)
{
    QString typeName = type.toUpper();
    if (typeName == QLatin1String("SCALAR"))
        return 1;
    if (typeName == QLatin1String("VEC2"))
        return 2;
    if (typeName == QLatin1String("VEC3"))
        return 3;
    if (typeName == QLatin1String("VEC4"))
        return 4;
    if (typeName == QLatin1String("MAT2"))
        return 4;
    if (typeName == QLatin1String("MAT3"))
        return 9;
    if (typeName == QLatin1String("MAT4"))
        return 16;

    return 0;
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_gltfgeometryloader.cpp"
