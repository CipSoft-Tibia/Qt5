/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "pluginmanager_p.h"
#include "qdesigner_utils_p.h"
#include "qdesigner_qsettings_p.h"

#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/qextensionmanager.h>
#include <QtDesigner/abstractlanguage.h>

#include <QtUiPlugin/customwidget.h>

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qset.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmap.h>
#include <QtCore/qsettings.h>
#include <QtCore/qcoreapplication.h>

#include <QtCore/qxmlstream.h>

static const char *uiElementC = "ui";
static const char *languageAttributeC = "language";
static const char *widgetElementC = "widget";
static const char *displayNameAttributeC = "displayname";
static const char *classAttributeC = "class";
static const char *customwidgetElementC = "customwidget";
static const char *extendsElementC = "extends";
static const char *addPageMethodC = "addpagemethod";
static const char *propertySpecsC = "propertyspecifications";
static const char *stringPropertySpecC = "stringpropertyspecification";
static const char propertyToolTipC[] = "tooltip";
static const char *stringPropertyNameAttrC = "name";
static const char *stringPropertyTypeAttrC = "type";
static const char *stringPropertyNoTrAttrC = "notr";
static const char *jambiLanguageC = "jambi";

enum { debugPluginManager = 0 };

/* Custom widgets: Loading custom widgets is a 2-step process: PluginManager
 * scans for its plugins in the constructor. At this point, it might not be safe
 * to immediately initialize the custom widgets it finds, because the rest of
 * Designer is not initialized yet.
 * Later on, in ensureInitialized(), the plugin instances (including static ones)
 * are iterated and the custom widget plugins are initialized and added to internal
 * list of custom widgets and parsed data. Should there be a parse error or a language
 * mismatch, it kicks out the respective custom widget. The m_initialized flag
 * is used to indicate the state.
 * Later, someone might call registerNewPlugins(), which agains clears the flag via
 * registerPlugin() and triggers the process again.
 * Also note that Jambi fakes a custom widget collection that changes its contents
 * every time the project is switched. So, custom widget plugins can actually
 * disappear, and the custom widget list must be cleared and refilled in
 * ensureInitialized() after registerNewPlugins. */

QT_BEGIN_NAMESPACE

static QStringList unique(const QStringList &lst)
{
    const QSet<QString> s = QSet<QString>::fromList(lst);
    return s.toList();
}

QStringList QDesignerPluginManager::defaultPluginPaths()
{
    QStringList result;

    const QStringList path_list = QCoreApplication::libraryPaths();

    const QString designer = QStringLiteral("designer");
    for (const QString &path : path_list) {
        QString libPath = path;
        libPath += QDir::separator();
        libPath += designer;
        result.append(libPath);
    }

    QString homeLibPath = QDir::homePath();
    homeLibPath += QDir::separator();
    homeLibPath += QStringLiteral(".designer");
    homeLibPath += QDir::separator();
    homeLibPath += QStringLiteral("plugins");

    result.append(homeLibPath);
    return result;
}

// Figure out the language designer is running. ToDo: Introduce some
// Language name API to QDesignerLanguageExtension?

static inline QString getDesignerLanguage(QDesignerFormEditorInterface *core)
{
    if (QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension *>(core->extensionManager(), core)) {
        if (lang->uiExtension() == QStringLiteral("jui"))
            return QLatin1String(jambiLanguageC);
        return QStringLiteral("unknown");
    }
    return QStringLiteral("c++");
}

// ----------------  QDesignerCustomWidgetSharedData

class QDesignerCustomWidgetSharedData : public QSharedData {
public:
    // Type of a string property
    typedef QPair<qdesigner_internal::TextPropertyValidationMode, bool> StringPropertyType;
    typedef QHash<QString, StringPropertyType> StringPropertyTypeMap;
    typedef QHash<QString, QString> PropertyToolTipMap;

    explicit QDesignerCustomWidgetSharedData(const QString &thePluginPath) : pluginPath(thePluginPath) {}
    void clearXML();

    QString pluginPath;

    QString xmlClassName;
    QString xmlDisplayName;
    QString xmlLanguage;
    QString xmlAddPageMethod;
    QString xmlExtends;

    StringPropertyTypeMap xmlStringPropertyTypeMap;
    PropertyToolTipMap propertyToolTipMap;
};

