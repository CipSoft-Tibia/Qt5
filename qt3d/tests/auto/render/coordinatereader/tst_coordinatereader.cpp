// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QTest>
#include <qbackendnodetester.h>
#include <Qt3DRender/qgeometryrenderer.h>
#include <Qt3DCore/qbuffer.h>
#include <private/trianglesvisitor_p.h>
#include <private/nodemanagers_p.h>
#include <private/managers_p.h>
#include <private/geometryrenderer_p.h>
#include <private/geometryrenderermanager_p.h>
#include <private/buffermanager_p.h>
#include "testrenderer.h"

using namespace Qt3DRender::Render;

class TestReader : public CoordinateReader
{
public:
    TestReader(NodeManagers *manager)
        : CoordinateReader(manager)
    {

    }
    NodeManagers *manager() const
    {
        return m_manager;
    }

    Attribute *attribute() const
    {
        return m_attribute;
    }

    Buffer *buffer() const
    {
        return m_buffer;
    }

    BufferInfo bufferInfo() const
    {
        return m_bufferInfo;
    }
    bool verifyCoordinate(uint index, Vector4D value)
    {
        return qFuzzyCompare(getCoordinate(index), value);
    }
};

class tst_CoordinateReader : public Qt3DCore::QBackendNodeTester
{
    Q_OBJECT

private Q_SLOTS:

    void checkInitialize()
    {
        // WHEN
        QScopedPointer<NodeManagers> nodeManagers(new NodeManagers());
        TestReader reader(nodeManagers.data());

        // THEN
        QCOMPARE(reader.manager(), nodeManagers.data());
    }

    void checkSetEmptyGeometry()
    {
        QScopedPointer<NodeManagers> nodeManagers(new NodeManagers());
        QScopedPointer<Qt3DRender::QGeometryRenderer> geometryRenderer(
                    new Qt3DRender::QGeometryRenderer());
        TestReader reader(nodeManagers.data());
        TestRenderer renderer;

        GeometryRenderer *backendRenderer = nodeManagers->geometryRendererManager()
                                            ->getOrCreateResource(geometryRenderer->id());
        backendRenderer->setRenderer(&renderer);
        backendRenderer->setManager(nodeManagers->geometryRendererManager());
        simulateInitializationSync(geometryRenderer.data(), backendRenderer);

        // WHEN
        bool ret = reader.setGeometry(backendRenderer, QString(""));

        // THEN
        QCOMPARE(ret, false);
        QCOMPARE(reader.attribute(), nullptr);
        QCOMPARE(reader.buffer(), nullptr);
    }

    void checkSetGeometry()
    {
        QScopedPointer<NodeManagers> nodeManagers(new NodeManagers());
        Qt3DCore::QGeometry *geometry = new Qt3DCore::QGeometry();
        QScopedPointer<Qt3DRender::QGeometryRenderer> geometryRenderer(
                    new Qt3DRender::QGeometryRenderer());
        QScopedPointer<Qt3DCore::QAttribute> positionAttribute(new Qt3DCore::QAttribute());
        QScopedPointer<Qt3DCore::QBuffer> dataBuffer(new Qt3DCore::QBuffer());
        TestReader reader(nodeManagers.data());
        TestRenderer renderer;

        QByteArray data;
        data.resize(sizeof(float) * 3 * 3 * 2);
        float *dataPtr = reinterpret_cast<float *>(data.data());
        dataPtr[0] = 0;
        dataPtr[1] = 0;
        dataPtr[2] = 1.0f;
        dataPtr[3] = 1.0f;
        dataPtr[4] = 0;
        dataPtr[5] = 0;
        dataPtr[6] = 0;
        dataPtr[7] = 1.0f;
        dataPtr[8] = 0;

        dataPtr[9] = 0;
        dataPtr[10] = 0;
        dataPtr[11] = 1.0f;
        dataPtr[12] = 1.0f;
        dataPtr[13] = 0;
        dataPtr[14] = 0;
        dataPtr[15] = 0;
        dataPtr[16] = 1.0f;
        dataPtr[17] = 0;
        dataBuffer->setData(data);
        Buffer *backendBuffer = nodeManagers->bufferManager()->getOrCreateResource(dataBuffer->id());
        backendBuffer->setRenderer(&renderer);
        backendBuffer->setManager(nodeManagers->bufferManager());
        simulateInitializationSync(dataBuffer.data(), backendBuffer);

        positionAttribute->setBuffer(dataBuffer.data());
        positionAttribute->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());
        positionAttribute->setVertexBaseType(Qt3DCore::QAttribute::Float);
        positionAttribute->setVertexSize(3);
        positionAttribute->setCount(6);
        positionAttribute->setByteStride(3 * 4);
        positionAttribute->setByteOffset(0);
        positionAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
        geometry->addAttribute(positionAttribute.data());

