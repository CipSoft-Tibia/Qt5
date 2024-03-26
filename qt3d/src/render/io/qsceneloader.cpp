// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsceneloader.h"
#include "qsceneloader_p.h"
#include <Qt3DCore/private/qscene_p.h>

#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qtransform.h>
#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qabstractlight.h>
#include <Qt3DRender/qcameralens.h>

#include <private/renderlogging_p.h>

#include <QThread>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {

/*!
    \class Qt3DRender::QSceneLoader
    \inmodule Qt3DRender
    \since 5.7
    \ingroup io

    \brief Provides the facility to load an existing Scene.

    Given a 3D source file, the Qt3DRender::QSceneLoader will try to parse it and
    build a tree of Qt3DCore::QEntity objects with proper Qt3DRender::QGeometryRenderer,
    Qt3DCore::QTransform and Qt3DRender::QMaterial components.

    The loader will try to determine the best material to be used based on the properties
    of the model file. If you wish to use a custom material, you will have to traverse
    the tree and replace the default associated materials with yours.

    As the name implies, Qt3DRender::QSceneLoader loads a complete scene subtree.
    If you wish to load a single piece of geometry, you should rather use
    the Qt3DRender::QMesh instead.

    Qt3DRender::QSceneLoader internally relies on the use of plugins to support a
    wide variety of 3D file formats. \l
    {http://assimp.sourceforge.net/main_features_formats.html}{Here} is a list of formats
    that are supported by Qt3D.

    \note this component shouldn't be shared among several Qt3DCore::QEntity instances.
    Undefined behavior will result.

    \sa Qt3DRender::QMesh
    \sa Qt3DRender::QGeometryRenderer
 */

/*!
    \qmltype SceneLoader
    \inqmlmodule Qt3D.Render
    \instantiates Qt3DRender::QSceneLoader
    \inherits Component
    \since 5.7
    \brief Provides the facility to load an existing Scene.

    Given a 3D source file, the SceneLoader will try to parse it and build a
    tree of Entity objects with proper GeometryRenderer, Transform and Material
    components.

    The loader will try to determine the best material to be used based on the
    properties of the model file. If you wish to use a custom material, you
    will have to traverse the tree and replace the default associated materials
    with yours.

    As the name implies, SceneLoader loads a complete scene subtree. If you
    wish to load a single piece of geometry, you should rather use the
    Mesh instead.

    SceneLoader internally relies on the use of plugins to support a wide
    variety of 3D file formats. \l
    {http://assimp.sourceforge.net/main_features_formats.html}{Here} is a list of
    formats that are supported by Qt3D.

    \note this component shouldn't be shared among several Entity instances.
    Undefined behavior will result.

    \sa Mesh
    \sa GeometryRenderer
 */

/*!
    \enum QSceneLoader::Status

    This enum identifies the state of loading
    \value None     The Qt3DRender::QSceneLoader hasn't been used yet.
    \value Loading  The Qt3DRender::QSceneLoader is currently loading the scene file.
    \value Ready    The Qt3DRender::QSceneLoader successfully loaded the scene file.
    \value Error    The Qt3DRender::QSceneLoader encountered an error while loading the scene file.
 */

/*!
    \enum QSceneLoader::ComponentType

    This enum specifies a component type.
    \value UnknownComponent Unknown component type
    \value GeometryRendererComponent Qt3DRender::QGeometryRenderer component
    \value TransformComponent Qt3DCore::QTransform component
    \value MaterialComponent Qt3DRender::QMaterial component
    \value LightComponent Qt3DRender::QAbstractLight component
    \value CameraLensComponent Qt3DRender::QCameraLens component
 */

/*!
    \qmlproperty url SceneLoader::source

    Holds the url to the source to be loaded.
 */

/*!
    \qmlproperty enumeration SceneLoader::status

    Holds the status of scene loading.
    \list
    \li SceneLoader.None
    \li SceneLoader.Loading
    \li SceneLoader.Ready
    \li SceneLoader.Error
    \endlist
    \sa Qt3DRender::QSceneLoader::Status
    \readonly
 */

/*!
    \property QSceneLoader::source

    Holds the url to the source to be loaded.
 */

/*!
    \property QSceneLoader::status

    Holds the status of scene loading.
    \list
    \li SceneLoader.None
    \li SceneLoader.Loading
    \li SceneLoader.Ready
    \li SceneLoader.Error
    \endlist
    \sa Qt3DRender::QSceneLoader::Status
 */

/*! \internal */
QSceneLoaderPrivate::QSceneLoaderPrivate()
    : QComponentPrivate()
    , m_status(QSceneLoader::None)
    , m_subTreeRoot(nullptr)
{
    m_shareable = false;
}

void QSceneLoaderPrivate::populateEntityMap(QEntity *parentEntity)
{
    // Topmost parent entity is not considered part of the scene as that is typically
    // an unnamed entity inserted by importer.
    const QNodeVector childNodes = parentEntity->childNodes();
    for (auto childNode : childNodes) {
        auto childEntity = qobject_cast<QEntity *>(childNode);
        if (childEntity) {
            m_entityMap.insert(childEntity->objectName(), childEntity);
            populateEntityMap(childEntity);
        }
    }
}

