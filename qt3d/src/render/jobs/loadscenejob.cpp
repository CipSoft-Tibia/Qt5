// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "loadscenejob_p.h"
#include <private/nodemanagers_p.h>
#include <private/scenemanager_p.h>
#include <QCoreApplication>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/private/qaspectmanager_p.h>
#include <Qt3DCore/private/qurlhelper_p.h>
#include <Qt3DRender/private/job_common_p.h>
#include <Qt3DRender/private/qsceneimporter_p.h>
#include <Qt3DRender/qsceneloader.h>
#include <Qt3DRender/private/qsceneloader_p.h>
#include <Qt3DRender/private/renderlogging_p.h>
#include <QFileInfo>
#include <QMimeDatabase>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {

LoadSceneJob::LoadSceneJob(const QUrl &source, Qt3DCore::QNodeId sceneComponent)
    : QAspectJob(*new LoadSceneJobPrivate(this))
    , m_source(source)
    , m_sceneComponent(sceneComponent)
    , m_managers(nullptr)
{
    SET_JOB_RUN_STAT_TYPE(this, JobTypes::LoadScene, 0)
}

void LoadSceneJob::setData(const QByteArray &data)
{
    m_data = data;
}

NodeManagers *LoadSceneJob::nodeManagers() const
{
    return m_managers;
}

QList<QSceneImporter *> LoadSceneJob::sceneImporters() const
{
    return m_sceneImporters;
}

QUrl LoadSceneJob::source() const
{
    return m_source;
}

Qt3DCore::QNodeId LoadSceneJob::sceneComponentId() const
{
    return m_sceneComponent;
}

void LoadSceneJob::run()
{
    // Iterate scene IO handlers until we find one that can handle this file type
    Qt3DCore::QEntity *sceneSubTree = nullptr;
    Scene *scene = m_managers->sceneManager()->lookupResource(m_sceneComponent);
    Q_ASSERT(scene);

    // Reset status
    QSceneLoader::Status finalStatus = QSceneLoader::None;

    // Perform the loading only if the source wasn't explicitly set to empty
    if (!m_source.isEmpty()) {
        finalStatus = QSceneLoader::Error;

        if (m_data.isEmpty()) {
            const QString path = Qt3DCore::QUrlHelper::urlToLocalFileOrQrc(m_source);
            const QFileInfo finfo(path);
            qCDebug(SceneLoaders) << Q_FUNC_INFO << "Attempting to load" << finfo.filePath();
            if (finfo.exists()) {
                const QStringList extensions(finfo.suffix());
                sceneSubTree = tryLoadScene(finalStatus,
                                            extensions,
                                            [this] (QSceneImporter *importer) {
                        importer->setSource(m_source);
            });
            } else {
                qCWarning(SceneLoaders) << Q_FUNC_INFO << finfo.filePath() << "doesn't exist";
            }
        } else {
            QStringList extensions;
            QMimeDatabase db;
            const QMimeType mtype = db.mimeTypeForData(m_data);

            if (mtype.isValid())
                extensions = mtype.suffixes();
            else
                qCWarning(SceneLoaders) << Q_FUNC_INFO << "Invalid mime type" << mtype;

            const QString basePath = m_source.adjusted(QUrl::RemoveFilename).toString();

            sceneSubTree = tryLoadScene(finalStatus,
                                        extensions,
                                        [this, basePath] (QSceneImporter *importer) {
                importer->setData(m_data, basePath);
            });
        }
    }

    Q_D(LoadSceneJob);
    d->m_sceneSubtree = std::unique_ptr<Qt3DCore::QEntity>(sceneSubTree);
    d->m_status = finalStatus;

    if (d->m_sceneSubtree) {
        // Move scene sub tree to the application thread so that it can be grafted in.
        const auto appThread = QCoreApplication::instance()->thread();
        d->m_sceneSubtree->moveToThread(appThread);
    }
}

Qt3DCore::QEntity *LoadSceneJob::tryLoadScene(QSceneLoader::Status &finalStatus,
                                              const QStringList &extensions,
                                              const std::function<void (QSceneImporter *)> &importerSetupFunc)
{
    Qt3DCore::QEntity *sceneSubTree = nullptr;
    bool foundSuitableLoggerPlugin = false;

    for (QSceneImporter *sceneImporter : std::as_const(m_sceneImporters)) {
        if (!sceneImporter->areFileTypesSupported(extensions))
            continue;

        foundSuitableLoggerPlugin = true;

        // Set source file or data on importer
        importerSetupFunc(sceneImporter);

        // File type is supported, try to load it
        sceneSubTree = sceneImporter->scene();
        if (sceneSubTree != nullptr) {
            // Successfully built a subtree
            finalStatus = QSceneLoader::Ready;
            break;
        }

        qCWarning(SceneLoaders) << Q_FUNC_INFO << "Failed to import" << m_source << "with errors" << sceneImporter->errors();
    }

    if (!foundSuitableLoggerPlugin)
        qCWarning(SceneLoaders) << Q_FUNC_INFO << "Found no suitable importer plugin for" << m_source;

    return sceneSubTree;
}

void LoadSceneJobPrivate::postFrame(Qt3DCore::QAspectManager *manager)
{
    Q_Q(LoadSceneJob);
    QSceneLoader *node =
            qobject_cast<QSceneLoader *>(manager->lookupNode(q->sceneComponentId()));
    if (!node)
        return;
    Qt3DRender::QSceneLoaderPrivate *dNode =
            static_cast<decltype(dNode)>(Qt3DCore::QNodePrivate::get(node));

    // If the sceneSubTree is null it will trigger the frontend to unload
    // any subtree it may hold
    // Set clone of sceneTree in sceneComponent. This will move the sceneSubTree
    // to the QCoreApplication thread which is where the frontend object tree lives.
    dNode->setSceneRoot(m_sceneSubtree.release());

    // Note: the status is set after the subtree so that bindinds depending on the status
    // in the frontend will be consistent
    dNode->setStatus(m_status);
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE
