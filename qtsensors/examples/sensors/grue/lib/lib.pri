INCLUDEPATH += $$PWD

macos: LIBS += -L$$OUT_PWD/../grue_app.app/Contents/Frameworks
else: LIBS += -L$$OUT_PWD/..

android: LIBS += -lgruesensor_$${QT_ARCH}
else: LIBS += -lgruesensor
