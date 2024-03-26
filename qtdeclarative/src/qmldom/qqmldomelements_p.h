// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMELEMENTS_P_H
#define QQMLDOMELEMENTS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldomitem_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomcomments_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>
#include <QtCore/QMutexLocker>
#include <QtCore/QPair>

#include <memory>
#include <private/qqmljsscope_p.h>

#include <functional>
#include <limits>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

// namespace for utility methods building specific paths
// using a namespace one can reopen it and add more methods in other places
namespace Paths {
Path moduleIndexPath(QString uri, int majorVersion, ErrorHandler errorHandler = nullptr);
Path moduleScopePath(QString uri, Version version, ErrorHandler errorHandler = nullptr);
Path moduleScopePath(QString uri, QString version, ErrorHandler errorHandler = nullptr);
inline Path moduleScopePath(QString uri, ErrorHandler errorHandler = nullptr)
{
    return moduleScopePath(uri, QString(), errorHandler);
}
inline Path qmlDirInfoPath(QString path)
{
    return Path::Root(PathRoot::Top).field(Fields::qmldirWithPath).key(path);
}
inline Path qmlDirPath(QString path)
{
    return qmlDirInfoPath(path).field(Fields::currentItem);
}
inline Path qmldirFileInfoPath(QString path)
{
    return Path::Root(PathRoot::Top).field(Fields::qmldirFileWithPath).key(path);
}
inline Path qmldirFilePath(QString path)
{
    return qmldirFileInfoPath(path).field(Fields::currentItem);
}
inline Path qmlFileInfoPath(QString canonicalFilePath)
{
    return Path::Root(PathRoot::Top).field(Fields::qmlFileWithPath).key(canonicalFilePath);
}
inline Path qmlFilePath(QString canonicalFilePath)
{
    return qmlFileInfoPath(canonicalFilePath).field(Fields::currentItem);
}
inline Path qmlFileObjectPath(QString canonicalFilePath)
{
    return qmlFilePath(canonicalFilePath)
            .field(Fields::components)
            .key(QString())
            .index(0)
            .field(Fields::objects)
            .index(0);
}
inline Path qmltypesFileInfoPath(QString path)
{
    return Path::Root(PathRoot::Top).field(Fields::qmltypesFileWithPath).key(path);
}
inline Path qmltypesFilePath(QString path)
{
    return qmltypesFileInfoPath(path).field(Fields::currentItem);
}
inline Path jsFileInfoPath(QString path)
{
    return Path::Root(PathRoot::Top).field(Fields::jsFileWithPath).key(path);
}
inline Path jsFilePath(QString path)
{
    return jsFileInfoPath(path).field(Fields::currentItem);
}
inline Path qmlDirectoryInfoPath(QString path)
{
    return Path::Root(PathRoot::Top).field(Fields::qmlDirectoryWithPath).key(path);
}
inline Path qmlDirectoryPath(QString path)
{
    return qmlDirectoryInfoPath(path).field(Fields::currentItem);
}
inline Path globalScopeInfoPath(QString name)
{
    return Path::Root(PathRoot::Top).field(Fields::globalScopeWithName).key(name);
}
inline Path globalScopePath(QString name)
{
    return globalScopeInfoPath(name).field(Fields::currentItem);
}
inline Path lookupCppTypePath(QString name)
{
    return Path::Current(PathCurrent::Lookup).field(Fields::cppType).key(name);
}
inline Path lookupPropertyPath(QString name)
{
    return Path::Current(PathCurrent::Lookup).field(Fields::propertyDef).key(name);
}
inline Path lookupSymbolPath(QString name)
{
    return Path::Current(PathCurrent::Lookup).field(Fields::symbol).key(name);
}
inline Path lookupTypePath(QString name)
{
    return Path::Current(PathCurrent::Lookup).field(Fields::type).key(name);
}
inline Path loadInfoPath(Path el)
{
    return Path::Root(PathRoot::Env).field(Fields::loadInfo).key(el.toString());
}
} // end namespace Paths

class QMLDOM_EXPORT CommentableDomElement : public DomElement
{
public:
    CommentableDomElement(Path pathFromOwner = Path()) : DomElement(pathFromOwner) { }
    CommentableDomElement(const CommentableDomElement &o) : DomElement(o), m_comments(o.m_comments)
    {
    }
    CommentableDomElement &operator=(const CommentableDomElement &o) = default;
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    RegionComments &comments() { return m_comments; }
    const RegionComments &comments() const { return m_comments; }

private:
    RegionComments m_comments;
};

class QMLDOM_EXPORT Version
{
public:
    constexpr static DomType kindValue = DomType::Version;
    constexpr static qint32 Undefined = -1;
    constexpr static qint32 Latest = -2;

