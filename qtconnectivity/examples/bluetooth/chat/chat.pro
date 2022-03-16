QT = core bluetooth quick
android: QT += androidextras #see QTBUG-61392
SOURCES += qmlchat.cpp

TARGET = qml_chat
TEMPLATE = app

RESOURCES += \
    chat.qrc

OTHER_FILES += \
    chat.qml \
    InputBox.qml \
    Search.qml \
    Button.qml

#DEFINES += QMLJSDEBUGGER

target.path = $$[QT_INSTALL_EXAMPLES]/bluetooth/chat
INSTALLS += target