void QDesignerCustomWidgetSharedData::clearXML()
{
    xmlClassName.clear();
    xmlDisplayName.clear();
    xmlLanguage.clear();
    xmlAddPageMethod.clear();
    xmlExtends.clear();
    xmlStringPropertyTypeMap.clear();
}

// ----------------  QDesignerCustomWidgetData

QDesignerCustomWidgetData::QDesignerCustomWidgetData(const QString &pluginPath) :
    m_d(new QDesignerCustomWidgetSharedData(pluginPath))
{
}

QDesignerCustomWidgetData::QDesignerCustomWidgetData(const QDesignerCustomWidgetData &o) :
     m_d(o.m_d)
{
}

QDesignerCustomWidgetData& QDesignerCustomWidgetData::operator=(const QDesignerCustomWidgetData &o)
{
    m_d.operator=(o.m_d);
    return *this;
}

QDesignerCustomWidgetData::~QDesignerCustomWidgetData()
{
}

bool QDesignerCustomWidgetData::isNull() const
{
    return m_d->xmlClassName.isEmpty() || m_d->pluginPath.isEmpty();
}

QString QDesignerCustomWidgetData::xmlClassName() const
{
    return m_d->xmlClassName;
}

QString QDesignerCustomWidgetData::xmlLanguage() const
{
    return m_d->xmlLanguage;
}

QString QDesignerCustomWidgetData::xmlAddPageMethod() const
{
    return m_d->xmlAddPageMethod;
}

QString QDesignerCustomWidgetData::xmlExtends() const
{
    return m_d->xmlExtends;
}

QString QDesignerCustomWidgetData::xmlDisplayName() const
{
    return m_d->xmlDisplayName;
}

QString QDesignerCustomWidgetData::pluginPath() const
{
    return m_d->pluginPath;
}

bool QDesignerCustomWidgetData::xmlStringPropertyType(const QString &name, StringPropertyType *type) const
{
    QDesignerCustomWidgetSharedData::StringPropertyTypeMap::const_iterator it = m_d->xmlStringPropertyTypeMap.constFind(name);
    if (it == m_d->xmlStringPropertyTypeMap.constEnd()) {
        *type = StringPropertyType(qdesigner_internal::ValidationRichText, true);
        return false;
    }
    *type = it.value();
    return true;
}

QString QDesignerCustomWidgetData::propertyToolTip(const QString &name) const
{
    return m_d->propertyToolTipMap.value(name);
}

// Wind a QXmlStreamReader  until it finds an element. Returns index or one of FindResult
enum FindResult { FindError = -2, ElementNotFound = -1 };

static int findElement(const QStringList &desiredElts, QXmlStreamReader &sr)
{
    while (true) {
        switch(sr.readNext()) {
        case QXmlStreamReader::EndDocument:
            return ElementNotFound;
        case QXmlStreamReader::Invalid:
            return FindError;
        case QXmlStreamReader::StartElement: {
            const int index = desiredElts.indexOf(sr.name().toString().toLower());
            if (index >= 0)
                return index;
        }
            break;
        default:
            break;
        }
    }
    return FindError;
}

static inline QString msgXmlError(const QString &name, const QString &errorMessage)
{
    return QDesignerPluginManager::tr("An XML error was encountered when parsing the XML of the custom widget %1: %2").arg(name, errorMessage);
}

static inline QString msgAttributeMissing(const QString &name)
{
    return QDesignerPluginManager::tr("A required attribute ('%1') is missing.").arg(name);
}

static qdesigner_internal::TextPropertyValidationMode typeStringToType(const QString &v, bool *ok)
{
    *ok = true;
    if (v  == QStringLiteral("multiline"))
        return qdesigner_internal::ValidationMultiLine;
    if (v  == QStringLiteral("richtext"))
        return qdesigner_internal::ValidationRichText;
    if (v  == QStringLiteral("stylesheet"))
        return qdesigner_internal::ValidationStyleSheet;
    if (v  == QStringLiteral("singleline"))
        return qdesigner_internal::ValidationSingleLine;
    if (v  == QStringLiteral("objectname"))
        return qdesigner_internal::ValidationObjectName;
    if (v  == QStringLiteral("objectnamescope"))
        return qdesigner_internal::ValidationObjectNameScope;
    if (v  == QStringLiteral("url"))
        return qdesigner_internal::ValidationURL;
    *ok = false;
    return qdesigner_internal::ValidationRichText;
}