    Version(qint32 majorVersion = Undefined, qint32 minorVersion = Undefined);
    static Version fromString(QStringView v);

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor);

    bool isLatest() const;
    bool isValid() const;
    QString stringValue() const;
    QString majorString() const
    {
        if (majorVersion >= 0 || majorVersion == Undefined)
            return QString::number(majorVersion);
        return QString();
    }
    QString majorSymbolicString() const
    {
        if (majorVersion == Version::Latest)
            return QLatin1String("Latest");
        if (majorVersion >= 0 || majorVersion == Undefined)
            return QString::number(majorVersion);
        return QString();
    }
    QString minorString() const
    {
        if (minorVersion >= 0 || minorVersion == Undefined)
            return QString::number(minorVersion);
        return QString();
    }
    int compare(const Version &o) const
    {
        int c = majorVersion - o.majorVersion;
        if (c != 0)
            return c;
        return minorVersion - o.minorVersion;
    }

    qint32 majorVersion;
    qint32 minorVersion;
};
inline bool operator==(const Version &v1, const Version &v2)
{
    return v1.compare(v2) == 0;
}
inline bool operator!=(const Version &v1, const Version &v2)
{
    return v1.compare(v2) != 0;
}
inline bool operator<(const Version &v1, const Version &v2)
{
    return v1.compare(v2) < 0;
}
inline bool operator<=(const Version &v1, const Version &v2)
{
    return v1.compare(v2) <= 0;
}
inline bool operator>(const Version &v1, const Version &v2)
{
    return v1.compare(v2) > 0;
}
inline bool operator>=(const Version &v1, const Version &v2)
{
    return v1.compare(v2) >= 0;
}

class QMLDOM_EXPORT QmlUri
{
public:
    enum class Kind { Invalid, ModuleUri, DirectoryUrl, RelativePath, AbsolutePath };
    QmlUri() = default;
    static QmlUri fromString(const QString &importStr);
    static QmlUri fromUriString(const QString &importStr);
    static QmlUri fromDirectoryString(const QString &importStr);
    bool isValid() const;
    bool isDirectory() const;
    bool isModule() const;
    QString moduleUri() const;
    QString localPath() const;
    QString absoluteLocalPath(const QString &basePath = QString()) const;
    QUrl directoryUrl() const;
    QString directoryString() const;
    QString toString() const;
    Kind kind() const;

    friend bool operator==(const QmlUri &i1, const QmlUri &i2)
    {
        return i1.m_kind == i2.m_kind && i1.m_value == i2.m_value;
    }
    friend bool operator!=(const QmlUri &i1, const QmlUri &i2) { return !(i1 == i2); }

private:
    QmlUri(const QUrl &url) : m_kind(Kind::DirectoryUrl), m_value(url) { }
    QmlUri(Kind kind, const QString &value) : m_kind(kind), m_value(value) { }
    Kind m_kind = Kind::Invalid;
    std::variant<QString, QUrl> m_value;
};

class QMLDOM_EXPORT Import
{
    Q_DECLARE_TR_FUNCTIONS(Import)
public:
    constexpr static DomType kindValue = DomType::Import;

    static Import fromUriString(QString importStr, Version v = Version(),
                                QString importId = QString(), ErrorHandler handler = nullptr);
    static Import fromFileString(QString importStr, QString importId = QString(),
                                 ErrorHandler handler = nullptr);

    Import(QmlUri uri = QmlUri(), Version version = Version(), QString importId = QString())
        : uri(uri), version(version), importId(importId)
    {
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor);
    Path importedPath() const
    {
        if (uri.isDirectory()) {
            QString path = uri.absoluteLocalPath();
            if (!path.isEmpty()) {
                return Paths::qmlDirPath(path);
            } else {
                Q_ASSERT_X(false, "Import", "url imports not supported");
                return Paths::qmldirFilePath(uri.directoryString());
            }
        } else {
            return Paths::moduleScopePath(uri.moduleUri(), version);
        }
    }
    Import baseImport() const { return Import { uri, version }; }

    friend bool operator==(const Import &i1, const Import &i2)
    {
        return i1.uri == i2.uri && i1.version == i2.version && i1.importId == i2.importId
                && i1.comments == i2.comments && i1.implicit == i2.implicit;
    }
    friend bool operator!=(const Import &i1, const Import &i2) { return !(i1 == i2); }

    void writeOut(DomItem &self, OutWriter &ow) const;

    static QRegularExpression importRe();

    QmlUri uri;
    Version version;
    QString importId;
    RegionComments comments;
    bool implicit = false;
};

class QMLDOM_EXPORT ModuleAutoExport
{
public:
    constexpr static DomType kindValue = DomType::ModuleAutoExport;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
    {
        bool cont = true;
        cont = cont && self.dvWrapField(visitor, Fields::import, import);
        cont = cont && self.dvValueField(visitor, Fields::inheritVersion, inheritVersion);
        return cont;
    }

    friend bool operator==(const ModuleAutoExport &i1, const ModuleAutoExport &i2)
    {
        return i1.import == i2.import && i1.inheritVersion == i2.inheritVersion;
    }
    friend bool operator!=(const ModuleAutoExport &i1, const ModuleAutoExport &i2)
    {
        return !(i1 == i2);
    }

    Import import;
    bool inheritVersion = false;
};

class QMLDOM_EXPORT Pragma
{
public:
    constexpr static DomType kindValue = DomType::Pragma;

    Pragma(QString pragmaName = QString(), const QStringList &pragmaValues = {})
        : name(pragmaName), values{ pragmaValues }
    {
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
    {
        bool cont = self.dvValueField(visitor, Fields::name, name);
        cont = cont && self.dvValueField(visitor, Fields::values, values);
        cont = cont && self.dvWrapField(visitor, Fields::comments, comments);
        return cont;
    }

    void writeOut(DomItem &self, OutWriter &ow) const;

    QString name;
    QStringList values;
    RegionComments comments;
};

class QMLDOM_EXPORT Id
{
public:
    constexpr static DomType kindValue = DomType::Id;

