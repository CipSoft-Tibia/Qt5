// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/private/qqmlopenmetaobject_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickpincharea_p.h>

#ifdef QT_WIDGETS_LIB
#include <QApplication>
#endif // QT_WIDGETS_LIB

#include <QtGui/QGuiApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/private/qobject_p.h>
#include <QtCore/private/qmetaobject_p.h>
#include <QtQmlTypeRegistrar/private/qqmljsstreamwriter_p.h>

#include <QRegularExpression>
#include <iostream>
#include <algorithm>

#include "qmltypereader.h"

#ifdef QT_SIMULATOR
#include <QtGui/private/qsimulatorconnection_p.h>
#endif

#ifdef Q_OS_WIN
#  if !defined(Q_CC_MINGW)
#    include <crtdbg.h>
#  endif
#include <qt_windows.h>
#endif

namespace {

const uint qtQmlMajorVersion = 2;
const uint qtQmlMinorVersion = 0;
const uint qtQuickMajorVersion = 2;
const uint qtQuickMinorVersion = 0;

const QString qtQuickQualifiedName = QString::fromLatin1("QtQuick %1.%2")
        .arg(qtQuickMajorVersion)
        .arg(qtQuickMinorVersion);

QString pluginImportPath;
bool verbose = false;
bool creatable = true;

QString currentProperty;
QString inObjectInstantiation;

}

static QString enquote(const QString &string)
{
    QString s = string;
    return QString("\"%1\"").arg(s.replace(QLatin1Char('\\'), QLatin1String("\\\\"))
                                 .replace(QLatin1Char('"'),QLatin1String("\\\"")));
}

struct QmlVersionInfo
{
    QString pluginImportUri;
    QTypeRevision version;
    bool strict;
};

static bool matchingImportUri(const QQmlType &ty, const QmlVersionInfo& versionInfo) {
    const QString &module = ty.module();
    if (versionInfo.strict) {
        return (versionInfo.pluginImportUri == module
                && (ty.version().majorVersion() == versionInfo.version.majorVersion()
                    || !ty.version().hasMajorVersion()))
                || module.isEmpty();
    }
    return module.isEmpty()
            || versionInfo.pluginImportUri == module
            || module.startsWith(versionInfo.pluginImportUri + QLatin1Char('.'));
}

void collectReachableMetaObjects(const QMetaObject *meta, QSet<const QMetaObject *> *metas,  const QmlVersionInfo &info, bool extended = false, bool alreadyChangedModule = false)
{
    auto ty = QQmlMetaType::qmlType(meta);
    if (! meta || metas->contains(meta))
        return;

    if (matchingImportUri(ty, info)) {
        if (!alreadyChangedModule) {
            // dynamic meta objects can break things badly
            // but extended types are usually fine
            const QMetaObjectPrivate *mop = reinterpret_cast<const QMetaObjectPrivate *>(meta->d.data);
            if (extended || !(mop->flags & DynamicMetaObject))
                metas->insert(meta);
        } else if (!ty.module().isEmpty()) { // empty module (e.g. from an attached property) would cause a (false) match; do not warn about them
            qWarning() << "Circular module dependency cannot be expressed in plugin.qmltypes file"
                       << "Object was:" << meta->className()
                       << ty.module() << info.pluginImportUri;
        }
    } else if (!ty.module().isEmpty()) {
        alreadyChangedModule = true;
    }

    collectReachableMetaObjects(meta->superClass(), metas, info, /*extended=*/ false, alreadyChangedModule);
}

void collectReachableMetaObjects(QObject *object, QSet<const QMetaObject *> *metas, const QmlVersionInfo &info)
{
    if (! object)
        return;

    const QMetaObject *meta = object->metaObject();
    if (verbose)
        std::cerr << "Processing object " << qPrintable( meta->className() ) << std::endl;
    collectReachableMetaObjects(meta, metas, info);

    for (int index = 0; index < meta->propertyCount(); ++index) {
        QMetaProperty prop = meta->property(index);
        if (prop.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
            if (verbose)
                std::cerr << "  Processing property " << qPrintable( prop.name() ) << std::endl;
            currentProperty = QString("%1::%2").arg(meta->className(), prop.name());

            // if the property was not initialized during construction,
            // accessing a member of oo is going to cause a segmentation fault
            QObject *oo = QQmlMetaType::toQObject(prop.read(object));
            if (oo && !metas->contains(oo->metaObject()))
                collectReachableMetaObjects(oo, metas, info);
            currentProperty.clear();
        }
    }
}

void collectReachableMetaObjects(QQmlEnginePrivate *engine, const QQmlType &ty, QSet<const QMetaObject *> *metas,  const QmlVersionInfo& info)
{
    collectReachableMetaObjects(ty.baseMetaObject(), metas, info, ty.isExtendedType());
    if (ty.attachedPropertiesType(engine) && matchingImportUri(ty, info)) {
        collectReachableMetaObjects(ty.attachedPropertiesType(engine), metas, info);
    }
}

/* When we dump a QMetaObject, we want to list all the types it is exported as.
   To do this, we need to find the QQmlTypes associated with this
   QMetaObject.
*/
static QHash<QByteArray, QSet<QQmlType> > qmlTypesByCppName;

static QHash<QByteArray, QByteArray> cppToId;

/* Takes a C++ type name, such as Qt::LayoutDirection or QString and
   maps it to how it should appear in the description file.

   These names need to be unique globally, so we don't change the C++ symbol's
   name much. It is mostly used to for explicit translations such as
   QString->string and translations for extended QML objects.
*/
QByteArray convertToId(const QByteArray &cppName)
{
    return cppToId.value(cppName, cppName);
}

QByteArray convertToId(const QMetaObject *mo)
{
    QByteArray className(mo->className());
    if (!className.isEmpty())
        return convertToId(className);

    // likely a metaobject generated for an extended qml object
    if (mo->superClass()) {
        className = convertToId(mo->superClass());
        className.append("_extended");
        return className;
    }

    static QHash<const QMetaObject *, QByteArray> generatedNames;
    className = generatedNames.value(mo);
    if (!className.isEmpty())
        return className;

    std::cerr << "Found a QMetaObject without a className, generating a random name" << std::endl;
    className = QByteArray("error-unknown-name-");
    className.append(QByteArray::number(generatedNames.size()));
    generatedNames.insert(mo, className);
    return className;
}


