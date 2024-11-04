# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

file(GLOB_RECURSE designer_image_files
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "images/*.png"
)
qt_path_join(images_destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtGraphs/designer/images")
qt_copy_or_install(
    FILES
        ${designer_image_files}
    DESTINATION "${images_destination}"
)

qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtGraphs/designer")
qt_copy_or_install(
    FILES
        GraphsViewSpecifics.qml
        qtgraphs2d.metainfo
    DESTINATION "${destination}"
)

add_subdirectory(default)
