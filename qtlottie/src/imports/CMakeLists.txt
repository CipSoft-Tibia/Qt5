# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(lottieqtplugin
    URI "Qt.labs.lottieqt"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME BodymovinPlugin
    PLUGIN_TARGET lottieqtplugin
    DEPENDENCIES
        QtQuick
    SOURCES
        lottieanimation.cpp lottieanimation.h
        rasterrenderer/batchrenderer.cpp rasterrenderer/batchrenderer.h
        rasterrenderer/lottierasterrenderer.cpp rasterrenderer/lottierasterrenderer.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::Quick
        Qt::BodymovinPrivate
)

qt_internal_add_docs(lottieqtplugin
    doc/qtlottieanimation.qdocconf
)
