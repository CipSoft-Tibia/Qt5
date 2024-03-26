// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <dlfcn.h>
#include <pthread.h>
#include <qplugin.h>
#include <semaphore.h>

#include "androidcontentfileengine.h"
#include "androiddeadlockprotector.h"
#include "androidjniaccessibility.h"
#include "androidjniclipboard.h"
#include "androidjniinput.h"
#include "androidjnimain.h"
#include "androidjnimenu.h"
#include "qandroidassetsfileenginehandler.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformdialoghelpers.h"
#include "qandroidplatformintegration.h"

#include <android/api-level.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qbasicatomic.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qresource.h>
#include <QtCore/qthread.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

static JavaVM *m_javaVM = nullptr;
static jclass m_applicationClass  = nullptr;
static jobject m_classLoaderObject = nullptr;
static jmethodID m_loadClassMethodID = nullptr;
static AAssetManager *m_assetManager = nullptr;
static jobject m_assets = nullptr;
static jobject m_resourcesObj = nullptr;
static QtJniTypes::Activity m_activityObject = nullptr;
static jmethodID m_createSurfaceMethodID = nullptr;
static QtJniTypes::Service m_serviceObject = nullptr;
static jmethodID m_setSurfaceGeometryMethodID = nullptr;
static jmethodID m_destroySurfaceMethodID = nullptr;

static int m_pendingApplicationState = -1;
static QBasicMutex m_platformMutex;

static jclass m_bitmapClass  = nullptr;
static jmethodID m_createBitmapMethodID = nullptr;
static jobject m_ARGB_8888_BitmapConfigValue = nullptr;
static jobject m_RGB_565_BitmapConfigValue = nullptr;

static jclass m_bitmapDrawableClass = nullptr;
static jmethodID m_bitmapDrawableConstructorMethodID = nullptr;

extern "C" typedef int (*Main)(int, char **); //use the standard main method to start the application
static Main m_main = nullptr;
static void *m_mainLibraryHnd = nullptr;
static QList<QByteArray> m_applicationParams;
static sem_t m_exitSemaphore, m_terminateSemaphore;

QHash<int, AndroidSurfaceClient *> m_surfaces;

Q_CONSTINIT static QBasicMutex m_surfacesMutex;


static QAndroidPlatformIntegration *m_androidPlatformIntegration = nullptr;

static int m_availableWidthPixels  = 0;
static int m_availableHeightPixels = 0;
static double m_scaledDensity = 0;
static double m_density = 1.0;

static AndroidAssetsFileEngineHandler *m_androidAssetsFileEngineHandler = nullptr;
static AndroidContentFileEngineHandler *m_androidContentFileEngineHandler = nullptr;



static const char m_qtTag[] = "Qt";
static const char m_classErrorMsg[] = "Can't find class \"%s\"";
static const char m_methodErrorMsg[] = "Can't find method \"%s%s\"";

Q_CONSTINIT static QBasicAtomicInt startQtAndroidPluginCalled = Q_BASIC_ATOMIC_INITIALIZER(0);

