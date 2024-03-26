// Copyright (C) 2016 Paul Lemire <paul.lemire350@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "commandexecuter_p.h"

#include <Qt3DCore/private/qabstractaspect_p.h>
#include <Qt3DCore/qbackendnode.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/geometryrenderermanager_p.h>
#include <Qt3DRender/private/stringtoint_p.h>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <graphicscontext_p.h>
#include <renderview_p.h>
#include <rendercommand_p.h>
#include <renderer_p.h>
#include <submissioncontext_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace Debug {

namespace {

template<typename Type>
QJsonObject typeToJsonObj(const Type &)
{
    return QJsonObject();
}

template<typename Type>
QJsonValue typeToJsonValue(const Type &t)
{
    Q_UNUSED(t);
    return QJsonValue();
}

template<>
QJsonValue typeToJsonValue<QRectF>(const QRectF &rect)
{
    QJsonArray value;

    value.push_back(rect.x());
    value.push_back(rect.y());
    value.push_back(rect.width());
    value.push_back(rect.height());

    return value;
}

template<>
QJsonValue typeToJsonValue<QSize>(const QSize &s)
{
    QJsonArray value;

    value.push_back(s.width());
    value.push_back(s.height());

    return value;
}

template<>
QJsonValue typeToJsonValue<QVector3D>(const QVector3D &v)
{
    QJsonArray value;

    value.push_back(v.x());
    value.push_back(v.y());
    value.push_back(v.z());

    return value;
}

template<>
QJsonValue typeToJsonValue<Qt3DCore::QNodeId>(const Qt3DCore::QNodeId &v)
{
    QJsonValue value(qint64(v.id()));
    return value;
}

template<>
QJsonValue typeToJsonValue<QVector4D>(const QVector4D &v)
{
    QJsonArray value;

    value.push_back(v.x());
    value.push_back(v.y());
    value.push_back(v.z());
    value.push_back(v.w());

    return value;
}

template<>
QJsonValue typeToJsonValue<QMatrix4x4>(const QMatrix4x4 &v)
{
    QJsonArray value;

    value.push_back(typeToJsonValue(v.row(0)));
    value.push_back(typeToJsonValue(v.row(1)));
    value.push_back(typeToJsonValue(v.row(2)));
    value.push_back(typeToJsonValue(v.row(3)));

    return value;
}

template<>
QJsonValue typeToJsonValue<QVariant>(const QVariant &v)
{
    const int nodeTypeId = qMetaTypeId<Qt3DCore::QNodeId>();

    if (v.userType() == nodeTypeId)
        return typeToJsonValue(v.value<Qt3DCore::QNodeId>());

    switch (v.userType()) {
    case QMetaType::QVector3D:
        return typeToJsonValue(v.value<QVector3D>());
    case QMetaType::QVector4D:
        return typeToJsonValue(v.value<QVector4D>());
    case QMetaType::QMatrix4x4:
        return typeToJsonValue(v.value<QMatrix4x4>());
    default:
        return QJsonValue::fromVariant(v);
    }
}

template<typename Handle, typename Manager>
QJsonObject backendNodeToJSon(Handle handle, Manager *manager)
{
    Qt3DCore::QBackendNode *node = manager->data(handle);
    QJsonObject obj;
    Qt3DCore::QNodeId id;
    if (node != nullptr)
        id = node->peerId();
    obj.insert(QLatin1String("id"), qint64(id.id()));
    return obj;
}

QJsonObject parameterPackToJson(const Render::OpenGL::ShaderParameterPack &pack)
{
    QJsonObject obj;

    const Render::OpenGL::PackUniformHash &uniforms = pack.uniforms();
    QJsonArray uniformsArray;
    for (qsizetype i = 0, m = uniforms.keys.size(); i < m; ++i) {
        QJsonObject uniformObj;
        uniformObj.insert(QLatin1String("name"), Render::StringToInt::lookupString(uniforms.keys.at(i)));
        const Render::UniformValue::ValueType type = uniforms.values.at(i).valueType();
        uniformObj.insert(QLatin1String("type"),
                          type == Render::UniformValue::ScalarValue
                          ? QLatin1String("value")
                          : QLatin1String("texture"));
        uniformsArray.push_back(uniformObj);
    }
    obj.insert(QLatin1String("uniforms"), uniformsArray);

    QJsonArray texturesArray;
    const std::vector<Render::OpenGL::ShaderParameterPack::NamedResource> &textures = pack.textures();
    for (const auto & texture : textures) {
        QJsonObject textureObj;
        textureObj.insert(QLatin1String("name"), Render::StringToInt::lookupString(texture.glslNameId));
        textureObj.insert(QLatin1String("id"), qint64(texture.nodeId.id()));
        texturesArray.push_back(textureObj);
    }
    obj.insert(QLatin1String("textures"), texturesArray);

    const std::vector<Render::OpenGL::BlockToUBO> &ubos = pack.uniformBuffers();
    QJsonArray ubosArray;
    for (const auto &ubo : ubos) {
        QJsonObject uboObj;
        uboObj.insert(QLatin1String("index"), ubo.m_blockIndex);
        uboObj.insert(QLatin1String("bufferId"), qint64(ubo.m_bufferID.id()));
        ubosArray.push_back(uboObj);

    }
    obj.insert(QLatin1String("ubos"), ubosArray);

    const std::vector<Render::OpenGL::BlockToSSBO> &ssbos = pack.shaderStorageBuffers();
    QJsonArray ssbosArray;
    for (const auto &ssbo : ssbos) {
        QJsonObject ssboObj;
        ssboObj.insert(QLatin1String("index"), ssbo.m_blockIndex);
        ssboObj.insert(QLatin1String("bufferId"), qint64(ssbo.m_bufferID.id()));
        ssbosArray.push_back(ssboObj);
    }
    obj.insert(QLatin1String("ssbos"), ssbosArray);

    return obj;
}

} // anonymous

