# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_internal_add_manual_test(tst_testbed
    GUI
    SOURCES
        main.cpp
)

target_link_libraries(tst_testbed PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Graphs
)

qt6_add_qml_module(tst_testbed
    URI Testbed
    NO_RESOURCE_TARGET_PATH
    SOURCES
        custombarseries.h custombarseries.cpp
        customlineseries.h customlineseries.cpp
    QML_FILES
        qml/testbed/main.qml
        qml/testbed/StartupView.qml
        qml/testbed/SettingsView.qml
        qml/testbed/CustomCheckBox.qml
        qml/testbed/CustomLabel.qml
        qml/testbed/CustomSelectionBox.qml
        qml/testbed/CustomSlider.qml
        qml/testbed/BarThemes.qml
        qml/testbed/BarSelection.qml
        qml/testbed/LineProperties.qml
        qml/testbed/AxisGridSetup.qml
        qml/testbed/Callout.qml
        qml/testbed/ScatterProperties.qml
        qml/testbed/CppBarSeries.qml
        qml/testbed/CppLineSeries.qml
    RESOURCES
        qml/testbed/images/arrow_icon.png
        qml/testbed/images/icon_settings.png
        qml/testbed/images/happy_box.png
        qml/testbed/images/Sevillana-Regular.ttf
)
