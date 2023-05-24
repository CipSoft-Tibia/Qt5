# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT ANDROID)
    qt_internal_add_example(pingpong)
endif()
if(TARGET Qt::Widgets)
    qt_internal_add_example(rogue)
    qt_internal_add_example(trafficlight)
    if(QT_FEATURE_animation)
        qt_internal_add_example(moveblocks)
    endif()
endif()
