# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qshaderbaker.pro.

#####################################################################
## tst_qshaderbaker Test:
#####################################################################

qt_internal_add_test(tst_qshaderbaker
    SOURCES
        tst_qshaderbaker.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::ShaderToolsPrivate
)

# Resources:
set(qshaderbaker_resource_files
    "data"
)

qt_internal_add_resource(tst_qshaderbaker "qshaderbaker"
    PREFIX
        "/"
    FILES
        ${qshaderbaker_resource_files}
)

