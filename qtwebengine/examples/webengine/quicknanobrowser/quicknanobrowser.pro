requires(qtConfig(accessibility))

TEMPLATE = app
TARGET = quicknanobrowser

HEADERS = utils.h
SOURCES = main.cpp

OTHER_FILES += ApplicationRoot.qml \
               BrowserDialog.qml \
               BrowserWindow.qml \
               DownloadView.qml \
               FullScreenNotification.qml

RESOURCES += resources.qrc

QT += qml quick webengine

qtHaveModule(widgets) {
    QT += widgets # QApplication is required to get native styling with QtQuickControls
}

target.path = $$[QT_INSTALL_EXAMPLES]/webengine/quicknanobrowser
INSTALLS += target
