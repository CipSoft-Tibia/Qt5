// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QTimer>

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QAspectEngine>
#include <Qt3DCore/QGeometry>
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>

#include <Qt3DInput/QInputAspect>

#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QRenderAspect>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPerVertexColorMaterial>

#include <Qt3DRender/QGeometryRenderer>

#include <QPropertyAnimation>
#include <Qt3DExtras/qt3dwindow.h>
#include <Qt3DExtras/qfirstpersoncameracontroller.h>
#include <qmath.h>

QByteArray vertexBufferData;
Qt3DCore::QBuffer *vertexDataBuffer;

class TimerObject: public QObject {
    Q_OBJECT

public:
    TimerObject(QObject *parent = 0): QObject(parent) {
    }
    ~TimerObject() {
    }
private slots:
    void timeout();
private:
    float angle = 0;
};

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    Qt3DExtras::Qt3DWindow view;
    view.defaultFrameGraph()->setClearColor(QColor::fromRgbF(0.0, 0.5, 1.0, 1.0));

    // Root entity
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();

    // Camera
    Qt3DRender::QCamera *cameraEntity = view.camera();

    cameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition(QVector3D(50.0f, 20.0f, 40.0f));
    cameraEntity->setUpVector(QVector3D(0, 1, 0));
    cameraEntity->setViewCenter(QVector3D(0, 0, 0));

    // For camera controls
    Qt3DExtras::QFirstPersonCameraController *camController = new Qt3DExtras::QFirstPersonCameraController(rootEntity);
    camController->setCamera(cameraEntity);

    // Material
    Qt3DRender::QMaterial *material = new Qt3DExtras::QPerVertexColorMaterial(rootEntity);

    // Torus
    Qt3DCore::QEntity *customMeshEntity = new Qt3DCore::QEntity(rootEntity);

    // Transform
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform;
    transform->setScale(20.0f);

    // Custom Mesh (TetraHedron)
    Qt3DRender::QGeometryRenderer *customMeshRenderer = new Qt3DRender::QGeometryRenderer;
    Qt3DCore::QGeometry *customGeometry = new Qt3DCore::QGeometry(customMeshRenderer);

    vertexDataBuffer = new Qt3DCore::QBuffer(customGeometry);
    Qt3DCore::QBuffer *indexDataBuffer = new Qt3DCore::QBuffer(customGeometry);

    // vec3 for position
    // vec3 for colors
    // vec3 for normals

    /*          2
               /|\
              / | \
             / /3\ \
             0/___\ 1
    */

    // 4 distinct vertices
    vertexBufferData.resize(4 * (3 + 3 + 3) * sizeof(float));

    // Vertices
    QVector3D v0(-1.0f, 0.0f, -1.0f);
    QVector3D v1(1.0f, 0.0f, -1.0f);
    QVector3D v2(0.0f, 1.0f, 0.0f);
    QVector3D v3(0.0f, 0.0f, 1.0f);

    // Faces Normals
    QVector3D n023 = QVector3D::normal(v0, v2, v3);
    QVector3D n012 = QVector3D::normal(v0, v1, v2);
    QVector3D n310 = QVector3D::normal(v3, v1, v0);
    QVector3D n132 = QVector3D::normal(v1, v3, v2);

    // Vector Normals
    QVector3D n0 = QVector3D(n023 + n012 + n310).normalized();
    QVector3D n1 = QVector3D(n132 + n012 + n310).normalized();
    QVector3D n2 = QVector3D(n132 + n012 + n023).normalized();
    QVector3D n3 = QVector3D(n132 + n310 + n023).normalized();

    // Colors
    QVector3D red(1.0f, 0.0f, 0.0f);
    QVector3D green(0.0f, 1.0f, 0.0f);
    QVector3D blue(0.0f, 0.0f, 1.0f);
    QVector3D white(1.0f, 1.0f, 1.0f);

    const QList<QVector3D> vertices = { v0, n0, red,
                                        v1, n1, blue,
                                        v2, n2, green,
                                        v3, n3, white };

    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());
    int idx = 0;

    for (const QVector3D &v : vertices) {
        rawVertexArray[idx++] = v.x();
        rawVertexArray[idx++] = v.y();
        rawVertexArray[idx++] = v.z();
    }

    // Indices (12)
    QByteArray indexBufferData;
    indexBufferData.resize(4 * 3 * sizeof(ushort));
    ushort *rawIndexArray = reinterpret_cast<ushort *>(indexBufferData.data());

    // Front
    rawIndexArray[0] = 0;
    rawIndexArray[1] = 1;
    rawIndexArray[2] = 2;
    // Bottom
    rawIndexArray[3] = 3;
    rawIndexArray[4] = 1;
    rawIndexArray[5] = 0;
    // Left
    rawIndexArray[6] = 0;
    rawIndexArray[7] = 2;
    rawIndexArray[8] = 3;
    // Right
    rawIndexArray[9] = 1;
    rawIndexArray[10] = 3;
    rawIndexArray[11] = 2;

    vertexDataBuffer->setData(vertexBufferData);
    indexDataBuffer->setData(indexBufferData);

    // Attributes
    Qt3DCore::QAttribute *positionAttribute = new Qt3DCore::QAttribute();
    positionAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexDataBuffer);
    positionAttribute->setVertexSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(9 * sizeof(float));
    positionAttribute->setCount(4);
    positionAttribute->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());

    Qt3DCore::QAttribute *normalAttribute = new Qt3DCore::QAttribute();
    normalAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(vertexDataBuffer);
    normalAttribute->setVertexSize(3);
    normalAttribute->setByteOffset(3 * sizeof(float));
    normalAttribute->setByteStride(9 * sizeof(float));
    normalAttribute->setCount(4);
    normalAttribute->setName(Qt3DCore::QAttribute::defaultNormalAttributeName());

    Qt3DCore::QAttribute *colorAttribute = new Qt3DCore::QAttribute();
    colorAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
    colorAttribute->setBuffer(vertexDataBuffer);
    colorAttribute->setVertexSize(3);
    colorAttribute->setByteOffset(6 * sizeof(float));
    colorAttribute->setByteStride(9 * sizeof(float));
    colorAttribute->setCount(4);
    colorAttribute->setName(Qt3DCore::QAttribute::defaultColorAttributeName());

    Qt3DCore::QAttribute *indexAttribute = new Qt3DCore::QAttribute();
    indexAttribute->setAttributeType(Qt3DCore::QAttribute::IndexAttribute);
    indexAttribute->setBuffer(indexDataBuffer);
    indexAttribute->setVertexBaseType(Qt3DCore::QAttribute::UnsignedShort);
    indexAttribute->setVertexSize(1);
    indexAttribute->setByteOffset(0);
    indexAttribute->setByteStride(0);
    indexAttribute->setCount(12);

    customGeometry->addAttribute(positionAttribute);
    customGeometry->addAttribute(normalAttribute);
    customGeometry->addAttribute(colorAttribute);
    customGeometry->addAttribute(indexAttribute);

    customMeshRenderer->setInstanceCount(1);
    customMeshRenderer->setIndexOffset(0);
    customMeshRenderer->setFirstInstance(0);
    customMeshRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    customMeshRenderer->setGeometry(customGeometry);
    // 4 faces of 3 points
    customMeshRenderer->setVertexCount(12);

    customMeshEntity->addComponent(customMeshRenderer);
    customMeshEntity->addComponent(transform);
    customMeshEntity->addComponent(material);

    view.setRootEntity(rootEntity);
    view.show();

    QTimer *timer = new QTimer();
    TimerObject *timerObject = new TimerObject();
    QObject::connect(timer, SIGNAL(timeout()), timerObject, SLOT(timeout()));
    timer->start(10);

    return app.exec();
}

void TimerObject::timeout()
{
    angle += qDegreesToRadians(0.5f);

    QByteArray updateData;
    updateData.resize(3*sizeof(float));

    // Colors
    QVector3D c1(qFabs(qCos(angle + M_PI_4)), qFabs(qSin(angle)), qFabs(qCos(angle)));
    QVector3D c2(qFabs(qSin(angle)), qFabs(qCos(angle + M_PI_4)), qFabs(qSin(angle + M_PI_4)));
    QVector3D c3(qFabs(qSin(angle + M_PI_4)), qFabs(qSin(angle)), qFabs(qCos(angle)));

    const QList<QVector3D> colors = { c1, c2, c3 };

    float *rawVertexArray = reinterpret_cast<float *>(updateData.data());

    int pos = 6 * sizeof(float); //color offset
    for (const QVector3D &v : colors) {
        rawVertexArray[0] = v.x();
        rawVertexArray[1] = v.y();
        rawVertexArray[2] = v.z();
        vertexDataBuffer->updateData(pos,updateData);
        pos += 9 * sizeof(float); //stride
    }
}

#include "main.moc"
