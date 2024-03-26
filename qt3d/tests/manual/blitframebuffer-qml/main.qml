// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.2 as QQ2
import Qt3D.Core 2.0
import Qt3D.Render 2.10
import Qt3D.Input 2.0
import Qt3D.Extras 2.0

Entity {
    id: sceneRoot

    Camera {
        id: camera
        projectionType: CameraLens.PerspectiveProjection
        fieldOfView: 45
        aspectRatio: 16/9
        nearPlane : 0.1
        farPlane : 1000.0
        position: Qt.vector3d( 0.0, 0.0, -40.0 )
        upVector: Qt.vector3d( 0.0, 1.0, 0.0 )
        viewCenter: Qt.vector3d( 0.0, 0.0, 0.0 )
    }

    RenderTarget {
        id: intermediateRenderTarget
        attachments : [
            RenderTargetOutput {
                objectName : "color"
                attachmentPoint : RenderTargetOutput.Color0
                texture : Texture2D {
                    id : colorAttachment0
                    width : 1024
                    height : 768
                    format : Texture.R32F
                    generateMipMaps : false
                    magnificationFilter : Texture.Linear
                    minificationFilter : Texture.Linear
                    wrapMode {
                        x: WrapMode.ClampToEdge
                        y: WrapMode.ClampToEdge
                    }
                }
            },
            RenderTargetOutput {
                objectName : "color"
                attachmentPoint : RenderTargetOutput.Color1
                texture : Texture2D {
                    id : colorAttachment1
                    width : 1024
                    height : 1024
                    format : Texture.R32F
                    generateMipMaps : false
                    magnificationFilter : Texture.Linear
                    minificationFilter : Texture.Linear
                    wrapMode {
                        x: WrapMode.ClampToEdge
                        y: WrapMode.ClampToEdge
                    }
                }
            }
        ]
    }

    components: [
        RenderSettings {
            activeFrameGraph: RenderSurfaceSelector {
                Viewport {
                    RenderTargetSelector {
                        target: intermediateRenderTarget
                        NoDraw  {}
                    }

                    RenderTargetSelector {
                        target: RenderTarget {
                            id: renderTarget
                            attachments : [
                                RenderTargetOutput {
                                    objectName : "color"
                                    attachmentPoint : RenderTargetOutput.Color0
                                    texture : Texture2D {
                                        id : colorAttachment
                                        width : 1024
                                        height : 768
                                        format : Texture.RGBA32F
                                        generateMipMaps : false
                                        magnificationFilter : Texture.Linear
                                        minificationFilter : Texture.Linear
                                        wrapMode {
                                            x: WrapMode.ClampToEdge
                                            y: WrapMode.ClampToEdge
                                        }
                                    }
                                },
                                RenderTargetOutput {
                                    objectName : "depth"
                                    attachmentPoint : RenderTargetOutput.Depth
                                    texture : Texture2D {
                                        id : depthAttachment
                                        width : 1024
                                        height : 1024
                                        format : Texture.D32F
                                        generateMipMaps : false
                                        magnificationFilter : Texture.Linear
                                        minificationFilter : Texture.Linear
                                        wrapMode {
                                            x: WrapMode.ClampToEdge
                                            y: WrapMode.ClampToEdge
                                        }
                                    }
                                }
                            ]
                        }
                        ClearBuffers {
                            clearColor: "white"
                            buffers: ClearBuffers.ColorDepthBuffer
                            CameraSelector {
                                camera: camera
                            }
                        }
                    }

                    NoDraw{

                        BlitFramebuffer {
                            source: renderTarget
                            destination: intermediateRenderTarget
                            sourceRect: Qt.rect(0,0,1024,768)
                            destinationRect: Qt.rect(0,0,1024,1024)
                            sourceAttachmentPoint: RenderTargetOutput.Color0
                            destinationAttachmentPoint: RenderTargetOutput.Color0
                            interpolationMethod: BlitFramebuffer.Linear
                        }

                        BlitFramebuffer {
                            source: intermediateRenderTarget
                            sourceRect: Qt.rect(0,0,1024,1024)
                            destinationRect: Qt.rect(0,0,512,384)
                            sourceAttachmentPoint: RenderTargetOutput.Color0
                            interpolationMethod: BlitFramebuffer.Linear
                        }

                        BlitFramebuffer {
                            source: renderTarget
                            sourceRect: Qt.rect(0,0,1024,768)
                            destinationRect: Qt.rect(0,384,512,384)
                            sourceAttachmentPoint: RenderTargetOutput.Color0
                            interpolationMethod: BlitFramebuffer.Linear
                        }

                        BlitFramebuffer {
                            source: renderTarget
                            sourceRect: Qt.rect(128,200,256,256)
                            destinationRect: Qt.rect(512,384,512,384)
                            sourceAttachmentPoint: RenderTargetOutput.Color0
                            interpolationMethod: BlitFramebuffer.Linear
                        }

                        BlitFramebuffer {
                            source: renderTarget
                            sourceRect: Qt.rect(128,200,256,256)
                            destinationRect: Qt.rect(512,0,512,384)
                            sourceAttachmentPoint: RenderTargetOutput.Color0
                            interpolationMethod: BlitFramebuffer.Nearest
                        }
                    }
                }
            }
        }
    ]

    PhongMaterial {
        id: material
    }

    TorusMesh {
        id: torusMesh
        radius: 5
        minorRadius: 1
        rings: 100
        slices: 20
    }

    Transform {
        id: torusTransform
        scale3D: Qt.vector3d(1.5, 1, 0.5)
        rotation: fromAxisAndAngle(Qt.vector3d(1, 0, 0), 45)
    }

    Entity {
        id: torusEntity
        components: [ torusMesh, material, torusTransform ]
    }

    SphereMesh {
        id: sphereMesh
        radius: 3
    }

    Transform {
        id: sphereTransform
        property real userAngle: 0.0
        matrix: {
            var m = Qt.matrix4x4();
            m.rotate(userAngle, Qt.vector3d(0, 1, 0));
            m.translate(Qt.vector3d(20, 0, 0));
            return m;
        }
    }

    QQ2.NumberAnimation {
        target: sphereTransform
        property: "userAngle"
        duration: 10000
        from: 0
        to: 360

        loops: QQ2.Animation.Infinite
        running: true
    }

    Entity {
        id: sphereEntity
        components: [ sphereMesh, material, sphereTransform ]
    }
}