// Collect all metaobjects for types registered with qmlRegisterType() without parameters
void collectReachableMetaObjectsWithoutQmlName(QQmlEnginePrivate *engine, QSet<const QMetaObject *>& metas,
                                               QMap<QString, QList<QQmlType>> &compositeTypes, const QmlVersionInfo &info) {
    const auto qmlAllTypes = QQmlMetaType::qmlAllTypes();
    for (const QQmlType &ty : qmlAllTypes) {
        if (!metas.contains(ty.baseMetaObject())) {
            if (!ty.isComposite()) {
                collectReachableMetaObjects(engine, ty, &metas, info);
            } else if (matchingImportUri(ty, info)) {
                compositeTypes[ty.elementName()].append(ty);
            }
       }
    }
}

QSet<const QMetaObject *> collectReachableMetaObjects(QQmlEngine *engine,
                                                      QSet<const QMetaObject *> &noncreatables,
                                                      QSet<const QMetaObject *> &singletons,
                                                      QMap<QString, QList<QQmlType>> &compositeTypes,
                                                      const QmlVersionInfo &info,
                                                      const QList<QQmlType> &skip = QList<QQmlType>()
                                                      )
{
    QSet<const QMetaObject *> metas;
    metas.insert(&Qt::staticMetaObject);

    const auto qmlTypes = QQmlMetaType::qmlTypes();
    for (const QQmlType &ty : qmlTypes) {
        if (!matchingImportUri(ty,info))
            continue;
        if (!ty.isCreatable())
            noncreatables.insert(ty.baseMetaObject());
        if (ty.isSingleton())
            singletons.insert(ty.baseMetaObject());
        if (!ty.isComposite()) {
            if (const QMetaObject *mo = ty.baseMetaObject())
                qmlTypesByCppName[mo->className()].insert(ty);
            collectReachableMetaObjects(QQmlEnginePrivate::get(engine), ty, &metas, info);
        } else {
            compositeTypes[ty.elementName()].append(ty);
        }
    }

    if (creatable) {
        // find even more QMetaObjects by instantiating QML types and running
        // over the instances
        for (const QQmlType &ty : qmlTypes) {
            if (!matchingImportUri(ty, info))
                continue;
            if (skip.contains(ty))
                continue;
            if (ty.isExtendedType())
                continue;
            if (!ty.isCreatable())
                continue;
            if (ty.typeName() == "QQmlComponent")
                continue;

            QString tyName = ty.qmlTypeName();
            tyName = tyName.mid(tyName.lastIndexOf(QLatin1Char('/')) + 1);
            if (tyName.isEmpty())
                continue;

            inObjectInstantiation = tyName;
            QObject *object = nullptr;

            if (ty.isSingleton()) {
                QQmlType::SingletonInstanceInfo *siinfo = ty.singletonInstanceInfo();
                if (!siinfo) {
                    std::cerr << "Internal error, " << qPrintable(tyName)
                              << "(" << qPrintable( QString::fromUtf8(ty.typeName()) ) << ")"
                              << " is singleton, but has no singletonInstanceInfo" << std::endl;
                    continue;
                }
                if (ty.isQObjectSingleton()) {
                    if (verbose)
                        std::cerr << "Trying to get singleton for " << qPrintable(tyName)
                                  << " (" << qPrintable( siinfo->typeName )  << ")" << std::endl;
                    collectReachableMetaObjects(object, &metas, info);
                    object = QQmlEnginePrivate::get(engine)->singletonInstance<QObject*>(ty);
                } else {
                    inObjectInstantiation.clear();
                    continue; // we don't handle QJSValue singleton types.
                }
            } else {
                if (verbose)
                    std::cerr << "Trying to create object " << qPrintable( tyName )
                              << " (" << qPrintable( QString::fromUtf8(ty.typeName()) )  << ")" << std::endl;
                object = ty.create();
            }

            inObjectInstantiation.clear();

            if (object) {
                if (verbose)
                    std::cerr << "Got " << qPrintable( tyName )
                              << " (" << qPrintable( QString::fromUtf8(ty.typeName()) ) << ")" << std::endl;
                collectReachableMetaObjects(object, &metas, info);
                object->deleteLater();
            } else {
                std::cerr << "Could not create " << qPrintable(tyName) << std::endl;
            }
        }
    }

    collectReachableMetaObjectsWithoutQmlName(QQmlEnginePrivate::get(engine), metas, compositeTypes, info);

    return metas;
}

class KnownAttributes {
    QHash<QByteArray, QTypeRevision> m_properties;
    QHash<QByteArray, QHash<int, QTypeRevision> > m_methods;
public:
    bool knownMethod(const QByteArray &name, int nArgs, QTypeRevision revision)
    {
        if (m_methods.contains(name)) {
            QHash<int, QTypeRevision> overloads = m_methods.value(name);
            if (overloads.contains(nArgs) && overloads.value(nArgs).toEncodedVersion<quint16>() <= revision.toEncodedVersion<quint16>())
                return true;
        }
        m_methods[name][nArgs] = revision;
        return false;
    }

    bool knownProperty(const QByteArray &name, QTypeRevision revision)
    {
        if (m_properties.contains(name) && m_properties.value(name).toEncodedVersion<quint16>() <= revision.toEncodedVersion<quint16>())
            return true;
        m_properties[name] = revision;
        return false;
    }
};

class Dumper
{
    QQmlJSStreamWriter *qml;
    QString relocatableModuleUri;

public:
    Dumper(QQmlJSStreamWriter *qml) : qml(qml) {}

    void setRelocatableModuleUri(const QString &uri)
    {
        relocatableModuleUri = uri;
    }

    QString getExportString(const QQmlType &type, const QmlVersionInfo &versionInfo)
    {
        const QString module = type.module().isEmpty() ? versionInfo.pluginImportUri
                                                       : type.module();
        QTypeRevision version = QTypeRevision::fromVersion(
                    type.version().hasMajorVersion() ? type.version().majorVersion()
                                                     : versionInfo.version.majorVersion(),
                    type.version().hasMinorVersion() ? type.version().minorVersion()
                                                     : versionInfo.version.minorVersion());

        const QString versionedElement = type.elementName()
                + QString::fromLatin1(" %1.%2").arg(version.majorVersion()).arg(version.minorVersion());

        return enquote((module == relocatableModuleUri)
                       ? versionedElement
                       : module + QLatin1Char('/') + versionedElement);
    }

