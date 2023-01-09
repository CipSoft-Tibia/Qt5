/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlproperty.h"
#include "qqmlproperty_p.h"

#include "qqml.h"
#include "qqmlbinding_p.h"
#include "qqmlboundsignal_p.h"
#include "qqmlcontext.h"
#include "qqmlcontext_p.h"
#include "qqmlboundsignal_p.h"
#include "qqmlengine.h"
#include "qqmlengine_p.h"
#include "qqmldata_p.h"
#include "qqmlstringconverters_p.h"
#include "qqmllist_p.h"
#include "qqmlvmemetaobject_p.h"
#include "qqmlexpression_p.h"
#include "qqmlvaluetypeproxybinding_p.h"
#include <private/qjsvalue_p.h>
#include <private/qv4functionobject_p.h>

#include <QStringList>
#include <QVector>
#include <private/qmetaobject_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <QtCore/qdebug.h>
#include <cmath>
#include <QtQml/QQmlPropertyMap>

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<qreal>)
Q_DECLARE_METATYPE(QList<bool>)
Q_DECLARE_METATYPE(QList<QString>)
Q_DECLARE_METATYPE(QList<QUrl>)

QT_BEGIN_NAMESPACE

/*!
\class QQmlProperty
\since 5.0
\inmodule QtQml
\brief The QQmlProperty class abstracts accessing properties on objects created from  QML.

As QML uses Qt's meta-type system all of the existing QMetaObject classes can be used to introspect
and interact with objects created by QML.  However, some of the new features provided by QML - such
as type safety and attached properties - are most easily used through the QQmlProperty class
that simplifies some of their natural complexity.

Unlike QMetaProperty which represents a property on a class type, QQmlProperty encapsulates
a property on a specific object instance.  To read a property's value, programmers create a
QQmlProperty instance and call the read() method.  Likewise to write a property value the
write() method is used.

For example, for the following QML code:

\qml
// MyItem.qml
import QtQuick 2.0

Text { text: "A bit of text" }
\endqml

The \l Text object's properties could be accessed using QQmlProperty, like this:

\code
#include <QQmlProperty>
#include <QGraphicsObject>

...

QQuickView view(QUrl::fromLocalFile("MyItem.qml"));
QQmlProperty property(view.rootObject(), "font.pixelSize");
qWarning() << "Current pixel size:" << property.read().toInt();
property.write(24);
qWarning() << "Pixel size should now be 24:" << property.read().toInt();
\endcode
*/

/*!
    Create an invalid QQmlProperty.
*/
QQmlProperty::QQmlProperty()
: d(nullptr)
{
}

/*!  \internal */
QQmlProperty::~QQmlProperty()
{
    if (d)
        d->release();
    d = nullptr;
}

/*!
    Creates a QQmlProperty for the default property of \a obj. If there is no
    default property, an invalid QQmlProperty will be created.
 */
QQmlProperty::QQmlProperty(QObject *obj)
: d(new QQmlPropertyPrivate)
{
    d->initDefault(obj);
}

/*!
    Creates a QQmlProperty for the default property of \a obj
    using the \l{QQmlContext} {context} \a ctxt. If there is
    no default property, an invalid QQmlProperty will be
    created.
 */
QQmlProperty::QQmlProperty(QObject *obj, QQmlContext *ctxt)
: d(new QQmlPropertyPrivate)
{
    d->context = ctxt?QQmlContextData::get(ctxt):nullptr;
    d->engine = ctxt?ctxt->engine():nullptr;
    d->initDefault(obj);
}

/*!
    Creates a QQmlProperty for the default property of \a obj
    using the environment for instantiating QML components that is
    provided by \a engine.  If there is no default property, an
    invalid QQmlProperty will be created.
 */
QQmlProperty::QQmlProperty(QObject *obj, QQmlEngine *engine)
  : d(new QQmlPropertyPrivate)
{
    d->context = nullptr;
    d->engine = engine;
    d->initDefault(obj);
}

/*!
    Initialize from the default property of \a obj
*/
void QQmlPropertyPrivate::initDefault(QObject *obj)
{
    if (!obj)
        return;

    QMetaProperty p = QQmlMetaType::defaultProperty(obj);
    core.load(p);
    if (core.isValid())
        object = obj;
}

/*!
    Creates a QQmlProperty for the property \a name of \a obj.
 */
QQmlProperty::QQmlProperty(QObject *obj, const QString &name)
: d(new QQmlPropertyPrivate)
{
    d->initProperty(obj, name);
    if (!isValid()) d->object = nullptr;
}

/*!
    Creates a QQmlProperty for the property \a name of \a obj
    using the \l{QQmlContext} {context} \a ctxt.

    Creating a QQmlProperty without a context will render some
    properties - like attached properties - inaccessible.
*/
QQmlProperty::QQmlProperty(QObject *obj, const QString &name, QQmlContext *ctxt)
: d(new QQmlPropertyPrivate)
{
    d->context = ctxt?QQmlContextData::get(ctxt):nullptr;
    d->engine = ctxt?ctxt->engine():nullptr;
    d->initProperty(obj, name);
    if (!isValid()) { d->object = nullptr; d->context = nullptr; d->engine = nullptr; }
}

/*!
    Creates a QQmlProperty for the property \a name of \a obj
    using the environment for instantiating QML components that is
    provided by \a engine.
 */
QQmlProperty::QQmlProperty(QObject *obj, const QString &name, QQmlEngine *engine)
: d(new QQmlPropertyPrivate)
{
    d->context = nullptr;
    d->engine = engine;
    d->initProperty(obj, name);
    if (!isValid()) { d->object = nullptr; d->context = nullptr; d->engine = nullptr; }
}

QQmlProperty QQmlPropertyPrivate::create(QObject *target, const QString &propertyName, QQmlContextData *context)
{
    QQmlProperty result;
    auto d = new QQmlPropertyPrivate;
    result.d = d;
    d->context = context;
    d->engine = context ? context->engine : nullptr;
    d->initProperty(target, propertyName);
    if (!result.isValid()) {
        d->object = nullptr;
        d->context = nullptr;
        d->engine = nullptr;
    }
    return result;
}

QQmlPropertyPrivate::QQmlPropertyPrivate()
: context(nullptr), engine(nullptr), object(nullptr), isNameCached(false)
{
}

QQmlContextData *QQmlPropertyPrivate::effectiveContext() const
{
    if (context) return context;
    else if (engine) return QQmlContextData::get(engine->rootContext());
    else return nullptr;
}

