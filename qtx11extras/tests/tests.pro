TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(xcb) {
  SUBDIRS += auto manual
}