    void writeMetaContent(const QMetaObject *meta, KnownAttributes *knownAttributes = nullptr)
    {
        QSet<QString> implicitSignals = dumpMetaProperties(meta, QTypeRevision::zero(), knownAttributes);

        if (meta == &QObject::staticMetaObject) {
            // for QObject, hide deleteLater() and onDestroyed
            for (int index = meta->methodOffset(); index < meta->methodCount(); ++index) {
                QMetaMethod method = meta->method(index);
                QByteArray signature = method.methodSignature();
                if (signature == QByteArrayLiteral("destroyed(QObject*)")
                        || signature == QByteArrayLiteral("destroyed()")
                        || signature == QByteArrayLiteral("deleteLater()"))
                    continue;
                dump(method, implicitSignals, knownAttributes);
            }

            // and add toString(), destroy() and destroy(int)
            if (!knownAttributes || !knownAttributes->knownMethod(QByteArray("toString"), 0, QTypeRevision::zero())) {
                qml->writeStartObject(QLatin1String("Method"));
                qml->writeScriptBinding(QLatin1String("name"), enquote(QLatin1String("toString")));
                qml->writeEndObject();
            }
            if (!knownAttributes || !knownAttributes->knownMethod(QByteArray("destroy"), 0, QTypeRevision::zero())) {
                qml->writeStartObject(QLatin1String("Method"));
                qml->writeScriptBinding(QLatin1String("name"), enquote(QLatin1String("destroy")));
                qml->writeEndObject();
            }
            if (!knownAttributes || !knownAttributes->knownMethod(QByteArray("destroy"), 1, QTypeRevision::zero())) {
                qml->writeStartObject(QLatin1String("Method"));
                qml->writeScriptBinding(QLatin1String("name"), enquote(QLatin1String("destroy")));
                qml->writeStartObject(QLatin1String("Parameter"));
                qml->writeScriptBinding(QLatin1String("name"), enquote(QLatin1String("delay")));
                qml->writeScriptBinding(QLatin1String("type"), enquote(QLatin1String("int")));
                qml->writeEndObject();
                qml->writeEndObject();
            }
        } else {
            for (int index = meta->methodOffset(); index < meta->methodCount(); ++index)
                dump(meta->method(index), implicitSignals, knownAttributes);
        }
    }

    QString getPrototypeNameForCompositeType(
            const QMetaObject *metaObject, QList<const QMetaObject *> *objectsToMerge,
            const QmlVersionInfo &versionInfo)
    {
        auto ty = QQmlMetaType::qmlType(metaObject);
        QString prototypeName;
        if (matchingImportUri(ty, versionInfo)) {
            // dynamic meta objects can break things badly
            // but extended types are usually fine
            const QMetaObjectPrivate *mop = reinterpret_cast<const QMetaObjectPrivate *>(metaObject->d.data);
            if (!(mop->flags & DynamicMetaObject) && objectsToMerge
                    && !objectsToMerge->contains(metaObject))
                objectsToMerge->append(metaObject);
            const QMetaObject *superMetaObject = metaObject->superClass();
            if (!superMetaObject) {
                prototypeName = "QObject";
            } else {
                QQmlType superType = QQmlMetaType::qmlType(superMetaObject);
                if (superType.isValid() && !superType.isComposite())
                    return convertToId(superMetaObject->className());
                prototypeName = getPrototypeNameForCompositeType(
                            superMetaObject, objectsToMerge, versionInfo);
            }
        } else {
            prototypeName = convertToId(metaObject->className());
        }
        return prototypeName;
    }

    void dumpComposite(QQmlEngine *engine, const QList<QQmlType> &compositeType, const QmlVersionInfo &versionInfo)
    {
        for (const QQmlType &type : compositeType)
            dumpCompositeItem(engine, type, versionInfo);
    }

    void dumpCompositeItem(QQmlEngine *engine, const QQmlType &compositeType, const QmlVersionInfo &versionInfo)
    {
        QQmlComponent e(engine, compositeType.sourceUrl());
        if (!e.isReady()) {
            std::cerr << "WARNING: skipping module " << compositeType.elementName().toStdString()
                      << std::endl << e.errorString().toStdString() << std::endl;
            return;
        }

        QObject *object = e.create();

        if (!object)
            return;

        qml->writeStartObject("Component");

        const QMetaObject *mainMeta = object->metaObject();

        QList<const QMetaObject *> objectsToMerge;
        KnownAttributes knownAttributes;
        // Get C++ base class name for the composite type
        QString prototypeName = getPrototypeNameForCompositeType(mainMeta, &objectsToMerge,
                                                                 versionInfo);
        qml->writeScriptBinding(QLatin1String("prototype"), enquote(prototypeName));

        QString qmlTyName = compositeType.qmlTypeName();
        const QString exportString = getExportString(compositeType, versionInfo);

        // TODO: why don't we simply output the compositeType.elementName() here?
        //       That would make more sense, but it would change the format quite a bit.
        qml->writeScriptBinding(QLatin1String("name"), exportString);

        qml->writeArrayBinding(QLatin1String("exports"), QStringList() << exportString);

        // TODO: shouldn't this be metaObjectRevision().value<quint16>()
        //       rather than version().minorVersion()
        qml->writeArrayBinding(QLatin1String("exportMetaObjectRevisions"), QStringList()
                               << QString::number(compositeType.version().minorVersion()));

        qml->writeBooleanBinding(QLatin1String("isComposite"), true);

        if (compositeType.isSingleton()) {
            qml->writeBooleanBinding(QLatin1String("isCreatable"), false);
            qml->writeBooleanBinding(QLatin1String("isSingleton"), true);
        }

        for (int index = mainMeta->classInfoCount() - 1 ; index >= 0 ; --index) {
            QMetaClassInfo classInfo = mainMeta->classInfo(index);
            if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
                qml->writeScriptBinding(QLatin1String("defaultProperty"), enquote(QLatin1String(classInfo.value())));
                break;
            }
        }

        for (const QMetaObject *meta : std::as_const(objectsToMerge)) {
            for (int index = meta->enumeratorOffset(); index < meta->enumeratorCount(); ++index)
                dump(meta->enumerator(index));

            writeMetaContent(meta, &knownAttributes);
        }

