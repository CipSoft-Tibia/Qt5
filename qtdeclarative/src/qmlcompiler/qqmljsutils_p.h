// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSUTILS_P_H
#define QQMLJSUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qtqmlcompilerexports_p.h>

#include "qqmljslogger_p.h"
#include "qqmljsregistercontent_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsmetatypes_p.h"

#include <QtCore/qstack.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstringbuilder.h>
#include <private/qduplicatetracker_p.h>

#include <optional>
#include <functional>
#include <type_traits>
#include <variant>

QT_BEGIN_NAMESPACE

namespace detail {
/*! \internal

    Utility method that returns proper value according to the type To. This
    version returns From.
*/
template<typename To, typename From, typename std::enable_if_t<!std::is_pointer_v<To>, int> = 0>
static auto getQQmlJSScopeFromSmartPtr(const From &p) -> From
{
    static_assert(!std::is_pointer_v<From>, "From has to be a smart pointer holding QQmlJSScope");
    return p;
}

/*! \internal

    Utility method that returns proper value according to the type To. This
    version returns From::get(), which is a raw pointer. The returned type
    is not necessarily equal to To (e.g. To might be `QQmlJSScope *` while
    returned is `const QQmlJSScope *`).
*/
template<typename To, typename From, typename std::enable_if_t<std::is_pointer_v<To>, int> = 0>
static auto getQQmlJSScopeFromSmartPtr(const From &p) -> decltype(p.get())
{
    static_assert(!std::is_pointer_v<From>, "From has to be a smart pointer holding QQmlJSScope");
    return p.get();
}
}

class QQmlJSTypeResolver;
class QQmlJSScopesById;
struct Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSUtils
{
    /*! \internal
        Returns escaped version of \a s. This function is mostly useful for code
        generators.
    */
    static QString escapeString(QString s)
    {
        using namespace Qt::StringLiterals;
        return s.replace(u'\\', u"\\\\"_s).replace(u'"', u"\\\""_s).replace(u'\n', u"\\n"_s);
    }

    /*! \internal
        Returns \a s wrapped into a literal macro specified by \a ctor. By
        default, returns a QStringLiteral-wrapped literal. This function is
        mostly useful for code generators.

        \note This function escapes \a s before wrapping it.
    */
    static QString toLiteral(const QString &s, QStringView ctor = u"QStringLiteral")
    {
        return ctor % u"(\"" % escapeString(s) % u"\")";
    }

    /*! \internal
        Returns \a type string conditionally wrapped into \c{const} and \c{&}.
        This function is mostly useful for code generators.
    */
    static QString constRefify(QString type)
    {
        if (!type.endsWith(u'*'))
            type = u"const " % type % u"&";
        return type;
    }

    /*! \internal
        Returns a signal name from \a handlerName string.
    */
    static std::optional<QString> signalName(QStringView handlerName)
    {
        if (handlerName.startsWith(u"on") && handlerName.size() > 2) {
            QString signal = handlerName.mid(2).toString();
            for (int i = 0; i < signal.size(); ++i) {
                QChar &ch = signal[i];
                if (ch.isLower())
                    return {};
                if (ch.isUpper()) {
                    ch = ch.toLower();
                    return signal;
                }
            }
        }
        return {};
    }

    static std::optional<QQmlJSMetaProperty>
    changeHandlerProperty(const QQmlJSScope::ConstPtr &scope, QStringView signalName)
    {
        if (!signalName.endsWith(QLatin1String("Changed")))
            return {};
        constexpr int length = int(sizeof("Changed") / sizeof(char)) - 1;
        signalName.chop(length);
        auto p = scope->property(signalName.toString());
        const bool isBindable = !p.bindable().isEmpty();
        const bool canNotify = !p.notify().isEmpty();
        if (p.isValid() && (isBindable || canNotify))
            return p;
        return {};
    }

    static bool hasCompositeBase(const QQmlJSScope::ConstPtr &scope)
    {
        if (!scope)
            return false;
        const auto base = scope->baseType();
        if (!base)
            return false;
        return base->isComposite() && base->scopeType() == QQmlSA::ScopeType::QMLScope;
    }

