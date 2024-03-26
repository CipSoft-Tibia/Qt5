// Copyright (C) 2014-2016 Klarälvdalens Datakonsult AB (KDAB).
// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef GLTFIMPORTER_H
#define GLTFIMPORTER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DCore/qattribute.h>
#include <Qt3DCore/qbuffer.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qloggingcategory.h>

#include <Qt3DRender/private/qsceneimporter_p.h>

QT_BEGIN_NAMESPACE

class QByteArray;

namespace Qt3DCore {
class QEntity;
}

namespace Qt3DRender {

class QCamera;
class QCameraLens;
class QMaterial;
class QShaderProgram;
class QEffect;
class QAbstractTexture;
class QRenderState;
class QTechnique;
class QParameter;
class QGeometryRenderer;
class QAbstractLight;
class QRenderPass;
class QTexture2D;

Q_DECLARE_LOGGING_CATEGORY(GLTFImporterLog)

class GLTFImporter : public QSceneImporter
{
    Q_OBJECT

public:
    GLTFImporter();
    ~GLTFImporter();

    void setBasePath(const QString& path);
    bool setJSON(const QJsonDocument &json);

    // SceneParserInterface interface
    void setSource(const QUrl &source) final;
    void setData(const QByteArray& data, const QString &basePath) final;
    bool areFileTypesSupported(const QStringList &extensions) const final;
    Qt3DCore::QEntity *node(const QString &id) final;
    Qt3DCore::QEntity *scene(const QString &id = QString()) final;

private:
    class BufferData
    {
    public:
        BufferData();
        explicit BufferData(const QJsonObject &json);

        quint64 length;
        QString path;
        QByteArray *data;
        // type if ever useful
    };

    class ParameterData
    {
    public:
        ParameterData();
        explicit ParameterData(const QJsonObject &json);

        QString semantic;
        int type;
    };

    class AccessorData
    {
    public:
        AccessorData();
        explicit AccessorData(const QJsonObject& json, int major, int minor);

        QString bufferViewName;
        Qt3DCore::QAttribute::VertexBaseType type;
        uint dataSize;
        int count;
        int offset;
        int stride;
    };

    static bool isGLTFSupported(const QStringList &extensions);
    static bool isEmbeddedResource(const QString &url);
    static void renameFromJson(const QJsonObject& json, QObject * const object );
    static bool hasStandardUniformNameFromSemantic(const QString &semantic);
    static QString standardAttributeNameFromSemantic(const QString &semantic);
    QParameter *parameterFromTechnique(QTechnique *technique, const QString &parameterName);

    Qt3DCore::QEntity *defaultScene();
    QMaterial *material(const QString &id);
    bool fillCamera(QCameraLens &lens, QCamera *cameraEntity, const QString &id) const;

    void parse();
    void parseV1();
    void parseV2();
    void cleanup();

    void processJSONAsset(const QJsonObject &json);
    void processJSONBuffer(const QString &id, const QJsonObject &json);
    void processJSONBufferView(const QString &id, const QJsonObject &json);
    void processJSONShader(const QString &id, const QJsonObject &jsonObject);
    void processJSONProgram(const QString &id, const QJsonObject &jsonObject);
    void processJSONTechnique(const QString &id, const QJsonObject &jsonObject);
    void processJSONAccessor(const QString &id, const QJsonObject &json);
    void processJSONMesh(const QString &id, const QJsonObject &json);
    void processJSONImage(const QString &id, const QJsonObject &jsonObject);
    void processJSONTexture(const QString &id, const QJsonObject &jsonObject);
    void processJSONExtensions(const QString &id, const QJsonObject &jsonObject);
    void processJSONEffect(const QString &id, const QJsonObject &jsonObject);
    void processJSONRenderPass(const QString &id, const QJsonObject &jsonObject);

    void loadBufferData();
    void unloadBufferData();

    QByteArray resolveLocalData(const QString &path) const;

    QVariant parameterValueFromJSON(int type, const QJsonValue &value) const;
    static Qt3DCore::QAttribute::VertexBaseType accessorTypeFromJSON(int componentType);
    static uint accessorDataSizeFromJson(const QString &type);

    static QRenderState *buildStateEnable(int state);
    static QRenderState *buildState(const QString& functionName, const QJsonValue &value, int &type);
    QParameter *buildParameter(const QString &key, const QJsonObject &paramObj);
    void populateRenderStates(QRenderPass *pass, const QJsonObject &states);
    void addProgramToPass(QRenderPass *pass, const QString &progName);

    void setTextureSamplerInfo(const QString &id, const QJsonObject &jsonObj, QTexture2D *tex);
    QMaterial *materialWithCustomShader(const QString &id, const QJsonObject &jsonObj);
    QMaterial *commonMaterial(const QJsonObject &jsonObj);
    QMaterial *pbrMaterial(const QJsonObject &jsonObj);

    QJsonDocument m_json;
    QString m_basePath;
    bool m_parseDone;
    int m_majorVersion;
    int m_minorVersion;
    QString m_defaultScene;

    // multi-hash because our QMeshData corresponds to a single primitive
    // in glTf.
    QMultiHash<QString, QGeometryRenderer*> m_meshDict;

    // GLTF assigns materials at the mesh level, but we do them as siblings,
    // so record the association here for when we instantiate meshes
    QHash<QGeometryRenderer*, QString> m_meshMaterialDict;

    QHash<QString, AccessorData> m_accessorDict;

    QHash<QString, QMaterial*> m_materialCache;

    QHash<QString, BufferData> m_bufferDatas;
    QHash<QString, Qt3DCore::QBuffer*> m_buffers;

    QHash<QString, QString> m_shaderPaths;
    QHash<QString, QShaderProgram*> m_programs;

    QHash<QString, QTechnique *> m_techniques;
    QHash<QString, QRenderPass *> m_renderPasses;
    QHash<QString, QEffect *> m_effects;
    QHash<QTechnique *, QList<QParameter *> > m_techniqueParameters;
    QHash<QParameter*, ParameterData> m_parameterDataDict;

    QHash<QString, QAbstractTexture*> m_textures;
    QHash<QString, QString> m_imagePaths;
    QHash<QString, QImage> m_imageData;
    QHash<QString, QAbstractLight *> m_lights;
};

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // GLTFIMPORTER_H