        qml->writeEndObject();
    }

    QString getDefaultProperty(const QMetaObject *meta)
    {
        for (int index = meta->classInfoCount() - 1; index >= 0; --index) {
            QMetaClassInfo classInfo = meta->classInfo(index);
            if (QLatin1String(classInfo.name()) == QLatin1String("DefaultProperty")) {
                return QLatin1String(classInfo.value());
            }
        }
        return QString();
    }

    struct QmlTypeInfo {
        QmlTypeInfo() {}
        QmlTypeInfo(const QString &exportString, QTypeRevision revision, const QMetaObject *extendedObject, QByteArray attachedTypeId)
            : exportString(exportString), revision(revision), extendedObject(extendedObject), attachedTypeId(attachedTypeId) {}
        QString exportString;
        QTypeRevision revision = QTypeRevision::zero();
        const QMetaObject *extendedObject = nullptr;
        QByteArray attachedTypeId;
    };

    void dump(QQmlEnginePrivate *engine, const QMetaObject *meta, bool isUncreatable, bool isSingleton)
    {
        qml->writeStartObject("Component");

        QByteArray id = convertToId(meta);
        qml->writeScriptBinding(QLatin1String("name"), enquote(id));

        // collect type information
        QVector<QmlTypeInfo> typeInfo;
        for (QQmlType type : qmlTypesByCppName.value(meta->className())) {
            const QMetaObject *extendedObject = type.extensionFunction() ? type.metaObject() : nullptr;
            QByteArray attachedTypeId;
            if (const QMetaObject *attachedType = type.attachedPropertiesType(engine)) {
                // Can happen when a type is registered that returns itself as attachedPropertiesType()
                // because there is no creatable type to attach to.
                if (attachedType != meta)
                    attachedTypeId = convertToId(attachedType);
            }
            const QString exportString = getExportString(type, { QString(), QTypeRevision(), false });
            QTypeRevision metaObjectRevision = type.metaObjectRevision();
            if (extendedObject) {
                // emulate custom metaobjectrevision out of import
                metaObjectRevision = type.version();
            }

            QmlTypeInfo info = { exportString, metaObjectRevision, extendedObject, attachedTypeId };
            typeInfo.append(info);
        }

        // sort to ensure stable output
        std::sort(typeInfo.begin(), typeInfo.end(), [](const QmlTypeInfo &i1, const QmlTypeInfo &i2) {
            return i1.revision.toEncodedVersion<quint16>() < i2.revision.toEncodedVersion<quint16>();
        });

        // determine default property
        // TODO: support revisioning of default property
        QString defaultProperty = getDefaultProperty(meta);
        if (defaultProperty.isEmpty()) {
            for (const QmlTypeInfo &iter : typeInfo) {
                if (iter.extendedObject) {
                    defaultProperty = getDefaultProperty(iter.extendedObject);
                    if (!defaultProperty.isEmpty())
                        break;
                }
            }
        }
        if (!defaultProperty.isEmpty())
            qml->writeScriptBinding(QLatin1String("defaultProperty"), enquote(defaultProperty));

        if (meta->superClass())
            qml->writeScriptBinding(QLatin1String("prototype"), enquote(convertToId(meta->superClass())));

        if (!typeInfo.isEmpty()) {
            QMap<QString, QString> exports; // sort exports
            for (const QmlTypeInfo &iter : typeInfo)
                exports.insert(iter.exportString, QString::number(iter.revision.toEncodedVersion<quint16>()));

            QStringList exportStrings = exports.keys();
            QStringList metaObjectRevisions = exports.values();
            qml->writeArrayBinding(QLatin1String("exports"), exportStrings);

            if (isUncreatable)
                qml->writeBooleanBinding(QLatin1String("isCreatable"), false);

            if (isSingleton)
                qml->writeBooleanBinding(QLatin1String("isSingleton"), true);

            qml->writeArrayBinding(QLatin1String("exportMetaObjectRevisions"), metaObjectRevisions);

            for (const QmlTypeInfo &iter : typeInfo) {
                if (!iter.attachedTypeId.isEmpty()) {
                    qml->writeScriptBinding(QLatin1String("attachedType"), enquote(iter.attachedTypeId));
                    break;
                }
            }
        }

        for (int index = meta->enumeratorOffset(); index < meta->enumeratorCount(); ++index)
            dump(meta->enumerator(index));

        writeMetaContent(meta);

        // dump properties from extended metaobjects last
        for (auto iter : typeInfo) {
            if (iter.extendedObject)
                dumpMetaProperties(iter.extendedObject, iter.revision);
        }

        qml->writeEndObject();
    }

private:

    /* Removes pointer and list annotations from a type name, returning
       what was removed in isList and isPointer
    */
    static void removePointerAndList(QByteArray *typeName, bool *isList, bool *isPointer)
    {
        static QByteArray declListPrefix = "QQmlListProperty<";

        if (typeName->endsWith('*')) {
            *isPointer = true;
            typeName->truncate(typeName->size() - 1);
            removePointerAndList(typeName, isList, isPointer);
        } else if (typeName->startsWith(declListPrefix)) {
            *isList = true;
            typeName->truncate(typeName->size() - 1); // get rid of the suffix '>'
            *typeName = typeName->mid(declListPrefix.size());
            removePointerAndList(typeName, isList, isPointer);
        }

        *typeName = convertToId(*typeName);
    }

    void writeTypeProperties(QByteArray typeName, bool isWritable)
    {
        bool isList = false, isPointer = false;
        removePointerAndList(&typeName, &isList, &isPointer);

        qml->writeScriptBinding(QLatin1String("type"), enquote(typeName));
        if (isList)
            qml->writeScriptBinding(QLatin1String("isList"), QLatin1String("true"));
        if (!isWritable)
            qml->writeScriptBinding(QLatin1String("isReadonly"), QLatin1String("true"));
        if (isPointer)
            qml->writeScriptBinding(QLatin1String("isPointer"), QLatin1String("true"));
    }

    void dump(const QMetaProperty &prop, QTypeRevision metaRevision = QTypeRevision(),
              KnownAttributes *knownAttributes = nullptr)
    {
        // TODO: should that not be metaRevision.isValid() rather than comparing to zero()?
        QTypeRevision revision = (metaRevision == QTypeRevision::zero())
                ? QTypeRevision::fromEncodedVersion(prop.revision())
                : metaRevision;
        QByteArray propName = prop.name();
        if (knownAttributes && knownAttributes->knownProperty(propName, revision))
            return;
        qml->writeStartObject("Property");
        qml->writeScriptBinding(QLatin1String("name"), enquote(QString::fromUtf8(prop.name())));
        if (revision != QTypeRevision::zero())
            qml->writeScriptBinding(QLatin1String("revision"), QString::number(revision.toEncodedVersion<quint16>()));
        writeTypeProperties(prop.typeName(), prop.isWritable());

        qml->writeEndObject();
    }

    QSet<QString> dumpMetaProperties(const QMetaObject *meta, QTypeRevision metaRevision = QTypeRevision(),
                                     KnownAttributes *knownAttributes = nullptr)
    {
        QSet<QString> implicitSignals;
        for (int index = meta->propertyOffset(); index < meta->propertyCount(); ++index) {
            const QMetaProperty &property = meta->property(index);
            dump(property, metaRevision, knownAttributes);
            if (knownAttributes)
                knownAttributes->knownMethod(QByteArray(property.name()).append("Changed"),
                                             0, QTypeRevision::fromEncodedVersion(property.revision()));
            implicitSignals.insert(QString("%1Changed").arg(QString::fromUtf8(property.name())));
        }
        return implicitSignals;
    }

    void dump(const QMetaMethod &meth, const QSet<QString> &implicitSignals,
              KnownAttributes *knownAttributes = nullptr)
    {
        if (meth.methodType() == QMetaMethod::Signal) {
            if (meth.access() != QMetaMethod::Public)
                return; // nothing to do.
        } else if (meth.access() != QMetaMethod::Public) {
            return; // nothing to do.
        }

        QByteArray name = meth.name();
        const QString typeName = convertToId(meth.typeName());

        if (implicitSignals.contains(name)
                && !meth.revision()
                && meth.methodType() == QMetaMethod::Signal
                && meth.parameterNames().isEmpty()
                && typeName == QLatin1String("void")) {
            // don't mention implicit signals
            return;
        }

        QTypeRevision revision = QTypeRevision::fromEncodedVersion(meth.revision());
        if (knownAttributes && knownAttributes->knownMethod(name, meth.parameterNames().size(), revision))
            return;
        if (meth.methodType() == QMetaMethod::Signal)
            qml->writeStartObject(QLatin1String("Signal"));
        else
            qml->writeStartObject(QLatin1String("Method"));

        qml->writeScriptBinding(QLatin1String("name"), enquote(name));

        if (revision != QTypeRevision::zero())
            qml->writeScriptBinding(QLatin1String("revision"), QString::number(revision.toEncodedVersion<quint16>()));

        if (typeName != QLatin1String("void"))
            qml->writeScriptBinding(QLatin1String("type"), enquote(typeName));

        for (int i = 0; i < meth.parameterTypes().size(); ++i) {
            QByteArray argName = meth.parameterNames().at(i);

            qml->writeStartObject(QLatin1String("Parameter"));
            if (! argName.isEmpty())
                qml->writeScriptBinding(QLatin1String("name"), enquote(argName));
            writeTypeProperties(meth.parameterTypes().at(i), true);
            qml->writeEndObject();
        }

        qml->writeEndObject();
    }

    void dump(const QMetaEnum &e)
    {
        qml->writeStartObject(QLatin1String("Enum"));
        qml->writeScriptBinding(QLatin1String("name"), enquote(QString::fromUtf8(e.name())));

        QList<QPair<QString, QString> > namesValues;
        const int keyCount = e.keyCount();
        namesValues.reserve(keyCount);
        for (int index = 0; index < keyCount; ++index) {
            namesValues.append(qMakePair(enquote(QString::fromUtf8(e.key(index))), QString::number(e.value(index))));
        }

        qml->writeScriptObjectLiteralBinding(QLatin1String("values"), namesValues);
        qml->writeEndObject();
    }
};

