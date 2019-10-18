TARGET = QtPurchasing

CONFIG += java
load(sdk)

DESTDIR = $$[QT_INSTALL_PREFIX/get]/jar

PATHPREFIX = $$PWD/src/org/qtproject/qt5/android/purchasing/

JAVACLASSPATH += \
    $$PWD/jars/billing_api_4.0.0.jar \
    $$PWD/jars/annotation-1.2.0.jar \
    $$PWD/src/

JAVASOURCES += \
    $$PATHPREFIX/QtInAppPurchase.java \
    $$PATHPREFIX/Security.java \
    $$PATHPREFIX/Base64.java \
    $$PATHPREFIX/Base64DecoderException.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target

OTHER_FILES += \
    $$JAVASOURCES \
    $$PWD/jars/third_party_licenses.txt \
    $$PWD/jars/qt_attribution.json \
    $$PATHPREFIX/LICENSE-APACHE-2.0.txt \
    $$PATHPREFIX/qt_attribution.json
