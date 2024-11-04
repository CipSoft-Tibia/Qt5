# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(TARGET Qt6::Quick
        AND TARGET Qt6::QuickControls2
        AND TARGET Qt6::qtprotobufgen
        AND TARGET Qt6::qtgrpcgen)
    qt_internal_add_example(magic8ball)
    if(QT_FEATURE_clipboard)
        qt_internal_add_example(chat)
    endif()
endif()
