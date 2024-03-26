// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdesignersupportitems_p.h"
#include "qquickdesignersupportproperties_p.h"

#include <private/qabstractanimation_p.h>
#include <private/qobject_p.h>
#include <private/qquickbehavior_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktextinput_p.h>
#include <private/qquicktextedit_p.h>
#include <private/qquicktransition_p.h>
#include <private/qquickloader_p.h>

#include <private/qquickanimation_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltimer_p.h>

QT_BEGIN_NAMESPACE

static void (*fixResourcePathsForObjectCallBack)(QObject*) = nullptr;

static void stopAnimation(QObject *object)
{
    if (object == nullptr)
        return;

    QQuickTransition *transition = qobject_cast<QQuickTransition*>(object);
    QQuickAbstractAnimation *animation = qobject_cast<QQuickAbstractAnimation*>(object);
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(object);
    if (transition) {
       transition->setFromState(QString());
       transition->setToState(QString());
    } else if (animation) {
//        QQuickScriptAction *scriptAimation = qobject_cast<QQuickScriptAction*>(animation);
//        if (scriptAimation) FIXME
//            scriptAimation->setScript(QQmlScriptString());
        animation->complete();
        animation->setDisableUserControl();
    } else if (timer) {
        timer->blockSignals(true);
    }
}

static void makeLoaderSynchronous(QObject *object)
{
    if (QQuickLoader *loader = qobject_cast<QQuickLoader*>(object))
        loader->setAsynchronous(false);
}

static void allSubObjects(QObject *object, QObjectList &objectList)
{
    // don't add null pointer and stop if the object is already in the list
    if (!object || objectList.contains(object))
        return;

    objectList.append(object);

    const QMetaObject *mo = object->metaObject();

    QByteArrayList deferredPropertyNames;
    const int namesIndex = mo->indexOfClassInfo("DeferredPropertyNames");
    if (namesIndex != -1) {
        QMetaClassInfo classInfo = mo->classInfo(namesIndex);
        deferredPropertyNames = QByteArray(classInfo.value()).split(',');
    }

    for (int index = QObject::staticMetaObject.propertyOffset();
         index < object->metaObject()->propertyCount();
         index++) {

        QMetaProperty metaProperty = object->metaObject()->property(index);

        if (deferredPropertyNames.contains(metaProperty.name()))
            continue;

        // search recursive in property objects
        if (metaProperty.isReadable()
                && metaProperty.isWritable()
                && metaProperty.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            if (qstrcmp(metaProperty.name(), "parent")) {
                QObject *propertyObject = QQmlMetaType::toQObject(metaProperty.read(object));
                allSubObjects(propertyObject, objectList);
            }

        }

        // search recursive in property object lists
        if (metaProperty.isReadable()
                && QQmlMetaType::isList(metaProperty.metaType())) {
            QQmlListReference list(object, metaProperty.name());
            if (list.canCount() && list.canAt()) {
                for (qsizetype i = 0; i < list.count(); i++) {
                    QObject *propertyObject = list.at(i);
                    allSubObjects(propertyObject, objectList);

                }
            }
        }
    }

    // search recursive in object children list
    for (QObject *childObject : object->children()) {
        allSubObjects(childObject, objectList);
    }

    // search recursive in quick item childItems list
    QQuickItem *quickItem = qobject_cast<QQuickItem*>(object);
    if (quickItem) {
        const auto childItems = quickItem->childItems();
        for (QQuickItem *childItem : childItems)
            allSubObjects(childItem, objectList);
    }
}

void QQuickDesignerSupportItems::tweakObjects(QObject *object)
{
    QObjectList objectList;
    allSubObjects(object, objectList);
    for (QObject* childObject : std::as_const(objectList)) {
        stopAnimation(childObject);
        makeLoaderSynchronous(childObject);
        if (fixResourcePathsForObjectCallBack)
            fixResourcePathsForObjectCallBack(childObject);
    }
}

static QObject *createDummyWindow(QQmlEngine *engine)
{
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/qtquickplugin/mockfiles/Window.qml")));
    return component.create();
}

static bool isWindowMetaObject(const QMetaObject *metaObject)
{
    if (metaObject) {
        if (metaObject->className() == QByteArrayLiteral("QWindow"))
            return true;

        return isWindowMetaObject(metaObject->superClass());
    }

    return false;
}

static bool isWindow(QObject *object) {
    if (object)
        return isWindowMetaObject(object->metaObject());

    return false;
}