void QQmlPropertyPrivate::initProperty(QObject *obj, const QString &name)
{
    if (!obj) return;

    QQmlRefPointer<QQmlTypeNameCache> typeNameCache = context?context->imports:nullptr;

    QObject *currentObject = obj;
    QVector<QStringRef> path;
    QStringRef terminal(&name);

    if (name.contains(QLatin1Char('.'))) {
        path = name.splitRef(QLatin1Char('.'));
        if (path.isEmpty()) return;

        // Everything up to the last property must be an "object type" property
        for (int ii = 0; ii < path.count() - 1; ++ii) {
            const QStringRef &pathName = path.at(ii);

            // Types must begin with an uppercase letter (see checkRegistration()
            // in qqmlmetatype.cpp for the enforcement of this).
            if (typeNameCache && !pathName.isEmpty() && pathName.at(0).isUpper()) {
                QQmlTypeNameCache::Result r = typeNameCache->query(pathName);
                if (r.isValid()) {
                    if (r.type.isValid()) {
                        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
                        QQmlAttachedPropertiesFunc func = r.type.attachedPropertiesFunction(enginePrivate);
                        if (!func) return; // Not an attachable type

                        currentObject = qmlAttachedPropertiesObject(currentObject, func);
                        if (!currentObject) return; // Something is broken with the attachable type
                    } else if (r.importNamespace) {
                        if ((ii + 1) == path.count()) return; // No type following the namespace

                        ++ii; r = typeNameCache->query(path.at(ii), r.importNamespace);
                        if (!r.type.isValid()) return; // Invalid type in namespace

                        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
                        QQmlAttachedPropertiesFunc func = r.type.attachedPropertiesFunction(enginePrivate);
                        if (!func) return; // Not an attachable type

                        currentObject = qmlAttachedPropertiesObject(currentObject, func);
                        if (!currentObject) return; // Something is broken with the attachable type

                    } else if (r.scriptIndex != -1) {
                        return; // Not a type
                    } else {
                        Q_ASSERT(!"Unreachable");
                    }
                    continue;
                }

            }

            QQmlPropertyData local;
            QQmlPropertyData *property =
                    QQmlPropertyCache::property(engine, currentObject, pathName, context, local);

            if (!property) return; // Not a property
            if (property->isFunction())
                return; // Not an object property

            if (ii == (path.count() - 2) && QQmlValueTypeFactory::isValueType(property->propType())) {
                // We're now at a value type property
                const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(property->propType());
                if (!valueTypeMetaObject) return; // Not a value type

                int idx = valueTypeMetaObject->indexOfProperty(path.last().toUtf8().constData());
                if (idx == -1) return; // Value type property does not exist

                QMetaProperty vtProp = valueTypeMetaObject->property(idx);

                Q_ASSERT(vtProp.userType() <= 0x0000FFFF);
                Q_ASSERT(idx <= 0x0000FFFF);

                object = currentObject;
                core = *property;
                valueTypeData.setFlags(QQmlPropertyData::flagsForProperty(vtProp));
                valueTypeData.setPropType(vtProp.userType());
                valueTypeData.setCoreIndex(idx);

                return;
            } else {
                if (!property->isQObject()) {
                    if (auto asPropertyMap = qobject_cast<QQmlPropertyMap*>(currentObject))
                        currentObject = asPropertyMap->value(path.at(ii).toString()).value<QObject*>();
                    else
                        return; // Not an object property, and not a property map
                } else {
                    property->readProperty(currentObject, &currentObject);
                }

                if (!currentObject) return; // No value

            }

        }

        terminal = path.last();
    }

    if (terminal.count() >= 3 &&
        terminal.at(0) == QLatin1Char('o') &&
        terminal.at(1) == QLatin1Char('n') &&
        (terminal.at(2).isUpper() || terminal.at(2) == '_')) {

        QString signalName = terminal.mid(2).toString();
        int firstNon_;
        int length = signalName.length();
        for (firstNon_ = 0; firstNon_ < length; ++firstNon_)
            if (signalName.at(firstNon_) != '_')
                break;
        signalName[firstNon_] = signalName.at(firstNon_).toLower();

        // XXX - this code treats methods as signals

        QQmlData *ddata = QQmlData::get(currentObject, false);
        if (ddata && ddata->propertyCache) {

            // Try method
            QQmlPropertyData *d = ddata->propertyCache->property(signalName, currentObject, context);
            while (d && !d->isFunction())
                d = ddata->propertyCache->overrideData(d);

            if (d) {
                object = currentObject;
                core = *d;
                return;
            }

            // Try property
            if (signalName.endsWith(QLatin1String("Changed"))) {
                const QStringRef propName = signalName.midRef(0, signalName.length() - 7);
                QQmlPropertyData *d = ddata->propertyCache->property(propName, currentObject, context);
                while (d && d->isFunction())
                    d = ddata->propertyCache->overrideData(d);

                if (d && d->notifyIndex() != -1) {
                    object = currentObject;
                    core = *ddata->propertyCache->signal(d->notifyIndex());
                    return;
                }
            }

        } else {
            QMetaMethod method = findSignalByName(currentObject->metaObject(),
                                                  signalName.toLatin1());
            if (method.isValid()) {
                object = currentObject;
                core.load(method);
                return;
            }
        }
    }

    // Property
    QQmlPropertyData local;
    QQmlPropertyData *property =
        QQmlPropertyCache::property(engine, currentObject, terminal, context, local);
    if (property && !property->isFunction()) {
        object = currentObject;
        core = *property;
        nameCache = terminal.toString();
        isNameCached = true;
    }
}

/*! \internal
    Returns the index of this property's signal, in the signal index range
    (see QObjectPrivate::signalIndex()). This is different from
    QMetaMethod::methodIndex().
*/
int QQmlPropertyPrivate::signalIndex() const
{
    Q_ASSERT(type() == QQmlProperty::SignalProperty);
    QMetaMethod m = object->metaObject()->method(core.coreIndex());
    return QMetaObjectPrivate::signalIndex(m);
}

/*!
    Create a copy of \a other.
*/
QQmlProperty::QQmlProperty(const QQmlProperty &other)
{
    d = other.d;
    if (d)
        d->addref();
}

/*!
  \enum QQmlProperty::PropertyTypeCategory

  This enum specifies a category of QML property.

  \value InvalidCategory The property is invalid, or is a signal property.
  \value List The property is a QQmlListProperty list property
  \value Object The property is a QObject derived type pointer
  \value Normal The property is a normal value property.
 */

/*!
  \enum QQmlProperty::Type

  This enum specifies a type of QML property.

  \value Invalid The property is invalid.
  \value Property The property is a regular Qt property.
  \value SignalProperty The property is a signal property.
*/

/*!
    Returns the property category.
*/
QQmlProperty::PropertyTypeCategory QQmlProperty::propertyTypeCategory() const
{
    return d ? d->propertyTypeCategory() : InvalidCategory;
}