CommandExecuter::CommandExecuter(Render::OpenGL::Renderer *renderer)
    : m_renderer(renderer)
{
}

// Render thread
void CommandExecuter::performAsynchronousCommandExecution(const std::vector<Render::OpenGL::RenderView *> &views)
{
    QMutexLocker lock(&m_pendingCommandsMutex);
    const QList<Qt3DCore::Debug::AsynchronousCommandReply *> shellCommands = Qt3DCore::moveAndClear(m_pendingCommands);
    lock.unlock();

    for (auto *reply : shellCommands) {
        if (reply->commandName() == QLatin1String("glinfo")) {
            QJsonObject replyObj;
            const GraphicsApiFilterData *contextInfo = m_renderer->submissionContext()->contextInfo();
            if (contextInfo != nullptr) {
                replyObj.insert(QLatin1String("api"),
                                contextInfo->m_api == QGraphicsApiFilter::OpenGL
                                ? QLatin1String("OpenGL")
                                : QLatin1String("OpenGLES"));
                const QString versionString =
                        QString::number(contextInfo->m_major)
                        + QStringLiteral(".")
                        + QString::number(contextInfo->m_minor);
                replyObj.insert(QLatin1String("version"), versionString);
                replyObj.insert(QLatin1String("profile"),
                                contextInfo->m_profile == QGraphicsApiFilter::CoreProfile
                                ? QLatin1String("Core")
                                : contextInfo->m_profile == QGraphicsApiFilter::CompatibilityProfile
                                  ? QLatin1String("Compatibility")
                                  : QLatin1String("None"));
            }
            reply->setData(QJsonDocument(replyObj).toJson());
        } else if (reply->commandName() == QLatin1String("rendercommands")) {
            QJsonObject replyObj;

            QJsonArray viewArray;
            for (Render::OpenGL::RenderView *v : views) {
                QJsonObject viewObj;
                viewObj.insert(QLatin1String("viewport"), typeToJsonValue(v->viewport()));
                viewObj.insert(QLatin1String("surfaceSize"), typeToJsonValue(v->surfaceSize()));
                viewObj.insert(QLatin1String("devicePixelRatio"), v->devicePixelRatio());
                viewObj.insert(QLatin1String("noDraw"), v->noDraw());
                viewObj.insert(QLatin1String("frustumCulling"), v->frustumCulling());
                viewObj.insert(QLatin1String("compute"), v->isCompute());
                viewObj.insert(QLatin1String("clearDepthValue"), v->clearDepthValue());
                viewObj.insert(QLatin1String("clearStencilValue"), v->clearStencilValue());

                QJsonArray renderCommandsArray;
                v->forEachCommand([&] (Render::OpenGL::RenderCommand &c) {
                    QJsonObject commandObj;
                    Render::NodeManagers *nodeManagers = m_renderer->nodeManagers();
                    commandObj.insert(QLatin1String("shader"), typeToJsonValue(QVariant::fromValue(c.m_shaderId)));
                    commandObj.insert(QLatin1String("vao"),  double(c.m_vao.handle()));
                    commandObj.insert(QLatin1String("instanceCount"), c.m_instanceCount);
                    commandObj.insert(QLatin1String("geometry"),  backendNodeToJSon(c.m_geometry, nodeManagers->geometryManager()));
                    commandObj.insert(QLatin1String("geometryRenderer"),  backendNodeToJSon(c.m_geometryRenderer, nodeManagers->geometryRendererManager()));
                    commandObj.insert(QLatin1String("shaderParameterPack"), parameterPackToJson(c.m_parameterPack));

                    renderCommandsArray.push_back(commandObj);
                });
                viewObj.insert(QLatin1String("commands"), renderCommandsArray);
                viewArray.push_back(viewObj);
            }

            replyObj.insert(QLatin1String("renderViews"), viewArray);
            reply->setData(QJsonDocument(replyObj).toJson());
        }
        reply->setFinished(true);
    }
}

// Main thread
QVariant CommandExecuter::executeCommand(const QStringList &args)
{
    // Note: The replies will be deleted by the AspectCommandDebugger
    if (args.size() > 0 &&
            (args.first() == QLatin1String("glinfo") ||
             args.first() == QLatin1String("rendercommands"))) {
        auto reply = new Qt3DCore::Debug::AsynchronousCommandReply(args.first());
        QMutexLocker lock(&m_pendingCommandsMutex);
        m_pendingCommands.push_back(reply);
        return QVariant::fromValue(reply);
    }
    return QVariant();
}

} // Debug

} // Qt3DRenderer

QT_END_NAMESPACE