    Id(QString idName = QString(), Path referredObject = Path());

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor);
    void updatePathFromOwner(Path pathFromOwner);
    Path addAnnotation(Path selfPathFromOwner, const QmlObject &ann, QmlObject **aPtr = nullptr);

    QString name;
    Path referredObjectPath;
    RegionComments comments;
    QList<QmlObject> annotations;
    std::shared_ptr<ScriptExpression> value;
};

// TODO: rename? it may contain statements and stuff, not only expressions
class QMLDOM_EXPORT ScriptExpression final : public OwningItem
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(ScriptExpression)
public:
    enum class ExpressionType {
        BindingExpression,
        FunctionBody,
        ArgInitializer,
        ArgumentStructure,
        ReturnType
    };
    Q_ENUM(ExpressionType);
    constexpr static DomType kindValue = DomType::ScriptExpression;
    DomType kind() const override { return kindValue; }

    explicit ScriptExpression(QStringView code, std::shared_ptr<QQmlJS::Engine> engine,
                              AST::Node *ast, std::shared_ptr<AstComments> comments,
                              ExpressionType expressionType,
                              SourceLocation localOffset = SourceLocation(), int derivedFrom = 0,
                              QStringView preCode = QStringView(),
                              QStringView postCode = QStringView());

    ScriptExpression()
        : ScriptExpression(QStringView(), std::shared_ptr<QQmlJS::Engine>(), nullptr,
                           std::shared_ptr<AstComments>(), ExpressionType::BindingExpression,
                           SourceLocation(), 0)
    {
    }

    explicit ScriptExpression(QString code, ExpressionType expressionType, int derivedFrom = 0,
                              QString preCode = QString(), QString postCode = QString())
        : OwningItem(derivedFrom), m_expressionType(expressionType)
    {
        setCode(code, preCode, postCode);
    }

    ScriptExpression(const ScriptExpression &e);

    std::shared_ptr<ScriptExpression> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<ScriptExpression>(doCopy(self));
    }

    std::shared_ptr<ScriptExpression> copyWithUpdatedCode(DomItem &self, QString code) const;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    Path canonicalPath(DomItem &self) const override { return self.m_ownerPath; }
    // parsed and created if not available
    AST::Node *ast() const { return m_ast; }
    // dump of the ast (without locations)
    void astDumper(Sink s, AstDumperOptions options) const;
    QString astRelocatableDump() const;

    // definedSymbols name, value, from
    // usedSymbols name, locations
    QStringView code() const
    {
        QMutexLocker l(mutex());
        return m_code;
    }

    ExpressionType expressionType() const
    {
        QMutexLocker l(mutex());
        return m_expressionType;
    }

    bool isNull() const
    {
        QMutexLocker l(mutex());
        return m_code.isNull();
    }
    std::shared_ptr<QQmlJS::Engine> engine() const
    {
        QMutexLocker l(mutex());
        return m_engine;
    }
    std::shared_ptr<AstComments> astComments() const { return m_astComments; }
    void writeOut(DomItem &self, OutWriter &lw) const override;
    SourceLocation globalLocation(DomItem &self) const;
    SourceLocation localOffset() const { return m_localOffset; }
    QStringView preCode() const { return m_preCode; }
    QStringView postCode() const { return m_postCode; }
    void setScriptElement(const ScriptElementVariant &p);
    ScriptElementVariant scriptElement() { return m_element; }

protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override
    {
        return std::make_shared<ScriptExpression>(*this);
    }

    std::function<SourceLocation(SourceLocation)> locationToGlobalF(DomItem &self) const
    {
        SourceLocation loc = globalLocation(self);
        return [loc, this](SourceLocation x) {
            return SourceLocation(x.offset - m_localOffset.offset + loc.offset, x.length,
                                  x.startLine - m_localOffset.startLine + loc.startLine,
                                  ((x.startLine == m_localOffset.startLine) ? x.startColumn
                                                   - m_localOffset.startColumn + loc.startColumn
                                                                            : x.startColumn));
        };
    }

    SourceLocation locationToLocal(SourceLocation x) const
    {
        return SourceLocation(
                x.offset - m_localOffset.offset, x.length, x.startLine - m_localOffset.startLine,
                ((x.startLine == m_localOffset.startLine)
                         ? x.startColumn - m_localOffset.startColumn
                         : x.startColumn)); // are line and column 1 based? then we should + 1
    }

    std::function<SourceLocation(SourceLocation)> locationToLocalF(DomItem &) const
    {
        return [this](SourceLocation x) { return locationToLocal(x); };
    }

private:
    void setCode(QString code, QString preCode, QString postCode);
    ExpressionType m_expressionType;
    QString m_codeStr;
    QStringView m_code;
    QStringView m_preCode;
    QStringView m_postCode;
    mutable std::shared_ptr<QQmlJS::Engine> m_engine;
    mutable AST::Node *m_ast;
    std::shared_ptr<AstComments> m_astComments;
    SourceLocation m_localOffset;
    ScriptElementVariant m_element;
};

class BindingValue;

class QMLDOM_EXPORT Binding
{
public:
    constexpr static DomType kindValue = DomType::Binding;

