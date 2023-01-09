HEADERS += \
    $$PWD/qquickdraghandler_p.h \
    $$PWD/qquickhandlerpoint_p.h \
    $$PWD/qquickhoverhandler_p.h \
    $$PWD/qquickmultipointhandler_p.h \
    $$PWD/qquickmultipointhandler_p_p.h \
    $$PWD/qquickpinchhandler_p.h \
    $$PWD/qquickpointerdevicehandler_p.h \
    $$PWD/qquickpointerdevicehandler_p_p.h \
    $$PWD/qquickpointerhandler_p.h \
    $$PWD/qquickpointerhandler_p_p.h \
    $$PWD/qquickpointhandler_p.h \
    $$PWD/qquicksinglepointhandler_p.h \
    $$PWD/qquicksinglepointhandler_p_p.h \
    $$PWD/qquicktaphandler_p.h \
    $$PWD/qquickdragaxis_p.h

SOURCES += \
    $$PWD/qquickdraghandler.cpp \
    $$PWD/qquickhandlerpoint.cpp \
    $$PWD/qquickhoverhandler.cpp \
    $$PWD/qquickmultipointhandler.cpp \
    $$PWD/qquickpinchhandler.cpp \
    $$PWD/qquickpointerdevicehandler.cpp \
    $$PWD/qquickpointerhandler.cpp \
    $$PWD/qquickpointhandler.cpp \
    $$PWD/qquicksinglepointhandler.cpp \
    $$PWD/qquicktaphandler.cpp \
    $$PWD/qquickdragaxis.cpp

qtConfig(wheelevent) {
    HEADERS += $$PWD/qquickwheelhandler_p.h $$PWD/qquickwheelhandler_p_p.h
    SOURCES += $$PWD/qquickwheelhandler.cpp
}