    enum PropertyAccessor {
        PropertyAccessor_Read,
        PropertyAccessor_Write,
    };
    /*! \internal

        Returns \c true if \a p is bindable and property accessor specified by
        \a accessor is equal to "default". Returns \c false otherwise.

        \note This function follows BINDABLE-only properties logic (e.g. in moc)
    */
    static bool bindablePropertyHasDefaultAccessor(const QQmlJSMetaProperty &p,
                                                   PropertyAccessor accessor)
    {
        if (p.bindable().isEmpty())
            return false;
        switch (accessor) {
        case PropertyAccessor::PropertyAccessor_Read:
            return p.read() == QLatin1String("default");
        case PropertyAccessor::PropertyAccessor_Write:
            return p.write() == QLatin1String("default");
        default:
            break;
        }
        return false;
    }

    enum ResolvedAliasTarget {
        AliasTarget_Invalid,
        AliasTarget_Property,
        AliasTarget_Object,
    };
    struct ResolvedAlias
    {
        QQmlJSMetaProperty property;
        QQmlJSScope::ConstPtr owner;
        ResolvedAliasTarget kind = ResolvedAliasTarget::AliasTarget_Invalid;
    };
    struct AliasResolutionVisitor
    {
        std::function<void()> reset = []() {};
        std::function<void(const QQmlJSScope::ConstPtr &)> processResolvedId =
                [](const QQmlJSScope::ConstPtr &) {};
        std::function<void(const QQmlJSMetaProperty &, const QQmlJSScope::ConstPtr &)>
                processResolvedProperty =
                        [](const QQmlJSMetaProperty &, const QQmlJSScope::ConstPtr &) {};
    };
    static ResolvedAlias resolveAlias(const QQmlJSTypeResolver *typeResolver,
                                      const QQmlJSMetaProperty &property,
                                      const QQmlJSScope::ConstPtr &owner,
                                      const AliasResolutionVisitor &visitor);
    static ResolvedAlias resolveAlias(const QQmlJSScopesById &idScopes,
                                      const QQmlJSMetaProperty &property,
                                      const QQmlJSScope::ConstPtr &owner,
                                      const AliasResolutionVisitor &visitor);

    template<typename QQmlJSScopePtr, typename Action>
    static bool searchBaseAndExtensionTypes(QQmlJSScopePtr type, const Action &check)
    {
        if (!type)
            return false;

        using namespace detail;

        // NB: among other things, getQQmlJSScopeFromSmartPtr() also resolves const
        // vs non-const pointer issue, so use it's return value as the type
        using T = decltype(getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(
                std::declval<QQmlJSScope::ConstPtr>()));

        const auto checkWrapper = [&](const auto &scope, QQmlJSScope::ExtensionKind mode) {
            if constexpr (std::is_invocable<Action, decltype(scope),
                                            QQmlJSScope::ExtensionKind>::value) {
                return check(scope, mode);
            } else {
                static_assert(std::is_invocable<Action, decltype(scope)>::value,
                              "Inferred type Action has unexpected arguments");
                Q_UNUSED(mode);
                return check(scope);
            }
        };

        const bool isValueOrSequenceType = [type]() {
            switch (type->accessSemantics()) {
            case QQmlJSScope::AccessSemantics::Value:
            case QQmlJSScope::AccessSemantics::Sequence:
                return true;
            default:
                break;
            }
            return false;
        }();

        QDuplicateTracker<T> seen;
        for (T scope = type; scope && !seen.hasSeen(scope);
             scope = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(scope->baseType())) {
            QDuplicateTracker<T> seenExtensions;
            // Extensions override the types they extend. However, usually base
            // types of extensions are ignored. The unusual cases are when we
            // have a value or sequence type or when we have the QObject type, in which
            // case we also study the extension's base type hierarchy.
            const bool isQObject = scope->internalName() == QLatin1String("QObject");
            auto [extensionPtr, extensionKind] = scope->extensionType();
            auto extension = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(extensionPtr);
            do {
                if (!extension || seenExtensions.hasSeen(extension))
                    break;

                if (checkWrapper(extension, extensionKind))
                    return true;
                extension = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(extension->baseType());
            } while (isValueOrSequenceType || isQObject);

            if (checkWrapper(scope, QQmlJSScope::NotExtension))
                return true;
        }

        return false;
    }