namespace QtAndroid
{
    QBasicMutex *platformInterfaceMutex()
    {
        return &m_platformMutex;
    }

    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration)
    {
        m_androidPlatformIntegration = androidPlatformIntegration;

        // flush the pending state if necessary.
        if (m_androidPlatformIntegration && (m_pendingApplicationState != -1)) {
            if (m_pendingApplicationState == Qt::ApplicationActive)
                QtAndroidPrivate::handleResume();
            else if (m_pendingApplicationState == Qt::ApplicationInactive)
                QtAndroidPrivate::handlePause();
            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(m_pendingApplicationState));
        }

        m_pendingApplicationState = -1;
    }

    QAndroidPlatformIntegration *androidPlatformIntegration()
    {
        return m_androidPlatformIntegration;
    }

    QWindow *topLevelWindowAt(const QPoint &globalPos)
    {
        return m_androidPlatformIntegration
               ? m_androidPlatformIntegration->screen()->topLevelAt(globalPos)
               : 0;
    }

    int availableWidthPixels()
    {
        return m_availableWidthPixels;
    }

    int availableHeightPixels()
    {
        return m_availableHeightPixels;
    }

    double scaledDensity()
    {
        return m_scaledDensity;
    }

    double pixelDensity()
    {
        return m_density;
    }

    JavaVM *javaVM()
    {
        return m_javaVM;
    }

    AAssetManager *assetManager()
    {
        return m_assetManager;
    }

    jclass applicationClass()
    {
        return m_applicationClass;
    }

    QtJniTypes::Activity activity()
    {
        return m_activityObject;
    }

    QtJniTypes::Service service()
    {
        return m_serviceObject;
    }

    void setSystemUiVisibility(SystemUiVisibility uiVisibility)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "setSystemUiVisibility", "(I)V", jint(uiVisibility));
    }

    void notifyAccessibilityLocationChange(uint accessibilityObjectId)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "notifyAccessibilityLocationChange",
                                           "(I)V", accessibilityObjectId);
    }

    void notifyObjectHide(uint accessibilityObjectId, uint parentObjectId)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "notifyObjectHide", "(II)V",
                                           accessibilityObjectId, parentObjectId);
    }

    void notifyObjectFocus(uint accessibilityObjectId)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "notifyObjectFocus","(I)V", accessibilityObjectId);
    }

    void notifyValueChanged(uint accessibilityObjectId, jstring value)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "notifyValueChanged",
                                           "(ILjava/lang/String;)V", accessibilityObjectId, value);
    }

    void notifyScrolledEvent(uint accessibilityObjectId)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "notifyScrolledEvent", "(I)V",
                                           accessibilityObjectId);
    }

    void notifyQtAndroidPluginRunning(bool running)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass, "notifyQtAndroidPluginRunning","(Z)V", running);
    }

    jobject createBitmap(QImage img, JNIEnv *env)
    {
        if (!m_bitmapClass)
            return 0;

        if (img.format() != QImage::Format_RGBA8888 && img.format() != QImage::Format_RGB16)
            img = img.convertToFormat(QImage::Format_RGBA8888);

        jobject bitmap = env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     img.width(),
                                                     img.height(),
                                                     img.format() == QImage::Format_RGBA8888
                                                        ? m_ARGB_8888_BitmapConfigValue
                                                        : m_RGB_565_BitmapConfigValue);
        if (!bitmap)
            return 0;

        AndroidBitmapInfo info;
        if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
            env->DeleteLocalRef(bitmap);
            return 0;
        }

        void *pixels;
        if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
            env->DeleteLocalRef(bitmap);
            return 0;
        }

        if (info.stride == uint(img.bytesPerLine())
                && info.width == uint(img.width())
                && info.height == uint(img.height())) {
            memcpy(pixels, img.constBits(), info.stride * info.height);
        } else {
            uchar *bmpPtr = static_cast<uchar *>(pixels);
            const unsigned width = qMin(info.width, (uint)img.width()); //should be the same
            const unsigned height = qMin(info.height, (uint)img.height()); //should be the same
            for (unsigned y = 0; y < height; y++, bmpPtr += info.stride)
                memcpy(bmpPtr, img.constScanLine(y), width);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return bitmap;
    }

    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env)
    {
        if (format != QImage::Format_RGBA8888
                && format != QImage::Format_RGB16)
            return 0;

        return env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     width,
                                                     height,
                                                     format == QImage::Format_RGB16
                                                        ? m_RGB_565_BitmapConfigValue
                                                        : m_ARGB_8888_BitmapConfigValue);
    }

    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env)
    {
        if (!bitmap || !m_bitmapDrawableClass || !m_resourcesObj)
            return 0;

        return env->NewObject(m_bitmapDrawableClass,
                              m_bitmapDrawableConstructorMethodID,
                              m_resourcesObj,
                              bitmap);
    }

    const char *classErrorMsgFmt()
    {
        return m_classErrorMsg;
    }

    const char *methodErrorMsgFmt()
    {
        return m_methodErrorMsg;
    }

    const char *qtTagText()
    {
        return m_qtTag;
    }

    QString deviceName()
    {
        QString manufacturer = QJniObject::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;").toString();
        QString model = QJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString();

        return manufacturer + u' ' + model;
    }

    jint generateViewId()
    {
        return QJniObject::callStaticMethod<jint>("android/view/View", "generateViewId", "()I");
    }

    int createSurface(AndroidSurfaceClient *client, const QRect &geometry, bool onTop, int imageDepth)
    {
        QJniEnvironment env;
        if (!env.jniEnv())
            return -1;

        m_surfacesMutex.lock();
        jint surfaceId = generateViewId();
        m_surfaces[surfaceId] = client;
        m_surfacesMutex.unlock();

        jint x = 0, y = 0, w = -1, h = -1;
        if (!geometry.isNull()) {
            x = geometry.x();
            y = geometry.y();
            w = std::max(geometry.width(), 1);
            h = std::max(geometry.height(), 1);
        }
        env->CallStaticVoidMethod(m_applicationClass,
                                     m_createSurfaceMethodID,
                                     surfaceId,
                                     jboolean(onTop),
                                     x, y, w, h,
                                     imageDepth);
        return surfaceId;
    }

    int insertNativeView(jobject view, const QRect &geometry)
    {
        m_surfacesMutex.lock();
        jint surfaceId = generateViewId();
        m_surfaces[surfaceId] = nullptr; // dummy
        m_surfacesMutex.unlock();

        jint x = 0, y = 0, w = -1, h = -1;
        if (!geometry.isNull())
            geometry.getRect(&x, &y, &w, &h);

        QJniObject::callStaticMethod<void>(m_applicationClass,
                                           "insertNativeView",
                                           "(ILandroid/view/View;IIII)V",
                                           surfaceId,
                                           view,
                                           x,
                                           y,
                                           qMax(w, 1),
                                           qMax(h, 1));

        return surfaceId;
    }

    void setViewVisibility(jobject view, bool visible)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass,
                                           "setViewVisibility",
                                           "(Landroid/view/View;Z)V",
                                           view,
                                           visible);
    }

    void setSurfaceGeometry(int surfaceId, const QRect &geometry)
    {
        if (surfaceId == -1)
            return;

        QJniEnvironment env;
        if (!env.jniEnv())
            return;
        jint x = 0, y = 0, w = -1, h = -1;
        if (!geometry.isNull()) {
            x = geometry.x();
            y = geometry.y();
            w = geometry.width();
            h = geometry.height();
        }
        env->CallStaticVoidMethod(m_applicationClass,
                                  m_setSurfaceGeometryMethodID,
                                  surfaceId,
                                  x, y, w, h);
    }


    void destroySurface(int surfaceId)
    {
        if (surfaceId == -1)
            return;

        {
            QMutexLocker lock(&m_surfacesMutex);
            const auto &it = m_surfaces.find(surfaceId);
            if (it != m_surfaces.end())
                m_surfaces.erase(it);
        }

        QJniEnvironment env;
        if (env.jniEnv())
            env->CallStaticVoidMethod(m_applicationClass,
                                      m_destroySurfaceMethodID,
                                      surfaceId);
    }

    void bringChildToFront(int surfaceId)
    {
        if (surfaceId == -1)
            return;

        QJniObject::callStaticMethod<void>(m_applicationClass,
                                           "bringChildToFront",
                                           "(I)V",
                                           surfaceId);
    }

    void bringChildToBack(int surfaceId)
    {
        if (surfaceId == -1)
            return;

        QJniObject::callStaticMethod<void>(m_applicationClass,
                                           "bringChildToBack",
                                           "(I)V",
                                           surfaceId);
    }

    bool blockEventLoopsWhenSuspended()
    {
        static bool block = qEnvironmentVariableIntValue("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED");
        return block;
    }

    jobject assets()
    {
        return m_assets;
    }

} // namespace QtAndroid