void QSceneLoaderPrivate::setStatus(QSceneLoader::Status status)
{
    if (m_status != status) {
        Q_Q(QSceneLoader);
        m_status = status;
        const bool wasBlocked = q->blockNotifications(true);
        emit q->statusChanged(status);
        q->blockNotifications(wasBlocked);
    }
}

void QSceneLoaderPrivate::setSceneRoot(QEntity *root)
{
    // If we already have a scene sub tree, delete it
    if (m_subTreeRoot) {
        delete m_subTreeRoot;
        m_subTreeRoot = nullptr;
    }

    // If we have successfully loaded a scene, graft it in
    if (root) {
        // Get the entity to which this component is attached
        const Qt3DCore::QNodeIdVector entities = m_scene->entitiesForComponent(m_id);
        Q_ASSERT(entities.size() == 1);
        Qt3DCore::QNodeId parentEntityId = entities.first();
        QEntity *parentEntity = qobject_cast<QEntity *>(m_scene->lookupNode(parentEntityId));
        root->setParent(parentEntity);
        m_subTreeRoot = root;
        populateEntityMap(m_subTreeRoot);
    }
}

/*!
    The constructor creates an instance with the specified \a parent.
 */
QSceneLoader::QSceneLoader(QNode *parent)
    : Qt3DCore::QComponent(*new QSceneLoaderPrivate, parent)
{
}

/*! \internal */
QSceneLoader::~QSceneLoader()
{
}

/*! \internal */
QSceneLoader::QSceneLoader(QSceneLoaderPrivate &dd, QNode *parent)
    : Qt3DCore::QComponent(dd, parent)
{
}

QUrl QSceneLoader::source() const
{
    Q_D(const QSceneLoader);
    return d->m_source;
}

void QSceneLoader::setSource(const QUrl &arg)
{
    Q_D(QSceneLoader);
    if (d->m_source != arg) {
        d->m_entityMap.clear();
        d->m_source = arg;
        emit sourceChanged(arg);
    }
}

QSceneLoader::Status QSceneLoader::status() const
{
    Q_D(const QSceneLoader);
    return d->m_status;
}

/*!
    \qmlmethod Entity SceneLoader::entity(string entityName)
    Returns a loaded entity with the \c objectName matching the \a entityName parameter.
    If multiple entities have the same name, it is undefined which one of them is returned, but it
    will always be the same one.
*/
/*!
    Returns a loaded entity with an \c objectName matching the \a entityName parameter.
    If multiple entities have the same name, it is undefined which one of them is returned, but it
    will always be the same one.
*/
QEntity *QSceneLoader::entity(const QString &entityName) const
{
    Q_D(const QSceneLoader);
    return d->m_entityMap.value(entityName);
}

/*!
    \qmlmethod list SceneLoader::entityNames()
    Returns a list of the \c objectNames of the loaded entities.
*/
/*!
    Returns a list of the \c objectNames of the loaded entities.
*/
QStringList QSceneLoader::entityNames() const
{
    Q_D(const QSceneLoader);
    return d->m_entityMap.keys();
}

/*!
    \qmlmethod Entity SceneLoader::component(string entityName, enumeration componentType)
    Returns a component matching \a componentType of a loaded entity with an \e objectName matching
    the \a entityName.
    If the entity has multiple matching components, the first match in the component list of
    the entity is returned.
    If there is no match, an undefined item is returned.
    \list
    \li SceneLoader.UnknownComponent Unknown component type
    \li SceneLoader.GeometryRendererComponent Qt3DRender::QGeometryRenderer component
    \li SceneLoader.TransformComponent Qt3DCore::QTransform component
    \li SceneLoader.MaterialComponent Qt3DRender::QMaterial component
    \li SceneLoader.LightComponent Qt3DRender::QAbstractLight component
    \li SceneLoader.CameraLensComponent Qt3DRender::QCameraLens component
    \endlist
    \sa Qt3DRender::QSceneLoader::ComponentType
*/
/*!
    Returns a component matching \a componentType of a loaded entity with an objectName matching
    the \a entityName.
    If the entity has multiple matching components, the first match in the component list of
    the entity is returned.
    If there is no match, a null pointer is returned.
*/
QComponent *QSceneLoader::component(const QString &entityName,
                                    QSceneLoader::ComponentType componentType) const
{
    QEntity *e = entity(entityName);
    if (!e)
        return nullptr;
    const QComponentVector components = e->components();
    for (auto component : components) {
        switch (componentType) {
        case GeometryRendererComponent:
            if (qobject_cast<Qt3DRender::QGeometryRenderer *>(component))
                return component;
            break;
        case TransformComponent:
            if (qobject_cast<Qt3DCore::QTransform *>(component))
                return component;
            break;
        case MaterialComponent:
            if (qobject_cast<Qt3DRender::QMaterial *>(component))
                return component;
            break;
        case LightComponent:
            if (qobject_cast<Qt3DRender::QAbstractLight *>(component))
                return component;
            break;
        case CameraLensComponent:
            if (qobject_cast<Qt3DRender::QCameraLens *>(component))
                return component;
            break;
        default:
            break;
        }
    }
    return nullptr;
}

} // namespace Qt3DRender

QT_END_NAMESPACE

#include "moc_qsceneloader.cpp"