QQmlProperty::PropertyTypeCategory
QQmlPropertyPrivate::propertyTypeCategory() const
{
    uint type = this->type();

    if (isValueType()) {
        return QQmlProperty::Normal;
    } else if (type & QQmlProperty::Property) {
        int type = propertyType();
        if (type == QMetaType::UnknownType)
            return QQmlProperty::InvalidCategory;
        else if (QQmlValueTypeFactory::isValueType((uint)type))
            return QQmlProperty::Normal;
        else if (core.isQObject())
            return QQmlProperty::Object;
        else if (core.isQList())
            return QQmlProperty::List;
        else
            return QQmlProperty::Normal;
    }

    return QQmlProperty::InvalidCategory;
}

/*!
    Returns the type name of the property, or 0 if the property has no type
    name.
*/
const char *QQmlProperty::propertyTypeName() const
{
    if (!d)
        return nullptr;
    if (d->isValueType()) {
        const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(d->core.propType());
        Q_ASSERT(valueTypeMetaObject);
        return valueTypeMetaObject->property(d->valueTypeData.coreIndex()).typeName();
    } else if (d->object && type() & Property && d->core.isValid()) {
        return d->object->metaObject()->property(d->core.coreIndex()).typeName();
    } else {
        return nullptr;
    }
}

/*!
    Returns true if \a other and this QQmlProperty represent the same
    property.
*/
bool QQmlProperty::operator==(const QQmlProperty &other) const
{
    if (!d || !other.d)
        return false;
    // category is intentially omitted here as it is generated
    // from the other members
    return d->object == other.d->object &&
           d->core.coreIndex() == other.d->core.coreIndex() &&
           d->valueTypeData.coreIndex() == other.d->valueTypeData.coreIndex();
}

/*!
    Returns the QVariant type of the property, or QVariant::Invalid if the
    property has no QVariant type.
*/
int QQmlProperty::propertyType() const
{
    return d ? d->propertyType() : int(QMetaType::UnknownType);
}

bool QQmlPropertyPrivate::isValueType() const
{
    return valueTypeData.isValid();
}

int QQmlPropertyPrivate::propertyType() const
{
    uint type = this->type();
    if (isValueType()) {
        return valueTypeData.propType();
    } else if (type & QQmlProperty::Property) {
        return core.propType();
    } else {
        return QMetaType::UnknownType;
    }
}

QQmlProperty::Type QQmlPropertyPrivate::type() const
{
    if (core.isFunction())
        return QQmlProperty::SignalProperty;
    else if (core.isValid())
        return QQmlProperty::Property;
    else
        return QQmlProperty::Invalid;
}

/*!
    Returns the type of the property.
*/
QQmlProperty::Type QQmlProperty::type() const
{
    return d ? d->type() : Invalid;
}

/*!
    Returns true if this QQmlProperty represents a regular Qt property.
*/
bool QQmlProperty::isProperty() const
{
    return type() & Property;
}

/*!
    Returns true if this QQmlProperty represents a QML signal property.
*/
bool QQmlProperty::isSignalProperty() const
{
    return type() & SignalProperty;
}

/*!
    Returns the QQmlProperty's QObject.
*/
QObject *QQmlProperty::object() const
{
    return d ? d->object : nullptr;
}

/*!
    Assign \a other to this QQmlProperty.
*/
QQmlProperty &QQmlProperty::operator=(const QQmlProperty &other)
{
    QQmlProperty copied(other);
    qSwap(d, copied.d);
    return *this;
}

/*!
    Returns true if the property is writable, otherwise false.
*/
bool QQmlProperty::isWritable() const
{
    if (!d)
        return false;
    if (!d->object)
        return false;
    if (d->core.isQList())           //list
        return true;
    else if (d->core.isFunction())   //signal handler
        return false;
    else if (d->core.isValid())      //normal property
        return d->core.isWritable();
    else
        return false;
}

/*!
    Returns true if the property is designable, otherwise false.
*/
bool QQmlProperty::isDesignable() const
{
    if (!d)
        return false;
    if (type() & Property && d->core.isValid() && d->object)
        return d->object->metaObject()->property(d->core.coreIndex()).isDesignable();
    else
        return false;
}

/*!
    Returns true if the property is resettable, otherwise false.
*/
bool QQmlProperty::isResettable() const
{
    if (!d)
        return false;
    if (type() & Property && d->core.isValid() && d->object)
        return d->core.isResettable();
    else
        return false;
}

/*!
    Returns true if the QQmlProperty refers to a valid property, otherwise
    false.
*/
bool QQmlProperty::isValid() const
{
    if (!d)
        return false;
    return type() != Invalid;
}

/*!
    Return the name of this QML property.
*/
QString QQmlProperty::name() const
{
    if (!d)
        return QString();
    if (!d->isNameCached) {
        // ###
        if (!d->object) {
        } else if (d->isValueType()) {
            const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(d->core.propType());
            Q_ASSERT(valueTypeMetaObject);

            const char *vtName = valueTypeMetaObject->property(d->valueTypeData.coreIndex()).name();
            d->nameCache = d->core.name(d->object) + QLatin1Char('.') + QString::fromUtf8(vtName);
        } else if (type() & SignalProperty) {
            QString name = QLatin1String("on") + d->core.name(d->object);
            name[2] = name.at(2).toUpper();
            d->nameCache = name;
        } else {
            d->nameCache = d->core.name(d->object);
        }
        d->isNameCached = true;
    }

    return d->nameCache;
}

/*!
  Returns the \l{QMetaProperty} {Qt property} associated with
  this QML property.
 */
QMetaProperty QQmlProperty::property() const
{
    if (!d)
        return QMetaProperty();
    if (type() & Property && d->core.isValid() && d->object)
        return d->object->metaObject()->property(d->core.coreIndex());
    else
        return QMetaProperty();
}

/*!
    Return the QMetaMethod for this property if it is a SignalProperty,
    otherwise returns an invalid QMetaMethod.
*/
QMetaMethod QQmlProperty::method() const
{
    if (!d)
        return QMetaMethod();
    if (type() & SignalProperty && d->object)
        return d->object->metaObject()->method(d->core.coreIndex());
    else
        return QMetaMethod();
}

/*!
    Returns the binding associated with this property, or 0 if no binding
    exists.
*/
QQmlAbstractBinding *
QQmlPropertyPrivate::binding(const QQmlProperty &that)
{
    if (!that.d || !that.isProperty() || !that.d->object)
        return nullptr;

    QQmlPropertyIndex thatIndex(that.d->core.coreIndex(), that.d->valueTypeData.coreIndex());
    return binding(that.d->object, thatIndex);
}

/*!
    Set the binding associated with this property to \a newBinding.  Returns
    the existing binding (if any), otherwise 0.

    \a newBinding will be enabled, and the returned binding (if any) will be
    disabled.

    Ownership of \a newBinding transfers to QML.  Ownership of the return value
    is assumed by the caller.

    \a flags is passed through to the binding and is used for the initial update (when
    the binding sets the initial value, it will use these flags for the write).
*/
void
QQmlPropertyPrivate::setBinding(const QQmlProperty &that, QQmlAbstractBinding *newBinding)
{
    if (!newBinding) {
        removeBinding(that);
        return;
    }

    if (!that.d || !that.isProperty() || !that.d->object) {
        if (!newBinding->ref)
            delete newBinding;
        return;
    }
    setBinding(newBinding);
}

