// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qloggingcategory.h"
#include "qloggingregistry_p.h"

QT_BEGIN_NAMESPACE

const char qtDefaultCategoryName[] = "default";
Q_GLOBAL_STATIC(QLoggingCategory, qtDefaultCategory, qtDefaultCategoryName)

/*!
    \class QLoggingCategory
    \inmodule QtCore
    \since 5.2
    \threadsafe

    \brief The QLoggingCategory class represents a category, or 'area' in the
    logging infrastructure.

    QLoggingCategory represents a certain logging category - identified by a
    string - at runtime. A category can be configured to enable or disable
    logging of messages per message type. An exception are fatal messages,
    which are always enabled.

    To check whether a message type is enabled or not, use one of these methods:
    \l isDebugEnabled(), \l isInfoEnabled(), \l isWarningEnabled(), and
    \l isCriticalEnabled().

    All objects are meant to be configured by a common registry, as described in
    \l{Configuring Categories}. Different objects can also represent the same
    category. Therefore, it's \b{not} recommended to export objects across
    module boundaries, to manipulate the objects directly, or to inherit from
    QLoggingCategory.

    \section1 Creating Category Objects

    The Q_DECLARE_LOGGING_CATEGORY() and Q_LOGGING_CATEGORY() macros
    conveniently declare and create QLoggingCategory objects:

    \snippet qloggingcategory/main.cpp 1

    There is also the Q_DECLARE_EXPORTED_LOGGING_CATEGORY() macro in
    order to use a logging category across library boundaries.

    Category names are free text; to configure categories using \l{Logging Rules}, their
    names should follow this convention:
    \list
       \li Use letters and numbers only.
       \li Use dots to further structure categories into common areas.
       \li Avoid the category names: \c{debug}, \c{info}, \c{warning}, and \c{critical}.
       \li Category names with the \c{qt} prefix are solely reserved for Qt modules.
    \endlist

    QLoggingCategory objects that are implicitly defined by Q_LOGGING_CATEGORY()
    are created on first use, in a thread-safe manner.

    \section1 Checking Category Configuration

    QLoggingCategory provides \l isDebugEnabled(), \l isInfoEnabled(),
    \l isWarningEnabled(), \l isCriticalEnabled(), as well as \l isEnabled()
    to check whether messages for the given message type should be logged.

    The qCDebug(), qCWarning(), and qCCritical() macros prevent arguments from
    being evaluated if the respective message types are not enabled for the
    category, so explicit checking is not needed:

    \snippet qloggingcategory/main.cpp 4

    \section1 Default Category Configuration

    Both the QLoggingCategory constructor and the Q_LOGGING_CATEGORY() macro
    accept an optional QtMsgType argument, which disables all message types with
    a lower severity. That is, a category declared with

    \snippet qloggingcategory/main.cpp 5

    logs messages of type \c QtWarningMsg, \c QtCriticalMsg, \c QtFatalMsg, but
    ignores messages of type \c QtDebugMsg and \c QtInfoMsg.

    If no argument is passed, all messages are logged. Only Qt internal categories
    which start with \c{qt} are handled differently: For these, only messages of type
    \c QtInfoMsg, \c QtWarningMsg, \c QtCriticalMsg, and \c QFatalMsg are logged by default.

    \note Logging categories are not affected by your C++ build configuration.
    That is, whether messages are printed does not change depending on whether
    the code is compiled with debug symbols ('Debug Build'), optimizations
    ('Release Build'), or some other combination.

    \section1 Configuring Categories

    You can override the default configuration for categories either by setting
    logging rules, or by installing a custom filter.

    \section2 Logging Rules

    Logging rules let you enable or disable logging for categories in a flexible
    way. Rules are specified in text, where every line must have the format:

    \snippet code/src_corelib_io_qloggingcategory.cpp 0

    \c <category> is the name of the category, potentially with \c{*} as a
    wildcard symbol for the first or last character; or at both positions.
    The optional \c <type> must be \c debug, \c info, \c warning, or \c critical.
    Lines that don't fit this scheme are ignored.

    Rules are evaluated in text order, from first to last. That is, if two rules
    apply to a category/type, the rule that comes later is applied.

    Rules can be set via \l setFilterRules():

    \snippet code/src_corelib_io_qloggingcategory.cpp 1

    Logging rules are automatically loaded from the \c [Rules] section in a logging
    configuration file. These configuration files are looked up in the QtProject
    configuration directory, or explicitly set in a \c QT_LOGGING_CONF environment
    variable:

    \snippet code/src_corelib_io_qloggingcategory.cpp 2

    Logging rules can also be specified in a \c QT_LOGGING_RULES environment variable;
    multiple rules can also be separated by semicolons:

    \snippet code/src_corelib_io_qloggingcategory.cpp 3

    Rules set by \l setFilterRules() take precedence over rules specified in the
    QtProject configuration directory. In turn, these rules can be overwritten by those
    from the configuration file specified by \c QT_LOGGING_CONF, and those set by
    \c QT_LOGGING_RULES.

    The order of evaluation is as follows:
    \list 1
        \li [QLibraryInfo::DataPath]/qtlogging.ini
        \li QtProject/qtlogging.ini
        \li \l setFilterRules()
        \li \c QT_LOGGING_CONF
        \li \c QT_LOGGING_RULES
    \endlist

    The \c QtProject/qtlogging.ini file is looked up in all directories returned
    by QStandardPaths::GenericConfigLocation.

    Set the \c QT_LOGGING_DEBUG environment variable to find out where your logging
    rules are loaded from.

    \section2 Installing a Custom Filter

    As a lower-level alternative to the text rules, you can also implement a
    custom filter via \l installFilter(). All filter rules are ignored in this
    case.

    \section1 Printing the Category

    Use the \c %{category} placeholder to print the category in the default
    message handler:

    \snippet qloggingcategory/main.cpp 3
*/

