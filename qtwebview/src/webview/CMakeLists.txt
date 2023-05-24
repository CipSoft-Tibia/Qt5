# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## WebView Module:
#####################################################################

qt_internal_add_module(WebView
    PLUGIN_TYPES webview
    DEPENDENCIES
        QtQuick/auto
    SOURCES
        qabstractwebview_p.h
        qnativeviewcontroller_p.h
        qtwebviewfunctions.cpp qtwebviewfunctions.h
        qwebview.cpp qwebview_p.h
        qwebview_global.h
        qwebviewfactory.cpp qwebviewfactory_p.h
        qwebviewinterface_p.h
        qwebviewloadrequest.cpp qwebviewloadrequest_p.h
        qwebviewplugin.cpp qwebviewplugin_p.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
)

if(ANDROID)
    set_property(TARGET WebView APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
        jar/QtAndroidWebView.jar
    )
    set_property(TARGET WebView APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
        ${INSTALL_PLUGINSDIR}/webview/libplugins_webview_qtwebview_android.so
    )
    set_property(TARGET WebView APPEND PROPERTY QT_ANDROID_PERMISSIONS
        android.permission.ACCESS_FINE_LOCATION
    )
endif()
qt_internal_add_docs(WebView
    doc/qtwebview.qdocconf
)