static void removeOldBinding(QObject *object, QQmlPropertyIndex index, QQmlPropertyPrivate::BindingFlags flags = QQmlPropertyPrivate::None)
{
    int coreIndex = index.coreIndex();
    int valueTypeIndex = index.valueTypeIndex();

    QQmlData *data = QQmlData::get(object, false);

    if (!data || !data->hasBindingBit(coreIndex))
        return;

    QQmlAbstractBinding::Ptr oldBinding;
    oldBinding = data->bindings;

    while (oldBinding && (oldBinding->targetPropertyIndex().coreIndex() != coreIndex ||
                          oldBinding->targetPropertyIndex().hasValueTypeIndex()))
        oldBinding = oldBinding->nextBinding();

    if (!oldBinding)
        return;

    if (valueTypeIndex != -1 && oldBinding->isValueTypeProxy())
        oldBinding = static_cast<QQmlValueTypeProxyBinding *>(oldBinding.data())->binding(index);

    if (!oldBinding)
        return;

    if (!(flags & QQmlPropertyPrivate::DontEnable))
        oldBinding->setEnabled(false, {});
    oldBinding->removeFromObject();
}

void QQmlPropertyPrivate::removeBinding(QQmlAbstractBinding *b)
{
    removeBinding(b->targetObject(), b->targetPropertyIndex());
}

void QQmlPropertyPrivate::removeBinding(QObject *o, QQmlPropertyIndex index)
{
    Q_ASSERT(o);

    QObject *target;
    QQmlPropertyIndex targetIndex;
    findAliasTarget(o, index, &target, &targetIndex);
    removeOldBinding(target, targetIndex);
}

void QQmlPropertyPrivate::removeBinding(const QQmlProperty &that)
{
    if (!that.d || !that.isProperty() || !that.d->object)
        return;

    removeBinding(that.d->object, that.d->encodedIndex());
}

QQmlAbstractBinding *
QQmlPropertyPrivate::binding(QObject *object, QQmlPropertyIndex index)
{
    findAliasTarget(object, index, &object, &index);

    QQmlData *data = QQmlData::get(object);
    if (!data)
        return nullptr;

    const int coreIndex = index.coreIndex();
    const int valueTypeIndex = index.valueTypeIndex();

    if (coreIndex < 0 || !data->hasBindingBit(coreIndex))
        return nullptr;

    QQmlAbstractBinding *binding = data->bindings;
    while (binding && (binding->targetPropertyIndex().coreIndex() != coreIndex ||
                       binding->targetPropertyIndex().hasValueTypeIndex()))
        binding = binding->nextBinding();

    if (binding && valueTypeIndex != -1) {
        if (binding->isValueTypeProxy()) {
            binding = static_cast<QQmlValueTypeProxyBinding *>(binding)->binding(index);
        }
    }

    return binding;
}

void QQmlPropertyPrivate::findAliasTarget(QObject *object, QQmlPropertyIndex bindingIndex,
                                          QObject **targetObject,
                                          QQmlPropertyIndex *targetBindingIndex)
{
    QQmlData *data = QQmlData::get(object, false);
    if (data) {
        int coreIndex = bindingIndex.coreIndex();
        int valueTypeIndex = bindingIndex.valueTypeIndex();

        QQmlPropertyData *propertyData =
            data->propertyCache?data->propertyCache->property(coreIndex):nullptr;
        if (propertyData && propertyData->isAlias()) {
            QQmlVMEMetaObject *vme = QQmlVMEMetaObject::getForProperty(object, coreIndex);

            QObject *aObject = nullptr; int aCoreIndex = -1; int aValueTypeIndex = -1;
            if (vme->aliasTarget(coreIndex, &aObject, &aCoreIndex, &aValueTypeIndex)) {
                // This will either be a value type sub-reference or an alias to a value-type sub-reference not both
                Q_ASSERT(valueTypeIndex == -1 || aValueTypeIndex == -1);

                QQmlPropertyIndex aBindingIndex(aCoreIndex);
                if (aValueTypeIndex != -1) {
                    aBindingIndex = QQmlPropertyIndex(aCoreIndex, aValueTypeIndex);
                } else if (valueTypeIndex != -1) {
                    aBindingIndex = QQmlPropertyIndex(aCoreIndex, valueTypeIndex);
                }

                findAliasTarget(aObject, aBindingIndex, targetObject, targetBindingIndex);
                return;
            }
        }
    }

    *targetObject = object;
    *targetBindingIndex = bindingIndex;
}


void QQmlPropertyPrivate::setBinding(QQmlAbstractBinding *binding, BindingFlags flags, QQmlPropertyData::WriteFlags writeFlags)
{
    Q_ASSERT(binding);
    Q_ASSERT(binding->targetObject());

    QObject *object = binding->targetObject();
    const QQmlPropertyIndex index = binding->targetPropertyIndex();

#ifndef QT_NO_DEBUG
    int coreIndex = index.coreIndex();
    QQmlData *data = QQmlData::get(object, true);
    if (data->propertyCache) {
        QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
        Q_ASSERT(propertyData);
    }
#endif

    removeOldBinding(object, index, flags);

    binding->addToObject();
    if (!(flags & DontEnable))
        binding->setEnabled(true, writeFlags);

}

/*!
    Returns the expression associated with this signal property, or 0 if no
    signal expression exists.
*/
QQmlBoundSignalExpression *
QQmlPropertyPrivate::signalExpression(const QQmlProperty &that)
{
    if (!(that.type() & QQmlProperty::SignalProperty))
        return nullptr;

    if (!that.d->object)
        return nullptr;
    QQmlData *data = QQmlData::get(that.d->object);
    if (!data)
        return nullptr;

    QQmlBoundSignal *signalHandler = data->signalHandlers;

    while (signalHandler && signalHandler->signalIndex() != QQmlPropertyPrivate::get(that)->signalIndex())
        signalHandler = signalHandler->m_nextSignal;

    if (signalHandler)
        return signalHandler->expression();

    return nullptr;
}

/*!
    Set the signal expression associated with this signal property to \a expr.
    A reference to \a expr will be added by QML.
*/
void QQmlPropertyPrivate::setSignalExpression(const QQmlProperty &that, QQmlBoundSignalExpression *expr)
{
    if (expr)
        expr->addref();
    QQmlPropertyPrivate::takeSignalExpression(that, expr);
}