    Binding(QString m_name = QString(),
            std::unique_ptr<BindingValue> value = std::unique_ptr<BindingValue>(),
            BindingType bindingType = BindingType::Normal);
    Binding(QString m_name, std::shared_ptr<ScriptExpression> value,
            BindingType bindingType = BindingType::Normal);
    Binding(QString m_name, QString scriptCode, BindingType bindingType = BindingType::Normal);
    Binding(QString m_name, QmlObject value, BindingType bindingType = BindingType::Normal);
    Binding(QString m_name, QList<QmlObject> value, BindingType bindingType = BindingType::Normal);
    Binding(const Binding &o);
    Binding(Binding &&o) = default;
    ~Binding();
    Binding &operator=(const Binding &);
    Binding &operator=(Binding &&) = default;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor);
    DomItem valueItem(DomItem &self) const; //  ### REVISIT: consider replacing return value with variant
    BindingValueKind valueKind() const;
    QString name() const { return m_name; }
    BindingType bindingType() const { return m_bindingType; }
    QmlObject const *objectValue() const;
    QList<QmlObject> const *arrayValue() const;
    std::shared_ptr<ScriptExpression> scriptExpressionValue() const;
    QmlObject *objectValue();
    QList<QmlObject> *arrayValue();
    std::shared_ptr<ScriptExpression> scriptExpressionValue();
    QList<QmlObject> annotations() const { return m_annotations; }
    void setAnnotations(QList<QmlObject> annotations) { m_annotations = annotations; }
    void setValue(std::unique_ptr<BindingValue> &&value) { m_value = std::move(value); }
    Path addAnnotation(Path selfPathFromOwner, const QmlObject &a, QmlObject **aPtr = nullptr);
    const RegionComments &comments() const { return m_comments; }
    RegionComments &comments() { return m_comments; }
    void updatePathFromOwner(Path newPath);
    void writeOut(DomItem &self, OutWriter &lw) const;
    void writeOutValue(DomItem &self, OutWriter &lw) const;
    bool isSignalHandler() const
    {
        QString baseName = m_name.split(QLatin1Char('.')).last();
        if (baseName.startsWith(u"on") && baseName.size() > 2 && baseName.at(2).isUpper())
            return true;
        return false;
    }
    static QString preCodeForName(QStringView n)
    {
        return QStringLiteral(u"QtObject{\n  %1: ").arg(n.split(u'.').last());
    }
    static QString postCodeForName(QStringView) { return QStringLiteral(u"\n}\n"); }
    QString preCode() const { return preCodeForName(m_name); }
    QString postCode() const { return postCodeForName(m_name); }

private:
    friend class QQmlDomAstCreator;
    BindingType m_bindingType;
    QString m_name;
    std::unique_ptr<BindingValue> m_value;
    QList<QmlObject> m_annotations;
    RegionComments m_comments;
};

class QMLDOM_EXPORT AttributeInfo
{
public:
    enum Access { Private, Protected, Public };

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor);

    Path addAnnotation(Path selfPathFromOwner, const QmlObject &annotation,
                       QmlObject **aPtr = nullptr);
    void updatePathFromOwner(Path newPath);

    QString name;
    Access access = Access::Public;
    QString typeName;
    bool isReadonly = false;
    bool isList = false;
    QList<QmlObject> annotations;
    RegionComments comments;
};

struct QMLDOM_EXPORT LocallyResolvedAlias
{
    enum class Status { Invalid, ResolvedProperty, ResolvedObject, Loop, TooDeep };
    bool valid()
    {
        switch (status) {
        case Status::ResolvedProperty:
        case Status::ResolvedObject:
            return true;
        default:
            return false;
        }
    }
    DomItem baseObject;
    DomItem localPropertyDef;
    QString typeName;
    QStringList accessedPath;
    Status status = Status::Invalid;
    int nAliases = 0;
};

class QMLDOM_EXPORT PropertyDefinition : public AttributeInfo
{
public:
    constexpr static DomType kindValue = DomType::PropertyDefinition;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
    {
        bool cont = AttributeInfo::iterateDirectSubpaths(self, visitor);
        cont = cont && self.dvValueField(visitor, Fields::isPointer, isPointer);
        cont = cont && self.dvValueField(visitor, Fields::isFinal, isFinal);
        cont = cont && self.dvValueField(visitor, Fields::isAlias, isAlias());
        cont = cont && self.dvValueField(visitor, Fields::isDefaultMember, isDefaultMember);
        cont = cont && self.dvValueField(visitor, Fields::isRequired, isRequired);
        cont = cont && self.dvValueField(visitor, Fields::read, read);
        cont = cont && self.dvValueField(visitor, Fields::write, write);
        cont = cont && self.dvValueField(visitor, Fields::bindable, bindable);
        cont = cont && self.dvValueField(visitor, Fields::notify, notify);
        cont = cont && self.dvReferenceField(visitor, Fields::type, typePath());
        return cont;
    }

    Path typePath() const { return Paths::lookupTypePath(typeName); }

    bool isAlias() const { return typeName == u"alias"; }
    bool isParametricType() const;
    void writeOut(DomItem &self, OutWriter &lw) const;

    QString read;
    QString write;
    QString bindable;
    QString notify;
    bool isFinal = false;
    bool isPointer = false;
    bool isDefaultMember = false;
    bool isRequired = false;
    std::optional<QQmlJSScope::Ptr> scope;
};

