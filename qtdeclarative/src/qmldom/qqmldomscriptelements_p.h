// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMSCRIPTELEMENTS_P_H
#define QQMLDOMSCRIPTELEMENTS_P_H

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
#include "qqmldomattachedinfo_p.h"
#include "qqmldompath_p.h"
#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>
#include <variant>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

namespace ScriptElements {

template<DomType type>
class ScriptElementBase : public ScriptElement
{
public:
    using BaseT = ScriptElementBase<type>;
    static constexpr DomType kindValue = type;
    static constexpr DomKind domKindValue = DomKind::ScriptElement;

    ScriptElementBase(QQmlJS::SourceLocation combinedLocation = QQmlJS::SourceLocation{})
        : ScriptElement(), m_combinedLocation(combinedLocation)
    {
    }
    ScriptElementBase(QQmlJS::SourceLocation first, QQmlJS::SourceLocation last)
        : ScriptElement(), m_combinedLocation(combine(first, last))
    {
    }
    DomType kind() const override { return type; }
    DomKind domKind() const override { return domKindValue; }

    void createFileLocations(FileLocations::Tree base) override
    {
        FileLocations::Tree res = FileLocations::ensure(base, pathFromOwner(), AttachedInfo::PathType::Relative);
        FileLocations::addRegion(res, QString(), m_combinedLocation);
    }

    /*
       Pretty prints the current DomItem. Currently, for script elements, this is done entirely on
       the parser representation (via the AST classes), but it could be moved here if needed.
    */
    // void writeOut(DomItem &self, OutWriter &lw) const override;

    /*!
       All of the following overloads are only required for optimization purposes.
       The base implementation will work fine, but might be slightly slower.
       You can override dump(), fields(), field(), indexes(), index(), keys() or key() if the
       performance of the base class becomes problematic.
    */

    // // needed for debug
    // void dump(DomItem &, Sink sink, int indent, FilterT filter) const override;

    // // just required for optimization if iterateDirectSubpaths is slow
    // QList<QString> fields(DomItem &self) const override;
    // DomItem field(DomItem &self, QStringView name) const override;

    // index_type indexes(DomItem &self) const override;
    // DomItem index(DomItem &self, index_type index) const override;

    // QSet<QString> const keys(DomItem &self) const override;
    // DomItem key(DomItem &self, QString name) const override;

protected:
    QQmlJS::SourceLocation m_combinedLocation;
};

class ScriptList : public ScriptElementBase<DomType::List>
{
public:
    using typename ScriptElementBase<DomType::List>::BaseT;

    using BaseT::BaseT;

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override
    {
        bool cont =
                asList(self.pathFromOwner().key(QString())).iterateDirectSubpaths(self, visitor);
        return cont;
    }
    void updatePathFromOwner(Path p) override
    {
        BaseT::updatePathFromOwner(p);
        for (int i = 0; i < m_list.size(); ++i) {
            Q_ASSERT(m_list[i].base());
            m_list[i].base()->updatePathFromOwner(p.index(i));
        }
    }
    void createFileLocations(FileLocations::Tree base) override
    {
        BaseT::createFileLocations(base);

        for (int i = 0; i < m_list.size(); ++i) {
            Q_ASSERT(m_list[i].base());
            m_list[i].base()->createFileLocations(base);
        }
    }

    List asList(Path path) const
    {
        auto asList = List::fromQList<ScriptElementVariant>(
                path, m_list,
                [](DomItem &list, const PathEls::PathComponent &, ScriptElementVariant &wrapped)
                        -> DomItem { return list.subScriptElementWrapperItem(wrapped); });

        return asList;
    }

    void append(const ScriptElementVariant &statement) { m_list.push_back(statement); }
    void append(const ScriptList &list) { m_list.append(list.m_list); }
    void reverse() { std::reverse(m_list.begin(), m_list.end()); }
    void replaceKindForGenericChildren(DomType oldType, DomType newType);
    const QList<ScriptElementVariant> &qList() { return std::as_const(m_list); };

private:
    QList<ScriptElementVariant> m_list;
};

class GenericScriptElement : public ScriptElementBase<DomType::ScriptGenericElement>
{
public:
    using BaseT::BaseT;
    using VariantT = std::variant<ScriptElementVariant, ScriptList>;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    DomType kind() const override { return m_kind; }
    void setKind(DomType kind) { m_kind = kind; }

    decltype(auto) insertChild(QStringView name, VariantT v)
    {
        return m_children.insert(std::make_pair(name, v));
    }

private:
    /*!
       \internal
       The DomItem interface will use iterateDirectSubpaths for all kinds of operations on the
       GenericScriptElement. Therefore, to avoid bad surprises when using the DomItem interface, use
       a sorted map to always iterate the children in the same order.
     */
    std::map<QQmlJS::Dom::FieldType, VariantT> m_children;
    DomType m_kind;
};

class BlockStatement : public ScriptElementBase<DomType::ScriptBlockStatement>
{
public:
    using BaseT::BaseT;

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    ScriptList statements() const { return m_statements; }
    void setStatements(const ScriptList &statements) { m_statements = statements; }

private:
    ScriptList m_statements;
};

class IdentifierExpression : public ScriptElementBase<DomType::ScriptIdentifierExpression>
{
public:
    using BaseT::BaseT;
    void setName(QStringView name) { m_name = name.toString(); }
    QString name() { return m_name; }

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    QCborValue value() const override { return QCborValue(m_name); }

private:
    QString m_name;
};

class Literal : public ScriptElementBase<DomType::ScriptLiteral>
{
public:
    using BaseT::BaseT;

