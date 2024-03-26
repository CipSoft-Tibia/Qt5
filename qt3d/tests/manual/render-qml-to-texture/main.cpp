// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QGuiApplication>
#include <QAnimationDriver>
#include <QPropertyAnimation>
#include <QQmlComponent>
#include <QQmlEngine>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QCamera>

#include <Qt3DInput/QInputAspect>

#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DQuickScene2D/QScene2D>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DRender/QTextureWrapMode>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QGeometryRenderer>

#include "qt3dwindow.h"
#include "qfirstpersoncameracontroller.h"
#include "planematerial.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    Qt3DExtras::Qt3DWindow view;

    // Scene Root
    Qt3DCore::QEntity *sceneRoot = new Qt3DCore::QEntity();

    // Scene Camera
    Qt3DRender::QCamera *basicCamera = view.camera();
    basicCamera->setProjectionType(Qt3DRender::QCameraLens::PerspectiveProjection);
    basicCamera->setAspectRatio(view.width() / view.height());
    basicCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    basicCamera->setPosition(QVector3D(0.0f, 0.0f, 6.0f));
    basicCamera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    basicCamera->setNearPlane(0.1f);
    basicCamera->setFarPlane(1000.0f);
    basicCamera->setFieldOfView(45.0f);

    // For camera controls
    Qt3DExtras::QFirstPersonCameraController *camController = new Qt3DExtras::QFirstPersonCameraController(sceneRoot);
    camController->setCamera(basicCamera);

    Qt3DRender::QFrameGraphNode* frameGraphNode = view.activeFrameGraph();
    while (frameGraphNode->childNodes().size() > 0)
        frameGraphNode = (Qt3DRender::QFrameGraphNode*)frameGraphNode->childNodes().at(0);
    view.defaultFrameGraph()->setClearColor(QColor::fromRgbF(1.0f, 1.0f, 1.0f));
    Qt3DRender::Quick::QScene2D *qmlTextureRenderer = new Qt3DRender::Quick::QScene2D(frameGraphNode);

    Qt3DRender::QTexture2D* offscreenTexture = new Qt3DRender::QTexture2D(qmlTextureRenderer);
    offscreenTexture->setSize(1024, 1024);
    offscreenTexture->setFormat(Qt3DRender::QAbstractTexture::RGBA8_UNorm);
    offscreenTexture->setGenerateMipMaps(true);
    offscreenTexture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);
    offscreenTexture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
    offscreenTexture->setWrapMode(Qt3DRender::QTextureWrapMode(Qt3DRender::QTextureWrapMode::ClampToEdge, offscreenTexture));

    Qt3DRender::QRenderTargetOutput *output = new Qt3DRender::QRenderTargetOutput(qmlTextureRenderer);
    output->setAttachmentPoint(Qt3DRender::QRenderTargetOutput::Color0);
    output->setTexture(offscreenTexture);

    qmlTextureRenderer->setOutput(output);
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/OffscreenGui.qml")));
    qmlTextureRenderer->setItem(static_cast<QQuickItem *>(component.create()));

    Qt3DCore::QEntity* planeEntity = new Qt3DCore::QEntity(sceneRoot);
    Qt3DExtras::QPlaneMesh* planeMesh = new Qt3DExtras::QPlaneMesh(planeEntity);
    planeMesh->setWidth(4);
    planeMesh->setHeight(4);
    planeEntity->addComponent(planeMesh);

    PlaneMaterial* material = new PlaneMaterial(offscreenTexture, planeEntity);
    planeEntity->addComponent(material);

    Qt3DCore::QTransform* transform = new Qt3DCore::QTransform(planeEntity);
    transform->setRotation(QQuaternion::fromAxisAndAngle(1,0,0,90));
    planeEntity->addComponent(transform);

    view.setRootEntity(sceneRoot);
    view.show();

    return app.exec();
}