/*!
    Constructs a QLoggingCategory object with the provided \a category name,
    and enables all messages with types at least as verbose as \a enableForLevel,
    which defaults to QtDebugMsg (which enables all categories).

    If \a category is \nullptr, the category name \c "default" is used.

    \note \a category must be kept valid during the lifetime of this object.
    Using a string literal for it is the usual way to achieve this.

    \since 5.4
*/
QLoggingCategory::QLoggingCategory(const char *category, QtMsgType enableForLevel)
    : d(nullptr),
      name(nullptr)
{
    init(category, enableForLevel);
}

void QLoggingCategory::init(const char *category, QtMsgType severityLevel)
{
    enabled.storeRelaxed(0x01010101);   // enabledDebug = enabledWarning = enabledCritical = true;

    if (category)
        name = category;
    else
        name = qtDefaultCategoryName;

    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->registerCategory(this, severityLevel);
}

/*!
    Destroys a QLoggingCategory object.
*/
QLoggingCategory::~QLoggingCategory()
{
    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->unregisterCategory(this);
}

/*!
   \fn const char *QLoggingCategory::categoryName() const

    Returns the name of the category.
*/

/*!
    \fn bool QLoggingCategory::isDebugEnabled() const

    Returns \c true if debug messages should be shown for this category;
    \c false otherwise.

    \note The \l qCDebug() macro already does this check before running any
    code. However, calling this method may be useful to avoid the
    expensive generation of data for debug output only.
*/


/*!
    \fn bool QLoggingCategory::isInfoEnabled() const

    Returns \c true if informational messages should be shown for this category;
    \c false otherwise.

    \note The \l qCInfo() macro already does this check before executing any
    code. However, calling this method may be useful to avoid the
    expensive generation of data for debug output only.

    \since 5.5
*/


/*!
    \fn bool QLoggingCategory::isWarningEnabled() const

    Returns \c true if warning messages should be shown for this category;
    \c false otherwise.

    \note The \l qCWarning() macro already does this check before executing any
    code. However, calling this method may be useful to avoid the
    expensive generation of data for debug output only.
*/

/*!
    \fn bool QLoggingCategory::isCriticalEnabled() const

    Returns \c true if critical messages should be shown for this category;
    \c false otherwise.

    \note The \l qCCritical() macro already does this check before executing any
    code. However, calling this method may be useful to avoid the
    expensive generation of data for debug output only.
*/

/*!
    Returns \c true if a message of type \a msgtype for the category should be
    shown; \c false otherwise.
*/
bool QLoggingCategory::isEnabled(QtMsgType msgtype) const
{
    switch (msgtype) {
    case QtDebugMsg: return isDebugEnabled();
    case QtInfoMsg: return isInfoEnabled();
    case QtWarningMsg: return isWarningEnabled();
    case QtCriticalMsg: return isCriticalEnabled();
    case QtFatalMsg: return true;
    }
    return false;
}

