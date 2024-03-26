// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QThread>
#include <QTimer>

#include <Qt3DInput/QInputAspect>

#include <Qt3DRender/qcamera.h>
#include <Qt3DRender/qcameralens.h>
#include <Qt3DExtras/qcylindermesh.h>
#include <Qt3DRender/qmesh.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DExtras/qphongmaterial.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtexture.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qrenderaspect.h>
#include <Qt3DExtras/qforwardrenderer.h>

#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qtransform.h>
#include <Qt3DCore/qaspectengine.h>

#include <Qt3DExtras/qt3dwindow.h>
#include <Qt3DExtras/qorbitcameracontroller.h>

#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>

using namespace Qt3DRender;

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    Qt3DExtras::Qt3DWindow view;

    // Root entity
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();

    // Camera
    Qt3DRender::QCamera *camera = view.camera();
    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 0, 20.0f));
    camera->setUpVector(QVector3D(0, 1, 0));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // For camera controls
    Qt3DExtras::QOrbitCameraController *cameraController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    cameraController->setCamera(camera);

    // Cylinder shape data
    Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh();
    mesh->setRadius(1);
    mesh->setLength(3);
    mesh->setRings(100);
    mesh->setSlices(20);

    // Transform for cylinder
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform;
    transform->setScale(1.5f);
    transform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 45.0f));

    // Material
    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(rootEntity);
    material->setDiffuse(Qt::red);

    // Cylinder
    Qt3DCore::QEntity *cylinder = new Qt3DCore::QEntity(rootEntity);
    cylinder->addComponent(mesh);
    cylinder->addComponent(transform);
    cylinder->addComponent(material);

    QTimer *timer = new QTimer(rootEntity);
    QObject::connect(timer, &QTimer::timeout, [=](){
        for (int i = 0; i < 2; i++) {
            auto *dummy = new Qt3DCore::QNode(rootEntity);
            auto *dummy2 = new Qt3DCore::QNode(dummy);
            auto *layerFilter = new QLayerFilter(dummy2);
            auto *layer = new QLayer();

            layerFilter->addLayer(layer);

            cylinder->addComponent(layer);
        }
    });
    timer->start(1000);

    QTimer *timer2 = new QTimer(rootEntity);
    QObject::connect(timer2, &QTimer::timeout, [](){
        QThread::msleep(100);
    });
    timer2->start(100);

    // Set root object of the scene
    view.setRootEntity(rootEntity);
    view.show();

    return app.exec();
}
