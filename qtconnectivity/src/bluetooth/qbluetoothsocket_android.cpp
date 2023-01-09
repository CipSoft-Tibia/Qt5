/****************************************************************************
**
** Copyright (C) 2016 Lauri Laanmets (Proekspert AS) <lauri.laanmets@eesti.ee>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbluetoothsocket.h"
#include "qbluetoothsocket_android_p.h"
#include "qbluetoothaddress.h"
#include "qbluetoothdeviceinfo.h"
#include "qbluetoothserviceinfo.h"
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtCore/QTime>
#include <QtCore/private/qjni_p.h>
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroid>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_BT_ANDROID)

#define FALLBACK_CHANNEL 1
#define USE_FALLBACK true

Q_DECLARE_METATYPE(QAndroidJniObject)

Q_BLUETOOTH_EXPORT bool useReverseUuidWorkAroundConnect = true;

/* BluetoothSocket.connect() can block up to 10s. Therefore it must be
 * in a separate thread. Unfortunately if BluetoothSocket.close() is
 * called while connect() is still blocking the resulting behavior is not reliable.
 * This may well be an Android platform bug. In any case, close() must
 * be queued up until connect() has returned.
 *
 * The WorkerThread manages the connect() and close() calls. Interaction
 * with the main thread happens via signals and slots. There is an accepted but
 * undesirable side effect of this approach as the user may call connect()
 * and close() and the socket would continue to successfully connect to
 * the remote device just to immidiately close the physical connection again.
 *
 * WorkerThread and SocketConnectWorker are cleaned up via the threads
 * finished() signal.
 */

class SocketConnectWorker : public QObject
{
    Q_OBJECT
public:
    SocketConnectWorker(const QAndroidJniObject& socket,
                        const QAndroidJniObject& targetUuid,
                        const QBluetoothUuid& qtTargetUuid)
        : QObject(),
          mSocketObject(socket),
          mTargetUuid(targetUuid),
          mQtTargetUuid(qtTargetUuid)
    {
        static int t = qRegisterMetaType<QAndroidJniObject>();
        Q_UNUSED(t);
    }

signals:
    void socketConnectDone(const QAndroidJniObject &socket);
    void socketConnectFailed(const QAndroidJniObject &socket,
                             const QAndroidJniObject &targetUuid,
                             const QBluetoothUuid &qtUuid);
public slots:
    void connectSocket()
    {
        QAndroidJniEnvironment env;

        qCDebug(QT_BT_ANDROID) << "Connecting socket";
        mSocketObject.callMethod<void>("connect");
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();

            emit socketConnectFailed(mSocketObject, mTargetUuid, mQtTargetUuid);
            QThread::currentThread()->quit();
            return;
        }

        qCDebug(QT_BT_ANDROID) << "Socket connection established";
        emit socketConnectDone(mSocketObject);
    }

    void closeSocket()
    {
        qCDebug(QT_BT_ANDROID) << "Executing queued closeSocket()";

        QAndroidJniEnvironment env;
        mSocketObject.callMethod<void>("close");
        if (env->ExceptionCheck()) {

            qCWarning(QT_BT_ANDROID) << "Error during closure of abandoned socket";
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        QThread::currentThread()->quit();
    }

private:
    QAndroidJniObject mSocketObject;
    QAndroidJniObject mTargetUuid;
    // same as mTargetUuid above - just the Qt C++ version rather than jni uuid
    QBluetoothUuid mQtTargetUuid;
};

class WorkerThread: public QThread
{
    Q_OBJECT
public:
    WorkerThread()
        : QThread(), workerPointer(0)
    {
    }

