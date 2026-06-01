// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qandroidtypeconverter_p.h>
#include <QtCore/private/qandroidtypes_p.h>
#include <QtQuick/private/qandroidquickviewembedding_p.h>
#include <QtQuick/private/qandroidviewsignalmanager_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qjnitypes.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <functional>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtDelegate, "org/qtproject/qt/android/QtEmbeddedContextDelegate");
Q_DECLARE_JNI_CLASS(QtQuickView, "org/qtproject/qt/android/QtQuickView");
Q_DECLARE_JNI_CLASS(QtWindow, "org/qtproject/qt/android/QtWindow");
Q_DECLARE_JNI_CLASS(View, "android/view/View");

namespace QtAndroidQuickViewEmbedding
{
    constexpr const char *uninitializedViewMessage = "because QtQuickView is not loaded or ready yet.";

    static void onQQuickViewStatusChanged(const QJniObject &qtViewObject,
                                          QAndroidQuickView::Status status)
    {
        auto future = QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                [qtViewObject, status] {
                    qtViewObject.callMethod<void>("handleStatusChange", status);
                }, QDeadlineTimer(1000));
        future.waitForFinished(); // Wait for the user to handle status change.
    }

    void createQuickView(JNIEnv *, jobject nativeWindow, jstring qmlUri, jint width, jint height,
                         jlong parentWindowReference, jlong viewReference,
                         const QJniArray<jstring> &qmlImportPaths)
    {
        static_assert (sizeof(jlong) >= sizeof(void*),
                      "Insufficient size of Java type to hold the c++ pointer");
        const QUrl qmlUrl(QJniObject(qmlUri).toString());

        const QStringList importPaths = qmlImportPaths.toContainer();
        QMetaObject::invokeMethod(qApp, [qtViewObject = QJniObject(nativeWindow),
                                        parentWindowReference,
                                        viewReference,
                                        width,
                                        height,
                                        qmlUrl,
                                        importPaths] {
            // If the view does not exists (viewReference==0) we should create and set it up.
            // Else we only reset the source of the view.
            QAndroidQuickView *view = reinterpret_cast<QAndroidQuickView *>(viewReference);
            if (!view) {
                QWindow *parentWindow = reinterpret_cast<QWindow *>(parentWindowReference);
                view = new QAndroidQuickView(parentWindow);
                QObject::connect(
                        view, &QAndroidQuickView::statusChanged,
                        std::bind(&onQQuickViewStatusChanged, qtViewObject, std::placeholders::_1));
                view->setResizeMode(QAndroidQuickView::SizeRootObjectToView);
                view->setColor(QColor(Qt::transparent));
                view->setWidth(width);
                view->setHeight(height);
                QQmlEngine *engine = view->engine();
                for (const QString &path : importPaths)
                    engine->addImportPath(path);

                QObject::connect(engine, &QQmlEngine::quit, QCoreApplication::instance(),
                                 &QCoreApplication::quit);

                const QtJniTypes::QtWindow window = reinterpret_cast<jobject>(view->winId());
                qtViewObject.callMethod<void>("addQtWindow",
                                              window,
                                              reinterpret_cast<jlong>(view),
                                              parentWindowReference);
            }
            view->setSource(qmlUrl);
        });
    }

    std::pair<QAndroidQuickView *, QQuickItem *> getViewAndRootObject(jlong windowReference)
    {
        QAndroidQuickView *view = reinterpret_cast<QAndroidQuickView *>(windowReference);
        QQuickItem *rootObject = Q_LIKELY(view) ? view->rootObject() : nullptr;
        return std::make_pair(view, rootObject);
    }

    void setRootObjectProperty(JNIEnv *env, jobject object, jlong windowReference,
                               jstring propertyName, jobject value)
    {
        Q_UNUSED(env);
        Q_UNUSED(object);

        auto [_, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot set property %s %s", qPrintable(QJniObject(propertyName).toString()),
                     uninitializedViewMessage);
            return;
        }

        const QString property = QJniObject(propertyName).toString();
        const QMetaObject *rootMetaObject = rootObject->metaObject();
        int propertyIndex = rootMetaObject->indexOfProperty(qPrintable(property));
        if (propertyIndex < 0) {
            qWarning("Property %s does not exist in the root QML object.", qPrintable(property));
            return;
        }

        const QJniObject propertyValue(value);
        const QVariant variantToWrite = QAndroidTypeConverter::toQVariant(propertyValue);

        if (!variantToWrite.isValid()) {
            qWarning("Setting the property type of %s is not supported.",
                     propertyValue.className().data());
        } else {
            QMetaObject::invokeMethod(rootObject,
                [metaProperty = rootMetaObject->property(propertyIndex),
                 rootObject = rootObject,
                 variantToWrite] {
                    metaProperty.write(rootObject, variantToWrite);
                });
        }
    }

    jobject getRootObjectProperty(JNIEnv *env, jobject object, jlong windowReference,
                                  jstring propertyName)
    {
        Q_UNUSED(object);
        Q_ASSERT(env);

        const QString property = QJniObject(propertyName).toString();
        auto [_, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot get property %s %s", qPrintable(property), uninitializedViewMessage);
            return nullptr;
        }

        const QMetaObject *rootMetaObject = rootObject->metaObject();
        int propertyIndex = rootMetaObject->indexOfProperty(property.toUtf8().constData());
        if (propertyIndex < 0) {
            qWarning("Cannot get property %s as it does not exist in the root QML object.",
                     qPrintable(property));
            return nullptr;
        }

        QMetaProperty metaProperty = rootMetaObject->property(propertyIndex);
        QVariant propertyValue;
        if (QCoreApplication::instance()->thread()->isCurrentThread()) {
            propertyValue = metaProperty.read(rootObject);
        } else {
            QMetaObject::invokeMethod(rootObject,
                [&propertyValue, &metaProperty, rootObject = rootObject] {
                    propertyValue = metaProperty.read(rootObject);
                }, Qt::BlockingQueuedConnection);
        }
        jobject jObject = QAndroidTypeConverter::toJavaObject(propertyValue, env);
        if (!jObject) {
            qWarning("Property %s cannot be converted to a supported Java data type.",
                     qPrintable(property));
        }
        return jObject;
    }

    int addRootObjectSignalListener(JNIEnv *env, jobject, jlong windowReference, jstring signalName,
                                    QJniArray<jclass> argTypes, jobject listener)
    {
        Q_ASSERT(env);

        auto [view, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot connect to signal %s %s",
                     qPrintable(QJniObject(signalName).toString()), uninitializedViewMessage);
            return -1;
        }

        QAndroidViewSignalManager *signalManager = view->signalManager();
        return signalManager->addConnection(QJniObject(signalName).toString(), argTypes,
                                            QJniObject(listener), *rootObject);
    }

    bool removeRootObjectSignalListener(JNIEnv *, jobject, jlong windowReference,
                                       jint signalListenerId)
    {
        auto [view, rootObject] = getViewAndRootObject(windowReference);
        if (!rootObject) {
            qWarning("Cannot disconnect the signal connection with id: %i %s", signalListenerId,
                     uninitializedViewMessage);
            return false;
        }
        view->signalManager()->removeConnection(signalListenerId);
        return true;
    }

    bool registerNatives(QJniEnvironment& env) {
        return env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtQuickView>::className(),
                                         {Q_JNI_NATIVE_SCOPED_METHOD(createQuickView,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(setRootObjectProperty,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(getRootObjectProperty,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(addRootObjectSignalListener,
                                                                     QtAndroidQuickViewEmbedding),
                                          Q_JNI_NATIVE_SCOPED_METHOD(removeRootObjectSignalListener,
                                                                     QtAndroidQuickViewEmbedding)});
    }
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    Q_UNUSED(vm)
    Q_UNUSED(reserved)

    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QJniEnvironment env;
    if (!env.isValid())
        return JNI_ERR;
    if (!QtAndroidQuickViewEmbedding::registerNatives(env))
        return JNI_ERR;
    return JNI_VERSION_1_6;
}

QT_END_NAMESPACE