    using VariantT = std::variant<QString, double, bool, std::nullptr_t>;

    void setLiteralValue(VariantT value) { m_value = value; }
    VariantT literalValue() const { return m_value; }

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    QCborValue value() const override
    {
        return std::visit([](auto &&e) -> QCborValue { return e; }, m_value);
    }

private:
    VariantT m_value;
};

// TODO: test this method + implement foreach etc
class ForStatement : public ScriptElementBase<DomType::ScriptForStatement>
{
public:
    using BaseT::BaseT;

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    ScriptElementVariant initializer() const { return m_initializer; }
    void setInitializer(const ScriptElementVariant &newInitializer)
    {
        m_initializer = newInitializer;
    }

    ScriptElementVariant declarations() const { return m_declarations; }
    void setDeclarations(const ScriptElementVariant &newDeclaration)
    {
        m_declarations = newDeclaration;
    }
    ScriptElementVariant condition() const { return m_condition; }
    void setCondition(const ScriptElementVariant &newCondition) { m_condition = newCondition; }
    ScriptElementVariant expression() const { return m_expression; }
    void setExpression(const ScriptElementVariant &newExpression) { m_expression = newExpression; }
    ScriptElementVariant body() const { return m_body; }
    void setBody(const ScriptElementVariant &newBody) { m_body = newBody; }

private:
    ScriptElementVariant m_initializer;
    ScriptElementVariant m_declarations;
    ScriptElementVariant m_condition;
    ScriptElementVariant m_expression;
    ScriptElementVariant m_body;
};

class IfStatement : public ScriptElementBase<DomType::ScriptIfStatement>
{
public:
    using BaseT::BaseT;

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    ScriptElementVariant condition() const { return m_condition; }
    void setCondition(const ScriptElementVariant &condition) { m_condition = condition; }
    ScriptElementVariant consequence() { return m_consequence; }
    void setConsequence(const ScriptElementVariant &consequence) { m_consequence = consequence; }
    ScriptElementVariant alternative() { return m_alternative; }
    void setAlternative(const ScriptElementVariant &alternative) { m_alternative = alternative; }

private:
    ScriptElementVariant m_condition;
    ScriptElementVariant m_consequence;
    ScriptElementVariant m_alternative;
};

class ReturnStatement : public ScriptElementBase<DomType::ScriptReturnStatement>
{
public:
    using BaseT::BaseT;

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    ScriptElementVariant expression() const { return m_expression; }
    void setExpression(ScriptElementVariant expression) { m_expression = expression; }

private:
    ScriptElementVariant m_expression;
};

class BinaryExpression : public ScriptElementBase<DomType::ScriptBinaryExpression>
{
public:
    using BaseT::BaseT;

    enum Operator : char {
        FieldMemberAccess,
        ArrayMemberAccess,
        TO_BE_IMPLEMENTED = std::numeric_limits<char>::max(), // not required by qmlls
    };

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    ScriptElementVariant left() const { return m_left; }
    void setLeft(const ScriptElementVariant &newLeft) { m_left = newLeft; }
    ScriptElementVariant right() const { return m_right; }
    void setRight(const ScriptElementVariant &newRight) { m_right = newRight; }
    int op() const { return m_operator; }
    void setOp(Operator op) { m_operator = op; }

private:
    ScriptElementVariant m_left;
    ScriptElementVariant m_right;
    Operator m_operator = TO_BE_IMPLEMENTED;
};

class VariableDeclarationEntry : public ScriptElementBase<DomType::ScriptVariableDeclarationEntry>
{
public:
    using BaseT::BaseT;

    enum ScopeType { Var, Let, Const };

    ScopeType scopeType() const { return m_scopeType; }
    void setScopeType(ScopeType scopeType) { m_scopeType = scopeType; }

    ScriptElementVariant identifier() const { return m_identifier; }
    void setIdentifier(const ScriptElementVariant &identifier) { m_identifier = identifier; }

    ScriptElementVariant initializer() const { return m_initializer; }
    void setInitializer(const ScriptElementVariant &initializer) { m_initializer = initializer; }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

private:
    ScopeType m_scopeType;
    ScriptElementVariant m_identifier;
    ScriptElementVariant m_initializer;
};

class VariableDeclaration : public ScriptElementBase<DomType::ScriptVariableDeclaration>
{
public:
    using BaseT::BaseT;

    // minimal required overload for this to be wrapped as DomItem:
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    void updatePathFromOwner(Path p) override;
    void createFileLocations(FileLocations::Tree base) override;

    void setDeclarations(const ScriptList &list) { m_declarations = list; }
    ScriptList declarations() { return m_declarations; }

private:
    ScriptList m_declarations;
};

} // namespace ScriptElements
} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLDOMSCRIPTELEMENTS_P_H
