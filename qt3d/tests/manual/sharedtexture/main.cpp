// Copyright (C) 2018 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QPropertyAnimation>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QAspectEngine>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DRender/QGeometryRenderer>

#include <Qt3DInput/QInputAspect>

#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/Qt3DWindow>

#include "videoplayer.h"

Qt3DCore::QEntity *createScene(Qt3DExtras::Qt3DWindow *view, Qt3DRender::QAbstractTexture *diffuseTexture)
{
    // Root entity
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity;

    // Material
    Qt3DExtras::QDiffuseMapMaterial *material = new Qt3DExtras::QDiffuseMapMaterial(rootEntity);
    material->setDiffuse(diffuseTexture);
    material->setAmbient(QColor(30, 30, 30));

    // Sphere
    Qt3DCore::QEntity *sphereEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DExtras::QCuboidMesh *cuboidMesh = new Qt3DExtras::QCuboidMesh;
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform;

    transform->setRotationX(180);

    QPropertyAnimation *cubeRotateTransformAnimation = new QPropertyAnimation(transform);
    cubeRotateTransformAnimation->setTargetObject(transform);
    cubeRotateTransformAnimation->setPropertyName("rotationY");
    cubeRotateTransformAnimation->setStartValue(QVariant::fromValue(0));
    cubeRotateTransformAnimation->setEndValue(QVariant::fromValue(360));
    cubeRotateTransformAnimation->setDuration(10000);
    cubeRotateTransformAnimation->setLoopCount(-1);
    cubeRotateTransformAnimation->start();

    sphereEntity->addComponent(cuboidMesh);
    sphereEntity->addComponent(transform);
    sphereEntity->addComponent(material);

    // Camera
    Qt3DRender::QCamera *camera = view->camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 0, -5.0f));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // For camera controls
    Qt3DExtras::QOrbitCameraController *camController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    camController->setLinearSpeed( 50.0f );
    camController->setLookSpeed( 180.0f );
    camController->setCamera(camera);

    Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight();
    light->setIntensity(0.8f);
    light->setWorldDirection(camera->viewVector());
    rootEntity->addComponent(light);

    return rootEntity;
}

int main(int argc, char* argv[])
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(4);
    format.setMinorVersion(5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);

    // Will make Qt3D and QOpenGLWidget share a common context
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);

    // Multimedia player
    TextureWidget textureWidget;
    VideoPlayer *videoPlayer = new VideoPlayer(&textureWidget);

    textureWidget.resize(800, 600);
    textureWidget.show();

    // Texture object that Qt3D uses to access the texture from the video player
    Qt3DRender::QSharedGLTexture *sharedTexture = new Qt3DRender::QSharedGLTexture();

    QObject::connect(&textureWidget, &TextureWidget::textureIdChanged,
                     sharedTexture, &Qt3DRender::QSharedGLTexture::setTextureId);

    // Qt3D Scene
    Qt3DExtras::Qt3DWindow view;
    Qt3DCore::QEntity *scene = createScene(&view, sharedTexture);
    view.setRootEntity(scene);
    view.show();

    return app.exec();
}
