# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("scxml-ecmascriptdatamodel" PUBLIC
    SECTION "SCXML"
    LABEL "ECMAScript data model for QtScxml"
    PURPOSE "Enables the usage of ecmascript data models in SCXML state machines."
    CONDITION TARGET Qt::Qml # special case
)
qt_configure_add_summary_section(NAME "Qt Scxml")
qt_configure_add_summary_entry(ARGS "scxml-ecmascriptdatamodel")
qt_configure_end_summary_section() # end of "Qt Scxml" section