/*!
    Set the signal expression associated with this signal property to \a expr.
    Ownership of \a expr transfers to QML.
*/
void QQmlPropertyPrivate::takeSignalExpression(const QQmlProperty &that,
                                         QQmlBoundSignalExpression *expr)
{
    if (!(that.type() & QQmlProperty::SignalProperty)) {
        if (expr)
            expr->release();
        return;
    }

    if (!that.d->object)
        return;
    QQmlData *data = QQmlData::get(that.d->object, nullptr != expr);
    if (!data)
        return;

    QQmlBoundSignal *signalHandler = data->signalHandlers;

    while (signalHandler && signalHandler->signalIndex() != QQmlPropertyPrivate::get(that)->signalIndex())
        signalHandler = signalHandler->m_nextSignal;

    if (signalHandler) {
        signalHandler->takeExpression(expr);
        return;
    }

    if (expr) {
        int signalIndex = QQmlPropertyPrivate::get(that)->signalIndex();
        QQmlBoundSignal *signal = new QQmlBoundSignal(that.d->object, signalIndex, that.d->object,
                                                      expr->context()->engine);
        signal->takeExpression(expr);
    }
}

/*!
    Returns the property value.
*/
QVariant QQmlProperty::read() const
{
    if (!d)
        return QVariant();
    if (!d->object)
        return QVariant();

    if (type() & SignalProperty) {

        return QVariant();

    } else if (type() & Property) {

        return d->readValueProperty();

    }
    return QVariant();
}

/*!
Return the \a name property value of \a object.  This method is equivalent to:
\code
    QQmlProperty p(object, name);
    p.read();
\endcode
*/
QVariant QQmlProperty::read(const QObject *object, const QString &name)
{
    QQmlProperty p(const_cast<QObject *>(object), name);
    return p.read();
}

/*!
  Return the \a name property value of \a object using the
  \l{QQmlContext} {context} \a ctxt.  This method is
  equivalent to:

  \code
    QQmlProperty p(object, name, context);
    p.read();
  \endcode
*/
QVariant QQmlProperty::read(const QObject *object, const QString &name, QQmlContext *ctxt)
{
    QQmlProperty p(const_cast<QObject *>(object), name, ctxt);
    return p.read();
}

/*!

  Return the \a name property value of \a object using the environment
  for instantiating QML components that is provided by \a engine. .
  This method is equivalent to:

  \code
    QQmlProperty p(object, name, engine);
    p.read();
  \endcode
*/
QVariant QQmlProperty::read(const QObject *object, const QString &name, QQmlEngine *engine)
{
    QQmlProperty p(const_cast<QObject *>(object), name, engine);
    return p.read();
}

QVariant QQmlPropertyPrivate::readValueProperty()
{
    auto doRead = [&](QQmlGadgetPtrWrapper *wrapper) {
        wrapper->read(object, core.coreIndex());
        return wrapper->property(valueTypeData.coreIndex()).read(wrapper);
    };

    if (isValueType()) {
        if (QQmlGadgetPtrWrapper *wrapper = QQmlGadgetPtrWrapper::instance(engine, core.propType()))
            return doRead(wrapper);
        if (QQmlValueType *valueType = QQmlValueTypeFactory::valueType(core.propType())) {
            QQmlGadgetPtrWrapper wrapper(valueType, nullptr);
            return doRead(&wrapper);
        }
        return QVariant();
    } else if (core.isQList()) {

        QQmlListProperty<QObject> prop;
        core.readProperty(object, &prop);
        return QVariant::fromValue(QQmlListReferencePrivate::init(prop, core.propType(), engine));

    } else if (core.isQObject()) {

        QObject *rv = nullptr;
        core.readProperty(object, &rv);
        return QVariant::fromValue(rv);

    } else {

        if (!core.propType()) // Unregistered type
            return object->metaObject()->property(core.coreIndex()).read(object);

        QVariant value;
        int status = -1;
        void *args[] = { nullptr, &value, &status };
        if (core.propType() == QMetaType::QVariant) {
            args[0] = &value;
        } else {
            value = QVariant(core.propType(), (void*)nullptr);
            args[0] = value.data();
        }
        core.readPropertyWithArgs(object, args);
        if (core.propType() != QMetaType::QVariant && args[0] != value.data())
            return QVariant((QVariant::Type)core.propType(), args[0]);

        return value;
    }
}

// helper function to allow assignment / binding to QList<QUrl> properties.
QVariant QQmlPropertyPrivate::resolvedUrlSequence(const QVariant &value, QQmlContextData *context)
{
    QList<QUrl> urls;
    if (value.userType() == qMetaTypeId<QUrl>()) {
        urls.append(value.toUrl());
    } else if (value.userType() == qMetaTypeId<QString>()) {
        urls.append(QUrl(value.toString()));
    } else if (value.userType() == qMetaTypeId<QByteArray>()) {
        urls.append(QUrl(QString::fromUtf8(value.toByteArray())));
    } else if (value.userType() == qMetaTypeId<QList<QUrl> >()) {
        urls = value.value<QList<QUrl> >();
    } else if (value.userType() == qMetaTypeId<QStringList>()) {
        QStringList urlStrings = value.value<QStringList>();
        const int urlStringsSize = urlStrings.size();
        urls.reserve(urlStringsSize);
        for (int i = 0; i < urlStringsSize; ++i)
            urls.append(QUrl(urlStrings.at(i)));
    } else if (value.userType() == qMetaTypeId<QList<QString> >()) {
        QList<QString> urlStrings = value.value<QList<QString> >();
        const int urlStringsSize = urlStrings.size();
        urls.reserve(urlStringsSize);
        for (int i = 0; i < urlStringsSize; ++i)
            urls.append(QUrl(urlStrings.at(i)));
    } // note: QList<QByteArray> is not currently supported.

    QList<QUrl> resolvedUrls;
    const int urlsSize = urls.size();
    resolvedUrls.reserve(urlsSize);
    for (int i = 0; i < urlsSize; ++i) {
        QUrl u = urls.at(i);
        if (context && u.isRelative() && !u.isEmpty())
            u = context->resolvedUrl(u);
        resolvedUrls.append(u);
    }

    return QVariant::fromValue<QList<QUrl> >(resolvedUrls);
}

