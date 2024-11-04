// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSSCOPE_P_H
#define QQMLJSSCOPE_P_H

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

#include "qqmljsmetatypes_p.h"
#include "qdeferredpointer_p.h"
#include "qqmljsannotation_p.h"
#include "qqmlsaconstants.h"
#include "qqmlsa_p.h"

#include <QtQml/private/qqmljssourcelocation_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>
#include "qqmlsaconstants.h"

#include <optional>

QT_BEGIN_NAMESPACE

class QQmlJSImporter;

namespace QQmlJS {

class ConstPtrWrapperIterator
{
public:
    using Ptr = QDeferredSharedPointer<QQmlJSScope>;
    using ConstPtr = QDeferredSharedPointer<const QQmlJSScope>;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ConstPtr;
    using pointer = value_type *;
    using reference = value_type &;

    ConstPtrWrapperIterator(QList<Ptr>::const_iterator iterator) : m_iterator(iterator) { }

    friend bool operator==(const ConstPtrWrapperIterator &a, const ConstPtrWrapperIterator &b)
    {
        return a.m_iterator == b.m_iterator;
    }
    friend bool operator!=(const ConstPtrWrapperIterator &a, const ConstPtrWrapperIterator &b)
    {
        return a.m_iterator != b.m_iterator;
    }

    reference operator*()
    {
        if (!m_pointer)
            m_pointer = *m_iterator;
        return m_pointer;
    }
    pointer operator->()
    {
        if (!m_pointer)
            m_pointer = *m_iterator;
        return &m_pointer;
    }

    ConstPtrWrapperIterator &operator++()
    {
        m_iterator++;
        m_pointer = {};
        return *this;
    }
    ConstPtrWrapperIterator operator++(int)
    {
        auto before = *this;
        ++(*this);
        return before;
    }

private:
    QList<Ptr>::const_iterator m_iterator;
    ConstPtr m_pointer;
};

class Export {
public:
    Export() = default;
    Export(QString package, QString type, QTypeRevision version, QTypeRevision revision);

    bool isValid() const;

    QString package() const { return m_package; }
    QString type() const { return m_type; }
    QTypeRevision version() const { return m_version; }
    QTypeRevision revision() const { return m_revision; }

private:
    QString m_package;
    QString m_type;
    QTypeRevision m_version;
    QTypeRevision m_revision;
};

template<typename Pointer>
struct ExportedScope {
    Pointer scope;
    QList<Export> exports;
};

template<typename Pointer>
struct ImportedScope {
    Pointer scope;
    QTypeRevision revision;
};

struct ContextualTypes;

} // namespace QQmlJS

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSScope
{
    friend QQmlSA::Element;

public:
    explicit QQmlJSScope(const QString &internalName);
    QQmlJSScope(QQmlJSScope &&) = default;
    QQmlJSScope &operator=(QQmlJSScope &&) = default;

    using Ptr = QDeferredSharedPointer<QQmlJSScope>;
    using WeakPtr = QDeferredWeakPointer<QQmlJSScope>;
    using ConstPtr = QDeferredSharedPointer<const QQmlJSScope>;
    using WeakConstPtr = QDeferredWeakPointer<const QQmlJSScope>;

    using AccessSemantics = QQmlSA::AccessSemantics;
    using ScopeType = QQmlSA::ScopeType;

    using InlineComponentNameType = QString;
    using RootDocumentNameType = std::monostate; // an empty type that has std::hash
    /*!
     *  A Hashable type to differentiate document roots from different inline components.
     */
    using InlineComponentOrDocumentRootName =
            std::variant<InlineComponentNameType, RootDocumentNameType>;

    enum Flag {
        Creatable = 0x1,
        Composite = 0x2,
        Singleton = 0x4,
        Script = 0x8,
        CustomParser = 0x10,
        Array = 0x20,
        InlineComponent = 0x40,
        WrappedInImplicitComponent = 0x80,
        HasBaseTypeError = 0x100,
        ExtensionIsNamespace = 0x200,
        IsListProperty = 0x400,
        Structured = 0x800,
        ExtensionIsJavaScript = 0x1000,
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAGS(Flags);

    using Export = QQmlJS::Export;
    template <typename Pointer>
    using ImportedScope = QQmlJS::ImportedScope<Pointer>;
    template <typename Pointer>
    using ExportedScope = QQmlJS::ExportedScope<Pointer>;

    struct JavaScriptIdentifier
    {
        enum Kind {
            Parameter,
            FunctionScoped,
            LexicalScoped,
            Injected
        };

        Kind kind = FunctionScoped;
        QQmlJS::SourceLocation location;
        std::optional<QString> typeName;
        bool isConst = false;
        QQmlJSScope::WeakConstPtr scope = {};
    };

    enum BindingTargetSpecifier {
        SimplePropertyTarget, // e.g. `property int p: 42`
        ListPropertyTarget, // e.g. `property list<Item> pList: [ Text {} ]`
        UnnamedPropertyTarget // default property bindings, where property name is unspecified
    };

    template <typename Key, typename Value>
    using QMultiHashRange = QPair<typename QMultiHash<Key, Value>::iterator,
                                  typename QMultiHash<Key, Value>::iterator>;

    static QQmlJSScope::Ptr create() { return QSharedPointer<QQmlJSScope>(new QQmlJSScope); }
    static QQmlJSScope::Ptr create(const QString &internalName);
    static QQmlJSScope::Ptr clone(const QQmlJSScope::ConstPtr &origin);

    static QQmlJSScope::ConstPtr findCurrentQMLScope(const QQmlJSScope::ConstPtr &scope);

    QQmlJSScope::Ptr parentScope();
    QQmlJSScope::ConstPtr parentScope() const;
    static void reparent(const QQmlJSScope::Ptr &parentScope, const QQmlJSScope::Ptr &childScope);

    void insertJSIdentifier(const QString &name, const JavaScriptIdentifier &identifier);
    QHash<QString, JavaScriptIdentifier> ownJSIdentifiers() const;
    void insertPropertyIdentifier(const QQmlJSMetaProperty &prop);

    ScopeType scopeType() const { return m_scopeType; }
    void setScopeType(ScopeType type) { m_scopeType = type; }

    void addOwnMethod(const QQmlJSMetaMethod &method) { m_methods.insert(method.methodName(), method); }
    QMultiHashRange<QString, QQmlJSMetaMethod> mutableOwnMethodsRange(const QString &name)
    {
        return m_methods.equal_range(name);
    }
    QMultiHash<QString, QQmlJSMetaMethod> ownMethods() const { return m_methods; }
    QList<QQmlJSMetaMethod> ownMethods(const QString &name) const { return m_methods.values(name); }
    bool hasOwnMethod(const QString &name) const { return m_methods.contains(name); }

    bool hasMethod(const QString &name) const;
    QHash<QString, QQmlJSMetaMethod> methods() const;
    QList<QQmlJSMetaMethod> methods(const QString &name) const;
    QList<QQmlJSMetaMethod> methods(const QString &name, QQmlJSMetaMethodType type) const;

    void addOwnEnumeration(const QQmlJSMetaEnum &enumeration) { m_enumerations.insert(enumeration.name(), enumeration); }
    QHash<QString, QQmlJSMetaEnum> ownEnumerations() const { return m_enumerations; }
    QQmlJSMetaEnum ownEnumeration(const QString &name) const { return m_enumerations.value(name); }
    bool hasOwnEnumeration(const QString &name) const { return m_enumerations.contains(name); }

    bool hasEnumeration(const QString &name) const;
    bool hasEnumerationKey(const QString &name) const;
    QQmlJSMetaEnum enumeration(const QString &name) const;
    QHash<QString, QQmlJSMetaEnum> enumerations() const;

    void setAnnotations(const QList<QQmlJSAnnotation> &annotation) { m_annotations = std::move(annotation); }
    const QList<QQmlJSAnnotation> &annotations() const { return m_annotations; }

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &file) { m_filePath = file; }

    // The name the type uses to refer to itself. Either C++ class name or base name of
    // QML file. isComposite tells us if this is a C++ or a QML name.
    QString internalName() const { return m_internalName; }
    void setInternalName(const QString &internalName) { m_internalName = internalName; }
    QString augmentedInternalName() const;

    // This returns a more user readable version of internalName / baseTypeName
    static QString prettyName(QAnyStringView name);

    bool isComponentRootElement() const;

    void setInterfaceNames(const QStringList& interfaces) { m_interfaceNames = interfaces; }
    QStringList interfaceNames() const { return m_interfaceNames; }

    bool hasInterface(const QString &name) const;
    bool hasOwnInterface(const QString &name) const { return m_interfaceNames.contains(name); }

    void setOwnDeferredNames(const QStringList &names) { m_ownDeferredNames = names; }
    QStringList ownDeferredNames() const { return m_ownDeferredNames; }
    void setOwnImmediateNames(const QStringList &names) { m_ownImmediateNames = names; }
    QStringList ownImmediateNames() const { return m_ownImmediateNames; }

    bool isNameDeferred(const QString &name) const;

    // If isComposite(), this is the QML/JS name of the prototype. Otherwise it's the
    // relevant base class (in the hierarchy starting from QObject) of a C++ type.
    void setBaseTypeName(const QString &baseTypeName);
    QString baseTypeName() const;

    QQmlJSScope::ConstPtr baseType() const { return m_baseType.scope; }
    QTypeRevision baseTypeRevision() const { return m_baseType.revision; }

    QString moduleName() const { return m_moduleName; }
    void setModuleName(const QString &moduleName) { m_moduleName = moduleName; }

    void clearBaseType() { m_baseType = {}; }
    void setBaseTypeError(const QString &baseTypeError);
    QString baseTypeError() const;

    void addOwnProperty(const QQmlJSMetaProperty &prop) { m_properties.insert(prop.propertyName(), prop); }
    QHash<QString, QQmlJSMetaProperty> ownProperties() const { return m_properties; }
    QQmlJSMetaProperty ownProperty(const QString &name) const { return m_properties.value(name); }
    bool hasOwnProperty(const QString &name) const { return m_properties.contains(name); }

    bool hasProperty(const QString &name) const;
    QQmlJSMetaProperty property(const QString &name) const;
    QHash<QString, QQmlJSMetaProperty> properties() const;

    void setPropertyLocallyRequired(const QString &name, bool isRequired);
    bool isPropertyRequired(const QString &name) const;
    bool isPropertyLocallyRequired(const QString &name) const;

    void addOwnPropertyBinding(
            const QQmlJSMetaPropertyBinding &binding,
            BindingTargetSpecifier specifier = BindingTargetSpecifier::SimplePropertyTarget);
    QMultiHash<QString, QQmlJSMetaPropertyBinding> ownPropertyBindings() const;
    QPair<QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator,
          QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator>
    ownPropertyBindings(const QString &name) const;
    QList<QQmlJSMetaPropertyBinding> ownPropertyBindingsInQmlIROrder() const;
    bool hasOwnPropertyBindings(const QString &name) const;

    bool hasPropertyBindings(const QString &name) const;
    QList<QQmlJSMetaPropertyBinding> propertyBindings(const QString &name) const;

    struct AnnotatedScope; // defined later
    static AnnotatedScope ownerOfProperty(const QQmlJSScope::ConstPtr &self, const QString &name);

    bool isResolved() const;
    bool isFullyResolved() const;

    QString ownDefaultPropertyName() const { return m_defaultPropertyName; }
    void setOwnDefaultPropertyName(const QString &name) { m_defaultPropertyName = name; }
    QString defaultPropertyName() const;

    QString ownParentPropertyName() const { return m_parentPropertyName; }
    void setOwnParentPropertyName(const QString &name) { m_parentPropertyName = name; }
    QString parentPropertyName() const;

    QString ownAttachedTypeName() const { return m_attachedTypeName; }
    void setOwnAttachedTypeName(const QString &name) { m_attachedTypeName = name; }
    QQmlJSScope::ConstPtr ownAttachedType() const { return m_attachedType; }

    QString attachedTypeName() const;
    QQmlJSScope::ConstPtr attachedType() const;

    QString extensionTypeName() const { return m_extensionTypeName; }
    void setExtensionTypeName(const QString &name) { m_extensionTypeName = name; }
    enum ExtensionKind {
        NotExtension,
        ExtensionType,
        ExtensionJavaScript,
        ExtensionNamespace,
    };
    struct AnnotatedScope
    {
        QQmlJSScope::ConstPtr scope;
        ExtensionKind extensionSpecifier = NotExtension;
    };
    AnnotatedScope extensionType() const;

    QString valueTypeName() const { return m_valueTypeName; }
    void setValueTypeName(const QString &name) { m_valueTypeName = name; }
    QQmlJSScope::ConstPtr valueType() const { return m_valueType; }
    QQmlJSScope::ConstPtr listType() const { return m_listType; }
    QQmlJSScope::Ptr listType() { return m_listType; }

    void addOwnRuntimeFunctionIndex(QQmlJSMetaMethod::AbsoluteFunctionIndex index);
    QQmlJSMetaMethod::AbsoluteFunctionIndex
    ownRuntimeFunctionIndex(QQmlJSMetaMethod::RelativeFunctionIndex index) const;


    /*!
     * \internal
     *
     * Returns true for objects defined from Qml, and false for objects declared from C++.
     */
    bool isComposite() const { return m_flags & Composite; }
    bool isScript() const { return m_flags & Script; }
    bool hasCustomParser() const { return m_flags & CustomParser; }
    bool isArrayScope() const { return m_flags & Array; }
    bool isInlineComponent() const { return m_flags & InlineComponent; }
    bool isWrappedInImplicitComponent() const { return m_flags & WrappedInImplicitComponent; }
    bool extensionIsJavaScript() const { return m_flags & ExtensionIsJavaScript; }
    bool extensionIsNamespace() const { return m_flags & ExtensionIsNamespace; }
    bool isListProperty() const { return m_flags.testFlag(IsListProperty); }
    void setIsListProperty(bool v) { m_flags.setFlag(IsListProperty, v); }
    bool isSingleton() const { return m_flags & Singleton; }
    bool isCreatable() const;
    bool isStructured() const;
    bool isReferenceType() const { return m_semantics == QQmlJSScope::AccessSemantics::Reference; }
    bool isValueType() const { return m_semantics == QQmlJSScope::AccessSemantics::Value; }

    void setIsSingleton(bool v) { m_flags.setFlag(Singleton, v); }
    void setCreatableFlag(bool v) { m_flags.setFlag(Creatable, v); }
    void setStructuredFlag(bool v) { m_flags.setFlag(Structured, v); }
    void setIsComposite(bool v) { m_flags.setFlag(Composite, v); }
    void setIsScript(bool v) { m_flags.setFlag(Script, v); }
    void setHasCustomParser(bool v);
    void setIsArrayScope(bool v) { m_flags.setFlag(Array, v); }
    void setIsInlineComponent(bool v) { m_flags.setFlag(InlineComponent, v); }
    void setIsWrappedInImplicitComponent(bool v) { m_flags.setFlag(WrappedInImplicitComponent, v); }
    void setExtensionIsJavaScript(bool v) { m_flags.setFlag(ExtensionIsJavaScript, v); }
    void setExtensionIsNamespace(bool v) { m_flags.setFlag(ExtensionIsNamespace, v); }


    void setAccessSemantics(AccessSemantics semantics) { m_semantics = semantics; }
    AccessSemantics accessSemantics() const { return m_semantics; }

    std::optional<JavaScriptIdentifier> jsIdentifier(const QString &id) const;
    std::optional<JavaScriptIdentifier> ownJSIdentifier(const QString &id) const;

    QQmlJS::ConstPtrWrapperIterator childScopesBegin() const { return m_childScopes.constBegin(); }
    QQmlJS::ConstPtrWrapperIterator childScopesEnd() const { return m_childScopes.constEnd(); }

    void setInlineComponentName(const QString &inlineComponentName);
    std::optional<QString> inlineComponentName() const;
    InlineComponentOrDocumentRootName enclosingInlineComponentName() const;

    QVector<QQmlJSScope::Ptr> childScopes();

    QVector<QQmlJSScope::ConstPtr> childScopes() const;

    static QTypeRevision resolveTypes(
            const Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
            QSet<QString> *usedTypes = nullptr);
    static void resolveNonEnumTypes(
            const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
            QSet<QString> *usedTypes = nullptr);
    static void resolveEnums(
            const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
            QSet<QString> *usedTypes = nullptr);
    static void resolveList(
            const QQmlJSScope::Ptr &self, const QQmlJSScope::ConstPtr &arrayType);
    static void resolveGeneralizedGroup(
            const QQmlJSScope::Ptr &self, const QQmlJSScope::ConstPtr &baseType,
            const QQmlJS::ContextualTypes &contextualTypes,
            QSet<QString> *usedTypes = nullptr);

    void setSourceLocation(const QQmlJS::SourceLocation &sourceLocation);
    QQmlJS::SourceLocation sourceLocation() const;

    static QQmlJSScope::ConstPtr nonCompositeBaseType(const QQmlJSScope::ConstPtr &type);

    static QTypeRevision
    nonCompositeBaseRevision(const ImportedScope<QQmlJSScope::ConstPtr> &scope);

    bool isSameType(const QQmlJSScope::ConstPtr &otherScope) const;
    bool inherits(const QQmlJSScope::ConstPtr &base) const;
    bool canAssign(const QQmlJSScope::ConstPtr &derived) const;

    bool isInCustomParserParent() const;


    static ImportedScope<QQmlJSScope::ConstPtr> findType(const QString &name,
                                                         const QQmlJS::ContextualTypes &contextualTypes,
                                                         QSet<QString> *usedTypes = nullptr);

    static QQmlSA::Element createQQmlSAElement(const ConstPtr &);
    static QQmlSA::Element createQQmlSAElement(ConstPtr &&);
    static const QQmlJSScope::ConstPtr &scope(const QQmlSA::Element &);
    static constexpr qsizetype sizeofQQmlSAElement() { return QQmlSA::Element::sizeofElement; }

private:
    /*! \internal

         Minimal information about a QQmlJSMetaPropertyBinding that allows it to
         be manipulated similarly to QmlIR::Binding.
     */
    template <typename T>
    friend class QTypeInfo; // so that we can Q_DECLARE_TYPEINFO QmlIRCompatibilityBindingData
    struct QmlIRCompatibilityBindingData
    {
        QmlIRCompatibilityBindingData() = default;
        QmlIRCompatibilityBindingData(const QString &name, quint32 offset)
            : propertyName(name), sourceLocationOffset(offset)
        {
        }
        QString propertyName; // bound property name
        quint32 sourceLocationOffset = 0; // binding's source location offset
    };

    QQmlJSScope() = default;
    QQmlJSScope(const QQmlJSScope &) = default;
    QQmlJSScope &operator=(const QQmlJSScope &) = default;
    static QTypeRevision resolveType(
            const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
            QSet<QString> *usedTypes);
    static void updateChildScope(
            const QQmlJSScope::Ptr &childScope, const QQmlJSScope::Ptr &self,
            const QQmlJS::ContextualTypes &contextualTypes, QSet<QString> *usedTypes);

    void addOwnPropertyBindingInQmlIROrder(const QQmlJSMetaPropertyBinding &binding,
                                           BindingTargetSpecifier specifier);
    bool hasCreatableFlag() const { return m_flags & Creatable; }
    bool hasStructuredFlag() const { return m_flags & Structured; }

    QHash<QString, JavaScriptIdentifier> m_jsIdentifiers;

    QMultiHash<QString, QQmlJSMetaMethod> m_methods;
    QHash<QString, QQmlJSMetaProperty> m_properties;
    QMultiHash<QString, QQmlJSMetaPropertyBinding> m_propertyBindings;

    // a special QmlIR compatibility bindings array, ordered the same way as
    // bindings in QmlIR::Object
    QList<QmlIRCompatibilityBindingData> m_propertyBindingsArray;

    // same as QmlIR::Object::runtimeFunctionIndices
    QList<QQmlJSMetaMethod::AbsoluteFunctionIndex> m_runtimeFunctionIndices;

    QHash<QString, QQmlJSMetaEnum> m_enumerations;

    QVector<QQmlJSAnnotation> m_annotations;
    QVector<QQmlJSScope::Ptr> m_childScopes;
    QQmlJSScope::WeakPtr m_parentScope;

    QString m_filePath;
    QString m_internalName;
    QString m_baseTypeNameOrError;

    // We only need the revision for the base type as inheritance is
    // the only relation between two types where the revisions matter.
    ImportedScope<QQmlJSScope::WeakConstPtr> m_baseType;

    ScopeType m_scopeType = ScopeType::QMLScope;
    QStringList m_interfaceNames;
    QStringList m_ownDeferredNames;
    QStringList m_ownImmediateNames;

    QString m_defaultPropertyName;
    QString m_parentPropertyName;
    /*! \internal
     *  The attached type name.
     *  This is an internal name, from a c++ type or a synthetic jsrootgen.
     */
    QString m_attachedTypeName;
    QStringList m_requiredPropertyNames;
    QQmlJSScope::WeakConstPtr m_attachedType;

    /*! \internal
     *  The Value type name.
     *  This is an internal name, from a c++ type or a synthetic jsrootgen.
     */
    QString m_valueTypeName;
    QQmlJSScope::WeakConstPtr m_valueType;
    QQmlJSScope::Ptr m_listType;

    /*!
       The extension is provided as either a type (QML_{NAMESPACE_}EXTENDED) or as a
       namespace (QML_EXTENDED_NAMESPACE).
       The bool HasExtensionNamespace helps differentiating both cases, as namespaces
       have a more limited lookup capaility.
       This is an internal name, from a c++ type or a synthetic jsrootgen.
    */
    QString m_extensionTypeName;
    QQmlJSScope::WeakConstPtr m_extensionType;

    Flags m_flags = Creatable; // all types are marked as creatable by default.
    AccessSemantics m_semantics = AccessSemantics::Reference;

    QQmlJS::SourceLocation m_sourceLocation;

    QString m_moduleName;

    std::optional<QString> m_inlineComponentName;
};