static jboolean startQtAndroidPlugin(JNIEnv *env, jobject /*object*/, jstring paramsString)
{
    m_androidPlatformIntegration = nullptr;
    m_androidAssetsFileEngineHandler = new AndroidAssetsFileEngineHandler();
    m_androidContentFileEngineHandler = new AndroidContentFileEngineHandler();
    m_mainLibraryHnd = nullptr;

    const char *nativeString = env->GetStringUTFChars(paramsString, 0);
    const QStringList argsList = QProcess::splitCommand(QString::fromUtf8(nativeString));
    env->ReleaseStringUTFChars(paramsString, nativeString);

    for (const QString &arg : argsList)
        m_applicationParams.append(arg.toUtf8());

    // Go home
    QDir::setCurrent(QDir::homePath());

    //look for main()
    if (m_applicationParams.length()) {
        // Obtain a handle to the main library (the library that contains the main() function).
        // This library should already be loaded, and calling dlopen() will just return a reference to it.
        m_mainLibraryHnd = dlopen(m_applicationParams.constFirst().data(), 0);
        if (Q_UNLIKELY(!m_mainLibraryHnd)) {
            qCritical() << "dlopen failed:" << dlerror();
            return false;
        }
        m_main = (Main)dlsym(m_mainLibraryHnd, "main");
    } else {
        qWarning("No main library was specified; searching entire process (this is slow!)");
        m_main = (Main)dlsym(RTLD_DEFAULT, "main");
    }

    if (Q_UNLIKELY(!m_main)) {
        qCritical() << "dlsym failed:" << dlerror() << Qt::endl
                    << "Could not find main method";
        return false;
    }

    if (sem_init(&m_exitSemaphore, 0, 0) == -1)
        return false;

    if (sem_init(&m_terminateSemaphore, 0, 0) == -1)
        return false;

    return true;
}