/*!
    Changes the message type \a type for the category to \a enable.

    This method is meant for use only from inside a filter installed with
    \l installFilter(). For an overview on how to configure categories globally,
    see \l {Configuring Categories}.

    \note \c QtFatalMsg cannot be changed; it will always remain \c true.
*/
void QLoggingCategory::setEnabled(QtMsgType type, bool enable)
{
    switch (type) {
    case QtDebugMsg: bools.enabledDebug.storeRelaxed(enable); break;
    case QtInfoMsg: bools.enabledInfo.storeRelaxed(enable); break;
    case QtWarningMsg: bools.enabledWarning.storeRelaxed(enable); break;
    case QtCriticalMsg: bools.enabledCritical.storeRelaxed(enable); break;
    case QtFatalMsg: break;
    }
}

/*!
    \fn QLoggingCategory &QLoggingCategory::operator()()

    Returns the object itself. This allows for both: a QLoggingCategory variable, and
    a factory method that returns a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical(), or \l qCFatal() macros.
 */

/*!
    \fn const QLoggingCategory &QLoggingCategory::operator()() const

    Returns the object itself. This allows for both: a QLoggingCategory variable, and
    a factory method that returns a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical(), or \l qCFatal() macros.
 */

/*!
    Returns a pointer to the global category \c "default" that is used, for
    example, by qDebug(), qInfo(), qWarning(), qCritical(), or qFatal().

    \note The pointer returned may be null during destruction of static objects.
    Also, don't \c delete this pointer, as ownership of the category isn't transferred.

 */
QLoggingCategory *QLoggingCategory::defaultCategory()
{
    return qtDefaultCategory();
}

/*!
    \typedef QLoggingCategory::CategoryFilter

    This is a typedef for a pointer to a function with the following signature:

    \snippet qloggingcategory/main.cpp 20

    A function with this signature can be installed with \l installFilter().
*/

/*!
    \brief Take control of how logging categories are configured.

    Installs a function \a filter that is used to determine which categories and
    message types should be enabled. If \a filter is \nullptr, the default
    message filter is reinstated. Returns a pointer to the previously-installed
    filter.

    Every QLoggingCategory object that already exists is passed to the filter
    before \c installFilter() returns, and the filter is free to change each
    category's configuration with \l setEnabled(). Any category it doesn't
    change will retain the configuration it was given by the prior filter, so
    the new filter does not need to delegate to the prior filter during this
    initial pass over existing categories.

    Any new categories added later will be passed to the new filter; a filter
    that only aims to tweak the configuration of a select few categories, rather
    than completely overriding the logging policy, can first pass the new
    category to the prior filter, to give it its standard configuration, and
    then tweak that as desired, if it is one of the categories of specific
    interest to the filter. The code that installs the new filter can record the
    return from \c installFilter() for the filter to use in such later calls.

    When you define your filter, note that it can be called from different threads; but never
    concurrently. This filter cannot call any static functions from QLoggingCategory.

    Example:
    \snippet qloggingcategory/main.cpp 21

    installed (in \c{main()}, for example) by

    \snippet qloggingcategory/main.cpp 22

    Alternatively, you can configure the default filter via \l setFilterRules().
 */
QLoggingCategory::CategoryFilter
QLoggingCategory::installFilter(QLoggingCategory::CategoryFilter filter)
{
    return QLoggingRegistry::instance()->installFilter(filter);
}

/*!
    Configures which categories and message types should be enabled through a
    set of \a rules.

    Example:

    \snippet qloggingcategory/main.cpp 2

    \note The rules might be ignored if a custom category filter is installed
    with \l installFilter(), or if the user has defined the \c QT_LOGGING_CONF
    or the \c QT_LOGGING_RULES environment variable.
*/
void QLoggingCategory::setFilterRules(const QString &rules)
{
    QLoggingRegistry::instance()->setApiRules(rules);
}

/*!
    \macro qCDebug(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.2

    Returns an output stream for debug messages in the logging category,
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isDebugEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 10

    \note Arguments aren't processed if the debug output for that \a category is not
    enabled, so don't rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCDebug(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.3

    Logs a debug message, \a message, in the logging category, \a category.
    \a message may contain place holders to be replaced by additional arguments,
    similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 13

    \note Arguments aren't processed if the debug output for that \a category is not
    enabled, so don't rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCInfo(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.5

    Returns an output stream for informational messages in the logging category,
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isInfoEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp qcinfo_stream

    \note If the debug output for a particular category isn't enabled, arguments
    won't be processed, so don't rely on any side effects.

    \sa qInfo()
*/

