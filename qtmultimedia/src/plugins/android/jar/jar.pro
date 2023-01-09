TARGET = QtMultimedia

load(qt_build_paths)
CONFIG += java
DESTDIR = $$MODULE_BASE_OUTDIR/jar

JAVACLASSPATH += $$PWD/src

JAVASOURCES += $$PWD/src/org/qtproject/qt5/android/multimedia/QtAndroidMediaPlayer.java \
               $$PWD/src/org/qtproject/qt5/android/multimedia/QtCameraListener.java \
               $$PWD/src/org/qtproject/qt5/android/multimedia/QtSurfaceTextureListener.java \
               $$PWD/src/org/qtproject/qt5/android/multimedia/QtSurfaceTextureHolder.java \
               $$PWD/src/org/qtproject/qt5/android/multimedia/QtMultimediaUtils.java \
               $$PWD/src/org/qtproject/qt5/android/multimedia/QtMediaRecorderListener.java \
               $$PWD/src/org/qtproject/qt5/android/multimedia/QtSurfaceHolderCallback.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target

OTHER_FILES += $$JAVASOURCES