//writeEnumProperty MIRRORS the relelvant bit of QMetaProperty::write AND MUST BE KEPT IN SYNC!
bool QQmlPropertyPrivate::writeEnumProperty(const QMetaProperty &prop, int idx, QObject *object, const QVariant &value, int flags)
{
    if (!object || !prop.isWritable())
        return false;

    QVariant v = value;
    if (prop.isEnumType()) {
        QMetaEnum menum = prop.enumerator();
        if (v.userType() == QMetaType::QString
#ifdef QT3_SUPPORT
            || v.userType() == QVariant::CString
#endif
            ) {
            bool ok;
            if (prop.isFlagType())
                v = QVariant(menum.keysToValue(value.toByteArray(), &ok));
            else
                v = QVariant(menum.keyToValue(value.toByteArray(), &ok));
            if (!ok)
                return false;
        } else if (v.userType() != QMetaType::Int && v.userType() != QMetaType::UInt) {
            int enumMetaTypeId = QMetaType::type(QByteArray(menum.scope() + QByteArray("::") + menum.name()));
            if ((enumMetaTypeId == QMetaType::UnknownType) || (v.userType() != enumMetaTypeId) || !v.constData())
                return false;
            v = QVariant(*reinterpret_cast<const int *>(v.constData()));
        }
        v.convert(QMetaType::Int);
    }

    // the status variable is changed by qt_metacall to indicate what it did
    // this feature is currently only used by QtDBus and should not be depended
    // upon. Don't change it without looking into QDBusAbstractInterface first
    // -1 (unchanged): normal qt_metacall, result stored in argv[0]
    // changed: result stored directly in value, return the value of status
    int status = -1;
    void *argv[] = { v.data(), &v, &status, &flags };
    QMetaObject::metacall(object, QMetaObject::WriteProperty, idx, argv);
    return status;
}

bool QQmlPropertyPrivate::writeValueProperty(const QVariant &value, QQmlPropertyData::WriteFlags flags)
{
    return writeValueProperty(object, core, valueTypeData, value, effectiveContext(), flags);
}

bool
QQmlPropertyPrivate::writeValueProperty(QObject *object,
                                        const QQmlPropertyData &core,
                                        const QQmlPropertyData &valueTypeData,
                                        const QVariant &value,
                                        QQmlContextData *context,QQmlPropertyData::WriteFlags flags)
{
    // Remove any existing bindings on this property
    if (!(flags & QQmlPropertyData::DontRemoveBinding) && object)
        removeBinding(object, encodedIndex(core, valueTypeData));

    bool rv = false;
    if (valueTypeData.isValid()) {
        auto doWrite = [&](QQmlGadgetPtrWrapper *wrapper) {
            wrapper->read(object, core.coreIndex());
            rv = write(wrapper, valueTypeData, value, context, flags);
            wrapper->write(object, core.coreIndex(), flags);
        };

        QQmlGadgetPtrWrapper *wrapper = context
                ? QQmlGadgetPtrWrapper::instance(context->engine, core.propType())
                : nullptr;
        if (wrapper) {
            doWrite(wrapper);
        } else if (QQmlValueType *valueType = QQmlValueTypeFactory::valueType(core.propType())) {
            QQmlGadgetPtrWrapper wrapper(valueType, nullptr);
            doWrite(&wrapper);
        }

    } else {
        rv = write(object, core, value, context, flags);
    }

    return rv;
}

