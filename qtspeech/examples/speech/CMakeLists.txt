# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from speech.pro.

if(TARGET Qt::Widgets)
    qt_internal_add_example(hello_speak)
endif()
if(TARGET Qt::Quick)
    qt_internal_add_example(quickspeech)
endif()