static void waitForServiceSetup(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    // The service must wait until the QCoreApplication starts otherwise onBind will be
    // called too early
    if (m_serviceObject)
        QtAndroidPrivate::waitForServiceSetup();
}

static void startQtApplication(JNIEnv */*env*/, jclass /*clazz*/)
{
    {
        JNIEnv* env = nullptr;
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        args.name = "QtMainThread";
        args.group = NULL;
        JavaVM *vm = QJniEnvironment::javaVM();
        if (vm)
            vm->AttachCurrentThread(&env, &args);
    }

    // Register type for invokeMethod() calls.
    qRegisterMetaType<Qt::ScreenOrientation>("Qt::ScreenOrientation");

    // Register resources if they are available
    if (QFile{QStringLiteral("assets:/android_rcc_bundle.rcc")}.exists())
        QResource::registerResource(QStringLiteral("assets:/android_rcc_bundle.rcc"));

    const int argc = m_applicationParams.size();
    QVarLengthArray<char *> argv(argc + 1);
    for (int i = 0; i < argc; i++)
        argv[i] = m_applicationParams[i].data();
    argv[argc] = nullptr;

    startQtAndroidPluginCalled.fetchAndAddRelease(1);
    int ret = m_main(argc, argv.data());

    if (m_mainLibraryHnd) {
        int res = dlclose(m_mainLibraryHnd);
        if (res < 0)
            qWarning() << "dlclose failed:" << dlerror();
    }

    if (m_applicationClass) {
        qWarning("exit app 0");
        QJniObject::callStaticMethod<void>(m_applicationClass, "quitApp", "()V");
    }

    sem_post(&m_terminateSemaphore);
    sem_wait(&m_exitSemaphore);
    sem_destroy(&m_exitSemaphore);

    // We must call exit() to ensure that all global objects will be destructed
    if (!qEnvironmentVariableIsSet("QT_ANDROID_NO_EXIT_CALL"))
        exit(ret);
}

static void quitQtCoreApplication(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    QCoreApplication::quit();
}