class QMLDOM_EXPORT PropertyInfo
{
public:
    constexpr static DomType kindValue = DomType::PropertyInfo; // used to get the correct kind in ObjectWrapper

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor);

    QList<DomItem> propertyDefs;
    QList<DomItem> bindings;
};

class QMLDOM_EXPORT MethodParameter
{
public:
    constexpr static DomType kindValue = DomType::MethodParameter;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor);

    void writeOut(DomItem &self, OutWriter &ow) const;
    void writeOutSignal(DomItem &self, OutWriter &ow) const;

    QString name;
    QString typeName;
    bool isPointer = false;
    bool isReadonly = false;
    bool isList = false;
    bool isRestElement = false;
    std::shared_ptr<ScriptExpression> defaultValue;
    /*!
        \internal
        Contains the scriptElement representing this argument, inclusive default value,
        deconstruction, etc.
     */
    std::shared_ptr<ScriptExpression> value;
    QList<QmlObject> annotations;
    RegionComments comments;
};

class QMLDOM_EXPORT MethodInfo : public AttributeInfo
{
    Q_GADGET
public:
    enum MethodType { Signal, Method };
    Q_ENUM(MethodType)

    constexpr static DomType kindValue = DomType::MethodInfo;

    Path typePath(DomItem &) const
    {
        return (typeName.isEmpty() ? Path() : Paths::lookupTypePath(typeName));
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor);
    QString preCode(DomItem &) const; // ### REVISIT, might be simplified by using different toplevel production rules at usage site
    QString postCode(DomItem &) const;
    void writePre(DomItem &self, OutWriter &ow) const;
    void writeOut(DomItem &self, OutWriter &ow) const;
    void setCode(QString code)
    {
        body = std::make_shared<ScriptExpression>(
                code, ScriptExpression::ExpressionType::FunctionBody, 0,
                                     QLatin1String("function foo(){\n"), QLatin1String("\n}\n"));
    }
    MethodInfo() = default;
    std::optional<QQmlJSScope::Ptr> semanticScope() { return m_semanticScope; }
    void setSemanticScope(QQmlJSScope::Ptr scope) { m_semanticScope = scope; }

    // TODO: make private + add getters/setters
    QList<MethodParameter> parameters;
    MethodType methodType = Method;
    std::shared_ptr<ScriptExpression> body;
    std::shared_ptr<ScriptExpression> returnType;
    bool isConstructor = false;
    std::optional<QQmlJSScope::Ptr> m_semanticScope;
};

class QMLDOM_EXPORT EnumItem
{
public:
    constexpr static DomType kindValue = DomType::EnumItem;

    EnumItem(QString name = QString(), int value = 0) : m_name(name), m_value(value) { }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor);

    QString name() const { return m_name; }
    double value() const { return m_value; }
    RegionComments &comments() { return m_comments; }
    const RegionComments &comments() const { return m_comments; }
    void writeOut(DomItem &self, OutWriter &lw) const;

private:
    QString m_name;
    double m_value;
    RegionComments m_comments;
};

class QMLDOM_EXPORT EnumDecl final : public CommentableDomElement
{
public:
    constexpr static DomType kindValue = DomType::EnumDecl;
    DomType kind() const override { return kindValue; }

    EnumDecl(QString name = QString(), QList<EnumItem> values = QList<EnumItem>(),
             Path pathFromOwner = Path())
        : CommentableDomElement(pathFromOwner), m_name(name), m_values(values)
    {
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    QString name() const { return m_name; }
    void setName(QString name) { m_name = name; }
    const QList<EnumItem> &values() const & { return m_values; }
    bool isFlag() const { return m_isFlag; }
    void setIsFlag(bool flag) { m_isFlag = flag; }
    QString alias() const { return m_alias; }
    void setAlias(QString aliasName) { m_alias = aliasName; }
    void setValues(QList<EnumItem> values) { m_values = values; }
    Path addValue(EnumItem value)
    {
        m_values.append(value);
        return Path::Field(Fields::values).index(index_type(m_values.size() - 1));
    }
    void updatePathFromOwner(Path newP) override;

    const QList<QmlObject> &annotations() const & { return m_annotations; }
    void setAnnotations(QList<QmlObject> annotations);
    Path addAnnotation(const QmlObject &child, QmlObject **cPtr = nullptr);
    void writeOut(DomItem &self, OutWriter &lw) const override;

private:
    QString m_name;
    bool m_isFlag = false;
    QString m_alias;
    QList<EnumItem> m_values;
    QList<QmlObject> m_annotations;
};

class QMLDOM_EXPORT QmlObject final : public CommentableDomElement
{
    Q_DECLARE_TR_FUNCTIONS(QmlObject)
public:
    constexpr static DomType kindValue = DomType::QmlObject;
    DomType kind() const override { return kindValue; }

    QmlObject(Path pathFromOwner = Path());
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    bool iterateBaseDirectSubpaths(DomItem &self, DirectVisitor);
    QList<QString> fields() const;
    QList<QString> fields(DomItem &) const override { return fields(); }
    DomItem field(DomItem &self, QStringView name);
    DomItem field(DomItem &self, QStringView name) const override
    {
        return const_cast<QmlObject *>(this)->field(self, name);
    }
    void updatePathFromOwner(Path newPath) override;
    QString localDefaultPropertyName() const;
    QString defaultPropertyName(DomItem &self) const;
    virtual bool iterateSubOwners(DomItem &self, function_ref<bool(DomItem &owner)> visitor) const;

