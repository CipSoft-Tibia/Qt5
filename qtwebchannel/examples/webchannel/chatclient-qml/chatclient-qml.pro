TEMPLATE = aux

exampleassets.files += \
    LoginForm.ui.qml \
    MainForm.ui.qml \
    qmlchatclient.qml

exampleassets.path = $$[QT_INSTALL_EXAMPLES]/webchannel/chatclient-qml
include(../exampleassets.pri)