    // Runs in same thread as QBluetoothSocketPrivateAndroid
    void setupWorker(QBluetoothSocketPrivateAndroid* d_ptr, const QAndroidJniObject& socketObject,
                     const QAndroidJniObject& uuidObject, bool useFallback,
                     const QBluetoothUuid& qtUuid = QBluetoothUuid())
    {
        SocketConnectWorker* worker = new SocketConnectWorker(
                                            socketObject, uuidObject, qtUuid);
        worker->moveToThread(this);

        connect(this, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &QThread::finished, this, &QObject::deleteLater);
        connect(d_ptr, &QBluetoothSocketPrivateAndroid::connectJavaSocket,
                worker, &SocketConnectWorker::connectSocket);
        connect(d_ptr, &QBluetoothSocketPrivateAndroid::closeJavaSocket,
                worker, &SocketConnectWorker::closeSocket);
        connect(worker, &SocketConnectWorker::socketConnectDone,
                d_ptr, &QBluetoothSocketPrivateAndroid::socketConnectSuccess);
        if (useFallback) {
            connect(worker, &SocketConnectWorker::socketConnectFailed,
                    d_ptr, &QBluetoothSocketPrivateAndroid::fallbackSocketConnectFailed);
        } else {
            connect(worker, &SocketConnectWorker::socketConnectFailed,
                d_ptr, &QBluetoothSocketPrivateAndroid::defaultSocketConnectFailed);
        }

        workerPointer = worker;
    }

private:
    QPointer<SocketConnectWorker> workerPointer;
};

QBluetoothSocketPrivateAndroid::QBluetoothSocketPrivateAndroid()
  :
    inputThread(0)
{
    secFlags = QBluetooth::Secure;
    adapter = QAndroidJniObject::callStaticObjectMethod("android/bluetooth/BluetoothAdapter",
                                                        "getDefaultAdapter",
                                                        "()Landroid/bluetooth/BluetoothAdapter;");
    qRegisterMetaType<QBluetoothSocket::SocketError>();
    qRegisterMetaType<QBluetoothSocket::SocketState>();
}

QBluetoothSocketPrivateAndroid::~QBluetoothSocketPrivateAndroid()
{
    if (state != QBluetoothSocket::UnconnectedState)
        emit closeJavaSocket();
}

bool QBluetoothSocketPrivateAndroid::ensureNativeSocket(QBluetoothServiceInfo::Protocol type)
{
    socketType = type;
    if (socketType == QBluetoothServiceInfo::RfcommProtocol)
        return true;

    return false;
}

bool QBluetoothSocketPrivateAndroid::fallBackConnect(QAndroidJniObject uuid, int channel)
{
    qCWarning(QT_BT_ANDROID) << "Falling back to getServiceChannel() workaround.";

    QAndroidJniEnvironment env;

    QAndroidJniObject remoteDeviceClass = remoteDevice.callObjectMethod("getClass", "()Ljava/lang/Class;");
    if (!remoteDeviceClass.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Could not invoke BluetoothDevice.getClass.";
        return false;
    }

    QAndroidJniObject integerObject = QAndroidJniObject::getStaticObjectField(
                                            "java/lang/Integer", "TYPE", "Ljava/lang/Class;");
    if (!integerObject.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Could not get Integer.TYPE";
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        return false;
    }

    jclass classClass = QJNIEnvironmentPrivate::findClass("java/lang/Class");
    jobjectArray rawArray = env->NewObjectArray(1, classClass,
                                                integerObject.object<jobject>());
    QAndroidJniObject paramTypes(rawArray);
    env->DeleteLocalRef(rawArray);
    if (!paramTypes.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Could not create new Class[]{Integer.TYPE}";

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return false;
    }

    QAndroidJniObject parcelUuid("android/os/ParcelUuid", "(Ljava/util/UUID;)V",
                                 uuid.object());
    if (parcelUuid.isValid()) {
        jint socketChannel = remoteDevice.callMethod<jint>("getServiceChannel",
                                                           "(Landroid/os/ParcelUuid;)I",
                                                           parcelUuid.object());
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        } else {
            if (socketChannel
                    == remoteDevice.getStaticField<jint>("android/bluetooth/BluetoothDevice", "ERROR")
                || socketChannel == -1) {
                qCWarning(QT_BT_ANDROID) << "Cannot determine RFCOMM service channel.";
            } else {
                qCWarning(QT_BT_ANDROID) << "Using found rfcomm channel" << socketChannel;
                channel = socketChannel;
            }
        }
    }

    QAndroidJniObject method;
    if (secFlags == QBluetooth::NoSecurity) {
        qCDebug(QT_BT_ANDROID) << "Connnecting via insecure rfcomm";
        method = remoteDeviceClass.callObjectMethod(
                "getMethod",
                "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;",
                QAndroidJniObject::fromString(QLatin1String("createInsecureRfcommSocket")).object<jstring>(),
                paramTypes.object<jobjectArray>());
    } else {
        qCDebug(QT_BT_ANDROID) << "Connnecting via secure rfcomm";
        method = remoteDeviceClass.callObjectMethod(
                "getMethod",
                "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;",
                QAndroidJniObject::fromString(QLatin1String("createRfcommSocket")).object<jstring>(),
                paramTypes.object<jobjectArray>());
    }
    if (!method.isValid() || env->ExceptionCheck()) {
        qCWarning(QT_BT_ANDROID) << "Could not invoke getMethod";
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return false;
    }

    jclass objectClass = QJNIEnvironmentPrivate::findClass("java/lang/Object");
    QAndroidJniObject channelObject = QAndroidJniObject::callStaticObjectMethod(
                                        "java/lang/Integer", "valueOf", "(I)Ljava/lang/Integer;", channel);
    rawArray = env->NewObjectArray(1, objectClass, channelObject.object<jobject>());

    QAndroidJniObject invokeResult = method.callObjectMethod("invoke",
                                         "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;",
                                         remoteDevice.object<jobject>(), rawArray);
    env->DeleteLocalRef(rawArray);
    if (!invokeResult.isValid())
    {
        qCWarning(QT_BT_ANDROID) << "Invoke Resulted with error.";
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        return false;
    }

    socketObject = QAndroidJniObject(invokeResult);

    WorkerThread *workerThread = new WorkerThread();
    workerThread->setupWorker(this, socketObject, uuid, USE_FALLBACK);
    workerThread->start();
    emit connectJavaSocket();

    qCWarning(QT_BT_ANDROID) << "Workaround thread invoked.";
    return true;
}