inline QQmlJSScope::Ptr QQmlJSScope::parentScope()
{
    return m_parentScope.toStrongRef();
}

inline QQmlJSScope::ConstPtr QQmlJSScope::parentScope() const
{
    QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU < 1400 && Q_CC_GNU >= 1200
            QT_WARNING_DISABLE_GCC("-Wuse-after-free")
#endif
            return QQmlJSScope::WeakConstPtr(m_parentScope).toStrongRef();
    QT_WARNING_POP
}

inline QMultiHash<QString, QQmlJSMetaPropertyBinding> QQmlJSScope::ownPropertyBindings() const
{
            return m_propertyBindings;
}

inline QPair<QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator, QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator> QQmlJSScope::ownPropertyBindings(const QString &name) const
{
    return m_propertyBindings.equal_range(name);
}

inline bool QQmlJSScope::hasOwnPropertyBindings(const QString &name) const
{
            return m_propertyBindings.contains(name);
}

inline QQmlJSMetaMethod::AbsoluteFunctionIndex QQmlJSScope::ownRuntimeFunctionIndex(QQmlJSMetaMethod::RelativeFunctionIndex index) const
{
    const int i = static_cast<int>(index);
    Q_ASSERT(i >= 0);
    Q_ASSERT(i < int(m_runtimeFunctionIndices.size()));
    return m_runtimeFunctionIndices[i];
}

