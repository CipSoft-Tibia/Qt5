# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from json.pro.

#####################################################################
## tst_json Test:
#####################################################################

# special case begin
qt_internal_add_test(tst_qt5compat_json
# special case end
    SOURCES
        tst_qtjson.cpp
    LIBRARIES
        Qt::Core5Compat
        Qt::CorePrivate
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:json.pro:NOT ANDROID:
# TESTDATA = "bom.json" "test.json" "test.bjson" "test3.json" "test2.json"

if(ANDROID)
    # Resources:
    set(json_resource_files
        "bom.json"
        "test.bjson"
        "test.json"
        "test2.json"
        "test3.json"
    )

# special case begin
    qt_internal_add_resource(tst_qt5compat_json "json"
# special case end
        PREFIX
            "/"
        FILES
            ${json_resource_files}
    )
endif()

# special case begin
qt_internal_extend_target(tst_qt5compat_json CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
# special case end
    DEFINES
        QT_NO_DOUBLECONVERSION
)
