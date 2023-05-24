# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QWinrtWebViewPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QWinrtWebViewPlugin
    OUTPUT_NAME qtwebview_winrt
    PLUGIN_TYPE webview
    SOURCES
        qwinrtwebview.cpp qwinrtwebview_p.h
        qwinrtwebviewplugin.cpp
    LIBRARIES
        urlmon
        Qt::Core
        Qt::Gui
        Qt::WebViewPrivate
    NO_PCH_SOURCES
        "qwinrtwebview.cpp"
)

#### Keys ignored in scope 1:.:.:winrt.pro:<TRUE>:
# DISTFILES = "winrt.json"
# OTHER_FILES = <EMPTY>
# WINDOWS_SDK_VERSION = "$$member($$list $$split WINDOWS_SDK_VERSION_STRING, . , 2)"
# WINDOWS_SDK_VERSION_STRING = "$$(WindowsSDKVersion)"

## Scopes:
#####################################################################

qt_internal_extend_target(QWinrtWebViewPlugin CONDITION QT_WINDOWS_SDK_VERSION STRLESS 16299
    DEFINES
        QT_UCRTVERSION=(  WINDOWS_SDK_VERSION_STRING, . , 2)
        QT_WINRT_URLMKGETSESSIONOPTION_NOT_AVAILABLE
        QT_WINRT_URLMKSETSESSIONOPTION_NOT_AVAILABLE
)