        geometryRenderer->setGeometry(geometry);
        geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);

        Attribute *backendAttribute = nodeManagers->attributeManager()
                                      ->getOrCreateResource(positionAttribute->id());
        backendAttribute->setRenderer(&renderer);
        simulateInitializationSync(positionAttribute.data(), backendAttribute);

        Geometry *backendGeometry = nodeManagers->geometryManager()
                                    ->getOrCreateResource(geometry->id());
        backendGeometry->setRenderer(&renderer);
        simulateInitializationSync(geometry, backendGeometry);

        GeometryRenderer *backendRenderer = nodeManagers->geometryRendererManager()
                                            ->getOrCreateResource(geometryRenderer->id());
        backendRenderer->setRenderer(&renderer);
        backendRenderer->setManager(nodeManagers->geometryRendererManager());
        simulateInitializationSync(geometryRenderer.data(), backendRenderer);

        // WHEN
        bool ret = reader.setGeometry(backendRenderer,
                                      Qt3DCore::QAttribute::defaultPositionAttributeName());

        // THEN
        QCOMPARE(ret, true);
        QCOMPARE(reader.attribute(), backendAttribute);
        QCOMPARE(reader.buffer(), backendBuffer);
        QCOMPARE(reader.bufferInfo().type, Qt3DCore::QAttribute::Float);
        QCOMPARE(reader.bufferInfo().dataSize, 3u);
        QCOMPARE(reader.bufferInfo().count, 6u);
        QCOMPARE(reader.bufferInfo().byteStride, 12u);
        QCOMPARE(reader.bufferInfo().byteOffset, 0u);
    }

    void testReadCoordinate()
    {
        QScopedPointer<NodeManagers> nodeManagers(new NodeManagers());
        Qt3DCore::QGeometry *geometry = new Qt3DCore::QGeometry();
        QScopedPointer<Qt3DRender::QGeometryRenderer> geometryRenderer(
                    new Qt3DRender::QGeometryRenderer());
        QScopedPointer<Qt3DCore::QAttribute> positionAttribute(new Qt3DCore::QAttribute());
        QScopedPointer<Qt3DCore::QBuffer> dataBuffer(new Qt3DCore::QBuffer());
        TestReader reader(nodeManagers.data());
        TestRenderer renderer;

        QByteArray data;
        data.resize(sizeof(float) * 3 * 3 * 2);
        float *dataPtr = reinterpret_cast<float *>(data.data());
        dataPtr[0] = 0;
        dataPtr[1] = 0;
        dataPtr[2] = 1.0f;
        dataPtr[3] = 1.0f;
        dataPtr[4] = 0;
        dataPtr[5] = 0;
        dataPtr[6] = 0;
        dataPtr[7] = 1.0f;
        dataPtr[8] = 0;

        dataPtr[9] = 0;
        dataPtr[10] = 0;
        dataPtr[11] = 1.0f;
        dataPtr[12] = 1.0f;
        dataPtr[13] = 0;
        dataPtr[14] = 0;
        dataPtr[15] = 0;
        dataPtr[16] = 1.0f;
        dataPtr[17] = 0;
        dataBuffer->setData(data);
        Buffer *backendBuffer = nodeManagers->bufferManager()->getOrCreateResource(dataBuffer->id());
        backendBuffer->setRenderer(&renderer);
        backendBuffer->setManager(nodeManagers->bufferManager());
        simulateInitializationSync(dataBuffer.data(), backendBuffer);

        positionAttribute->setBuffer(dataBuffer.data());
        positionAttribute->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());
        positionAttribute->setVertexBaseType(Qt3DCore::QAttribute::Float);
        positionAttribute->setVertexSize(3);
        positionAttribute->setCount(6);
        positionAttribute->setByteStride(3 * 4);
        positionAttribute->setByteOffset(0);
        positionAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
        geometry->addAttribute(positionAttribute.data());

        geometryRenderer->setGeometry(geometry);
        geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);

        Attribute *backendAttribute = nodeManagers->attributeManager()
                                      ->getOrCreateResource(positionAttribute->id());
        backendAttribute->setRenderer(&renderer);
        simulateInitializationSync(positionAttribute.data(), backendAttribute);

        Geometry *backendGeometry = nodeManagers->geometryManager()
                                    ->getOrCreateResource(geometry->id());
        backendGeometry->setRenderer(&renderer);
        simulateInitializationSync(geometry, backendGeometry);

        GeometryRenderer *backendRenderer = nodeManagers->geometryRendererManager()
                                            ->getOrCreateResource(geometryRenderer->id());
        backendRenderer->setRenderer(&renderer);
        backendRenderer->setManager(nodeManagers->geometryRendererManager());
        simulateInitializationSync(geometryRenderer.data(), backendRenderer);

        // WHEN
        bool ret = reader.setGeometry(backendRenderer,
                                      Qt3DCore::QAttribute::defaultPositionAttributeName());

        // THEN
        QCOMPARE(ret, true);

        QVERIFY(reader.verifyCoordinate(0, Vector4D(0, 0, 1, 1)));
        QVERIFY(reader.verifyCoordinate(1, Vector4D(1, 0, 0, 1)));
        QVERIFY(reader.verifyCoordinate(2, Vector4D(0, 1, 0, 1)));
        QVERIFY(reader.verifyCoordinate(3, Vector4D(0, 0, 1, 1)));
        QVERIFY(reader.verifyCoordinate(4, Vector4D(1, 0, 0, 1)));
        QVERIFY(reader.verifyCoordinate(5, Vector4D(0, 1, 0, 1)));
    }

    void testReadCoordinateVec4()
    {
        QScopedPointer<NodeManagers> nodeManagers(new NodeManagers());
        Qt3DCore::QGeometry *geometry = new Qt3DCore::QGeometry();
        QScopedPointer<Qt3DRender::QGeometryRenderer> geometryRenderer(
                    new Qt3DRender::QGeometryRenderer());
        QScopedPointer<Qt3DCore::QAttribute> positionAttribute(new Qt3DCore::QAttribute());
        QScopedPointer<Qt3DCore::QBuffer> dataBuffer(new Qt3DCore::QBuffer());
        TestReader reader(nodeManagers.data());
        TestRenderer renderer;

        QByteArray data;
        data.resize(sizeof(float) * 4 * 3 * 2);
        float *dataPtr = reinterpret_cast<float *>(data.data());
        dataPtr[0] = 0;
        dataPtr[1] = 0;
        dataPtr[2] = 1.0f;
        dataPtr[3] = 1.0f;

        dataPtr[4] = 1.0f;
        dataPtr[5] = 0;
        dataPtr[6] = 0;
        dataPtr[7] = 1.0f;

        dataPtr[8] = 0;
        dataPtr[9] = 1.0f;
        dataPtr[10] = 0;
        dataPtr[11] = 0;

        dataPtr[12] = 0;
        dataPtr[13] = 0;
        dataPtr[14] = 1.0f;
        dataPtr[15] = 0;

        dataPtr[16] = 1.0f;
        dataPtr[17] = 0;
        dataPtr[18] = 0;
        dataPtr[19] = 0;

        dataPtr[20] = 0;
        dataPtr[21] = 1.0f;
        dataPtr[22] = 0;
        dataPtr[23] = 1.0f;

        dataBuffer->setData(data);
        Buffer *backendBuffer = nodeManagers->bufferManager()->getOrCreateResource(dataBuffer->id());
        backendBuffer->setRenderer(&renderer);
        backendBuffer->setManager(nodeManagers->bufferManager());
        simulateInitializationSync(dataBuffer.data(), backendBuffer);

        positionAttribute->setBuffer(dataBuffer.data());
        positionAttribute->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());
        positionAttribute->setVertexBaseType(Qt3DCore::QAttribute::Float);
        positionAttribute->setVertexSize(4);
        positionAttribute->setCount(6);
        positionAttribute->setByteStride(4 * 4);
        positionAttribute->setByteOffset(0);
        positionAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
        geometry->addAttribute(positionAttribute.data());

        geometryRenderer->setGeometry(geometry);
        geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);

        Attribute *backendAttribute = nodeManagers->attributeManager()
                                      ->getOrCreateResource(positionAttribute->id());
        backendAttribute->setRenderer(&renderer);
        simulateInitializationSync(positionAttribute.data(), backendAttribute);

        Geometry *backendGeometry = nodeManagers->geometryManager()
                                    ->getOrCreateResource(geometry->id());
        backendGeometry->setRenderer(&renderer);
        simulateInitializationSync(geometry, backendGeometry);

        GeometryRenderer *backendRenderer = nodeManagers->geometryRendererManager()
                                            ->getOrCreateResource(geometryRenderer->id());
        backendRenderer->setRenderer(&renderer);
        backendRenderer->setManager(nodeManagers->geometryRendererManager());
        simulateInitializationSync(geometryRenderer.data(), backendRenderer);

        // WHEN
        bool ret = reader.setGeometry(backendRenderer,
                                      Qt3DCore::QAttribute::defaultPositionAttributeName());

        // THEN
        QCOMPARE(ret, true);

        QVERIFY(reader.verifyCoordinate(0, Vector4D(0, 0, 1, 1)));
        QVERIFY(reader.verifyCoordinate(1, Vector4D(1, 0, 0, 1)));
        QVERIFY(reader.verifyCoordinate(2, Vector4D(0, 1, 0, 0)));
        QVERIFY(reader.verifyCoordinate(3, Vector4D(0, 0, 1, 0)));
        QVERIFY(reader.verifyCoordinate(4, Vector4D(1, 0, 0, 0)));
        QVERIFY(reader.verifyCoordinate(5, Vector4D(0, 1, 0, 1)));
    }

    void testReadCoordinateFromAttribute()
    {
        QScopedPointer<NodeManagers> nodeManagers(new NodeManagers());
        Qt3DCore::QGeometry *geometry = new Qt3DCore::QGeometry();
        QScopedPointer<Qt3DRender::QGeometryRenderer> geometryRenderer(
                    new Qt3DRender::QGeometryRenderer());
        QScopedPointer<Qt3DCore::QAttribute> positionAttribute(new Qt3DCore::QAttribute());
        QScopedPointer<Qt3DCore::QAttribute> texcoordAttribute(new Qt3DCore::QAttribute());
        QScopedPointer<Qt3DCore::QBuffer> dataBuffer(new Qt3DCore::QBuffer());
        TestReader reader(nodeManagers.data());
        TestRenderer renderer;

        QByteArray data;
        data.resize(sizeof(float) * 3 * 3 * 2);
        float *dataPtr = reinterpret_cast<float *>(data.data());
        dataPtr[0] = 0;
        dataPtr[1] = 0;
        dataPtr[2] = 1.0f;

        dataPtr[3] = 1.0f;
        dataPtr[4] = 0;
        dataPtr[5] = 0;

        dataPtr[6] = 0;
        dataPtr[7] = 1.0f;
        dataPtr[8] = 0;

        dataPtr[9] = 0;
        dataPtr[10] = 0;
        dataPtr[11] = 1.0f;

        dataPtr[12] = 1.0f;
        dataPtr[13] = 0;
        dataPtr[14] = 0;

        dataPtr[15] = 0;
        dataPtr[16] = 1.0f;
        dataPtr[17] = 0;

        dataBuffer->setData(data);
        Buffer *backendBuffer = nodeManagers->bufferManager()->getOrCreateResource(dataBuffer->id());
        backendBuffer->setRenderer(&renderer);
        backendBuffer->setManager(nodeManagers->bufferManager());
        simulateInitializationSync(dataBuffer.data(), backendBuffer);

        positionAttribute->setBuffer(dataBuffer.data());
        positionAttribute->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());
        positionAttribute->setVertexBaseType(Qt3DCore::QAttribute::Float);
        positionAttribute->setVertexSize(3);
        positionAttribute->setCount(3);
        positionAttribute->setByteStride(3 * 4 * 2);
        positionAttribute->setByteOffset(0);
        positionAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
        geometry->addAttribute(positionAttribute.data());

        texcoordAttribute->setBuffer(dataBuffer.data());
        texcoordAttribute->setName(Qt3DCore::QAttribute::defaultTextureCoordinateAttributeName());
        texcoordAttribute->setVertexBaseType(Qt3DCore::QAttribute::Float);
        texcoordAttribute->setVertexSize(3);
        texcoordAttribute->setCount(6);
        texcoordAttribute->setByteStride(3 * 4 * 2);
        texcoordAttribute->setByteOffset(3 * 4);
        texcoordAttribute->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
        geometry->addAttribute(texcoordAttribute.data());

        geometryRenderer->setGeometry(geometry);
        geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);

        Attribute *backendAttribute = nodeManagers->attributeManager()->getOrCreateResource(
                                        positionAttribute->id());
        backendAttribute->setRenderer(&renderer);
        simulateInitializationSync(positionAttribute.data(), backendAttribute);

        Attribute *backendTexcoordAttribute = nodeManagers->attributeManager()
                                              ->getOrCreateResource(texcoordAttribute->id());
        backendTexcoordAttribute->setRenderer(&renderer);
        simulateInitializationSync(texcoordAttribute.data(), backendTexcoordAttribute);

        Geometry *backendGeometry = nodeManagers->geometryManager()
                                    ->getOrCreateResource(geometry->id());
        backendGeometry->setRenderer(&renderer);
        simulateInitializationSync(geometry, backendGeometry);

        GeometryRenderer *backendRenderer = nodeManagers->geometryRendererManager()
                                            ->getOrCreateResource(geometryRenderer->id());
        backendRenderer->setRenderer(&renderer);
        backendRenderer->setManager(nodeManagers->geometryRendererManager());
        simulateInitializationSync(geometryRenderer.data(), backendRenderer);

        // WHEN
        bool ret = reader.setGeometry(backendRenderer,
                                      Qt3DCore::QAttribute::defaultPositionAttributeName());

        // THEN
        QCOMPARE(ret, true);

        QVERIFY(reader.verifyCoordinate(0, Vector4D(0, 0, 1, 1)));
        QVERIFY(reader.verifyCoordinate(1, Vector4D(0, 1, 0, 1)));
        QVERIFY(reader.verifyCoordinate(2, Vector4D(1, 0, 0, 1)));

        // WHEN
        ret = reader.setGeometry(backendRenderer,
                                 Qt3DCore::QAttribute::defaultTextureCoordinateAttributeName());

        // THEN
        QCOMPARE(ret, true);

        QVERIFY(reader.verifyCoordinate(0, Vector4D(1, 0, 0, 1)));
        QVERIFY(reader.verifyCoordinate(1, Vector4D(0, 0, 1, 1)));
        QVERIFY(reader.verifyCoordinate(2, Vector4D(0, 1, 0, 1)));
    }

};

QTEST_MAIN(tst_CoordinateReader)

#include "tst_coordinatereader.moc"
