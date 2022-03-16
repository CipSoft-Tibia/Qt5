TARGET = QtBluetooth
QT = core core-private
DEFINES += QT_NO_FOREACH

QMAKE_DOCS = $$PWD/doc/qtbluetooth.qdocconf
OTHER_FILES += doc/src/*.qdoc   # show .qdoc files in Qt Creator

PUBLIC_HEADERS += \
    qtbluetoothglobal.h \
    qbluetoothaddress.h\
    qbluetoothhostinfo.h \
    qbluetoothuuid.h\
    qbluetoothdeviceinfo.h\
    qbluetoothserviceinfo.h\
    qbluetoothdevicediscoveryagent.h\
    qbluetoothservicediscoveryagent.h\
    qbluetoothsocket.h\
    qbluetoothserver.h \
    qbluetooth.h \
    qbluetoothlocaldevice.h \
    qbluetoothtransfermanager.h \
    qbluetoothtransferrequest.h \
    qlowenergyservice.h \
    qlowenergyservicedata.h \
    qlowenergycharacteristic.h \
    qlowenergycharacteristicdata.h \
    qlowenergydescriptor.h \
    qlowenergydescriptordata.h \
    qbluetoothtransferreply.h \
    qlowenergyadvertisingdata.h \
    qlowenergyadvertisingparameters.h \
    qlowenergyconnectionparameters.h \
    qlowenergycontroller.h

PRIVATE_HEADERS += \
    qtbluetoothglobal_p.h \
    qbluetoothaddress_p.h\
    qbluetoothhostinfo_p.h \
    qbluetoothdeviceinfo_p.h\
    qbluetoothserviceinfo_p.h\
    qbluetoothdevicediscoveryagent_p.h\
    qbluetoothservicediscoveryagent_p.h\
    qbluetoothsocketbase_p.h \
    qbluetoothserver_p.h\
    qbluetoothtransferreply_p.h \
    qbluetoothtransferrequest_p.h \
    qprivatelinearbuffer_p.h \
    qbluetoothlocaldevice_p.h \
    qlowenergycontrollerbase_p.h \
    qlowenergyserviceprivate_p.h \
    qleadvertiser_p.h \
    lecmaccalculator_p.h

SOURCES += \
    qbluetoothaddress.cpp\
    qbluetoothhostinfo.cpp \
    qbluetoothuuid.cpp\
    qbluetoothdeviceinfo.cpp\
    qbluetoothserviceinfo.cpp\
    qbluetoothdevicediscoveryagent.cpp\
    qbluetoothservicediscoveryagent.cpp\
    qbluetoothsocket.cpp\
    qbluetoothsocketbase.cpp \
    qbluetoothserver.cpp \
    qbluetoothlocaldevice.cpp \
    qbluetooth.cpp \
    qbluetoothtransfermanager.cpp \
    qbluetoothtransferrequest.cpp \
    qbluetoothtransferreply.cpp \
    qlowenergyadvertisingdata.cpp \
    qlowenergyadvertisingparameters.cpp \
    qlowenergyconnectionparameters.cpp \
    qlowenergyservice.cpp \
    qlowenergyservicedata.cpp \
    qlowenergycharacteristic.cpp \
    qlowenergycharacteristicdata.cpp \
    qlowenergydescriptor.cpp \
    qlowenergydescriptordata.cpp \
    qlowenergycontroller.cpp \
    qlowenergycontrollerbase.cpp \
    qlowenergyserviceprivate.cpp

win32 {
    WINDOWS_SDK_VERSION_STRING = $$(WindowsSDKVersion)
    WINDOWS_SDK_VERSION = $$member($$list($$split(WINDOWS_SDK_VERSION_STRING, .)), 2)
}

qtConfig(bluez) {
    QT_PRIVATE = concurrent
    QT_FOR_PRIVATE += dbus network

    # do not link against QtNetwork but use inline qt_safe_* functions
    INCLUDEPATH += $$QT.network_private.includes

    include(bluez/bluez.pri)

    PRIVATE_HEADERS += \
        qbluetoothtransferreply_bluez_p.h \
        qbluetoothsocket_bluez_p.h \
        qbluetoothsocket_bluezdbus_p.h

    SOURCES += \
        qbluetoothserviceinfo_bluez.cpp \
        qbluetoothdevicediscoveryagent_bluez.cpp\
        qbluetoothservicediscoveryagent_bluez.cpp \
        qbluetoothsocket_bluez.cpp \
        qbluetoothsocket_bluezdbus.cpp \
        qbluetoothserver_bluez.cpp \
        qbluetoothlocaldevice_bluez.cpp \
        qbluetoothtransferreply_bluez.cpp


    # old versions of Bluez do not have the required BTLE symbols
    qtConfig(bluez_le) {
        SOURCES +=  \
            qleadvertiser_bluez.cpp \
            qlowenergycontroller_bluez.cpp \
            lecmaccalculator.cpp \
            qlowenergycontroller_bluezdbus.cpp

        PRIVATE_HEADERS += qlowenergycontroller_bluezdbus_p.h \
                           qlowenergycontroller_bluez_p.h

        qtConfig(linux_crypto_api): DEFINES += CONFIG_LINUX_CRYPTO_API
    } else {
        DEFINES += QT_BLUEZ_NO_BTLE
        include(dummy/dummy.pri)
        SOURCES += \
            qlowenergycontroller_p.cpp

        PRIVATE_HEADERS += qlowenergycontroller_p.h
    }

} else:android:!android-embedded {
    include(android/android.pri)
    DEFINES += QT_ANDROID_BLUETOOTH
    QT_FOR_PRIVATE += core-private androidextras

    ANDROID_PERMISSIONS = \
        android.permission.BLUETOOTH \
        android.permission.BLUETOOTH_ADMIN \
        android.permission.ACCESS_FINE_LOCATION \
        android.permission.ACCESS_COARSE_LOCATION # since Android 6.0 (API lvl 23)
    ANDROID_BUNDLED_JAR_DEPENDENCIES = \
        jar/QtAndroidBluetooth.jar:org.qtproject.qt5.android.bluetooth.QtBluetoothBroadcastReceiver

    SOURCES += \
        qbluetoothdevicediscoveryagent_android.cpp \
        qbluetoothlocaldevice_android.cpp \
        qbluetoothserviceinfo_android.cpp \
        qbluetoothservicediscoveryagent_android.cpp \
        qbluetoothsocket_android.cpp \
        qbluetoothserver_android.cpp \
        qlowenergycontroller_android.cpp

    PRIVATE_HEADERS += qlowenergycontroller_android_p.h \
                       qbluetoothsocket_android_p.h
} else:osx {
    QT_PRIVATE = concurrent
    DEFINES += QT_OSX_BLUETOOTH
    LIBS_PRIVATE += -framework Foundation -framework IOBluetooth

    include(osx/osxbt.pri)
    OBJECTIVE_SOURCES += \
        qbluetoothlocaldevice_osx.mm \
        qbluetoothdevicediscoveryagent_osx.mm \
        qbluetoothserviceinfo_osx.mm \
        qbluetoothservicediscoveryagent_osx.mm \
        qbluetoothsocket_osx.mm \
        qbluetoothserver_osx.mm \
        qbluetoothtransferreply_osx.mm \
        qlowenergycontroller_osx.mm \
        qlowenergyservice_osx.mm

    PRIVATE_HEADERS += qbluetoothsocket_osx_p.h \
                       qbluetoothserver_osx_p.h \
                       qbluetoothtransferreply_osx_p.h \
                       qbluetoothtransferreply_osx_p.h \
                       qlowenergycontroller_osx_p.h

    SOURCES -= qbluetoothdevicediscoveryagent.cpp
    SOURCES -= qbluetoothserviceinfo.cpp
    SOURCES -= qbluetoothservicediscoveryagent.cpp
    SOURCES -= qbluetoothsocket.cpp
    SOURCES -= qbluetoothsocketbase.cpp
    SOURCES -= qbluetoothserver.cpp
    SOURCES -= qlowenergyservice_p.cpp
    SOURCES -= qlowenergyservice.cpp
    SOURCES -= qlowenergycontroller.cpp
    SOURCES -= qlowenergycontrollerbase.cpp
} else:ios|tvos {
    DEFINES += QT_IOS_BLUETOOTH
    LIBS_PRIVATE += -framework Foundation -framework CoreBluetooth

    OBJECTIVE_SOURCES += \
        qbluetoothdevicediscoveryagent_ios.mm \
        qlowenergycontroller_osx.mm \
        qlowenergyservice_osx.mm

    PRIVATE_HEADERS += \
        qlowenergycontroller_osx_p.h \
        qbluetoothsocket_dummy_p.h

    include(osx/osxbt.pri)
    SOURCES += \
        qbluetoothlocaldevice_p.cpp \
        qbluetoothserviceinfo_p.cpp \
        qbluetoothservicediscoveryagent_p.cpp \
        qbluetoothsocket_dummy.cpp \
        qbluetoothserver_p.cpp

    SOURCES -= qbluetoothdevicediscoveryagent.cpp
    SOURCES -= qlowenergyservice.cpp
    SOURCES -= qlowenergycontroller.cpp
    SOURCES -= qlowenergycontrollerbase.cpp
} else: qtConfig(winrt_bt) {
    DEFINES += QT_WINRT_BLUETOOTH
    !winrt {
        SOURCES += qbluetoothutils_win.cpp
        DEFINES += CLASSIC_APP_BUILD
        LIBS += runtimeobject.lib user32.lib
    }

    QT += core-private

    SOURCES += \
        qbluetoothdevicediscoveryagent_winrt.cpp \
        qbluetoothlocaldevice_p.cpp \
        qbluetoothserver_winrt.cpp \
        qbluetoothservicediscoveryagent_winrt.cpp \
        qbluetoothserviceinfo_winrt.cpp \
        qbluetoothsocket_winrt.cpp \
        qlowenergycontroller_winrt.cpp

    PRIVATE_HEADERS += qlowenergycontroller_winrt_p.h \
                       qbluetoothsocket_winrt_p.h

    lessThan(WINDOWS_SDK_VERSION, 14393) {
        DEFINES += QT_WINRT_LIMITED_SERVICEDISCOVERY
        DEFINES += QT_UCRTVERSION=$$WINDOWS_SDK_VERSION
    }
} else {
    message("Unsupported Bluetooth platform, will not build a working QtBluetooth library.")
    message("Either no Qt D-Bus found or no BlueZ headers available.")
    include(dummy/dummy.pri)
    SOURCES += \
        qbluetoothdevicediscoveryagent_p.cpp \
        qbluetoothlocaldevice_p.cpp \
        qbluetoothserviceinfo_p.cpp \
        qbluetoothservicediscoveryagent_p.cpp \
        qbluetoothsocket_dummy.cpp \
        qbluetoothserver_p.cpp \
        qlowenergycontroller_p.cpp

    PRIVATE_HEADERS += qlowenergycontroller_p.h \
                       qbluetoothsocket_dummy_p.h
}

winrt {
    MODULE_WINRT_CAPABILITIES_DEVICE += \
        bluetooth.genericAttributeProfile \
        bluetooth.rfcomm
}

OTHER_FILES +=

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
