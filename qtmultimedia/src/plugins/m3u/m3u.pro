TARGET = qtmultimedia_m3u
QT += multimedia-private

HEADERS += qm3uhandler.h
SOURCES += qm3uhandler.cpp

OTHER_FILES += \
    m3u.json

PLUGIN_TYPE = playlistformats
PLUGIN_CLASS_NAME = QM3uPlaylistPlugin
load(qt_plugin)