static bool parsePropertySpecs(QXmlStreamReader &sr,
                               QDesignerCustomWidgetSharedData *data,
                               QString *errorMessage)
{
    const QString propertySpecs = QLatin1String(propertySpecsC);
    const QString stringPropertySpec = QLatin1String(stringPropertySpecC);
    const QString propertyToolTip = QLatin1String(propertyToolTipC);
    const QString stringPropertyTypeAttr = QLatin1String(stringPropertyTypeAttrC);
    const QString stringPropertyNoTrAttr = QLatin1String(stringPropertyNoTrAttrC);
    const QString stringPropertyNameAttr = QLatin1String(stringPropertyNameAttrC);

    while (!sr.atEnd()) {
        switch(sr.readNext()) {
        case QXmlStreamReader::StartElement: {
            if (sr.name() == stringPropertySpec) {
                const QXmlStreamAttributes atts = sr.attributes();
                const QString name = atts.value(stringPropertyNameAttr).toString();
                const QString type = atts.value(stringPropertyTypeAttr).toString();
                const QString notrS = atts.value(stringPropertyNoTrAttr).toString(); //Optional

                if (type.isEmpty()) {
                    *errorMessage = msgAttributeMissing(stringPropertyTypeAttr);
                    return false;
                }
                if (name.isEmpty()) {
                    *errorMessage = msgAttributeMissing(stringPropertyNameAttr);
                    return false;
                }
                bool typeOk;
                const bool noTr = notrS == QStringLiteral("true") || notrS == QStringLiteral("1");
                QDesignerCustomWidgetSharedData::StringPropertyType v(typeStringToType(type, &typeOk), !noTr);
                if (!typeOk) {
                    *errorMessage = QDesignerPluginManager::tr("'%1' is not a valid string property specification.").arg(type);
                    return false;
                }
                data->xmlStringPropertyTypeMap.insert(name, v);
            } else if (sr.name() == propertyToolTip) {
                const QString name = sr.attributes().value(stringPropertyNameAttr).toString();
                if (name.isEmpty()) {
                    *errorMessage = msgAttributeMissing(stringPropertyNameAttr);
                    return false;
                }
                data->propertyToolTipMap.insert(name, sr.readElementText().trimmed());
            } else {
                *errorMessage = QDesignerPluginManager::tr("An invalid property specification ('%1') was encountered. Supported types: %2").arg(sr.name().toString(), stringPropertySpec);
                return false;
            }
        }
            break;
        case QXmlStreamReader::EndElement: // Outer </stringproperties>
            if (sr.name() == propertySpecs)
                return true;
        default:
            break;
        }
    }
    return true;
}

QDesignerCustomWidgetData::ParseResult
                       QDesignerCustomWidgetData::parseXml(const QString &xml, const QString &name, QString *errorMessage)
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO << name;

    QDesignerCustomWidgetSharedData &data =  *m_d;
    data.clearXML();

    QXmlStreamReader sr(xml);

    bool foundUI = false;
    bool foundWidget = false;
    ParseResult rc = ParseOk;
    // Parse for the (optional) <ui> or the first <widget> element
    QStringList elements;
    elements.push_back(QLatin1String(uiElementC));
    elements.push_back(QLatin1String(widgetElementC));
    for (int i = 0; i < 2 && !foundWidget; i++) {
        switch (findElement(elements, sr)) {
        case FindError:
            *errorMessage = msgXmlError(name, sr.errorString());
            return ParseError;
        case ElementNotFound:
            *errorMessage = QDesignerPluginManager::tr("The XML of the custom widget %1 does not contain any of the elements <widget> or <ui>.").arg(name);
            return ParseError;
        case 0: { // <ui>
            const QXmlStreamAttributes attributes = sr.attributes();
            data.xmlLanguage = attributes.value(QLatin1String(languageAttributeC)).toString();
            data.xmlDisplayName = attributes.value(QLatin1String(displayNameAttributeC)).toString();
            foundUI = true;
        }
            break;
        case 1: // <widget>: Do some sanity checks
            data.xmlClassName = sr.attributes().value(QLatin1String(classAttributeC)).toString();
            if (data.xmlClassName.isEmpty()) {
                *errorMessage = QDesignerPluginManager::tr("The class attribute for the class %1 is missing.").arg(name);
                rc = ParseWarning;
            } else {
                if (data.xmlClassName != name) {
                    *errorMessage = QDesignerPluginManager::tr("The class attribute for the class %1 does not match the class name %2.").arg(data.xmlClassName, name);
                    rc = ParseWarning;
                }
            }
            foundWidget = true;
            break;
        }
    }
    // Parse out the <customwidget> element which might be present if  <ui> was there
    if (!foundUI)
        return rc;
    elements.clear();
    elements.push_back(QLatin1String(customwidgetElementC));
    switch (findElement(elements, sr)) {
    case FindError:
        *errorMessage = msgXmlError(name, sr.errorString());
        return ParseError;
    case ElementNotFound:
        return rc;
    default:
        break;
    }
    // Find <extends>, <addPageMethod>, <stringproperties>
    elements.clear();
    elements.push_back(QLatin1String(extendsElementC));
    elements.push_back(QLatin1String(addPageMethodC));
    elements.push_back(QLatin1String(propertySpecsC));
    while (true) {
        switch (findElement(elements, sr)) {
        case FindError:
            *errorMessage = msgXmlError(name, sr.errorString());
            return ParseError;
        case ElementNotFound:
            return rc;
        case 0: // <extends>
            data.xmlExtends = sr.readElementText();
            if (sr.tokenType() != QXmlStreamReader::EndElement) {
                *errorMessage = msgXmlError(name, sr.errorString());
                return ParseError;
            }
            break;
        case 1: // <addPageMethod>
            data.xmlAddPageMethod = sr.readElementText();
            if (sr.tokenType() != QXmlStreamReader::EndElement) {
                *errorMessage = msgXmlError(name, sr.errorString());
                return ParseError;
            }
            break;
        case 2: // <stringproperties>
            if (!parsePropertySpecs(sr, m_d.data(), errorMessage)) {
                *errorMessage = msgXmlError(name, *errorMessage);
                return ParseError;
            }
            break;
        }
    }
    return rc;
}

