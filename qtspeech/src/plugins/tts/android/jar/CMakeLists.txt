# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_get_android_sdk_jar_for_api("android-21" android_sdk)

set(java_sources
    src/org/qtproject/qt/android/speech/QtTextToSpeech.java
)

qt_internal_add_jar(QtAndroidTextToSpeech
    INCLUDE_JARS ${android_sdk}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(QtAndroidTextToSpeech
    DESTINATION ${destination}
    COMPONENT Devel
)
