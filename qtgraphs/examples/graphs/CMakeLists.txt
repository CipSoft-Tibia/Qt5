# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt::Quick)
    qt_internal_add_example(axishandling)
    qt_internal_add_example(bars)
    qt_internal_add_example(scatter)
    qt_internal_add_example(surfacegallery)
endif()
if(NOT ANDROID AND NOT IOS AND NOT WINRT)
    qt_internal_add_example(widgetgraphgallery)
    qt_internal_add_example(widgetvolumetric)
endif()