// ---------------- QDesignerPluginManagerPrivate

class QDesignerPluginManagerPrivate {
    public:
    typedef QPair<QString, QString> ClassNamePropertyNameKey;

    QDesignerPluginManagerPrivate(QDesignerFormEditorInterface *core);

    void clearCustomWidgets();
    bool addCustomWidget(QDesignerCustomWidgetInterface *c,
                         const QString &pluginPath,
                         const QString &designerLanguage);
    void addCustomWidgets(const QObject *o,
                          const QString &pluginPath,
                          const QString &designerLanguage);

    QDesignerFormEditorInterface *m_core;
    QStringList m_pluginPaths;
    QStringList m_registeredPlugins;
    // TODO: QPluginLoader also caches invalid plugins -> This seems to be dead code
    QStringList m_disabledPlugins;

    typedef QMap<QString, QString> FailedPluginMap;
    FailedPluginMap m_failedPlugins;

    // Synced lists of custom widgets and their data. Note that the list
    // must be ordered for collections to appear in order.
    QList<QDesignerCustomWidgetInterface *> m_customWidgets;
    QList<QDesignerCustomWidgetData> m_customWidgetData;

    QStringList defaultPluginPaths() const;

    bool m_initialized;
};

QDesignerPluginManagerPrivate::QDesignerPluginManagerPrivate(QDesignerFormEditorInterface *core) :
   m_core(core),
   m_initialized(false)
{
}

void QDesignerPluginManagerPrivate::clearCustomWidgets()
{
    m_customWidgets.clear();
    m_customWidgetData.clear();
}

// Add a custom widget to the list if it parses correctly
// and is of the right language
bool QDesignerPluginManagerPrivate::addCustomWidget(QDesignerCustomWidgetInterface *c,
                                                    const QString &pluginPath,
                                                    const QString &designerLanguage)
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO << c->name();

    if (!c->isInitialized())
        c->initialize(m_core);
    // Parse the XML even if the plugin is initialized as Jambi might play tricks here
    QDesignerCustomWidgetData data(pluginPath);
    const QString domXml = c->domXml();
    if (!domXml.isEmpty()) { // Legacy: Empty XML means: Do not show up in widget box.
        QString errorMessage;
        const QDesignerCustomWidgetData::ParseResult pr = data.parseXml(domXml, c->name(), &errorMessage);
        switch (pr) {
            case QDesignerCustomWidgetData::ParseOk:
            break;
            case QDesignerCustomWidgetData::ParseWarning:
            qdesigner_internal::designerWarning(errorMessage);
            break;
            case QDesignerCustomWidgetData::ParseError:
            qdesigner_internal::designerWarning(errorMessage);
            return false;
        }
        // Does the language match?
        const QString pluginLanguage = data.xmlLanguage();
        if (!pluginLanguage.isEmpty() && pluginLanguage.compare(designerLanguage, Qt::CaseInsensitive))
            return false;
    }
    m_customWidgets.push_back(c);
    m_customWidgetData.push_back(data);
    return true;
}