    QString idStr() const { return m_idStr; }
    QString name() const { return m_name; }
    const QList<Path> &prototypePaths() const & { return m_prototypePaths; }
    Path nextScopePath() const { return m_nextScopePath; }
    const QMultiMap<QString, PropertyDefinition> &propertyDefs() const & { return m_propertyDefs; }
    const QMultiMap<QString, Binding> &bindings() const & { return m_bindings; }
    const QMultiMap<QString, MethodInfo> &methods() const & { return m_methods; }
    QList<QmlObject> children() const { return m_children; }
    QList<QmlObject> annotations() const { return m_annotations; }

    void setIdStr(QString id) { m_idStr = id; }
    void setName(QString name) { m_name = name; }
    void setDefaultPropertyName(QString name) { m_defaultPropertyName = name; }
    void setPrototypePaths(QList<Path> prototypePaths) { m_prototypePaths = prototypePaths; }
    Path addPrototypePath(Path prototypePath)
    {
        index_type idx = index_type(m_prototypePaths.indexOf(prototypePath));
        if (idx == -1) {
            idx = index_type(m_prototypePaths.size());
            m_prototypePaths.append(prototypePath);
        }
        return Path::Field(Fields::prototypes).index(idx);
    }
    void setNextScopePath(Path nextScopePath) { m_nextScopePath = nextScopePath; }
    void setPropertyDefs(QMultiMap<QString, PropertyDefinition> propertyDefs)
    {
        m_propertyDefs = propertyDefs;
    }
    void setBindings(QMultiMap<QString, Binding> bindings) { m_bindings = bindings; }
    void setMethods(QMultiMap<QString, MethodInfo> functionDefs) { m_methods = functionDefs; }
    void setChildren(QList<QmlObject> children)
    {
        m_children = children;
        if (pathFromOwner())
            updatePathFromOwner(pathFromOwner());
    }
    void setAnnotations(QList<QmlObject> annotations)
    {
        m_annotations = annotations;
        if (pathFromOwner())
            updatePathFromOwner(pathFromOwner());
    }
    Path addPropertyDef(PropertyDefinition propertyDef, AddOption option,
                        PropertyDefinition **pDef = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().field(Fields::propertyDefs),
                                                m_propertyDefs, propertyDef.name, propertyDef,
                                                option, pDef);
    }
    MutableDomItem addPropertyDef(MutableDomItem &self, PropertyDefinition propertyDef,
                                  AddOption option);

    Path addBinding(Binding binding, AddOption option, Binding **bPtr = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().field(Fields::bindings), m_bindings,
                                                binding.name(), binding, option, bPtr);
    }
    MutableDomItem addBinding(MutableDomItem &self, Binding binding, AddOption option);
    Path addMethod(MethodInfo functionDef, AddOption option, MethodInfo **mPtr = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().field(Fields::methods), m_methods,
                                                functionDef.name, functionDef, option, mPtr);
    }
    MutableDomItem addMethod(MutableDomItem &self, MethodInfo functionDef, AddOption option);
    Path addChild(QmlObject child, QmlObject **cPtr = nullptr)
    {
        return appendUpdatableElementInQList(pathFromOwner().field(Fields::children), m_children,
                                             child, cPtr);
    }
    MutableDomItem addChild(MutableDomItem &self, QmlObject child)
    {
        Path p = addChild(child);
        return MutableDomItem(self.owner().item(), p);
    }
    Path addAnnotation(const QmlObject &annotation, QmlObject **aPtr = nullptr)
    {
        return appendUpdatableElementInQList(pathFromOwner().field(Fields::annotations),
                                             m_annotations, annotation, aPtr);
    }
    void writeOut(DomItem &self, OutWriter &ow, QString onTarget) const;
    void writeOut(DomItem &self, OutWriter &lw) const override { writeOut(self, lw, QString()); }

    LocallyResolvedAlias resolveAlias(DomItem &self,
                                      std::shared_ptr<ScriptExpression> accessSequence) const;
    LocallyResolvedAlias resolveAlias(DomItem &self, const QStringList &accessSequence) const;

    std::optional<QQmlJSScope::Ptr> semanticScope() const { return m_scope; }
    void setSemanticScope(const QQmlJSScope::Ptr &scope) { m_scope = scope; }

private:
    friend class QQmlDomAstCreator;
    QString m_idStr;
    QString m_name;
    QList<Path> m_prototypePaths;
    Path m_nextScopePath;
    QString m_defaultPropertyName;
    QMultiMap<QString, PropertyDefinition> m_propertyDefs;
    QMultiMap<QString, Binding> m_bindings;
    QMultiMap<QString, MethodInfo> m_methods;
    QList<QmlObject> m_children;
    QList<QmlObject> m_annotations;
    std::optional<QQmlJSScope::Ptr> m_scope;
};

class Export
{
    Q_DECLARE_TR_FUNCTIONS(Export)
public:
    constexpr static DomType kindValue = DomType::Export;
    static Export fromString(Path source, QStringView exp, Path typePath, ErrorHandler h);
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
    {
        bool cont = true;
        cont = cont && self.dvValueField(visitor, Fields::uri, uri);
        cont = cont && self.dvValueField(visitor, Fields::typeName, typeName);
        cont = cont && self.dvWrapField(visitor, Fields::version, version);
        if (typePath)
            cont = cont && self.dvReferenceField(visitor, Fields::type, typePath);
        cont = cont && self.dvValueField(visitor, Fields::isInternal, isInternal);
        cont = cont && self.dvValueField(visitor, Fields::isSingleton, isSingleton);
        if (exportSourcePath)
            cont = cont && self.dvReferenceField(visitor, Fields::exportSource, exportSourcePath);
        return cont;
    }