static void quitQtAndroidPlugin(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    m_androidPlatformIntegration = nullptr;
    delete m_androidAssetsFileEngineHandler;
    m_androidAssetsFileEngineHandler = nullptr;
    delete m_androidContentFileEngineHandler;
    m_androidContentFileEngineHandler = nullptr;
}

static void terminateQt(JNIEnv *env, jclass /*clazz*/)
{
    // QAndroidEventDispatcherStopper is stopped when the user uses the task manager to kill the application
    if (QAndroidEventDispatcherStopper::instance()->stopped()) {
        QAndroidEventDispatcherStopper::instance()->startAll();
        QCoreApplication::quit();
        QAndroidEventDispatcherStopper::instance()->goingToStop(false);
    }

    if (startQtAndroidPluginCalled.loadAcquire())
        sem_wait(&m_terminateSemaphore);

    sem_destroy(&m_terminateSemaphore);

    env->DeleteGlobalRef(m_applicationClass);
    env->DeleteGlobalRef(m_classLoaderObject);
    if (m_resourcesObj)
        env->DeleteGlobalRef(m_resourcesObj);
    if (m_activityObject)
        env->DeleteGlobalRef(m_activityObject);
    if (m_serviceObject)
        env->DeleteGlobalRef(m_serviceObject);
    if (m_bitmapClass)
        env->DeleteGlobalRef(m_bitmapClass);
    if (m_ARGB_8888_BitmapConfigValue)
        env->DeleteGlobalRef(m_ARGB_8888_BitmapConfigValue);
    if (m_RGB_565_BitmapConfigValue)
        env->DeleteGlobalRef(m_RGB_565_BitmapConfigValue);
    if (m_bitmapDrawableClass)
        env->DeleteGlobalRef(m_bitmapDrawableClass);
    if (m_assets)
        env->DeleteGlobalRef(m_assets);
    m_androidPlatformIntegration = nullptr;
    delete m_androidAssetsFileEngineHandler;
    m_androidAssetsFileEngineHandler = nullptr;
    sem_post(&m_exitSemaphore);
}

static void setSurface(JNIEnv *env, jobject /*thiz*/, jint id, jobject jSurface, jint w, jint h)
{
    QMutexLocker lock(&m_surfacesMutex);
    const auto &it = m_surfaces.find(id);
    if (it == m_surfaces.end())
        return;

    auto surfaceClient = it.value();
    if (surfaceClient)
        surfaceClient->surfaceChanged(env, jSurface, w, h);
}

static void setDisplayMetrics(JNIEnv * /*env*/, jclass /*clazz*/, jint screenWidthPixels,
                              jint screenHeightPixels, jint availableLeftPixels,
                              jint availableTopPixels, jint availableWidthPixels,
                              jint availableHeightPixels, jdouble xdpi, jdouble ydpi,
                              jdouble scaledDensity, jdouble density, jfloat refreshRate)
{
    Q_UNUSED(availableLeftPixels)
    Q_UNUSED(availableTopPixels)

    m_availableWidthPixels = availableWidthPixels;
    m_availableHeightPixels = availableHeightPixels;
    m_scaledDensity = scaledDensity;
    m_density = density;

    const QSize screenSize(screenWidthPixels, screenHeightPixels);
    // available geometry always starts from top left
    const QRect availableGeometry(0, 0, availableWidthPixels, availableHeightPixels);
    const QSize physicalSize(qRound(double(screenWidthPixels) / xdpi * 25.4),
                             qRound(double(screenHeightPixels) / ydpi * 25.4));

    QMutexLocker lock(&m_platformMutex);
    if (!m_androidPlatformIntegration) {
        QAndroidPlatformIntegration::setDefaultDisplayMetrics(
                availableGeometry.left(), availableGeometry.top(), availableGeometry.width(),
                availableGeometry.height(), physicalSize.width(), physicalSize.height(),
                screenSize.width(), screenSize.height());
    } else {
        m_androidPlatformIntegration->setScreenSizeParameters(physicalSize, screenSize,
                                                              availableGeometry);
        m_androidPlatformIntegration->setRefreshRate(refreshRate);
    }
}