enum ExitCode {
    EXIT_INVALIDARGUMENTS = 1,
    EXIT_SEGV = 2,
    EXIT_IMPORTERROR = 3
};

void printUsage(const QString &appName)
{
    std::cerr << qPrintable(QString(
                                 "Usage: %1 [-v] [-qapp] [-noinstantiate] [-defaultplatform] [-[non]relocatable] [-dependencies <dependencies.json>] [-merge <file-to-merge.qmltypes>] [-output <output-file.qmltypes>] [-noforceqtquick] module.uri version [module/import/path]\n"
                                 "       %1 [-v] [-qapp] [-noinstantiate] -path path/to/qmldir/directory [version]\n"
                                 "       %1 [-v] -builtins\n"
                                 "Example: %1 Qt.labs.folderlistmodel 2.0 /home/user/dev/qt-install/imports").arg(
                                 appName)) << std::endl;
}

static bool readDependenciesData(QString dependenciesFile, const QByteArray &fileData,
                                 QStringList *dependencies, const QStringList &urisToSkip,
                                 bool forceQtQuickDependency = true) {
    if (verbose) {
        std::cerr << "parsing "
                  << qPrintable( dependenciesFile ) << " skipping";
        for (const QString &uriToSkip : urisToSkip)
            std::cerr << ' '  << qPrintable(uriToSkip);
        std::cerr << std::endl;
    }
    QJsonParseError parseError;
    parseError.error = QJsonParseError::NoError;
    QJsonDocument doc = QJsonDocument::fromJson(fileData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        std::cerr << "Error parsing dependencies file " << dependenciesFile.toStdString()
                  << ":" << parseError.errorString().toStdString() << " at " << parseError.offset
                  << std::endl;
        return false;
    }
    if (doc.isArray()) {
        const QStringList requiredKeys = QStringList() << QStringLiteral("name")
                                                       << QStringLiteral("type");
        const auto deps = doc.array();
        for (const QJsonValue dep : deps) {
            if (dep.isObject()) {
                QJsonObject obj = dep.toObject();
                for (const QString &requiredKey : requiredKeys)
                    if (!obj.contains(requiredKey) || obj.value(requiredKey).isString())
                        continue;
                if (obj.value(QStringLiteral("type")).toString() != QLatin1String("module"))
                    continue;
                QString name = obj.value((QStringLiteral("name"))).toString();
                QString version = obj.value(QStringLiteral("version")).toString();
                if (name.isEmpty() || urisToSkip.contains(name))
                    continue;
                if (name.contains(QLatin1String("Private"), Qt::CaseInsensitive)) {
                    if (verbose)
                        std::cerr << "skipping private dependency "
                                  << qPrintable( name ) << " "  << qPrintable(version) << std::endl;
                    continue;
                }
                if (verbose)
                    std::cerr << "appending dependency "
                              << qPrintable( name ) << " "  << qPrintable(version) << std::endl;
                dependencies->append(version.isEmpty() ? name
                                                       : (name + QLatin1Char(' ') + version));
            }
        }
    } else {
        std::cerr << "Error parsing dependencies file " << dependenciesFile.toStdString()
                  << ": expected an array" << std::endl;
        return false;
    }
    // Workaround for avoiding conflicting types when no dependency has been found.
    //
    // qmlplugindump used to import QtQuick, so all types defined in QtQuick used to be skipped when dumping.
    // Now that it imports only Qt, it is no longer the case: if no dependency is found all the types defined
    // in QtQuick will be dumped, causing conflicts.
    if (forceQtQuickDependency && dependencies->isEmpty())
        dependencies->push_back(qtQuickQualifiedName);
    return true;
}