    Path exportSourcePath;
    QString uri;
    QString typeName;
    Version version;
    Path typePath;
    bool isInternal = false;
    bool isSingleton = false;
};

class QMLDOM_EXPORT Component : public CommentableDomElement
{
public:
    Component(QString name);
    Component(Path pathFromOwner = Path());
    Component(const Component &o) = default;
    Component &operator=(const Component &) = default;

    bool iterateDirectSubpaths(DomItem &, DirectVisitor) override;
    void updatePathFromOwner(Path newPath) override;
    DomItem field(DomItem &self, QStringView name) const override
    {
        return const_cast<Component *>(this)->field(self, name);
    }
    DomItem field(DomItem &self, QStringView name);

    QString name() const { return m_name; }
    const QMultiMap<QString, EnumDecl> &enumerations() const & { return m_enumerations; }
    const QList<QmlObject> &objects() const & { return m_objects; }
    bool isSingleton() const { return m_isSingleton; }
    bool isCreatable() const { return m_isCreatable; }
    bool isComposite() const { return m_isComposite; }
    QString attachedTypeName() const { return m_attachedTypeName; }
    Path attachedTypePath(DomItem &) const { return m_attachedTypePath; }

    void setName(QString name) { m_name = name; }
    void setEnumerations(QMultiMap<QString, EnumDecl> enumerations)
    {
        m_enumerations = enumerations;
    }
    Path addEnumeration(const EnumDecl &enumeration, AddOption option = AddOption::Overwrite,
                        EnumDecl **ePtr = nullptr)
    {
        return insertUpdatableElementInMultiMap(pathFromOwner().field(Fields::enumerations),
                                                m_enumerations, enumeration.name(), enumeration,
                                                option, ePtr);
    }
    void setObjects(QList<QmlObject> objects) { m_objects = objects; }
    Path addObject(const QmlObject &object, QmlObject **oPtr = nullptr);
    void setIsSingleton(bool isSingleton) { m_isSingleton = isSingleton; }
    void setIsCreatable(bool isCreatable) { m_isCreatable = isCreatable; }
    void setIsComposite(bool isComposite) { m_isComposite = isComposite; }
    void setAttachedTypeName(QString name) { m_attachedTypeName = name; }
    void setAttachedTypePath(Path p) { m_attachedTypePath = p; }

private:
    friend class QQmlDomAstCreator;
    QString m_name;
    QMultiMap<QString, EnumDecl> m_enumerations;
    QList<QmlObject> m_objects;
    bool m_isSingleton = false;
    bool m_isCreatable = true;
    bool m_isComposite = true;
    QString m_attachedTypeName;
    Path m_attachedTypePath;
};

class QMLDOM_EXPORT JsResource final : public Component
{
public:
    constexpr static DomType kindValue = DomType::JsResource;
    DomType kind() const override { return kindValue; }

    JsResource(Path pathFromOwner = Path()) : Component(pathFromOwner) { }
    bool iterateDirectSubpaths(DomItem &, DirectVisitor) override
    { // to do: complete
        return true;
    }
    // globalSymbols defined/exported, required/used
};

class QMLDOM_EXPORT QmltypesComponent final : public Component
{
public:
    constexpr static DomType kindValue = DomType::QmltypesComponent;
    DomType kind() const override { return kindValue; }

    QmltypesComponent(Path pathFromOwner = Path()) : Component(pathFromOwner) { }
    bool iterateDirectSubpaths(DomItem &, DirectVisitor) override;
    const QList<Export> &exports() const & { return m_exports; }
    QString fileName() const { return m_fileName; }
    void setExports(QList<Export> exports) { m_exports = exports; }
    void addExport(const Export &exportedEntry) { m_exports.append(exportedEntry); }
    void setFileName(QString fileName) { m_fileName = fileName; }
    const QList<int> &metaRevisions() const & { return m_metaRevisions; }
    void setMetaRevisions(QList<int> metaRevisions) { m_metaRevisions = metaRevisions; }
    void setInterfaceNames(const QStringList& interfaces) { m_interfaceNames = interfaces; }
    const QStringList &interfaceNames() const & { return m_interfaceNames; }
    QString extensionTypeName() const { return m_extensionTypeName; }
    void setExtensionTypeName(const QString &name) { m_extensionTypeName =  name; }
    QString valueTypeName() const { return m_valueTypeName; }
    void setValueTypeName(const QString &name) { m_valueTypeName = name; }
    bool hasCustomParser() const { return m_hasCustomParser; }
    void setHasCustomParser(bool v) { m_hasCustomParser = v; }
    bool extensionIsNamespace() const { return m_extensionIsNamespace; }
    void setExtensionIsNamespace(bool v) { m_extensionIsNamespace = v; }
    QQmlJSScope::AccessSemantics accessSemantics() const { return m_accessSemantics; }
    void setAccessSemantics(QQmlJSScope::AccessSemantics v) { m_accessSemantics = v; }
private:
    QList<Export> m_exports;
    QList<int> m_metaRevisions;
    QString m_fileName; // remove?
    QStringList m_interfaceNames;
    bool m_hasCustomParser = false;
    bool m_extensionIsNamespace = false;
    QString m_valueTypeName;
    QString m_extensionTypeName;
    QQmlJSScope::AccessSemantics m_accessSemantics;
};