static void updateWindow(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidPlatformIntegration)
        return;

    if (QGuiApplication::instance() != nullptr) {
        const auto tlw = QGuiApplication::topLevelWindows();
        for (QWindow *w : tlw) {

            // Skip non-platform windows, e.g., offscreen windows.
            if (!w->handle())
                continue;

            QRect availableGeometry = w->screen()->availableGeometry();
            if (w->geometry().width() > 0 && w->geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
                QWindowSystemInterface::handleExposeEvent(w, QRegion(QRect(QPoint(), w->geometry().size())));
        }
    }

    QAndroidPlatformScreen *screen = static_cast<QAndroidPlatformScreen *>(m_androidPlatformIntegration->screen());
    if (screen->rasterSurfaces())
        QMetaObject::invokeMethod(screen, "setDirty", Qt::QueuedConnection, Q_ARG(QRect,screen->geometry()));
}

static void updateApplicationState(JNIEnv */*env*/, jobject /*thiz*/, jint state)
{
    QMutexLocker lock(&m_platformMutex);
    if (!m_main || !m_androidPlatformIntegration) {
        m_pendingApplicationState = state;
        return;
    }

    // We're about to call user code from the Android thread, since we don't know
    //the side effects we'll unlock first!
    lock.unlock();
    if (state == Qt::ApplicationActive)
        QtAndroidPrivate::handleResume();
    else if (state == Qt::ApplicationInactive)
        QtAndroidPrivate::handlePause();
    lock.relock();
    if (!m_androidPlatformIntegration)
        return;

    if (state <= Qt::ApplicationInactive) {
        // NOTE: sometimes we will receive two consecutive suspended notifications,
        // In the second suspended notification, QWindowSystemInterface::flushWindowSystemEvents()
        // will deadlock since the dispatcher has been stopped in the first suspended notification.
        // To avoid the deadlock we simply return if we found the event dispatcher has been stopped.
        if (QAndroidEventDispatcherStopper::instance()->stopped())
            return;

        // Don't send timers and sockets events anymore if we are going to hide all windows
        QAndroidEventDispatcherStopper::instance()->goingToStop(true);
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
        if (state == Qt::ApplicationSuspended)
            QAndroidEventDispatcherStopper::instance()->stopAll();
    } else {
        QAndroidEventDispatcherStopper::instance()->startAll();
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
        QAndroidEventDispatcherStopper::instance()->goingToStop(false);
    }
}

static void handleOrientationChanged(JNIEnv */*env*/, jobject /*thiz*/, jint newRotation, jint nativeOrientation)
{
    // Array of orientations rotated in 90 degree increments, counterclockwise
    // (same direction as Android measures angles)
    static const Qt::ScreenOrientation orientations[] = {
        Qt::PortraitOrientation,
        Qt::LandscapeOrientation,
        Qt::InvertedPortraitOrientation,
        Qt::InvertedLandscapeOrientation
    };

    // The Android API defines the following constants:
    // ROTATION_0 :   0
    // ROTATION_90 :  1
    // ROTATION_180 : 2
    // ROTATION_270 : 3
    // ORIENTATION_PORTRAIT :  1
    // ORIENTATION_LANDSCAPE : 2

    // and newRotation is how much the current orientation is rotated relative to nativeOrientation

    // which means that we can be really clever here :)
    Qt::ScreenOrientation screenOrientation = orientations[(nativeOrientation - 1 + newRotation) % 4];
    Qt::ScreenOrientation native = orientations[nativeOrientation - 1];

    QAndroidPlatformIntegration::setScreenOrientation(screenOrientation, native);
    QMutexLocker lock(&m_platformMutex);
    if (m_androidPlatformIntegration) {
        QAndroidPlatformScreen *screen = m_androidPlatformIntegration->screen();
        // Use invokeMethod to keep the certain order of the "geometry change"
        // and "orientation change" event handling.
        if (screen) {
            QMetaObject::invokeMethod(screen, "setOrientation", Qt::AutoConnection,
                                      Q_ARG(Qt::ScreenOrientation, screenOrientation));
        }
    }
}

