// Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Extras 2.0

Entity {
    property alias particleStep: computeMaterial.particleStep
    property alias finalCollisionFactor: computeMaterial.finalCollisionFactor

    readonly property int _SPHERE: 0
    readonly property int _CUBE: 1
    readonly property int _CYLINDER: 2
    readonly property int _TORUS: 3

    property int particlesShape: _SPHERE

    signal reset()

    components: [
        RenderSettings {
            ComputeFrameGraph {
                camera: sceneCamera
            }
            // explicitly set RenderingPolicy to AlwaysRender, as changes in the
            // scene won't be reflected in actual Qt scene-graph changes (due to
            // GPU compute calls)
            renderPolicy: RenderSettings.Always
        }
    ]

    FirstPersonCameraController { camera: sceneCamera }

    Camera {
        id: sceneCamera
        projectionType: CameraLens.PerspectiveProjection
        viewCenter: Qt.vector3d(0, 0, 0)
        position: Qt.vector3d(0, 0, -800)
        nearPlane: 0.1
        farPlane: 1000
        fieldOfView: 25
        aspectRatio: 1.33
    }

    property int particlesCount: 50 * 1024
    readonly property int floatSize: 4

    function buildParticlesBuffer() {
        var byteSizeOfParticleData = 12;
        var bufferData = new Float32Array(particlesCount * byteSizeOfParticleData);
        var factor = 500.0;
        for (var i = 0; i < particlesCount; ++i) {
            var positionIdx = i * byteSizeOfParticleData;
            var velocityIdx = i * byteSizeOfParticleData + 4;
            var colorIdx = i * byteSizeOfParticleData + 8;

            for (var j = 0; j < 3; ++j) {
                bufferData[positionIdx + j] = (Math.random() - 0.5) * factor;
                bufferData[velocityIdx + j] = Math.random() * 2.0;
                bufferData[colorIdx + j] = 0.75 + Math.sin(((i / 1024.0) + j * 0.333) * 6.0) * 0.25;
            }

            bufferData[positionIdx + 3] = 1.0;
            bufferData[velocityIdx + 3] = 0.0;
            bufferData[colorIdx + 3] = 1.0;
        }
        return bufferData
    }

    Buffer {
        id: particleBuffer
        //        struct ParticleData
        //        {
        //            vec3 position;      // Aligned to 4 floats
        //            vec3 velocity;      // Aligned to 4 floats
        //            vec3 color;         // Aligned to 4 floats
        //        };
        data: buildParticlesBuffer()
    }

    onReset : {
        particleBuffer.data = buildParticlesBuffer()
    }

    Attribute {
        id: particlePositionDataAttribute
        name: "particlePosition"
        attributeType: Attribute.VertexAttribute
        vertexBaseType: Attribute.Float
        vertexSize: 3
        divisor: 1
        byteStride: 12 * floatSize
        buffer: particleBuffer
    }

    Attribute {
        id: particleColorDataAttribute
        name: "particleColor"
        attributeType: Attribute.VertexAttribute
        vertexBaseType: Attribute.Float
        vertexSize: 3
        divisor: 1
        byteOffset: 8 * floatSize
        byteStride: 12 * floatSize
        buffer: particleBuffer
    }

    ComputeMaterial {
        id: computeMaterial
        dataBuffer: particleBuffer
    }

    Entity {
        id: particleComputeEntity
        readonly property ComputeCommand particlesComputeJob: ComputeCommand {}
        components: [
            particlesComputeJob,
            computeMaterial
        ]
    }

    SphereGeometry {
        id: sphereGeometry
        rings: 10
        slices: 10
        radius: 1
        // Additional Attributes
        attributes: [
            particlePositionDataAttribute,
            particleColorDataAttribute
        ]
    }

    CuboidGeometry {
        id: cubeGeometry
        yzMeshResolution: Qt.size(2, 2)
        xzMeshResolution: Qt.size(2, 2)
        xyMeshResolution: Qt.size(2, 2)
        // Additional Attributes
        attributes: [
            particlePositionDataAttribute,
            particleColorDataAttribute
        ]
    }

    CylinderGeometry {
        id: cylinderGeometry
        rings: 10
        slices: 10
        radius: 1
        length: 1.5
        // Additional Attributes
        attributes: [
            particlePositionDataAttribute,
            particleColorDataAttribute
        ]
    }

    TorusGeometry {
        id: torusGeometry
        rings: 10
        slices: 10
        radius: 1
        minorRadius: 0.5
        // Additional Attributes
        attributes: [
            particlePositionDataAttribute,
            particleColorDataAttribute
        ]
    }

    Entity {
        id: particleRenderEntity
        readonly property GeometryRenderer particlesRenderer: GeometryRenderer {
            instanceCount: particlesCount
            indexOffset: 0
            firstInstance: 0
            primitiveType: GeometryRenderer.Triangles
            geometry:  {
                switch (particlesShape) {
                case _SPHERE:
                    return sphereGeometry;
                case _CUBE:
                    return cubeGeometry;
                case _CYLINDER:
                    return cylinderGeometry;
                case _TORUS:
                    return torusGeometry;
                }
            }
        }

        components: [
            particlesRenderer,
            computeMaterial
        ]
    }
}