static bool isCrashingType(const QQmlType &type)
{
    QString name = type.qmlTypeName();

    if (name == QLatin1String("QtMultimedia/MediaPlayer"))
        return true;

    if (name == QLatin1String("QtMultimedia/Audio"))
        return true;

    if (name == QLatin1String("QtQuick.Controls/MenuItem"))
        return true;

    if (name == QLatin1String("QtQuick.Controls/Menu"))
        return true;

    if (name == QLatin1String("QtQuick/Timer"))
        return true;

    return false;
}

QObject *QQuickDesignerSupportItems::createPrimitive(const QString &typeName, QTypeRevision version, QQmlContext *context)
{
    ComponentCompleteDisabler disableComponentComplete;

    Q_UNUSED(disableComponentComplete);

    QObject *object = nullptr;
    QQmlType type = QQmlMetaType::qmlType(typeName, version);

    if (isCrashingType(type)) {
        object = new QObject;
    } else if (type.isValid()) {
        if ( type.isComposite()) {
             object = createComponent(type.sourceUrl(), context);
        } else
        {
            if (type.typeName() == "QQmlComponent") {
                object = new QQmlComponent(context->engine(), nullptr);
            } else  {
                object = type.create();
            }
        }

        if (isWindow(object)) {
            delete object;
            object = createDummyWindow(context->engine());
        }

    }

    if (!object) {
        qWarning() << "QuickDesigner: Cannot create an object of type"
                   << QString::fromLatin1("%1 %2,%3").arg(typeName)
                      .arg(version.majorVersion()).arg(version.minorVersion())
                   << "- type isn't known to declarative meta type system";
    }

    tweakObjects(object);

    if (object && QQmlEngine::contextForObject(object) == nullptr)
        QQmlEngine::setContextForObject(object, context);

    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    return object;
}

QObject *QQuickDesignerSupportItems::createComponent(const QUrl &componentUrl, QQmlContext *context)
{
    ComponentCompleteDisabler disableComponentComplete;
    Q_UNUSED(disableComponentComplete);

    QQmlComponent component(context->engine(), componentUrl);

    QObject *object = component.beginCreate(context);
    tweakObjects(object);
    component.completeCreate();
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);

    if (component.isError()) {
        qWarning() << "Error in:" << Q_FUNC_INFO << componentUrl;
        const auto errors = component.errors();
        for (const QQmlError &error : errors)
            qWarning() << error;
    }
    return object;
}

bool QQuickDesignerSupportItems::objectWasDeleted(QObject *object)
{
    return QObjectPrivate::get(object)->wasDeleted;
}

void QQuickDesignerSupportItems::disableNativeTextRendering(QQuickItem *item)
{
    QQuickText *text = qobject_cast<QQuickText*>(item);
    if (text)
        text->setRenderType(QQuickText::QtRendering);

    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(item);
    if (textInput)
        textInput->setRenderType(QQuickTextInput::QtRendering);

    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(item);
    if (textEdit)
        textEdit->setRenderType(QQuickTextEdit::QtRendering);
}

void QQuickDesignerSupportItems::disableTextCursor(QQuickItem *item)
{
    const auto childItems = item->childItems();
    for (QQuickItem *childItem : childItems)
        disableTextCursor(childItem);

    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(item);
    if (textInput)
        textInput->setCursorVisible(false);

    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(item);
    if (textEdit)
        textEdit->setCursorVisible(false);
}

void QQuickDesignerSupportItems::disableTransition(QObject *object)
{
    QQuickTransition *transition = qobject_cast<QQuickTransition*>(object);
    Q_ASSERT(transition);
    const QString invalidState = QLatin1String("invalidState");
    transition->setToState(invalidState);
    transition->setFromState(invalidState);
}

void QQuickDesignerSupportItems::disableBehaivour(QObject *object)
{
    QQuickBehavior* behavior = qobject_cast<QQuickBehavior*>(object);
    Q_ASSERT(behavior);
    behavior->setEnabled(false);
}

void QQuickDesignerSupportItems::stopUnifiedTimer()
{
    QUnifiedTimer::instance()->setSlowdownFactor(0.00001);
    QUnifiedTimer::instance()->setSlowModeEnabled(true);
}

void QQuickDesignerSupportItems::registerFixResourcePathsForObjectCallBack(void (*callback)(QObject *))
{
    fixResourcePathsForObjectCallBack = callback;
}

QT_END_NAMESPACE


