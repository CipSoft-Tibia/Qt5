# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QuickVectorGraphics Module:
#####################################################################

qt_internal_add_module(QuickVectorImageGeneratorPrivate
    INTERNAL_MODULE
    SOURCES
        generator/qsvgvisitorimpl_p.h generator/qsvgvisitorimpl.cpp
        generator/qquickgenerator_p.h generator/qquickgenerator.cpp
        generator/qquickitemgenerator_p.h generator/qquickitemgenerator.cpp
        generator/qquickqmlgenerator_p.h generator/qquickqmlgenerator.cpp
        generator/qquickanimatedproperty_p.h
        generator/qquicknodeinfo_p.h
        generator/utils_p.h
        qquickvectorimageglobal_p.h
        qquickvectorimageplugin_p.h qquickvectorimageplugin.cpp
    LIBRARIES
        Qt::Core
        Qt::QuickPrivate
        Qt::QuickShapesPrivate
        Qt::SvgPrivate
)

qt_internal_add_qml_module(QuickVectorImage
    URI "QtQuick.VectorImage"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qquickvectorimageplugin
    CLASS_NAME QtQuickVectorImagePlugin
    IMPORTS QtQuick.VectorImage.Helpers
    DEFAULT_IMPORTS QtQuick.Shapes
    SOURCES
    qquickvectorimage_p.h qquickvectorimage.cpp
    qquickvectorimage_p_p.h
    LIBRARIES
        Qt::QuickPrivate
        Qt::QuickVectorImageGeneratorPrivate
        Qt::SvgPrivate
)

qt_internal_add_qml_module(QuickVectorImageHelpers
    URI "QtQuick.VectorImage.Helpers"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qquickvectorimagehelpersplugin
    NO_PLUGIN_OPTIONAL
    CLASS_NAME QtQuickVectorImageHelpersPlugin
    SOURCES
        helpers/qquickcoloropacityanimation_p.h helpers/qquickcoloropacityanimation.cpp
        helpers/qquicktransformgroup_p.h helpers/qquicktransformgroup.cpp
    LIBRARIES
        Qt::QuickPrivate
        Qt::QuickVectorImageGeneratorPrivate
)