static bool readDependenciesFile(const QString &dependenciesFile, QStringList *dependencies,
                                const QStringList &urisToSkip) {
    if (!QFileInfo::exists(dependenciesFile)) {
        std::cerr << "non existing dependencies file " << dependenciesFile.toStdString()
                  << std::endl;
        return false;
    }
    QFile f(dependenciesFile);
    if (!f.open(QFileDevice::ReadOnly)) {
        std::cerr << "non existing dependencies file " << dependenciesFile.toStdString()
                  << ", " << f.errorString().toStdString() << std::endl;
        return false;
    }
    QByteArray fileData = f.readAll();
    return readDependenciesData(dependenciesFile, fileData, dependencies, urisToSkip, false);
}

static bool getDependencies(const QQmlEngine &engine, const QString &pluginImportUri,
                            const QString &pluginImportVersion, QStringList *dependencies,
                            bool forceQtQuickDependency)
{
    QString importScannerExe = QLatin1String("qmlimportscanner");
    QFileInfo selfExe(QCoreApplication::applicationFilePath());
    if (!selfExe.suffix().isEmpty())
        importScannerExe += QLatin1String(".") + selfExe.suffix();
    QString command = selfExe.absoluteDir().filePath(importScannerExe);

    QStringList commandArgs = QStringList()
            << QLatin1String("-qmlFiles")
            << QLatin1String("-");
    QStringList importPathList = engine.importPathList();
    importPathList.removeOne(QStringLiteral("qrc:/qt-project.org/imports"));
    for (const QString &path : importPathList)
        commandArgs << QLatin1String("-importPath") << path;

    QProcess importScanner;
    importScanner.start(command, commandArgs, QProcess::ReadWrite);
    if (!importScanner.waitForStarted())
        return false;

    importScanner.write("import ");
    importScanner.write(pluginImportUri.toUtf8());
    importScanner.write(" ");
    importScanner.write(pluginImportVersion.toUtf8());
    importScanner.write("\nQtObject{}\n");
    importScanner.closeWriteChannel();

    if (!importScanner.waitForFinished()) {
        std::cerr << "failure to start " << qPrintable(command);
        for (const QString &arg : std::as_const(commandArgs))
            std::cerr << ' ' << qPrintable(arg);
        std::cerr << std::endl;
        return false;
    }
    QByteArray depencenciesData = importScanner.readAllStandardOutput();
    if (!readDependenciesData(QLatin1String("<outputOfQmlimportscanner>"), depencenciesData,
                             dependencies, QStringList(pluginImportUri), forceQtQuickDependency)) {
        std::cerr << "failed to process output of qmlimportscanner" << std::endl;
        if (importScanner.exitCode() != 0)
            std::cerr << importScanner.readAllStandardError().toStdString();
        return false;
    }

    return true;
}

bool dependencyBetter(const QString &lhs, const QString &rhs)
{
    QStringList leftSegments = lhs.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList rightSegments = rhs.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    if (leftSegments.isEmpty())
        return false;
    if (rightSegments.isEmpty())
        return true;

    const QString leftModule = leftSegments.first();
    const QString rightModule = rightSegments.first();

    if (leftModule < rightModule)
        return true;
    if (leftModule > rightModule)
        return false;

    if (leftSegments.size() == 1)
        return false;
    if (rightSegments.size() == 1)
        return true;

    const QStringList leftVersion = leftSegments.at(1).split(QLatin1Char('.'));
    const QStringList rightVersion = rightSegments.at(1).split(QLatin1Char('.'));

    auto compareSegment = [&](int segmentIndex) {
        if (leftVersion.size() <= segmentIndex)
            return rightVersion.size() > segmentIndex ? 1 : 0;
        if (rightVersion.size() <= segmentIndex)
            return -1;

        bool leftOk = false;
        bool rightOk = false;
        const int leftSegment = leftSegments[segmentIndex].toUShort(&leftOk);
        const int rightSegment = rightSegments[segmentIndex].toUShort(&rightOk);

        if (!leftOk)
            return rightOk ? 1 : 0;
        if (!rightOk)
            return -1;

        return rightSegment - leftSegment;
    };

    const int major = compareSegment(0);
    return (major == 0) ? compareSegment(1) < 0 : major < 0;
}

void compactDependencies(QStringList *dependencies)
{
    std::sort(dependencies->begin(), dependencies->end(), dependencyBetter);
    QString currentModule;
    for (auto it = dependencies->begin(); it != dependencies->end();) {
        QStringList segments = it->split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (segments.isEmpty() || segments.first() == currentModule) {
            it = dependencies->erase(it);
        } else {
            currentModule = segments.first();
            ++it;
        }
    }
}

void printDebugMessage(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    std::cerr << msg.toStdString() << std::endl;
    // In case of QtFatalMsg the calling code will abort() when appropriate.
}

QT_BEGIN_NAMESPACE
static bool operator<(const QQmlType &a, const QQmlType &b)
{
    return a.qmlTypeName() < b.qmlTypeName()
            || (a.qmlTypeName() == b.qmlTypeName()
                && ((a.version().majorVersion() < b.version().majorVersion())
                    || (a.version().majorVersion() == b.version().majorVersion()
                        && a.version().minorVersion() < b.version().minorVersion())));
}
QT_END_NAMESPACE

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN) && !defined(Q_CC_MINGW)
    // we do not want windows popping up if the module loaded triggers an assert
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif // Q_OS_WIN && !Q_CC_MINGW
    // The default message handler might not print to console on some systems. Enforce this.
    qInstallMessageHandler(printDebugMessage);

#ifdef QT_SIMULATOR
    // Running this application would bring up the Qt Simulator (since it links Qt GUI), avoid that!
    QtSimulatorPrivate::SimulatorConnection::createStubInstance();
#endif

    // don't require a window manager even though we're a QGuiApplication
    bool requireWindowManager = false;
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == "--defaultplatform"
                || QString::fromLocal8Bit(argv[index]) == "-defaultplatform") {
            requireWindowManager = true;
            break;
        }
    }

    if (!requireWindowManager && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("minimal"));
    else
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    // Check which kind of application should be instantiated.
    bool useQApplication = false;
    for (int i = 0; i < argc; ++i) {
        QString arg = QLatin1String(argv[i]);
        if (arg == QLatin1String("--qapp") || arg == QLatin1String("-qapp"))
            useQApplication = true;
    }

#ifdef QT_WIDGETS_LIB
    QScopedPointer<QCoreApplication> app(useQApplication
            ? new QApplication(argc, argv)
            : new QGuiApplication(argc, argv));
#else
    Q_UNUSED(useQApplication);
    QScopedPointer<QCoreApplication> app(new QGuiApplication(argc, argv));
#endif // QT_WIDGETS_LIB

    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));
    QStringList args = app->arguments();
    const QString appName = QFileInfo(app->applicationFilePath()).baseName();
    if (args.size() < 2) {
        printUsage(appName);
        return EXIT_INVALIDARGUMENTS;
    }

    QString outputFilename;
    QString pluginImportUri;
    QString pluginImportVersion;
    bool relocatable = true;
    QString dependenciesFile;
    QString mergeFile;
    bool forceQtQuickDependency = true;
    bool strict = false;
    enum Action { Uri, Path, Builtins };
    Action action = Uri;
    {
        QStringList positionalArgs;

        for (int iArg = 0; iArg < args.size(); ++iArg) {
            const QString &arg = args.at(iArg);
            if (!arg.startsWith(QLatin1Char('-'))) {
                positionalArgs.append(arg);
                continue;
            }
            if (arg == QLatin1String("--dependencies")
                    || arg == QLatin1String("-dependencies")) {
                if (++iArg == args.size()) {
                    std::cerr << "missing dependencies file" << std::endl;
                    return EXIT_INVALIDARGUMENTS;
                }
                dependenciesFile = args.at(iArg);

                // Remove absolute path so that it does not show up in the
                // printed command line inside the plugins.qmltypes file.
                args[iArg] = QFileInfo(args.at(iArg)).fileName();
            } else if (arg == QLatin1String("--merge")
                       || arg == QLatin1String("-merge")) {
                if (++iArg == args.size()) {
                    std::cerr << "missing merge file" << std::endl;
                    return EXIT_INVALIDARGUMENTS;
                }
                mergeFile = args.at(iArg);
            } else if (arg == QLatin1String("--notrelocatable")
                    || arg == QLatin1String("-notrelocatable")
                    || arg == QLatin1String("--nonrelocatable")
                    || arg == QLatin1String("-nonrelocatable")) {
                relocatable = false;
            } else if (arg == QLatin1String("--relocatable")
                        || arg == QLatin1String("-relocatable")) {
                relocatable = true;
            } else if (arg == QLatin1String("--noinstantiate")
                       || arg == QLatin1String("-noinstantiate")) {
                creatable = false;
            } else if (arg == QLatin1String("--path")
                       || arg == QLatin1String("-path")) {
                action = Path;
            } else if (arg == QLatin1String("--builtins")
                       || arg == QLatin1String("-builtins")) {
                action = Builtins;
            } else if (arg == QLatin1String("-v")) {
                verbose = true;
            } else if (arg == QLatin1String("--noforceqtquick")
                       || arg == QLatin1String("-noforceqtquick")){
                forceQtQuickDependency = false;
            } else if (arg == QLatin1String("--output")
                       || arg == QLatin1String("-output")) {
                if (++iArg == args.size()) {
                    std::cerr << "missing output file" << std::endl;
                    return EXIT_INVALIDARGUMENTS;
                }
                outputFilename = args.at(iArg);
            } else if (arg == QLatin1String("--defaultplatform")
                       || arg == QLatin1String("-defaultplatform")) {
                continue;
            } else if (arg == QLatin1String("--qapp")
                       || arg == QLatin1String("-qapp")) {
                continue;
            } else if (arg == QLatin1String("--strict")
                       || arg == QLatin1String("-strict")) {
                strict = true;
                continue;
            } else {
                std::cerr << "Invalid argument: " << qPrintable(arg) << std::endl;
                return EXIT_INVALIDARGUMENTS;
            }
        }

        std::cerr << "qmlplugindump is deprecated.\n"
                  << "Please declare your types using QML_ELEMENT and related macros.\n"
                  << "Then utilize the build system to invoke qmltyperegistrar in order to\n"
                  << "generate qmltypes files.\n";

        if (action == Uri) {
            if (positionalArgs.size() != 3 && positionalArgs.size() != 4) {
                std::cerr << "Incorrect number of positional arguments" << std::endl;
                return EXIT_INVALIDARGUMENTS;
            }
            pluginImportUri = positionalArgs.at(1);
            pluginImportVersion = positionalArgs[2];
            if (positionalArgs.size() >= 4)
                pluginImportPath = positionalArgs.at(3);
        } else if (action == Path) {
            if (positionalArgs.size() != 2 && positionalArgs.size() != 3) {
                std::cerr << "Incorrect number of positional arguments" << std::endl;
                return EXIT_INVALIDARGUMENTS;
            }
            pluginImportPath = QDir::fromNativeSeparators(positionalArgs.at(1));
            if (positionalArgs.size() == 3)
                pluginImportVersion = positionalArgs.at(2);
        } else if (action == Builtins) {
            if (positionalArgs.size() != 1) {
                std::cerr << "Incorrect number of positional arguments" << std::endl;
                return EXIT_INVALIDARGUMENTS;
            }
        }
    }

    QQmlEngine engine;
    if (!pluginImportPath.isEmpty()) {
        QDir cur = QDir::current();
        cur.cd(pluginImportPath);
        pluginImportPath = cur.canonicalPath();
        if (!QDir::setCurrent(pluginImportPath)) {
            std::cerr << "Cannot set current directory to import path "
                      << qPrintable(pluginImportPath) << std::endl;
        }
        engine.addImportPath(pluginImportPath);
    }

    // Merge file.
    QStringList mergeDependencies;
    QString mergeComponents;
    if (!mergeFile.isEmpty()) {
        const QStringList merge = readQmlTypes(mergeFile);
        if (!merge.isEmpty()) {
            QRegularExpression re("(\\w+\\.*\\w*\\s*\\d+\\.\\d+)");
            QRegularExpressionMatchIterator i = re.globalMatch(merge[1]);
            while (i.hasNext()) {
                QRegularExpressionMatch m = i.next();
                mergeDependencies << m.captured(1);
            }
            mergeComponents = merge [2];
        }
    }

    // Dependencies.

    bool calculateDependencies = !pluginImportUri.isEmpty() && !pluginImportVersion.isEmpty();
    QStringList dependencies;
    if (!dependenciesFile.isEmpty())
        calculateDependencies = !readDependenciesFile(dependenciesFile, &dependencies,
                                                      QStringList(pluginImportUri)) && calculateDependencies;
    if (calculateDependencies)
        getDependencies(engine, pluginImportUri, pluginImportVersion, &dependencies,
                        forceQtQuickDependency);

    compactDependencies(&dependencies);


    QString qtQmlImportString = QString::fromLatin1("import QtQml %1.%2")
        .arg(qtQmlMajorVersion)
        .arg(qtQmlMinorVersion);

    // load the QtQml builtins and the dependencies
    {
        QByteArray code(qtQmlImportString.toUtf8());
        for (const QString &moduleToImport : std::as_const(dependencies)) {
            code.append("\nimport ");
            code.append(moduleToImport.toUtf8());
        }
        code.append("\nQtObject {}");
        QQmlComponent c(&engine);
        c.setData(code, QUrl::fromLocalFile(pluginImportPath + "/loaddependencies.qml"));
        c.create();
        const auto errors = c.errors();
        if (!errors.isEmpty()) {
            for (const QQmlError &error : errors)
                std::cerr << qPrintable( error.toString() ) << std::endl;
            return EXIT_IMPORTERROR;
        }
    }

    // find all QMetaObjects reachable from the builtin module
    QSet<const QMetaObject *> uncreatableMetas;
    QSet<const QMetaObject *> singletonMetas;

    // this will hold the meta objects we want to dump information of
    QSet<const QMetaObject *> metas;

    // composite types we want to dump information of
    QMap<QString, QList<QQmlType>> compositeTypes;

    QTypeRevision version = QTypeRevision::fromVersion(qtQmlMajorVersion, qtQmlMinorVersion);
    QmlVersionInfo info;
    if (action == Builtins) {
        QMap<QString, QList<QQmlType>> defaultCompositeTypes;
        QSet<const QMetaObject *> builtins = collectReachableMetaObjects(
                    &engine, uncreatableMetas, singletonMetas, defaultCompositeTypes,
                    {QLatin1String("Qt"), version, strict});
        Q_ASSERT(builtins.size() == 1);
        metas.insert(*builtins.begin());
    } else {
        auto versionSplitted = pluginImportVersion.split(".");
        bool ok = versionSplitted.size() == 2;
        if (!ok)
            qCritical("Invalid version number");
        else {
            const int majorVersion = versionSplitted.at(0).toInt(&ok);
            if (!ok)
                qCritical("Invalid major version");
            const int minorVersion = versionSplitted.at(1).toInt(&ok);
            if (!ok)
                qCritical("Invalid minor version");
            version = QTypeRevision::fromVersion(majorVersion, minorVersion);
        }
        QList<QQmlType> defaultTypes = QQmlMetaType::qmlTypes();
        // find a valid QtQuick import
        QByteArray importCode;
        QQmlType qtObjectType = QQmlMetaType::qmlType(&QObject::staticMetaObject);
        if (!qtObjectType.isValid()) {
            std::cerr << "Could not find QtObject type" << std::endl;
            importCode = qtQmlImportString.toUtf8();
        } else {
            QString module = qtObjectType.qmlTypeName();
            module = module.mid(0, module.lastIndexOf(QLatin1Char('/')));
            importCode = QString("import %1 %2.%3").arg(
                        module, QString::number(qtObjectType.version().majorVersion()),
                        QString::number(qtObjectType.version().minorVersion())).toUtf8();
        }
        // avoid importing dependencies?
        for (const QString &moduleToImport : std::as_const(dependencies)) {
            importCode.append("\nimport ");
            importCode.append(moduleToImport.toUtf8());
        }

        // find all QMetaObjects reachable when the specified module is imported
        if (action != Path) {
            importCode += QString("\nimport %0 %1\n").arg(pluginImportUri, pluginImportVersion).toLatin1();
        } else {
            // pluginImportVersion can be empty
            importCode += QString("\nimport \".\" %2\n").arg(pluginImportVersion).toLatin1();
        }

        // create a component with these imports to make sure the imports are valid
        // and to populate the declarative meta type system
        {
            QByteArray code = importCode;
            code += "\nQtObject {}";
            QQmlComponent c(&engine);

            c.setData(code, QUrl::fromLocalFile(pluginImportPath + "/typelist.qml"));
            c.create();
            const auto errors = c.errors();
            if (!errors.isEmpty()) {
                for (const QQmlError &error : errors)
                    std::cerr << qPrintable( error.toString() ) << std::endl;
                return EXIT_IMPORTERROR;
            }
        }
        info = {pluginImportUri, version, strict};
        QSet<const QMetaObject *> candidates = collectReachableMetaObjects(&engine, uncreatableMetas, singletonMetas, compositeTypes, info, defaultTypes);

        for (auto it = compositeTypes.begin(), end = compositeTypes.end(); it != end; ++it) {
            std::sort(it->begin(), it->end());
            it->erase(std::unique(it->begin(), it->end()), it->end());
        }

        for (const QMetaObject *mo : std::as_const(candidates)) {
            if (mo->className() != QLatin1String("Qt"))
                metas.insert(mo);
        }
    }

    // setup static rewrites of type names
    cppToId.insert("QString", "string");

    // start dumping data
    QByteArray bytes;
    QQmlJSStreamWriter qml(&bytes);

    qml.writeStartDocument();
    qml.writeLibraryImport(QLatin1String("QtQuick.tooling"), 1, 2);
    qml.write(QString("\n"
              "// This file describes the plugin-supplied types contained in the library.\n"
              "// It is used for QML tooling purposes only.\n"
              "//\n"
              "// This file was auto-generated by:\n"
              "// '%1 %2'\n"
              "//\n"
              "// qmlplugindump is deprecated! You should use qmltyperegistrar instead.\n"
              "\n").arg(QFileInfo(args.at(0)).baseName(), args.mid(1).join(QLatin1Char(' '))));
    qml.writeStartObject("Module");

    // put the metaobjects into a map so they are always dumped in the same order
    QMap<QString, const QMetaObject *> nameToMeta;
    for (const QMetaObject *meta : std::as_const(metas))
        nameToMeta.insert(convertToId(meta), meta);

    Dumper dumper(&qml);
    if (relocatable)
        dumper.setRelocatableModuleUri(pluginImportUri);
    for (const QMetaObject *meta : std::as_const(nameToMeta)) {
        dumper.dump(QQmlEnginePrivate::get(&engine), meta, uncreatableMetas.contains(meta), singletonMetas.contains(meta));
    }

    QMap<QString, QList<QQmlType>>::const_iterator iter = compositeTypes.constBegin();
    for (; iter != compositeTypes.constEnd(); ++iter)
        dumper.dumpComposite(&engine, iter.value(), info);

    // Insert merge file.
    qml.write(mergeComponents);

    qml.writeEndObject();
    qml.writeEndDocument();

    if (!outputFilename.isEmpty()) {
        QFile file(outputFilename);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << bytes.constData();
        }
    } else {
        std::cout << bytes.constData() << std::flush;
    }

    // workaround to avoid crashes on exit
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(0);
    QObject::connect(&timer, SIGNAL(timeout()), app.data(), SLOT(quit()));
    timer.start();

    return app->exec();
}