static void handleRefreshRateChanged(JNIEnv */*env*/, jclass /*cls*/, jfloat refreshRate)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->setRefreshRate(refreshRate);
}

static void handleScreenAdded(JNIEnv */*env*/, jclass /*cls*/, jint displayId)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->handleScreenAdded(displayId);
}

static void handleScreenChanged(JNIEnv */*env*/, jclass /*cls*/, jint displayId)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->handleScreenChanged(displayId);
}

static void handleScreenRemoved(JNIEnv */*env*/, jclass /*cls*/, jint displayId)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->handleScreenRemoved(displayId);
}

static void handleUiDarkModeChanged(JNIEnv */*env*/, jobject /*thiz*/, jint newUiMode)
{
    QAndroidPlatformIntegration::setColorScheme(
        (newUiMode == 1 ) ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
}

static void onActivityResult(JNIEnv */*env*/, jclass /*cls*/,
                             jint requestCode,
                             jint resultCode,
                             jobject data)
{
    QtAndroidPrivate::handleActivityResult(requestCode, resultCode, data);
}

static void onNewIntent(JNIEnv *env, jclass /*cls*/, jobject data)
{
    QtAndroidPrivate::handleNewIntent(env, data);
}

static jobject onBind(JNIEnv */*env*/, jclass /*cls*/, jobject intent)
{
    return QtAndroidPrivate::callOnBindListener(intent);
}

static JNINativeMethod methods[] = {
    { "startQtAndroidPlugin", "(Ljava/lang/String;)Z", (void *)startQtAndroidPlugin },
    { "startQtApplication", "()V", (void *)startQtApplication },
    { "quitQtAndroidPlugin", "()V", (void *)quitQtAndroidPlugin },
    { "quitQtCoreApplication", "()V", (void *)quitQtCoreApplication },
    { "terminateQt", "()V", (void *)terminateQt },
    { "waitForServiceSetup", "()V", (void *)waitForServiceSetup },
    { "setDisplayMetrics", "(IIIIIIDDDDF)V", (void *)setDisplayMetrics },
    { "setSurface", "(ILjava/lang/Object;II)V", (void *)setSurface },
    { "updateWindow", "()V", (void *)updateWindow },
    { "updateApplicationState", "(I)V", (void *)updateApplicationState },
    { "handleUiDarkModeChanged", "(I)V", (void *)handleUiDarkModeChanged },
    { "handleOrientationChanged", "(II)V", (void *)handleOrientationChanged },
    { "onActivityResult", "(IILandroid/content/Intent;)V", (void *)onActivityResult },
    { "onNewIntent", "(Landroid/content/Intent;)V", (void *)onNewIntent },
    { "onBind", "(Landroid/content/Intent;)Landroid/os/IBinder;", (void *)onBind },
    { "handleRefreshRateChanged", "(F)V", (void *)handleRefreshRateChanged },
    { "handleScreenAdded", "(I)V", (void *)handleScreenAdded },
    { "handleScreenChanged", "(I)V", (void *)handleScreenChanged },
    { "handleScreenRemoved", "(I)V", (void *)handleScreenRemoved }
};

#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_classErrorMsg, CLASS_NAME); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetStaticFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return JNI_FALSE; \
}