/*
 * Workaround for QTBUG-61392
 */
bool QBluetoothSocketPrivateAndroid::fallBackReversedConnect(const QBluetoothUuid &uuid)
{
    Q_Q(QBluetoothSocket);

    qCWarning(QT_BT_ANDROID) << "Falling back to reverse uuid workaround.";
    const QBluetoothUuid reverse = reverseUuid(uuid);
    if (reverse.isNull())
        return false;

    //cut leading { and trailing } {xxx-xxx}
    QString tempUuid = reverse.toString();
    tempUuid.chop(1); //remove trailing '}'
    tempUuid.remove(0, 1); //remove first '{'

    QAndroidJniEnvironment env;
    const QAndroidJniObject inputString = QAndroidJniObject::fromString(tempUuid);
    const QAndroidJniObject uuidObject = QAndroidJniObject::callStaticObjectMethod("java/util/UUID", "fromString",
                                                                       "(Ljava/lang/String;)Ljava/util/UUID;",
                                                                       inputString.object<jstring>());

    if (secFlags == QBluetooth::NoSecurity) {
        qCDebug(QT_BT_ANDROID) << "Connnecting via insecure rfcomm";
        socketObject = remoteDevice.callObjectMethod("createInsecureRfcommSocketToServiceRecord",
                                                 "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;",
                                                 uuidObject.object<jobject>());
    } else {
        qCDebug(QT_BT_ANDROID) << "Connnecting via secure rfcomm";
        socketObject = remoteDevice.callObjectMethod("createRfcommSocketToServiceRecord",
                                                 "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;",
                                                 uuidObject.object<jobject>());
    }

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        socketObject = remoteDevice = QAndroidJniObject();
        errorString = QBluetoothSocket::tr("Cannot connect to %1",
                                           "%1 = uuid").arg(reverse.toString());
        q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return false;
    }

    WorkerThread *workerThread = new WorkerThread();
    workerThread->setupWorker(this, socketObject, uuidObject, USE_FALLBACK);
    workerThread->start();
    emit connectJavaSocket();

    return true;
}

/*
 * The call order during a connectToServiceHelper() is as follows:
 *
 * 1. call connectToServiceHelper()
 * 2. wait for execution of SocketConnectThread::run()
 * 3. if threaded connect succeeds call socketConnectSuccess() via signals
 *      -> done
 * 4. if threaded connect fails call defaultSocketConnectFailed() via signals
 * 5. call fallBackConnect() if Android version 22 or below
 *     -> Android 23+ complete failure of entire connectToServiceHelper()
 * 6. call fallBackReversedConnect() if Android version 23 or above
 *     -> if failure entire connectToServiceHelper() fails
 * 7. if threaded connect on one of above fallbacks succeeds call socketConnectSuccess()
 *    via signals
 *      -> done
 * 8. if threaded connect on fallback channel fails call fallbackSocketConnectFailed()
 *      -> complete failure of entire connectToServiceHelper()
 * */
