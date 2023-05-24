# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

TEMPLATE = app
TARGET = weatherinfo

QT += core network positioning qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = Weather
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += main.cpp \
    appmodel.cpp \
    openmeteobackend.cpp \
    openweathermapbackend.cpp \
    providerbackend.cpp \
    weatherapibackend.cpp

HEADERS += appmodel.h \
    openmeteobackend.h \
    openweathermapbackend.h \
    providerbackend.h \
    weatherapibackend.h

qml_resources.files = \
    qmldir \
    BigForecastIcon.qml \
    ForecastIcon.qml \
    WeatherIcon.qml \
    WeatherInfo.qml \
    icons/weather-few-clouds.png \
    icons/weather-fog.png \
    icons/weather-haze.png \
    icons/weather-icy.png \
    icons/weather-overcast.png \
    icons/weather-showers.png \
    icons/weather-showers-scattered.png \
    icons/weather-sleet.png \
    icons/weather-snow.png \
    icons/weather-storm.png \
    icons/weather-sunny-very-few-clouds.png \
    icons/weather-sunny.png \
    icons/weather-thundershower.png

qml_resources.prefix = /qt/qml/Weather

RESOURCES += qml_resources

ios: QMAKE_INFO_PLIST = ../shared/Info.qmake.ios.plist
macos: QMAKE_INFO_PLIST = ../shared/Info.qmake.macos.plist

target.path = $$[QT_INSTALL_EXAMPLES]/positioning/weatherinfo
INSTALLS += target

# Sign the app for location permissions to work properly
macos:!macx-xcode: QMAKE_POST_LINK += codesign -s - weatherinfo.app