inline void QQmlJSScope::setHasCustomParser(bool v)
{
    m_flags.setFlag(CustomParser, v);;
}

inline void QQmlJSScope::setInlineComponentName(const QString &inlineComponentName)
{
    Q_ASSERT(isInlineComponent());
    m_inlineComponentName = inlineComponentName;
}

inline QVector<QQmlJSScope::Ptr> QQmlJSScope::childScopes()
{
    return m_childScopes;
}

inline void QQmlJSScope::setSourceLocation(const QQmlJS::SourceLocation &sourceLocation)
{
    m_sourceLocation = sourceLocation;
}

inline QQmlJS::SourceLocation QQmlJSScope::sourceLocation() const
{
    return m_sourceLocation;
}

inline QQmlJSScope::ConstPtr QQmlJSScope::nonCompositeBaseType(const ConstPtr &type)
{
    for (QQmlJSScope::ConstPtr base = type; base; base = base->baseType()) {
        if (!base->isComposite())
            return base;
    }
    return {};
}

Q_DECLARE_TYPEINFO(QQmlJSScope::QmlIRCompatibilityBindingData, Q_RELOCATABLE_TYPE);

template<>
class Q_QMLCOMPILER_PRIVATE_EXPORT QDeferredFactory<QQmlJSScope>
{
public:
    QDeferredFactory() = default;

