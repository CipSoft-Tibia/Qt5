TARGET = QtTextToSpeech

load(qt_build_paths)
CONFIG += java
API_VERSION = android-21

DESTDIR = $$MODULE_BASE_OUTDIR/jar

PATHPREFIX = $$PWD/src/org/qtproject/qt5/android/speech

JAVACLASSPATH += $$PWD/src
JAVASOURCES += $$PATHPREFIX/QtTextToSpeech.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target

OTHER_FILES += $$JAVASOURCES