    template<typename Action>
    static void traverseFollowingQmlIrObjectStructure(const QQmlJSScope::Ptr &root, Action act)
    {
        // We *have* to perform DFS here: QmlIR::Object entries within the
        // QmlIR::Document are stored in the order they appear during AST traversal
        // (which does DFS)
        QStack<QQmlJSScope::Ptr> stack;
        stack.push(root);

        while (!stack.isEmpty()) {
            QQmlJSScope::Ptr current = stack.pop();

            act(current);

            auto children = current->childScopes();
            // arrays are special: they are reverse-processed in QmlIRBuilder
            if (!current->isArrayScope())
                std::reverse(children.begin(), children.end()); // left-to-right DFS
            stack.append(std::move(children));
        }
    }

    /*! \internal

        Traverses the base types and extensions of \a scope in the order aligned
        with QMetaObjects created at run time for these types and extensions
        (except that QQmlVMEMetaObject is ignored). \a start is the starting
        type in the hierarchy where \a act is applied.

        \note To call \a act for every type in the hierarchy, use
        scope->extensionType().scope as \a start
    */
    template<typename Action>
    static void traverseFollowingMetaObjectHierarchy(const QQmlJSScope::ConstPtr &scope,
                                                     const QQmlJSScope::ConstPtr &start, Action act)
    {
        // Meta objects are arranged in the following way:
        // * static meta objects are chained first
        // * dynamic meta objects are added on top - they come from extensions.
        //   QQmlVMEMetaObject ignored here
        //
        // Example:
        // ```
        //   class A : public QObject {
        //       QML_EXTENDED(Ext)
        //   };
        //   class B : public A {
        //       QML_EXTENDED(Ext2)
        //   };
        // ```
        // gives: Ext2 -> Ext -> B -> A -> QObject
        //        ^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^
        //        ^^^^^^^^^^^    static meta objects
        //        dynamic meta objects

        using namespace Qt::StringLiterals;
        // ignore special extensions
        const QLatin1String ignoredExtensionNames[] = {
            // QObject extensions: (not related to C++)
            "Object"_L1,
            "ObjectPrototype"_L1,
        };

        QList<QQmlJSScope::AnnotatedScope> types;
        QList<QQmlJSScope::AnnotatedScope> extensions;
        const auto collect = [&](const QQmlJSScope::ConstPtr &type, QQmlJSScope::ExtensionKind m) {
            if (m == QQmlJSScope::NotExtension) {
                types.append(QQmlJSScope::AnnotatedScope { type, m });
                return false;
            }

            for (const auto &name : ignoredExtensionNames) {
                if (type->internalName() == name)
                    return false;
            }
            extensions.append(QQmlJSScope::AnnotatedScope { type, m });
            return false;
        };
        searchBaseAndExtensionTypes(scope, collect);

        QList<QQmlJSScope::AnnotatedScope> all;
        all.reserve(extensions.size() + types.size());
        // first extensions then types
        all.append(std::move(extensions));
        all.append(std::move(types));

        auto begin = all.cbegin();
        // skip to start
        while (begin != all.cend() && !begin->scope->isSameType(start))
            ++begin;

        // iterate over extensions and types starting at a specified point
        for (; begin != all.cend(); ++begin)
            act(begin->scope, begin->extensionSpecifier);
    }

    static std::optional<QQmlJSFixSuggestion> didYouMean(const QString &userInput,
                                                   QStringList candidates,
                                                   QQmlJS::SourceLocation location);

    static std::variant<QString, QQmlJS::DiagnosticMessage>
    sourceDirectoryPath(const QQmlJSImporter *importer, const QString &buildDirectoryPath);
};

bool Q_QMLCOMPILER_PRIVATE_EXPORT canStrictlyCompareWithVar(
        const QQmlJSTypeResolver *typeResolver, const QQmlJSRegisterContent &lhsContent,
        const QQmlJSRegisterContent &rhsContent);

bool Q_QMLCOMPILER_PRIVATE_EXPORT canCompareWithQObject(const QQmlJSTypeResolver *typeResolver,
                                                        const QQmlJSRegisterContent &lhsContent,
                                                        const QQmlJSRegisterContent &rhsContent);

bool Q_QMLCOMPILER_PRIVATE_EXPORT canCompareWithQUrl(const QQmlJSTypeResolver *typeResolver,
                                                     const QQmlJSRegisterContent &lhsContent,
                                                     const QQmlJSRegisterContent &rhsContent);

QT_END_NAMESPACE

#endif // QQMLJSUTILS_P_H