    QDeferredFactory(QQmlJSImporter *importer, const QString &filePath) :
        m_filePath(filePath), m_importer(importer)
    {}

    bool isValid() const
    {
        return !m_filePath.isEmpty() && m_importer != nullptr;
    }

    QString internalName() const
    {
        return QFileInfo(m_filePath).baseName();
    }

    void setIsSingleton(bool isSingleton)
    {
        m_isSingleton = isSingleton;
    }

    void setModuleName(const QString &moduleName) { m_moduleName = moduleName; }

private:
    friend class QDeferredSharedPointer<QQmlJSScope>;
    friend class QDeferredSharedPointer<const QQmlJSScope>;
    friend class QDeferredWeakPointer<QQmlJSScope>;
    friend class QDeferredWeakPointer<const QQmlJSScope>;

    // Should only be called when lazy-loading the type in a deferred pointer.
    void populate(const QSharedPointer<QQmlJSScope> &scope) const;

    QString m_filePath;
    QQmlJSImporter *m_importer = nullptr;
    bool m_isSingleton = false;
    QString m_moduleName;
};

using QQmlJSExportedScope = QQmlJSScope::ExportedScope<QQmlJSScope::Ptr>;
using QQmlJSImportedScope = QQmlJSScope::ImportedScope<QQmlJSScope::ConstPtr>;

QT_END_NAMESPACE

#endif // QQMLJSSCOPE_P_H
