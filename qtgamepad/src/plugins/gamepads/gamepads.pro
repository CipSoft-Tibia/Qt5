TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private gamepad-private
qtConfig(sdl2): SUBDIRS += sdl2
!android: qtConfig(evdev): SUBDIRS += evdev
win32: SUBDIRS += xinput
darwin: !watchos: SUBDIRS += darwin
android: !android-embedded: SUBDIRS += android
