# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(weatherinfo LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/positioning/weatherinfo")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Positioning Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(weatherinfo
    main.cpp
)

set_target_properties(weatherinfo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.qt.examples.weatherinfo"
)

target_link_libraries(weatherinfo PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Positioning
    Qt::Qml
    Qt::Quick
)

if (APPLE)
    # Using absolute path for shared plist files is a Ninja bug workaround
    get_filename_component(SHARED_PLIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../shared ABSOLUTE)
    if (IOS)
        set_target_properties(weatherinfo PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.ios.plist"
        )
    else()
        set_target_properties(weatherinfo PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${SHARED_PLIST_DIR}/Info.cmake.macos.plist"
        )
    endif()
endif()

qt_add_qml_module(weatherinfo
    URI Weather
    VERSION 1.0
    SOURCES
        appmodel.cpp appmodel.h
        openmeteobackend.cpp openmeteobackend.h
        openweathermapbackend.cpp openweathermapbackend.h
        providerbackend.cpp providerbackend.h
        weatherapibackend.cpp weatherapibackend.h
    QML_FILES
        BigForecastIcon.qml
        ForecastIcon.qml
        WeatherIcon.qml
        WeatherInfo.qml
    RESOURCES
        icons/weather-few-clouds.png
        icons/weather-fog.png
        icons/weather-haze.png
        icons/weather-icy.png
        icons/weather-overcast.png
        icons/weather-showers.png
        icons/weather-sleet.png
        icons/weather-snow.png
        icons/weather-storm.png
        icons/weather-sunny-very-few-clouds.png
        icons/weather-sunny.png
        icons/weather-thundershower.png
        icons/weather-showers-scattered.png
)

install(TARGETS weatherinfo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

if(APPLE AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
    # Need to sign application for location permissions to work
    add_custom_command(TARGET weatherinfo
        POST_BUILD COMMAND codesign -s - weatherinfo.app)
endif()