static int registerNatives(JNIEnv *env)
{
    jclass clazz;
    FIND_AND_CHECK_CLASS("org/qtproject/qt/android/QtNative");
    m_applicationClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    if (env->RegisterNatives(m_applicationClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
        return JNI_FALSE;
    }

    GET_AND_CHECK_STATIC_METHOD(m_createSurfaceMethodID, m_applicationClass, "createSurface", "(IZIIIII)V");
    GET_AND_CHECK_STATIC_METHOD(m_setSurfaceGeometryMethodID, m_applicationClass, "setSurfaceGeometry", "(IIIII)V");
    GET_AND_CHECK_STATIC_METHOD(m_destroySurfaceMethodID, m_applicationClass, "destroySurface", "(I)V");

    jmethodID methodID;
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "activity", "()Landroid/app/Activity;");
    jobject activityObject = env->CallStaticObjectMethod(m_applicationClass, methodID);
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "service", "()Landroid/app/Service;");
    jobject serviceObject = env->CallStaticObjectMethod(m_applicationClass, methodID);
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "classLoader", "()Ljava/lang/ClassLoader;");
    m_classLoaderObject = env->NewGlobalRef(env->CallStaticObjectMethod(m_applicationClass, methodID));
    clazz = env->GetObjectClass(m_classLoaderObject);
    GET_AND_CHECK_METHOD(m_loadClassMethodID, clazz, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (serviceObject)
        m_serviceObject = env->NewGlobalRef(serviceObject);

    if (activityObject)
        m_activityObject = env->NewGlobalRef(activityObject);

    jobject object = activityObject ? activityObject : serviceObject;
    if (object) {
        FIND_AND_CHECK_CLASS("android/content/ContextWrapper");
        GET_AND_CHECK_METHOD(methodID, clazz, "getAssets", "()Landroid/content/res/AssetManager;");
        m_assets = env->NewGlobalRef(env->CallObjectMethod(object, methodID));
        m_assetManager = AAssetManager_fromJava(env, m_assets);

        GET_AND_CHECK_METHOD(methodID, clazz, "getResources", "()Landroid/content/res/Resources;");
        m_resourcesObj = env->NewGlobalRef(env->CallObjectMethod(object, methodID));

        FIND_AND_CHECK_CLASS("android/graphics/Bitmap");
        m_bitmapClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        GET_AND_CHECK_STATIC_METHOD(m_createBitmapMethodID, m_bitmapClass
                                    , "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
        FIND_AND_CHECK_CLASS("android/graphics/Bitmap$Config");
        jfieldID fieldId;
        GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
        m_ARGB_8888_BitmapConfigValue = env->NewGlobalRef(env->GetStaticObjectField(clazz, fieldId));
        GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "RGB_565", "Landroid/graphics/Bitmap$Config;");
        m_RGB_565_BitmapConfigValue = env->NewGlobalRef(env->GetStaticObjectField(clazz, fieldId));

        FIND_AND_CHECK_CLASS("android/graphics/drawable/BitmapDrawable");
        m_bitmapDrawableClass = static_cast<jclass>(env->NewGlobalRef(clazz));
        GET_AND_CHECK_METHOD(m_bitmapDrawableConstructorMethodID,
                             m_bitmapDrawableClass,
                             "<init>",
                             "(Landroid/content/res/Resources;Landroid/graphics/Bitmap;)V");
    }

    return JNI_TRUE;
}

QT_END_NAMESPACE

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void */*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QT_USE_NAMESPACE
    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    UnionJNIEnvToVoid uenv;
    uenv.venv = nullptr;
    m_javaVM = nullptr;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "GetEnv failed");
        return -1;
    }

    JNIEnv *env = uenv.nativeEnvironment;
    if (!registerNatives(env)
            || !QtAndroidInput::registerNatives(env)
            || !QtAndroidMenu::registerNatives(env)
            || !QtAndroidAccessibility::registerNatives(env)
            || !QtAndroidDialogHelpers::registerNatives(env)) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "registerNatives failed");
        return -1;
    }
    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);

    m_javaVM = vm;
    // attach qt main thread data to this thread
    QObject threadSetter;
    if (threadSetter.thread())
        threadSetter.thread()->setObjectName("QtMainLoopThread");
    __android_log_print(ANDROID_LOG_INFO, "Qt", "qt started");
    return JNI_VERSION_1_6;
}