bool QQmlPropertyPrivate::write(QObject *object,
                                        const QQmlPropertyData &property,
                                        const QVariant &value, QQmlContextData *context,
                                        QQmlPropertyData::WriteFlags flags)
{
    const int propertyType = property.propType();
    const int variantType = value.userType();

    if (property.isEnum()) {
        QMetaProperty prop = object->metaObject()->property(property.coreIndex());
        QVariant v = value;
        // Enum values come through the script engine as doubles
        if (variantType == QMetaType::Double) {
            double integral;
            double fractional = std::modf(value.toDouble(), &integral);
            if (qFuzzyIsNull(fractional))
                v.convert(QMetaType::Int);
        }
        return writeEnumProperty(prop, property.coreIndex(), object, v, flags);
    }

    QQmlEnginePrivate *enginePriv = QQmlEnginePrivate::get(context);
    const bool isUrl = propertyType == QMetaType::QUrl; // handled separately

    // The cases below are in approximate order of likelyhood:
    if (propertyType == variantType && !isUrl && propertyType != qMetaTypeId<QList<QUrl>>() && !property.isQList()) {
        return property.writeProperty(object, const_cast<void *>(value.constData()), flags);
    } else if (property.isQObject()) {
        QVariant val = value;
        int varType = variantType;
        if (variantType == QMetaType::Nullptr) {
            // This reflects the fact that you can assign a nullptr to a QObject pointer
            // Without the change to QObjectStar, rawMetaObjectForType would not give us a QQmlMetaObject
            varType = QMetaType::QObjectStar;
            val = QVariant(QMetaType::QObjectStar, nullptr);
        }
        QQmlMetaObject valMo = rawMetaObjectForType(enginePriv, varType);
        if (valMo.isNull())
            return false;
        QObject *o = *static_cast<QObject *const *>(val.constData());
        QQmlMetaObject propMo = rawMetaObjectForType(enginePriv, propertyType);

        if (o)
            valMo = o;

        if (QQmlMetaObject::canConvert(valMo, propMo)) {
            return property.writeProperty(object, &o, flags);
        } else if (!o && QQmlMetaObject::canConvert(propMo, valMo)) {
            // In the case of a null QObject, we assign the null if there is
            // any change that the null variant type could be up or down cast to
            // the property type.
            return property.writeProperty(object, &o, flags);
        } else {
            return false;
        }
    } else if (value.canConvert(propertyType) && !isUrl && variantType != QMetaType::QString && propertyType != qMetaTypeId<QList<QUrl>>() && !property.isQList()) {
        // common cases:
        switch (propertyType) {
        case QMetaType::Bool: {
            bool b = value.toBool();
            return property.writeProperty(object, &b, flags);
        }
        case QMetaType::Int: {
            int i = value.toInt();
            return property.writeProperty(object, &i, flags);
        }
        case QMetaType::Double: {
            double d = value.toDouble();
            return property.writeProperty(object, &d, flags);
        }
        case QMetaType::Float: {
            float f = value.toFloat();
            return property.writeProperty(object, &f, flags);
        }
        case QMetaType::QString: {
            QString s = value.toString();
            return property.writeProperty(object, &s, flags);
        }
        default: { // "fallback":
            QVariant v = value;
            v.convert(propertyType);
            return property.writeProperty(object, const_cast<void *>(v.constData()), flags);
        }
        }
    } else if (propertyType == qMetaTypeId<QVariant>()) {
        return property.writeProperty(object, const_cast<QVariant *>(&value), flags);
    } else if (isUrl) {
        QUrl u;
        if (variantType == QMetaType::QUrl) {
            u = value.toUrl();
        } else if (variantType == QMetaType::QByteArray) {
            QString input(QString::fromUtf8(value.toByteArray()));
            // Encoded dir-separators defeat QUrl processing - decode them first
            input.replace(QLatin1String("%2f"), QLatin1String("/"), Qt::CaseInsensitive);
            u = QUrl(input);
        } else if (variantType == QMetaType::QString) {
            QString input(value.toString());
            // Encoded dir-separators defeat QUrl processing - decode them first
            input.replace(QLatin1String("%2f"), QLatin1String("/"), Qt::CaseInsensitive);
            u = QUrl(input);
        } else {
            return false;
        }

        if (context && u.isRelative() && !u.isEmpty())
            u = context->resolvedUrl(u);
        return property.writeProperty(object, &u, flags);
    } else if (propertyType == qMetaTypeId<QList<QUrl>>()) {
        QList<QUrl> urlSeq = resolvedUrlSequence(value, context).value<QList<QUrl>>();
        return property.writeProperty(object, &urlSeq, flags);
    } else if (property.isQList()) {
        QQmlMetaObject listType;

        if (enginePriv) {
            listType = enginePriv->rawMetaObjectForType(enginePriv->listType(property.propType()));
        } else {
            QQmlType type = QQmlMetaType::qmlType(QQmlMetaType::listType(property.propType()));
            if (!type.isValid())
                return false;
            listType = type.baseMetaObject();
        }
        if (listType.isNull())
            return false;

        QQmlListProperty<void> prop;
        property.readProperty(object, &prop);

        if (!prop.clear)
            return false;

        prop.clear(&prop);

        if (variantType == qMetaTypeId<QQmlListReference>()) {
            QQmlListReference qdlr = value.value<QQmlListReference>();

            for (int ii = 0; ii < qdlr.count(); ++ii) {
                QObject *o = qdlr.at(ii);
                if (o && !QQmlMetaObject::canConvert(o, listType))
                    o = nullptr;
                prop.append(&prop, o);
            }
        } else if (variantType == qMetaTypeId<QList<QObject *> >()) {
            const QList<QObject *> &list = qvariant_cast<QList<QObject *> >(value);

            for (int ii = 0; ii < list.count(); ++ii) {
                QObject *o = list.at(ii);
                if (o && !QQmlMetaObject::canConvert(o, listType))
                    o = nullptr;
                prop.append(&prop, o);
            }
        } else {
            QObject *o = enginePriv?enginePriv->toQObject(value):QQmlMetaType::toQObject(value);
            if (o && !QQmlMetaObject::canConvert(o, listType))
                o = nullptr;
            prop.append(&prop, o);
        }
    } else {
        Q_ASSERT(variantType != propertyType);

        bool ok = false;
        QVariant v;
        if (variantType == QMetaType::QString)
            v = QQmlStringConverters::variantFromString(value.toString(), propertyType, &ok);

        if (!ok) {
            v = value;
            if (v.convert(propertyType)) {
                ok = true;
            } else if (v.isValid() && value.isNull()) {
                // For historical reasons converting a null QVariant to another type will do the trick
                // but return false anyway. This is caught with the above condition and considered a
                // successful conversion.
                Q_ASSERT(v.userType() == propertyType);
                ok = true;
            } else if (static_cast<uint>(propertyType) >= QMetaType::User &&
                       variantType == QMetaType::QString) {
                QQmlMetaType::StringConverter con = QQmlMetaType::customStringConverter(propertyType);
                if (con) {
                    v = con(value.toString());
                    if (v.userType() == propertyType)
                        ok = true;
                }
            }
        }
        if (!ok) {
            // the only other options are that they are assigning a single value
            // to a sequence type property (eg, an int to a QList<int> property).
            // or that we encountered an interface type
            // Note that we've already handled single-value assignment to QList<QUrl> properties.
            if (variantType == QMetaType::Int && propertyType == qMetaTypeId<QList<int> >()) {
                QList<int> list;
                list << value.toInt();
                v = QVariant::fromValue<QList<int> >(list);
                ok = true;
            } else if ((variantType == QMetaType::Double || variantType == QMetaType::Int)
                       && (propertyType == qMetaTypeId<QList<qreal> >())) {
                QList<qreal> list;
                list << value.toReal();
                v = QVariant::fromValue<QList<qreal> >(list);
                ok = true;
            } else if (variantType == QMetaType::Bool && propertyType == qMetaTypeId<QList<bool> >()) {
                QList<bool> list;
                list << value.toBool();
                v = QVariant::fromValue<QList<bool> >(list);
                ok = true;
            } else if (variantType == QMetaType::QString && propertyType == qMetaTypeId<QList<QString> >()) {
                QList<QString> list;
                list << value.toString();
                v = QVariant::fromValue<QList<QString> >(list);
                ok = true;
            } else if (variantType == QMetaType::QString && propertyType == qMetaTypeId<QStringList>()) {
                QStringList list;
                list << value.toString();
                v = QVariant::fromValue<QStringList>(list);
                ok = true;
            }
        }

        if (!ok && QQmlMetaType::isInterface(propertyType)) {
            auto valueAsQObject = qvariant_cast<QObject *>(value);
            if (valueAsQObject && valueAsQObject->qt_metacast(QQmlMetaType::interfaceIId(propertyType))) {
                // this case can occur when object has an interface type
                // and the variant contains a type implementing the interface
                return property.writeProperty(object, const_cast<void *>(value.constData()), flags);
            }
        }

        if (ok) {
            return property.writeProperty(object, const_cast<void *>(v.constData()), flags);
        } else {
            return false;
        }
    }

    return true;
}

QQmlMetaObject QQmlPropertyPrivate::rawMetaObjectForType(QQmlEnginePrivate *engine, int userType)
{
    QMetaType metaType(userType);
    if ((metaType.flags() & QMetaType::PointerToQObject) && metaType.metaObject())
        return metaType.metaObject();
    if (engine)
        return engine->rawMetaObjectForType(userType);
    QQmlType type = QQmlMetaType::qmlType(userType);
    if (type.isValid())
        return QQmlMetaObject(type.baseMetaObject());
    return QQmlMetaObject();
}

/*!
    Sets the property value to \a value. Returns \c true on success, or
    \c false if the property can't be set because the \a value is the
    wrong type, for example.
 */
bool QQmlProperty::write(const QVariant &value) const
{
    return QQmlPropertyPrivate::write(*this, value, {});
}

/*!
  Writes \a value to the \a name property of \a object.  This method
  is equivalent to:

  \code
    QQmlProperty p(object, name);
    p.write(value);
  \endcode

  Returns \c true on success, \c false otherwise.
*/
bool QQmlProperty::write(QObject *object, const QString &name, const QVariant &value)
{
    QQmlProperty p(object, name);
    return p.write(value);
}

/*!
  Writes \a value to the \a name property of \a object using the
  \l{QQmlContext} {context} \a ctxt.  This method is
  equivalent to:

  \code
    QQmlProperty p(object, name, ctxt);
    p.write(value);
  \endcode

  Returns \c true on success, \c false otherwise.
*/
bool QQmlProperty::write(QObject *object,
                                 const QString &name,
                                 const QVariant &value,
                                 QQmlContext *ctxt)
{
    QQmlProperty p(object, name, ctxt);
    return p.write(value);
}

/*!

  Writes \a value to the \a name property of \a object using the
  environment for instantiating QML components that is provided by
  \a engine.  This method is equivalent to:

  \code
    QQmlProperty p(object, name, engine);
    p.write(value);
  \endcode

  Returns \c true on success, \c false otherwise.
*/
bool QQmlProperty::write(QObject *object, const QString &name, const QVariant &value,
                                 QQmlEngine *engine)
{
    QQmlProperty p(object, name, engine);
    return p.write(value);
}

