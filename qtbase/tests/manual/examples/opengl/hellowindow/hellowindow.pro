QT += gui-private core-private opengl

HEADERS += hellowindow.h
SOURCES += hellowindow.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellowindow
INSTALLS += target