/*!
    \macro qCInfo(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.5

    Logs an informational message, \a message, in the logging category, \a category.
    \a message may contain place holders to be replaced by additional arguments,
    similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp qcinfo_printf

    \note If the debug output for a particular category isn't enabled, arguments
    won't be processed, so don't rely on any side effects.

    \sa qInfo()
*/

/*!
    \macro qCWarning(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.2

    Returns an output stream for warning messages in the logging category,
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isWarningEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 11

    \note If the warning output for a particular category isn't enabled, arguments
    won't be processed, so don't rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCWarning(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.3

    Logs a warning message, \a message, in the logging category, \a category.
    \a message may contain place holders to be replaced by additional arguments,
    similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 14

    \note If the warning output for a particular category isn't enabled, arguments
    won't be processed, so don't rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCCritical(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.2

    Returns an output stream for critical messages in the logging category,
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isCriticalEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 12


    \note If the critical output for a particular category isn't enabled, arguments
    won't be processed, so don't rely on any side effects.

    \sa qCritical()
*/

/*!
    \macro qCCritical(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.3

    Logs a critical message, \a message, in the logging category, \a category.
    \a message may contain place holders to be replaced by additional arguments,
    similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 15

    \note If the critical output for a particular category isn't enabled, arguments
    won't be processed, so don't rely on any side effects.

    \sa qCritical()
*/

/*!
    \macro qCFatal(category)
    \relates QLoggingCategory
    \since 6.5

    Returns an output stream for fatal messages in the logging category,
    \a category.

    If you are using the \b{default message handler}, the returned stream will abort
    to create a core dump. On Windows, for debug builds, this function will
    report a \c _CRT_ERROR enabling you to connect a debugger to the application.

    Example:

    \snippet qloggingcategory/main.cpp 16

    \sa qFatal()
*/

/*!
    \macro qCFatal(category, const char *message, ...)
    \relates QLoggingCategory
    \since 6.5

    Logs a fatal message, \a message, in the logging category, \a category.
    \a message may contain place holders to be replaced by additional arguments,
    similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 17

    If you are using the \b{default message handler}, this function will abort
    to create a core dump. On Windows, for debug builds, this function will
    report a \c _CRT_ERROR enabling you to connect a debugger to the application.

    \sa qFatal()
*/

/*!
    \macro Q_DECLARE_LOGGING_CATEGORY(name)
    \sa Q_LOGGING_CATEGORY(), Q_DECLARE_EXPORTED_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 5.2

    Declares a logging category \a name. The macro can be used to declare
    a common logging category shared in different parts of the program.

    This macro must be used outside of a class or method.
*/

/*!
    \macro Q_DECLARE_EXPORTED_LOGGING_CATEGORY(name, EXPORT_MACRO)
    \sa Q_LOGGING_CATEGORY(), Q_DECLARE_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 6.5

    Declares a logging category \a name. The macro can be used to declare
    a common logging category shared in different parts of the program.

    This works exactly like Q_DECLARE_LOGGING_CATEGORY(). However,
    the logging category declared by this macro is additionally
    qualified with \a EXPORT_MACRO. This is useful if the logging
    category needs to be exported from a dynamic library.

    For example:

    \code
    Q_DECLARE_EXPORTED_LOGGING_CATEGORY(lcCore, LIB_EXPORT_MACRO)
    \endcode

    This macro must be used outside of a class or function.
*/

/*!
    \macro Q_LOGGING_CATEGORY(name, string)
    \sa Q_DECLARE_LOGGING_CATEGORY(), Q_DECLARE_EXPORTED_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 5.2

    Defines a logging category \a name, and makes it configurable under the
    \a string identifier. By default, all message types are enabled.

    Only one translation unit in a library or executable can define a category
    with a specific name. The implicitly-defined QLoggingCategory object is
    created on first use, in a thread-safe manner.

    This macro must be used outside of a class or method.
*/

/*!
    \macro Q_LOGGING_CATEGORY(name, string, msgType)
    \sa Q_DECLARE_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 5.4

    Defines a logging category \a name, and makes it configurable under the
    \a string identifier. By default, messages of QtMsgType \a msgType
    and more severe are enabled, types with a lower severity are disabled.

    Only one translation unit in a library or executable can define a category
    with a specific name. The implicitly-defined QLoggingCategory object is
    created on first use, in a thread-safe manner.

    This macro must be used outside of a class or method.
*/

QT_END_NAMESPACE
