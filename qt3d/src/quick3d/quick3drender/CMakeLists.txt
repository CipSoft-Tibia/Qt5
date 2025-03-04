# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quick3drender.pro.

#####################################################################
## Qt::3DQuickRender Module:
#####################################################################

qt_internal_add_module(3DQuickRender
    SOURCES
        items/quick3deffect.cpp items/quick3deffect_p.h
        items/quick3dlayerfilter.cpp items/quick3dlayerfilter_p.h
        items/quick3dmaterial.cpp items/quick3dmaterial_p.h
        items/quick3dmemorybarrier.cpp items/quick3dmemorybarrier_p.h
        items/quick3dparameter.cpp items/quick3dparameter_p.h
        items/quick3dparameter_p_p.h
        items/quick3draycaster.cpp items/quick3draycaster_p.h
        items/quick3draycaster_p_p.h
        items/quick3drenderpass.cpp items/quick3drenderpass_p.h
        items/quick3drenderpassfilter.cpp items/quick3drenderpassfilter_p.h
        items/quick3drendertargetoutput.cpp items/quick3drendertargetoutput_p.h
        items/quick3dscene.cpp items/quick3dscene_p.h
        items/quick3dscreenraycaster.cpp items/quick3dscreenraycaster_p.h
        items/quick3dscreenraycaster_p_p.h
        items/quick3dshaderdata.cpp items/quick3dshaderdata_p.h
        items/quick3dshaderdataarray.cpp items/quick3dshaderdataarray_p.h
        items/quick3dstateset.cpp items/quick3dstateset_p.h
        items/quick3dtechnique.cpp items/quick3dtechnique_p.h
        items/quick3dtechniquefilter.cpp items/quick3dtechniquefilter_p.h
        items/quick3dtexture.cpp items/quick3dtexture_p.h
        items/quick3dviewport.cpp items/quick3dviewport_p.h
        qt3dquickrender_global.cpp qt3dquickrender_global.h qt3dquickrender_global_p.h
        qt3dquickrendernodefactory.cpp qt3dquickrendernodefactory_p.h
    DEFINES
        QT_NO_CONTEXTLESS_CONNECT
    INCLUDE_DIRECTORIES
        items
    LIBRARIES
        Qt::3DCorePrivate
        Qt::3DQuickPrivate
        Qt::3DRenderPrivate
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::3DCore
        Qt::3DQuick
        Qt::3DRender
        Qt::Core
        Qt::Gui
        Qt::Qml
    PRIVATE_MODULE_INTERFACE
        Qt::3DCorePrivate
        Qt::3DQuickPrivate
        Qt::3DRenderPrivate
        Qt::CorePrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:quick3drender.pro:<TRUE>:
# MODULE = "3DQuickRender"

## Scopes:
#####################################################################

qt_internal_extend_target(3DQuickRender CONDITION gcov
    COMPILE_OPTIONS
        -fprofile-arcs
        -ftest-coverage
    LINK_OPTIONS
        "-fprofile-arcs"
        "-ftest-coverage"
)