/*!
    Resets the property and returns true if the property is
    resettable.  If the property is not resettable, nothing happens
    and false is returned.
*/
bool QQmlProperty::reset() const
{
    if (isResettable()) {
        void *args[] = { nullptr };
        QMetaObject::metacall(d->object, QMetaObject::ResetProperty, d->core.coreIndex(), args);
        return true;
    } else {
        return false;
    }
}

bool QQmlPropertyPrivate::write(const QQmlProperty &that,
                                const QVariant &value, QQmlPropertyData::WriteFlags flags)
{
    if (!that.d)
        return false;
    if (that.d->object && that.type() & QQmlProperty::Property &&
        that.d->core.isValid() && that.isWritable())
        return that.d->writeValueProperty(value, flags);
    else
        return false;
}

/*!
    Returns true if the property has a change notifier signal, otherwise false.
*/
bool QQmlProperty::hasNotifySignal() const
{
    if (type() & Property && d->object) {
        return d->object->metaObject()->property(d->core.coreIndex()).hasNotifySignal();
    }
    return false;
}

/*!
    Returns true if the property needs a change notifier signal for bindings
    to remain upto date, false otherwise.

    Some properties, such as attached properties or those whose value never
    changes, do not require a change notifier.
*/
bool QQmlProperty::needsNotifySignal() const
{
    return type() & Property && !property().isConstant();
}

/*!
    Connects the property's change notifier signal to the
    specified \a method of the \a dest object and returns
    true. Returns false if this metaproperty does not
    represent a regular Qt property or if it has no
    change notifier signal, or if the \a dest object does
    not have the specified \a method.
*/
bool QQmlProperty::connectNotifySignal(QObject *dest, int method) const
{
    if (!(type() & Property) || !d->object)
        return false;

    QMetaProperty prop = d->object->metaObject()->property(d->core.coreIndex());
    if (prop.hasNotifySignal()) {
        return QQmlPropertyPrivate::connect(d->object, prop.notifySignalIndex(), dest, method, Qt::DirectConnection);
    } else {
        return false;
    }
}

/*!
    Connects the property's change notifier signal to the
    specified \a slot of the \a dest object and returns
    true. Returns false if this metaproperty does not
    represent a regular Qt property or if it has no
    change notifier signal, or if the \a dest object does
    not have the specified \a slot.

    \note \a slot should be passed using the SLOT() macro so it is
    correctly identified.
*/
bool QQmlProperty::connectNotifySignal(QObject *dest, const char *slot) const
{
    if (!(type() & Property) || !d->object)
        return false;

    QMetaProperty prop = d->object->metaObject()->property(d->core.coreIndex());
    if (prop.hasNotifySignal()) {
        QByteArray signal('2' + prop.notifySignal().methodSignature());
        return QObject::connect(d->object, signal.constData(), dest, slot);
    } else  {
        return false;
    }
}

/*!
    Return the Qt metaobject index of the property.
*/
int QQmlProperty::index() const
{
    return d ? d->core.coreIndex() : -1;
}

QQmlPropertyIndex QQmlPropertyPrivate::propertyIndex(const QQmlProperty &that)
{
    return that.d ? that.d->encodedIndex() : QQmlPropertyIndex();
}

QQmlProperty
QQmlPropertyPrivate::restore(QObject *object, const QQmlPropertyData &data,
                             const QQmlPropertyData *valueTypeData, QQmlContextData *ctxt)
{
    QQmlProperty prop;

    prop.d = new QQmlPropertyPrivate;
    prop.d->object = object;
    prop.d->context = ctxt;
    prop.d->engine = ctxt ? ctxt->engine : nullptr;

    prop.d->core = data;
    if (valueTypeData)
        prop.d->valueTypeData = *valueTypeData;

    return prop;
}

/*!
    Return the signal corresponding to \a name
*/
QMetaMethod QQmlPropertyPrivate::findSignalByName(const QMetaObject *mo, const QByteArray &name)
{
    Q_ASSERT(mo);
    int methods = mo->methodCount();
    for (int ii = methods - 1; ii >= 2; --ii) { // >= 2 to block the destroyed signal
        QMetaMethod method = mo->method(ii);

        if (method.name() == name && (method.methodType() & QMetaMethod::Signal))
            return method;
    }

    // If no signal is found, but the signal is of the form "onBlahChanged",
    // return the notify signal for the property "Blah"
    if (name.endsWith("Changed")) {
        QByteArray propName = name.mid(0, name.length() - 7);
        int propIdx = mo->indexOfProperty(propName.constData());
        if (propIdx >= 0) {
            QMetaProperty prop = mo->property(propIdx);
            if (prop.hasNotifySignal())
                return prop.notifySignal();
        }
    }

    return QMetaMethod();
}

/*! \internal
    If \a indexInSignalRange is true, \a index is treated as a signal index
    (see QObjectPrivate::signalIndex()), otherwise it is treated as a
    method index (QMetaMethod::methodIndex()).
*/
static inline void flush_vme_signal(const QObject *object, int index, bool indexInSignalRange)
{
    QQmlData *data = QQmlData::get(object);
    if (data && data->propertyCache) {
        QQmlPropertyData *property = indexInSignalRange ? data->propertyCache->signal(index)
                                                        : data->propertyCache->method(index);

        if (property && property->isVMESignal()) {
            QQmlVMEMetaObject *vme;
            if (indexInSignalRange)
                vme = QQmlVMEMetaObject::getForSignal(const_cast<QObject *>(object), index);
            else
                vme = QQmlVMEMetaObject::getForMethod(const_cast<QObject *>(object), index);
            vme->connectAliasSignal(index, indexInSignalRange);
        }
    }
}

/*!
Connect \a sender \a signal_index to \a receiver \a method_index with the specified
\a type and \a types.  This behaves identically to QMetaObject::connect() except that
it connects any lazy "proxy" signal connections set up by QML.

It is possible that this logic should be moved to QMetaObject::connect().
*/
bool QQmlPropertyPrivate::connect(const QObject *sender, int signal_index,
                                          const QObject *receiver, int method_index,
                                          int type, int *types)
{
    static const bool indexInSignalRange = false;
    flush_vme_signal(sender, signal_index, indexInSignalRange);
    flush_vme_signal(receiver, method_index, indexInSignalRange);

    return QMetaObject::connect(sender, signal_index, receiver, method_index, type, types);
}

/*! \internal
    \a signal_index MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
void QQmlPropertyPrivate::flushSignal(const QObject *sender, int signal_index)
{
    static const bool indexInSignalRange = true;
    flush_vme_signal(sender, signal_index, indexInSignalRange);
}

QT_END_NAMESPACE