void QBluetoothSocketPrivateAndroid::connectToServiceHelper(const QBluetoothAddress &address,
                                               const QBluetoothUuid &uuid,
                                               QIODevice::OpenMode openMode)
{
    Q_Q(QBluetoothSocket);
    Q_UNUSED(openMode);

    qCDebug(QT_BT_ANDROID) << "connectToServiceHelper()" << address.toString() << uuid.toString();

    q->setSocketState(QBluetoothSocket::ConnectingState);

    if (!adapter.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Device does not support Bluetooth";
        errorString = QBluetoothSocket::tr("Device does not support Bluetooth");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    const int state = adapter.callMethod<jint>("getState");
    if (state != 12 ) { //BluetoothAdapter.STATE_ON
        qCWarning(QT_BT_ANDROID) << "Bt device offline";
        errorString = QBluetoothSocket::tr("Device is powered off");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    QAndroidJniEnvironment env;
    QAndroidJniObject inputString = QAndroidJniObject::fromString(address.toString());
    remoteDevice = adapter.callObjectMethod("getRemoteDevice",
                                            "(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;",
                                            inputString.object<jstring>());
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        errorString = QBluetoothSocket::tr("Cannot access address %1", "%1 = Bt address e.g. 11:22:33:44:55:66").arg(address.toString());
        q->setSocketError(QBluetoothSocket::HostNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    //cut leading { and trailing } {xxx-xxx}
    QString tempUuid = uuid.toString();
    tempUuid.chop(1); //remove trailing '}'
    tempUuid.remove(0, 1); //remove first '{'

    inputString = QAndroidJniObject::fromString(tempUuid);
    QAndroidJniObject uuidObject = QAndroidJniObject::callStaticObjectMethod("java/util/UUID", "fromString",
                                                                       "(Ljava/lang/String;)Ljava/util/UUID;",
                                                                       inputString.object<jstring>());

    if (secFlags == QBluetooth::NoSecurity) {
        qCDebug(QT_BT_ANDROID) << "Connnecting via insecure rfcomm";
        socketObject = remoteDevice.callObjectMethod("createInsecureRfcommSocketToServiceRecord",
                                                 "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;",
                                                 uuidObject.object<jobject>());
    } else {
        qCDebug(QT_BT_ANDROID) << "Connnecting via secure rfcomm";
        socketObject = remoteDevice.callObjectMethod("createRfcommSocketToServiceRecord",
                                                 "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothSocket;",
                                                 uuidObject.object<jobject>());
    }

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        socketObject = remoteDevice = QAndroidJniObject();
        errorString = QBluetoothSocket::tr("Cannot connect to %1 on %2",
                                           "%1 = uuid, %2 = Bt address").arg(uuid.toString()).arg(address.toString());
        q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    WorkerThread *workerThread = new WorkerThread();
    workerThread->setupWorker(this, socketObject, uuidObject, !USE_FALLBACK, uuid);
    workerThread->start();
    emit connectJavaSocket();
}

void QBluetoothSocketPrivateAndroid::connectToService(
        const QBluetoothServiceInfo &service, QIODevice::OpenMode openMode)
{
    Q_Q(QBluetoothSocket);

    if (q->state() != QBluetoothSocket::UnconnectedState
            && q->state() != QBluetoothSocket::ServiceLookupState) {
        qCWarning(QT_BT_ANDROID) << "QBluetoothSocketPrivateAndroid::connectToService called on busy socket";
        errorString = QBluetoothSocket::tr("Trying to connect while connection is in progress");
        q->setSocketError(QBluetoothSocket::OperationError);
        return;
    }

    // Workaround for QTBUG-75035
    /* Not all Android devices publish or discover the SPP uuid for serial services.
     * Also, Android does not permit the detection of the protocol used by a serial
     * Bluetooth connection.
     *
     * Therefore, QBluetoothServiceDiscoveryAgentPrivate::populateDiscoveredServices()
     * may have to guess what protocol a potential custom uuid uses. The guessing works
     * reasonably well as long as the SDP discovery finds the SPP uuid. Otherwise
     * the SPP and rfcomm protocol info is missing in \a service.
     *
     * Android only supports RFCOMM (no L2CP). We assume (in favor of user experience)
     * that a non-RFCOMM protocol implies a missing SPP uuid during discovery but the user
     * still wanting to connect with the given \a service instance.
     */

    auto protocol = service.socketProtocol();
    switch (protocol) {
    case QBluetoothServiceInfo::L2capProtocol:
    case QBluetoothServiceInfo::UnknownProtocol:
        qCWarning(QT_BT_ANDROID) << "Changing socket protocol to RFCOMM";
        protocol = QBluetoothServiceInfo::RfcommProtocol;
        break;
    case QBluetoothServiceInfo::RfcommProtocol:
        break;
    }

    if (!ensureNativeSocket(protocol)) {
        errorString = QBluetoothSocket::tr("Socket type not supported");
        q->setSocketError(QBluetoothSocket::UnsupportedProtocolError);
        return;
    }
    connectToServiceHelper(service.device().address(), service.serviceUuid(), openMode);
}

void QBluetoothSocketPrivateAndroid::connectToService(
        const QBluetoothAddress &address, const QBluetoothUuid &uuid,
        QIODevice::OpenMode openMode)
{
    Q_Q(QBluetoothSocket);

    if (q->state() != QBluetoothSocket::UnconnectedState) {
        qCWarning(QT_BT_ANDROID) << "QBluetoothSocketPrivateAndroid::connectToService called on busy socket";
        errorString = QBluetoothSocket::tr("Trying to connect while connection is in progress");
        q->setSocketError(QBluetoothSocket::OperationError);
        return;
    }

    if (q->socketType() == QBluetoothServiceInfo::UnknownProtocol) {
        qCWarning(QT_BT_ANDROID) << "QBluetoothSocketPrivateAndroid::connectToService cannot "
                                  "connect with 'UnknownProtocol' (type provided by given service)";
        errorString = QBluetoothSocket::tr("Socket type not supported");
        q->setSocketError(QBluetoothSocket::UnsupportedProtocolError);
        return;
    }

    if (!ensureNativeSocket(q->socketType())) {
        errorString = QBluetoothSocket::tr("Socket type not supported");
        q->setSocketError(QBluetoothSocket::UnsupportedProtocolError);
        return;
    }
    connectToServiceHelper(address, uuid, openMode);
}

void QBluetoothSocketPrivateAndroid::connectToService(
        const QBluetoothAddress &address, quint16 port, QIODevice::OpenMode openMode)
{
    Q_UNUSED(port);
    Q_UNUSED(openMode);
    Q_UNUSED(address);

    Q_Q(QBluetoothSocket);

    errorString = tr("Connecting to port is not supported");
    q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
    qCWarning(QT_BT_ANDROID) << "Connecting to port is not supported";
}

void QBluetoothSocketPrivateAndroid::socketConnectSuccess(const QAndroidJniObject &socket)
{
    Q_Q(QBluetoothSocket);
    QAndroidJniEnvironment env;

    // test we didn't get a success from a previous connect
    // which was cleaned up late
    if (socket != socketObject)
        return;

    if (inputThread) {
        inputThread->deleteLater();
        inputThread = 0;
    }

    inputStream = socketObject.callObjectMethod("getInputStream", "()Ljava/io/InputStream;");
    outputStream = socketObject.callObjectMethod("getOutputStream", "()Ljava/io/OutputStream;");

    if (env->ExceptionCheck() || !inputStream.isValid() || !outputStream.isValid()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        emit closeJavaSocket();
        socketObject = inputStream = outputStream = remoteDevice = QAndroidJniObject();


        errorString = QBluetoothSocket::tr("Obtaining streams for service failed");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    inputThread = new InputStreamThread(this);
    QObject::connect(inputThread, SIGNAL(dataAvailable()),
                     q, SIGNAL(readyRead()), Qt::QueuedConnection);
    QObject::connect(inputThread, SIGNAL(error(int)),
                     this, SLOT(inputThreadError(int)), Qt::QueuedConnection);

    if (!inputThread->run()) {
        //close socket again
        emit closeJavaSocket();

        socketObject = inputStream = outputStream = remoteDevice = QAndroidJniObject();

        delete inputThread;
        inputThread = 0;

        errorString = QBluetoothSocket::tr("Input stream thread cannot be started");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return;
    }

    // only unbuffered behavior supported at this stage
    q->setOpenMode(QIODevice::ReadWrite|QIODevice::Unbuffered);

    q->setSocketState(QBluetoothSocket::ConnectedState);
}

void QBluetoothSocketPrivateAndroid::defaultSocketConnectFailed(
        const QAndroidJniObject &socket, const QAndroidJniObject &targetUuid,
        const QBluetoothUuid &qtTargetUuid)
{
    Q_Q(QBluetoothSocket);

    // test we didn't get a fail from a previous connect
    // which was cleaned up late - should be same socket
    if (socket != socketObject)
        return;

    bool success = false;
    if (QtAndroid::androidSdkVersion() <= 22)
        success = fallBackConnect(targetUuid, FALLBACK_CHANNEL);
    else if (useReverseUuidWorkAroundConnect) // version 23+ has Android bug (see QTBUG-61392)
        success = fallBackReversedConnect(qtTargetUuid);

    if (!success) {
        errorString = QBluetoothSocket::tr("Connection to service failed");
        socketObject = remoteDevice = QAndroidJniObject();
        q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);

        QAndroidJniEnvironment env;
        env->ExceptionClear(); // just in case
        qCWarning(QT_BT_ANDROID) << "Workaround failed";
    }
}

void QBluetoothSocketPrivateAndroid::fallbackSocketConnectFailed(
        const QAndroidJniObject &socket, const QAndroidJniObject &targetUuid)
{
    Q_UNUSED(targetUuid);
    Q_Q(QBluetoothSocket);

    // test we didn't get a fail from a previous connect
    // which was cleaned up late - should be same socket
    if (socket != socketObject)
        return;

    qCWarning(QT_BT_ANDROID) << "Socket connect via workaround failed.";
    errorString = QBluetoothSocket::tr("Connection to service failed");
    socketObject = remoteDevice = QAndroidJniObject();

    q->setSocketError(QBluetoothSocket::ServiceNotFoundError);
    q->setSocketState(QBluetoothSocket::UnconnectedState);
}

void QBluetoothSocketPrivateAndroid::abort()
{
    if (state == QBluetoothSocket::UnconnectedState)
        return;

    if (socketObject.isValid()) {
        QAndroidJniEnvironment env;

        /*
         * BluetoothSocket.close() triggers an abort of the input stream
         * thread because inputStream.read() throws IOException
         * In turn the thread stops and throws an error which sets
         * new state, error and emits relevant signals.
         * See QBluetoothSocketPrivateAndroid::inputThreadError() for details
         */

        if (inputThread)
            inputThread->prepareForClosure();

        emit closeJavaSocket();

        inputStream = outputStream = socketObject = remoteDevice = QAndroidJniObject();

        if (inputThread) {
            // inputThread exists hence we had a successful connect
            // which means inputThread is responsible for setting Unconnected

            //don't delete here as signals caused by Java Thread are still
            //going to be emitted
            //delete occurs in inputThreadError()
            inputThread = 0;
        } else {
            // inputThread doesn't exist hence
            // we abort in the middle of connect(). WorkerThread will do
            // close() without further feedback. Therefore we have to set
            // Unconnected (now) in advance
            Q_Q(QBluetoothSocket);
            q->setOpenMode(QIODevice::NotOpen);
            q->setSocketState(QBluetoothSocket::UnconnectedState);
            emit q->readChannelFinished();
        }
    }
}

QString QBluetoothSocketPrivateAndroid::localName() const
{
    if (adapter.isValid())
        return adapter.callObjectMethod<jstring>("getName").toString();

    return QString();
}

QBluetoothAddress QBluetoothSocketPrivateAndroid::localAddress() const
{
    QString result;
    if (adapter.isValid())
        result = adapter.callObjectMethod("getAddress", "()Ljava/lang/String;").toString();

    return QBluetoothAddress(result);
}

quint16 QBluetoothSocketPrivateAndroid::localPort() const
{
    // Impossible to get channel number with current Android API (Levels 5 to 19)
    return 0;
}

QString QBluetoothSocketPrivateAndroid::peerName() const
{
    if (!remoteDevice.isValid())
        return QString();

    return remoteDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();
}

QBluetoothAddress QBluetoothSocketPrivateAndroid::peerAddress() const
{
    if (!remoteDevice.isValid())
        return QBluetoothAddress();

    const QString address = remoteDevice.callObjectMethod("getAddress",
                                                          "()Ljava/lang/String;").toString();

    return QBluetoothAddress(address);
}

quint16 QBluetoothSocketPrivateAndroid::peerPort() const
{
    // Impossible to get channel number with current Android API (Levels 5 to 13)
    return 0;
}

qint64 QBluetoothSocketPrivateAndroid::writeData(const char *data, qint64 maxSize)
{
    //TODO implement buffered behavior (so far only unbuffered)
    Q_Q(QBluetoothSocket);
    if (state != QBluetoothSocket::ConnectedState || !outputStream.isValid()) {
        qCWarning(QT_BT_ANDROID) << "Socket::writeData: " << state << outputStream.isValid();
        errorString = QBluetoothSocket::tr("Cannot write while not connected");
        q->setSocketError(QBluetoothSocket::OperationError);
        return -1;
    }

    QAndroidJniEnvironment env;
    jbyteArray nativeData = env->NewByteArray((qint32)maxSize);
    env->SetByteArrayRegion(nativeData, 0, (qint32)maxSize, reinterpret_cast<const jbyte*>(data));
    outputStream.callMethod<void>("write", "([BII)V", nativeData, 0, (qint32)maxSize);
    env->DeleteLocalRef(nativeData);

    if (env->ExceptionCheck()) {
        qCWarning(QT_BT_ANDROID) << "Error while writing";
        env->ExceptionDescribe();
        env->ExceptionClear();
        errorString = QBluetoothSocket::tr("Error during write on socket.");
        q->setSocketError(QBluetoothSocket::NetworkError);
        return -1;
    }

    emit q->bytesWritten(maxSize);
    return maxSize;
}

qint64 QBluetoothSocketPrivateAndroid::readData(char *data, qint64 maxSize)
{
    Q_Q(QBluetoothSocket);
    if (state != QBluetoothSocket::ConnectedState || !inputThread) {
        qCWarning(QT_BT_ANDROID) << "Socket::readData: " << state << inputThread ;
        errorString = QBluetoothSocket::tr("Cannot read while not connected");
        q->setSocketError(QBluetoothSocket::OperationError);
        return -1;
    }

    return inputThread->readData(data, maxSize);
}

void QBluetoothSocketPrivateAndroid::inputThreadError(int errorCode)
{
    Q_Q(QBluetoothSocket);

    if (errorCode != -1) { //magic error which is expected and can be ignored
        errorString = QBluetoothSocket::tr("Network error during read");
        q->setSocketError(QBluetoothSocket::NetworkError);
    }

    //finally we can delete the InputStreamThread
    InputStreamThread *client = qobject_cast<InputStreamThread *>(sender());
    if (client)
        client->deleteLater();

    if (socketObject.isValid()) {
        //triggered when remote side closed the socket
        //cleanup internal objects
        //if it was call to local close()/abort() the objects are cleaned up already

        emit closeJavaSocket();

        inputStream = outputStream = remoteDevice = socketObject = QAndroidJniObject();
        if (inputThread) {
            // deleted already above (client->deleteLater())
            inputThread = 0;
        }
    }

    q->setOpenMode(QIODevice::NotOpen);
    q->setSocketState(QBluetoothSocket::UnconnectedState);
    emit q->readChannelFinished();
}

void QBluetoothSocketPrivateAndroid::close()
{
    /* This function is called by QBluetoothSocket::close and softer version
       QBluetoothSocket::disconnectFromService() which difference I do not quite fully understand.
       Anyways we end up in Android "close" function call.
     */
    abort();
}

bool QBluetoothSocketPrivateAndroid::setSocketDescriptor(int socketDescriptor, QBluetoothServiceInfo::Protocol socketType,
                         QBluetoothSocket::SocketState socketState, QBluetoothSocket::OpenMode openMode)
{
    Q_UNUSED(socketDescriptor);
    Q_UNUSED(socketType)
    Q_UNUSED(socketState);
    Q_UNUSED(openMode);
    qCWarning(QT_BT_ANDROID) << "No socket descriptor support on Android.";
    return false;
}

bool QBluetoothSocketPrivateAndroid::setSocketDescriptor(const QAndroidJniObject &socket, QBluetoothServiceInfo::Protocol socketType_,
                         QBluetoothSocket::SocketState socketState, QBluetoothSocket::OpenMode openMode)
{
    Q_Q(QBluetoothSocket);

    if (q->state() != QBluetoothSocket::UnconnectedState || !socket.isValid())
        return false;

    if (!ensureNativeSocket(socketType_))
        return false;

    socketObject = socket;

    QAndroidJniEnvironment env;
    inputStream = socketObject.callObjectMethod("getInputStream", "()Ljava/io/InputStream;");
    outputStream = socketObject.callObjectMethod("getOutputStream", "()Ljava/io/OutputStream;");

    if (env->ExceptionCheck() || !inputStream.isValid() || !outputStream.isValid()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        //close socket again
        socketObject.callMethod<void>("close");
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        socketObject = inputStream = outputStream = remoteDevice = QAndroidJniObject();


        errorString = QBluetoothSocket::tr("Obtaining streams for service failed");
        q->setSocketError(QBluetoothSocket::NetworkError);
        q->setSocketState(QBluetoothSocket::UnconnectedState);
        return false;
    }

    remoteDevice = socketObject.callObjectMethod("getRemoteDevice", "()Landroid/bluetooth/BluetoothDevice;");

    if (inputThread) {
        inputThread->deleteLater();
        inputThread = 0;
    }
    inputThread = new InputStreamThread(this);
    QObject::connect(inputThread, SIGNAL(dataAvailable()),
                     q, SIGNAL(readyRead()), Qt::QueuedConnection);
    QObject::connect(inputThread, SIGNAL(error(int)),
                     this, SLOT(inputThreadError(int)), Qt::QueuedConnection);
    inputThread->run();

    // WorkerThread manages all sockets for us
    // When we come through here the socket was already connected by
    // server socket listener (see QBluetoothServer)
    // Therefore we only use WorkerThread to potentially close it later on
    WorkerThread *workerThread = new WorkerThread();
    workerThread->setupWorker(this, socketObject, QAndroidJniObject(), !USE_FALLBACK);
    workerThread->start();

    q->setOpenMode(openMode | QIODevice::Unbuffered);
    q->setSocketState(socketState);

    return true;
}

qint64 QBluetoothSocketPrivateAndroid::bytesAvailable() const
{
    //We cannot access buffer directly as it is part of different thread
    if (inputThread)
        return inputThread->bytesAvailable();

    return 0;
}

qint64 QBluetoothSocketPrivateAndroid::bytesToWrite() const
{
    return 0; // nothing because always unbuffered
}

/*
 * This function is part of a workaround for QTBUG-61392
 *
 * Returns null uuid if the given \a serviceUuid is not a uuid
 * derived from the Bluetooth base uuid.
 */
QBluetoothUuid QBluetoothSocketPrivateAndroid::reverseUuid(const QBluetoothUuid &serviceUuid)
{
    if (QtAndroid::androidSdkVersion() < 23)
        return serviceUuid;

    if (serviceUuid.isNull())
        return QBluetoothUuid();

    bool isBaseUuid = false;
    serviceUuid.toUInt32(&isBaseUuid);
    if (isBaseUuid)
        return serviceUuid;

    const quint128 original = serviceUuid.toUInt128();
    quint128 reversed;
    for (int i = 0; i < 16; i++)
        reversed.data[15-i] = original.data[i];
    return QBluetoothUuid{reversed};
}

bool QBluetoothSocketPrivateAndroid::canReadLine() const
{
    // We cannot access buffer directly as it is part of different thread
    if (inputThread)
        return inputThread->canReadLine();

    return false;
}

QT_END_NAMESPACE

#include <qbluetoothsocket_android.moc>