// Check the plugin interface for either a custom widget or a collection and
// add all contained custom widgets.
void QDesignerPluginManagerPrivate::addCustomWidgets(const QObject *o,
                                                     const QString &pluginPath,
                                                     const QString &designerLanguage)
{
    if (QDesignerCustomWidgetInterface *c = qobject_cast<QDesignerCustomWidgetInterface*>(o)) {
        addCustomWidget(c, pluginPath, designerLanguage);
        return;
    }
    if (const QDesignerCustomWidgetCollectionInterface *coll = qobject_cast<QDesignerCustomWidgetCollectionInterface*>(o)) {
        const QList<QDesignerCustomWidgetInterface *> &collCustomWidgets = coll->customWidgets();
        for (QDesignerCustomWidgetInterface *c : collCustomWidgets)
            addCustomWidget(c, pluginPath, designerLanguage);
    }
}


// ---------------- QDesignerPluginManager
// As of 4.4, the header will be distributed with the Eclipse plugin.

QDesignerPluginManager::QDesignerPluginManager(QDesignerFormEditorInterface *core) :
    QObject(core),
    m_d(new QDesignerPluginManagerPrivate(core))
{
    m_d->m_pluginPaths = defaultPluginPaths();
    const QSettings settings(qApp->organizationName(), QDesignerQSettings::settingsApplicationName());
    m_d->m_disabledPlugins = unique(settings.value(QStringLiteral("PluginManager/DisabledPlugins")).toStringList());

    // Register plugins
    updateRegisteredPlugins();

    if (debugPluginManager)
        qDebug() << "QDesignerPluginManager::disabled: " <<  m_d->m_disabledPlugins << " static " << m_d->m_customWidgets.size();
}

QDesignerPluginManager::~QDesignerPluginManager()
{
    syncSettings();
    delete m_d;
}

QDesignerFormEditorInterface *QDesignerPluginManager::core() const
{
    return m_d->m_core;
}

QStringList QDesignerPluginManager::findPlugins(const QString &path)
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO << path;
    const QDir dir(path);
    if (!dir.exists())
        return QStringList();

    const QFileInfoList infoList = dir.entryInfoList(QDir::Files);
    if (infoList.empty())
        return QStringList();

    // Load symbolic links but make sure all file names are unique as not
    // to fall for something like 'libplugin.so.1 -> libplugin.so'
    QStringList result;
    const QFileInfoList::const_iterator icend = infoList.constEnd();
    for (QFileInfoList::const_iterator it = infoList.constBegin(); it != icend; ++it) {
        QString fileName;
        if (it->isSymLink()) {
            const QFileInfo linkTarget = QFileInfo(it->symLinkTarget());
            if (linkTarget.exists() && linkTarget.isFile())
                fileName = linkTarget.absoluteFilePath();
        } else {
            fileName = it->absoluteFilePath();
        }
        if (!fileName.isEmpty() && QLibrary::isLibrary(fileName) && !result.contains(fileName))
            result += fileName;
    }
    return result;
}

void QDesignerPluginManager::setDisabledPlugins(const QStringList &disabled_plugins)
{
    m_d->m_disabledPlugins = disabled_plugins;
    updateRegisteredPlugins();
}

void QDesignerPluginManager::setPluginPaths(const QStringList &plugin_paths)
{
    m_d->m_pluginPaths = plugin_paths;
    updateRegisteredPlugins();
}

QStringList QDesignerPluginManager::disabledPlugins() const
{
    return m_d->m_disabledPlugins;
}

QStringList QDesignerPluginManager::failedPlugins() const
{
    return m_d->m_failedPlugins.keys();
}

QString QDesignerPluginManager::failureReason(const QString &pluginName) const
{
    return m_d->m_failedPlugins.value(pluginName);
}

QStringList QDesignerPluginManager::registeredPlugins() const
{
    return m_d->m_registeredPlugins;
}

QStringList QDesignerPluginManager::pluginPaths() const
{
    return m_d->m_pluginPaths;
}

QObject *QDesignerPluginManager::instance(const QString &plugin) const
{
    if (m_d->m_disabledPlugins.contains(plugin))
        return 0;

    QPluginLoader loader(plugin);
    return loader.instance();
}

void QDesignerPluginManager::updateRegisteredPlugins()
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO;
    m_d->m_registeredPlugins.clear();
    for (const QString &path : qAsConst(m_d->m_pluginPaths))
        registerPath(path);
}

bool QDesignerPluginManager::registerNewPlugins()
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO;

    const int before = m_d->m_registeredPlugins.size();
    for (const QString &path : qAsConst(m_d->m_pluginPaths))
        registerPath(path);
    const bool newPluginsFound = m_d->m_registeredPlugins.size() > before;
    // We force a re-initialize as Jambi collection might return
    // different widget lists when switching projects.
    m_d->m_initialized = false;
    ensureInitialized();

    return newPluginsFound;
}

void QDesignerPluginManager::registerPath(const QString &path)
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO << path;
    const QStringList &candidates = findPlugins(path);
    for (const QString &plugin : candidates)
        registerPlugin(plugin);
}

void QDesignerPluginManager::registerPlugin(const QString &plugin)
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO << plugin;
    if (m_d->m_disabledPlugins.contains(plugin))
        return;
    if (m_d->m_registeredPlugins.contains(plugin))
        return;

    QPluginLoader loader(plugin);
    if (loader.isLoaded() || loader.load()) {
        m_d->m_registeredPlugins += plugin;
        QDesignerPluginManagerPrivate::FailedPluginMap::iterator fit = m_d->m_failedPlugins.find(plugin);
        if (fit != m_d->m_failedPlugins.end())
            m_d->m_failedPlugins.erase(fit);
        return;
    }

    const QString errorMessage = loader.errorString();
    m_d->m_failedPlugins.insert(plugin, errorMessage);
}



bool QDesignerPluginManager::syncSettings()
{
    QSettings settings(qApp->organizationName(), QDesignerQSettings::settingsApplicationName());
    settings.beginGroup(QStringLiteral("PluginManager"));
    settings.setValue(QStringLiteral("DisabledPlugins"), m_d->m_disabledPlugins);
    settings.endGroup();
    return settings.status() == QSettings::NoError;
}

void QDesignerPluginManager::ensureInitialized()
{
    if (debugPluginManager)
        qDebug() << Q_FUNC_INFO <<  m_d->m_initialized << m_d->m_customWidgets.size();

    if (m_d->m_initialized)
        return;

    const QString designerLanguage = getDesignerLanguage(m_d->m_core);

    m_d->clearCustomWidgets();
    // Add the static custom widgets
    const QObjectList staticPluginObjects = QPluginLoader::staticInstances();
    if (!staticPluginObjects.empty()) {
        const QString staticPluginPath = QCoreApplication::applicationFilePath();
        for (QObject *o : staticPluginObjects)
            m_d->addCustomWidgets(o, staticPluginPath, designerLanguage);
    }
    for (const QString &plugin : qAsConst(m_d->m_registeredPlugins)) {
        if (QObject *o = instance(plugin))
            m_d->addCustomWidgets(o, plugin, designerLanguage);
    }

    m_d->m_initialized = true;
}

QDesignerPluginManager::CustomWidgetList QDesignerPluginManager::registeredCustomWidgets() const
{
    const_cast<QDesignerPluginManager*>(this)->ensureInitialized();
    return m_d->m_customWidgets;
}

QDesignerCustomWidgetData QDesignerPluginManager::customWidgetData(QDesignerCustomWidgetInterface *w) const
{
    const int index = m_d->m_customWidgets.indexOf(w);
    if (index == -1)
        return QDesignerCustomWidgetData();
    return m_d->m_customWidgetData.at(index);
}

QDesignerCustomWidgetData QDesignerPluginManager::customWidgetData(const QString &name) const
{
    const int count = m_d->m_customWidgets.size();
    for (int i = 0; i < count; i++)
        if (m_d->m_customWidgets.at(i)->name() == name)
            return m_d->m_customWidgetData.at(i);
    return QDesignerCustomWidgetData();
}

QObjectList QDesignerPluginManager::instances() const
{
    const QStringList &plugins = registeredPlugins();

    QObjectList lst;
    for (const QString &plugin : plugins) {
        if (QObject *o = instance(plugin))
            lst.append(o);
    }

    return lst;
}

QT_END_NAMESPACE
