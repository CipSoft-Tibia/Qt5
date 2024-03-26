// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef GLTFEXPORTER_H
#define GLTFEXPORTER_H

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

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtGui/qvector3d.h>

#include <Qt3DRender/qabstractlight.h>
#include <Qt3DRender/qshaderprogram.h>

#include <private/qsceneexporter_p.h>

QT_BEGIN_NAMESPACE

class QByteArray;

namespace Qt3DCore {
class QEntity;
class QTransform;
}

namespace Qt3DRender {

class QCamera;
class QCameraLens;
class QMaterial;
class QGeometryRenderer;
class QTechnique;
class QRenderPass;
class QEffect;

Q_DECLARE_LOGGING_CATEGORY(GLTFExporterLog)

class GLTFExporter : public QSceneExporter
{
    Q_OBJECT

public:
    GLTFExporter();
    ~GLTFExporter();

    bool exportScene(Qt3DCore::QEntity *sceneRoot, const QString &outDir,
                     const QString &exportName, const QVariantHash &options) final;

    struct GltfOptions {
        bool compactJson;
    };

private:
    enum PropertyCacheType {
        TypeNone = 0,
        TypeConeMesh,
        TypeCuboidMesh,
        TypeCylinderMesh,
        TypePlaneMesh,
        TypeSphereMesh,
        TypeTorusMesh
    };

    struct MeshInfo {
        struct BufferView {
            BufferView() : bufIndex(0), offset(0), length(0), componentType(0), target(0) { }
            QString name;
            uint bufIndex;
            uint offset;
            uint length;
            uint componentType;
            uint target;
        };
        QList<BufferView> views;
        struct Accessor {
            Accessor() : offset(0), stride(0), count(0), componentType(0) { }
            QString name;
            QString usage;
            QString bufferView;
            uint offset;
            uint stride;
            uint count;
            uint componentType;
            QString type;
        };
        QList<Accessor> accessors;
        QString name; // generated
        QString originalName; // may be empty
        QString materialName;
        Qt3DRender::QGeometryRenderer *meshComponent;
        PropertyCacheType meshType;
        QString meshTypeStr;
    };

    struct MaterialInfo {
        enum MaterialType {
            TypeCustom,
            TypePhong,
            TypePhongAlpha,
            TypeDiffuseMap,
            TypeDiffuseSpecularMap,
            TypeNormalDiffuseMap,
            TypeNormalDiffuseMapAlpha,
            TypeNormalDiffuseSpecularMap,
            TypeGooch,
            TypePerVertex
        };

        QString name;
        QString originalName;
        MaterialType type;

        // These are only used for default materials
        QHash<QString, QColor> colors;
        QHash<QString, QString> textures;
        QHash<QString, QVariant> values;
        std::vector<int> blendArguments;
        std::vector<int> blendEquations;
    };

    struct ProgramInfo {
        QString name;
        QString vertexShader;
        QString tessellationControlShader;
        QString tessellationEvaluationShader;
        QString geometryShader;
        QString fragmentShader;
        QString computeShader;
    };

    struct ShaderInfo {
        QString name;
        QString uri;
        QShaderProgram::ShaderType type;
        QByteArray code;
    };

    struct CameraInfo {
        QString name;
        QString originalName;
        bool perspective; // Orthographic if false
        float zfar;
        float znear;

        // Perspective properties
        float aspectRatio;
        float yfov;

        // Orthographic properties
        float xmag;
        float ymag;

        QCamera *cameraEntity;
    };

    struct LightInfo {
        QString name;
        QString originalName;
        QAbstractLight::Type type;
        QColor color;
        float intensity;
        QVector3D direction; // Spot and diractional lights
        QVector3D attenuation; // Spot and point lights
        float cutOffAngle; // Spot light only
    };

    struct Node {
        QString name;
        QString uniqueName; // generated
        QList<Node *> children;
    };

    void cacheDefaultProperties(PropertyCacheType type);
    void copyTextures();
    void createShaders();
    void parseEntities(const Qt3DCore::QEntity *entity, Node *parentNode);
    void parseScene();
    void parseMaterials();
    void parseMeshes();
    void parseCameras();
    void parseLights();
    void parseTechniques(QMaterial *material);
    void parseRenderPasses(QTechnique *technique);
    QString addShaderInfo(QShaderProgram::ShaderType type, QByteArray code);

    bool saveScene();
    void delNode(Node *n);
    QString exportNodes(Node *n, QJsonObject &nodes);
    void exportMaterials(QJsonObject &materials);
    void exportGenericProperties(QJsonObject &jsonObj, PropertyCacheType type, QObject *obj);
    void clearOldExport(const QString &dir);
    void exportParameter(QJsonObject &jsonObj, const QString &name, const QVariant &variant);
    void exportRenderStates(QJsonObject &jsonObj, const QRenderPass *pass);

    QString newBufferViewName();
    QString newAccessorName();
    QString newMeshName();
    QString newMaterialName();
    QString newTechniqueName();
    QString newTextureName();
    QString newImageName();
    QString newShaderName();
    QString newProgramName();
    QString newNodeName();
    QString newCameraName();
    QString newLightName();
    QString newRenderPassName();
    QString newEffectName();

    QString textureVariantToUrl(const QVariant &var);
    void setVarToJSonObject(QJsonObject &jsObj, const QString &key, const QVariant &var);

    int m_bufferViewCount;
    int m_accessorCount;
    int m_meshCount;
    int m_materialCount;
    int m_techniqueCount;
    int m_textureCount;
    int m_imageCount;
    int m_shaderCount;
    int m_programCount;
    int m_nodeCount;
    int m_cameraCount;
    int m_lightCount;
    int m_renderPassCount;
    int m_effectCount;

    Qt3DCore::QEntity *m_sceneRoot;
    QString m_exportName;
    QString m_exportDir;

    GltfOptions m_gltfOpts;

    QJsonObject m_obj;
    QJsonDocument m_doc;

    QByteArray m_buffer;
    QHash<Node *, Qt3DRender::QGeometryRenderer *> m_meshMap;
    QHash<Node *, Qt3DRender::QMaterial *> m_materialMap;
    QHash<Node *, Qt3DRender::QCameraLens *> m_cameraMap;
    QHash<Node *, Qt3DRender::QAbstractLight *> m_lightMap;
    QHash<Node *, Qt3DCore::QTransform *> m_transformMap;
    QHash<QString, QString> m_imageMap; // Original texture URL -> generated filename
    QHash<QString, QString> m_textureIdMap;
    QHash<Qt3DRender::QRenderPass *, QString> m_renderPassIdMap;
    QHash<Qt3DRender::QEffect *, QString> m_effectIdMap;
    QHash<Qt3DRender::QTechnique *, QString> m_techniqueIdMap;
    QHash<PropertyCacheType, QObject *> m_defaultObjectCache;
    QHash<PropertyCacheType, QList<QMetaProperty>> m_propertyCache;

    QHash<Qt3DRender::QGeometryRenderer *, MeshInfo> m_meshInfo;
    QHash<Qt3DRender::QMaterial *, MaterialInfo> m_materialInfo;
    QHash<Qt3DRender::QCameraLens *, CameraInfo> m_cameraInfo;
    QHash<Qt3DRender::QAbstractLight *, LightInfo> m_lightInfo;
    QHash<Qt3DRender::QShaderProgram *, ProgramInfo> m_programInfo;
    QList<ShaderInfo> m_shaderInfo;

    Node *m_rootNode;
    bool m_rootNodeEmpty;
    QSet<QString> m_exportedFiles;
};

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // GLTFEXPORTER_H