class QMLDOM_EXPORT QmlComponent final : public Component
{
public:
    constexpr static DomType kindValue = DomType::QmlComponent;
    DomType kind() const override { return kindValue; }

    QmlComponent(QString name = QString()) : Component(name)
    {
        setIsComposite(true);
        setIsCreatable(true);
    }

    QmlComponent(const QmlComponent &o)
        : Component(o), m_nextComponentPath(o.m_nextComponentPath), m_ids(o.m_ids)
    {
    }
    QmlComponent &operator=(const QmlComponent &) = default;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;

    const QMultiMap<QString, Id> &ids() const & { return m_ids; }
    Path nextComponentPath() const { return m_nextComponentPath; }
    void setIds(QMultiMap<QString, Id> ids) { m_ids = ids; }
    void setNextComponentPath(Path p) { m_nextComponentPath = p; }
    void updatePathFromOwner(Path newPath) override;
    Path addId(const Id &id, AddOption option = AddOption::Overwrite, Id **idPtr = nullptr)
    {
        // warning does nor remove old idStr when overwriting...
        return insertUpdatableElementInMultiMap(pathFromOwner().field(Fields::ids), m_ids, id.name,
                                                id, option, idPtr);
    }
    void writeOut(DomItem &self, OutWriter &) const override;
    QList<QString> subComponentsNames(DomItem &self) const;
    QList<DomItem> subComponents(DomItem &self) const;

    void setSemanticScope(const QQmlJSScope::Ptr &scope) { m_semanticScope = scope; }
    std::optional<QQmlJSScope::Ptr> semanticScope() { return m_semanticScope; }

private:
    friend class QQmlDomAstCreator;
    Path m_nextComponentPath;
    QMultiMap<QString, Id> m_ids;
    std::optional<QQmlJSScope::Ptr> m_semanticScope;
};

class QMLDOM_EXPORT GlobalComponent final : public Component
{
public:
    constexpr static DomType kindValue = DomType::GlobalComponent;
    DomType kind() const override { return kindValue; }

    GlobalComponent(Path pathFromOwner = Path()) : Component(pathFromOwner) { }
};

static ErrorGroups importErrors = { { DomItem::domErrorGroup, NewErrorGroup("importError") } };

class QMLDOM_EXPORT ImportScope
{
    Q_DECLARE_TR_FUNCTIONS(ImportScope)
public:
    constexpr static DomType kindValue = DomType::ImportScope;

    ImportScope() = default;
    ~ImportScope() = default;

    const QList<Path> &importSourcePaths() const & { return m_importSourcePaths; }

    const QMap<QString, ImportScope> &subImports() const & { return m_subImports; }

    QList<Path> allSources(DomItem &self) const;

    QSet<QString> importedNames(DomItem &self) const
    {
        QSet<QString> res;
        for (Path p : allSources(self)) {
            QSet<QString> ks = self.path(p.field(Fields::exports), self.errorHandler()).keys();
            res += ks;
        }
        return res;
    }

    QList<DomItem> importedItemsWithName(DomItem &self, QString name) const
    {
        QList<DomItem> res;
        for (Path p : allSources(self)) {
            DomItem source = self.path(p.field(Fields::exports), self.errorHandler());
            DomItem els = source.key(name);
            int nEls = els.indexes();
            for (int i = 0; i < nEls; ++i)
                res.append(els.index(i));
            if (nEls == 0 && els) {
                self.addError(importErrors.warning(
                        tr("Looking up '%1' expected a list of exports, not %2")
                                .arg(name, els.toString())));
            }
        }
        return res;
    }

    QList<Export> importedExportsWithName(DomItem &self, QString name) const
    {
        QList<Export> res;
        for (DomItem &i : importedItemsWithName(self, name))
            if (const Export *e = i.as<Export>())
                res.append(*e);
            else
                self.addError(importErrors.warning(
                        tr("Expected Export looking up '%1', not %2").arg(name, i.toString())));
        return res;
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor);

    void addImport(QStringList p, Path targetExports)
    {
        if (!p.isEmpty()) {
            QString current = p.takeFirst();
            m_subImports[current].addImport(p, targetExports);
        } else if (!m_importSourcePaths.contains(targetExports)) {
            m_importSourcePaths.append(targetExports);
        }
    }

private:
    QList<Path> m_importSourcePaths;
    QMap<QString, ImportScope> m_subImports;
};

class BindingValue
{
public:
    BindingValue();
    BindingValue(const QmlObject &o);
    BindingValue(std::shared_ptr<ScriptExpression> o);
    BindingValue(const QList<QmlObject> &l);
    ~BindingValue();
    BindingValue(const BindingValue &o);
    BindingValue &operator=(const BindingValue &o);

    DomItem value(DomItem &binding);
    void updatePathFromOwner(Path newPath);

private:
    friend class Binding;
    void clearValue();

    BindingValueKind kind;
    union {
        int dummy;
        QmlObject object;
        std::shared_ptr<ScriptExpression> scriptExpression;
        QList<QmlObject> array;
    };
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMELEMENTS_P_H
