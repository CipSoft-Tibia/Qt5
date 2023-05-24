# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(QuickTimeline
    URI "QtQuick.Timeline"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASS_NAME QtQuickTimelinePlugin
    PLUGIN_TARGET qtquicktimelineplugin
    DEPENDENCIES
        QtQuick
    SOURCES
        qquickkeyframedatautils_p.h
        qquickkeyframe.cpp qquickkeyframe_p.h
        qquicktimeline.cpp qquicktimeline_p.h
        qquicktimelineanimation.cpp qquicktimelineanimation_p.h
        qtquicktimelineglobal.h qtquicktimelineglobal_p.h
    DEFINES
        QT_BUILD_QUICK_TIMELINE_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::Quick
    LIBRARIES
        Qt::QuickPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_add_docs(QuickTimeline
    doc/qtquicktimeline.qdocconf
)
