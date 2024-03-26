// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMASTCREATOR_P_H
#define QQMLDOMASTCREATOR_P_H

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

#include "qqmldomelements_p.h"
#include "qqmldomitem_p.h"
#include "qqmldompath_p.h"
#include "qqmldomscriptelements_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

#include <QtQml/private/qqmljsastvisitor_p.h>
#include <memory>
#include <type_traits>
#include <variant>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QQmlDomAstCreator final : public AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QQmlDomAstCreator)
    using AST::Visitor::endVisit;
    using AST::Visitor::visit;

    static constexpr const auto className = "QmlDomAstCreator";

    class DomValue
    {
    public:
        template<typename T>
        DomValue(const T &obj) : kind(T::kindValue), value(obj)
        {
        }
        DomType kind;
        std::variant<QmlObject, MethodInfo, QmlComponent, PropertyDefinition, Binding, EnumDecl,
                     EnumItem, ConstantData, Id>
                value;
    };

    class QmlStackElement
    {
    public:
        Path path;
        DomValue item;
        FileLocations::Tree fileLocations;
    };

    /*!
      \internal
      Contains a ScriptElementVariant, that can be used everywhere in the DOM representation, or a
      List that should always be inside of something else, e.g., that cannot be the root of the
      script element DOM representation.

      Also, it makes sure you do not mistreat a list as a regular script element and vice versa.

      The reason for this is that Lists can get pretty unintuitive, as a List could be a Block of
      statements or a list of variable declarations (let i = 3, j = 4, ...) or something completely
      different. Instead, always put lists inside named construct (BlockStatement,
      VariableDeclaration, ...).
     */
    class ScriptStackElement
    {
    public:
        template<typename T>
        static ScriptStackElement from(const T &obj)
        {
            if constexpr (std::is_same_v<T, ScriptElements::ScriptList>) {
                ScriptStackElement s{ ScriptElements::ScriptList::kindValue, obj };
                return s;
            } else {
                ScriptStackElement s{ obj->kind(), ScriptElementVariant::fromElement(obj) };
                return s;
            }
            Q_UNREACHABLE();
        }

        DomType kind;
        using Variant = std::variant<ScriptElementVariant, ScriptElements::ScriptList>;
        Variant value;

        ScriptElementVariant takeVariant()
        {
            Q_ASSERT_X(std::holds_alternative<ScriptElementVariant>(value), "takeVariant",
                       "Should be a variant, did the parser change?");
            return std::get<ScriptElementVariant>(std::move(value));
        }

        bool isList() const { return std::holds_alternative<ScriptElements::ScriptList>(value); };

        ScriptElements::ScriptList takeList()
        {
            Q_ASSERT_X(std::holds_alternative<ScriptElements::ScriptList>(value), "takeList",
                       "Should be a List, did the parser change?");
            return std::get<ScriptElements::ScriptList>(std::move(value));
        }

        void setSemanticScope(const QQmlJSScope::Ptr &scope)
        {
            if (auto x = std::get_if<ScriptElementVariant>(&value)) {
                x->base()->setSemanticScope(scope);
                return;
            } else if (auto x = std::get_if<ScriptElements::ScriptList>(&value)) {
                x->setSemanticScope(scope);
                return;
            }
            Q_UNREACHABLE();
        }
    };

public:
    void enableScriptExpressions(bool enable = true) { m_enableScriptExpressions = enable; }

private:

    MutableDomItem qmlFile;
    std::shared_ptr<QmlFile> qmlFilePtr;
    QVector<QmlStackElement> nodeStack;
    QList<ScriptStackElement> scriptNodeStack;
    QVector<int> arrayBindingLevels;
    FileLocations::Tree rootMap;
    bool m_enableScriptExpressions;

    template<typename T>
    QmlStackElement &currentEl(int idx = 0)
    {
        Q_ASSERT_X(idx < nodeStack.size() && idx >= 0, "currentQmlObjectOrComponentEl",
                   "Stack does not contain enough elements!");
        int i = nodeStack.size() - idx;
        while (i-- > 0) {
            DomType k = nodeStack.at(i).item.kind;
            if (k == T::kindValue)
                return nodeStack[i];
        }
        Q_ASSERT_X(false, "currentEl", "Stack does not contan object of type ");
        return nodeStack.last();
    }

    template<typename T>
    ScriptStackElement &currentScriptEl(int idx = 0)
    {
        Q_ASSERT_X(m_enableScriptExpressions, "currentScriptEl",
                   "Cannot access script elements when they are disabled!");

        Q_ASSERT_X(idx < scriptNodeStack.size() && idx >= 0, "currentQmlObjectOrComponentEl",
                   "Stack does not contain enough elements!");
        int i = scriptNodeStack.size() - idx;
        while (i-- > 0) {
            DomType k = scriptNodeStack.at(i).kind;
            if (k == T::element_type::kindValue)
                return scriptNodeStack[i];
        }
        Q_ASSERT_X(false, "currentEl", "Stack does not contain object of type ");
        return scriptNodeStack.last();
    }

    template<typename T>
    T &current(int idx = 0)
    {
        return std::get<T>(currentEl<T>(idx).item.value);
    }

    index_type currentIndex() { return currentNodeEl().path.last().headIndex(); }

    QmlStackElement &currentQmlObjectOrComponentEl(int idx = 0);

    QmlStackElement &currentNodeEl(int i = 0);
    ScriptStackElement &currentScriptNodeEl(int i = 0);

    DomValue &currentNode(int i = 0);

    void removeCurrentNode(std::optional<DomType> expectedType);
    void removeCurrentScriptNode(std::optional<DomType> expectedType);

    void pushEl(Path p, DomValue it, AST::Node *n)
    {
        nodeStack.append({ p, it, createMap(it.kind, p, n) });
    }

    FileLocations::Tree createMap(FileLocations::Tree base, Path p, AST::Node *n);

    FileLocations::Tree createMap(DomType k, Path p, AST::Node *n);

    const ScriptElementVariant &finalizeScriptExpression(const ScriptElementVariant &element,
                                                         Path pathFromOwner,
                                                         const FileLocations::Tree &base);

    const ScriptElementVariant &finalizeScriptList(AST::Node *ast, FileLocations::Tree base);
    void setScriptExpression (const std::shared_ptr<ScriptExpression>& value);

    Path pathOfLastScriptNode() const;

    /*!
       \internal
       Helper to create string literals from AST nodes.
     */
    template<typename AstNodeT>
    static std::shared_ptr<ScriptElements::Literal> makeStringLiteral(QStringView value,
                                                                      AstNodeT *ast)
    {
        auto myExp = std::make_shared<ScriptElements::Literal>(ast->firstSourceLocation(),
                                                               ast->lastSourceLocation());
        myExp->setLiteralValue(value.toString());
        return myExp;
    }

    static std::shared_ptr<ScriptElements::Literal> makeStringLiteral(QStringView value,
                                                                      QQmlJS::SourceLocation loc)
    {
        auto myExp = std::make_shared<ScriptElements::Literal>(loc);
        myExp->setLiteralValue(value.toString());
        return myExp;
    }

    /*!
       \internal
       Helper to create script elements from AST nodes, as the DOM classes should be completely
       dependency-free from AST and parser classes. Using the AST classes in qqmldomastcreator is
       fine because it needs them for the construction/visit. \sa makeScriptList
     */
    template<typename ScriptElementT, typename AstNodeT,
             typename Enable =
                     std::enable_if_t<!std::is_same_v<ScriptElementT, ScriptElements::ScriptList>>>
    static decltype(auto) makeScriptElement(AstNodeT *ast)
    {
        auto myExp = std::make_shared<ScriptElementT>(ast->firstSourceLocation(),
                                                      ast->lastSourceLocation());
        return myExp;
    }

    /*!
       \internal
       Helper to create generic script elements from AST nodes.
       \sa makeScriptElement
     */
    template<typename AstNodeT>
    static std::shared_ptr<ScriptElements::GenericScriptElement>
    makeGenericScriptElement(AstNodeT *ast, DomType kind)
    {
        auto myExp = std::make_shared<ScriptElements::GenericScriptElement>(
                ast->firstSourceLocation(), ast->lastSourceLocation());
        myExp->setKind(kind);
        return myExp;
    }

    static std::shared_ptr<ScriptElements::GenericScriptElement>
    makeGenericScriptElement(SourceLocation location, DomType kind)
    {
        auto myExp = std::make_shared<ScriptElements::GenericScriptElement>(location);
        myExp->setKind(kind);
        return myExp;
    }

    /*!
       \internal
       Helper to create script lists from AST nodes.
       \sa makeScriptElement
     */
    template<typename AstNodeT>
    static decltype(auto) makeScriptList(AstNodeT *ast)
    {
        auto myExp =
                ScriptElements::ScriptList(ast->firstSourceLocation(), ast->lastSourceLocation());
        return myExp;
    }

    template<typename ScriptElementT>
    void pushScriptElement(ScriptElementT element)
    {
        Q_ASSERT_X(m_enableScriptExpressions, "pushScriptElement",
                   "Cannot create script elements when they are disabled!");
        scriptNodeStack.append(ScriptStackElement::from(element));
    }

    void disableScriptElements()
    {
        m_enableScriptExpressions = false;
        scriptNodeStack.clear();
    }

    ScriptElementVariant scriptElementForQualifiedId(AST::UiQualifiedId *expression);

public:
    QQmlDomAstCreator(MutableDomItem qmlFile);

    bool visit(AST::UiProgram *program) override;
    void endVisit(AST::UiProgram *) override;

    bool visit(AST::UiPragma *el) override;

    bool visit(AST::UiImport *el) override;

    bool visit(AST::UiPublicMember *el) override;
    void endVisit(AST::UiPublicMember *el) override;

    bool visit(AST::UiSourceElement *el) override;
    void endVisit(AST::UiSourceElement *) override;

    void loadAnnotations(AST::UiObjectMember *el) { AST::Node::accept(el->annotations, this); }

    bool visit(AST::UiObjectDefinition *el) override;
    void endVisit(AST::UiObjectDefinition *) override;

    bool visit(AST::UiObjectBinding *el) override;
    void endVisit(AST::UiObjectBinding *) override;

    bool visit(AST::UiScriptBinding *el) override;
    void endVisit(AST::UiScriptBinding *) override;

    bool visit(AST::UiArrayBinding *el) override;
    void endVisit(AST::UiArrayBinding *) override;

    bool visit(AST::UiQualifiedId *) override;

    bool visit(AST::UiEnumDeclaration *el) override;
    void endVisit(AST::UiEnumDeclaration *) override;

    bool visit(AST::UiEnumMemberList *el) override;
    void endVisit(AST::UiEnumMemberList *el) override;

    bool visit(AST::UiInlineComponent *el) override;
    void endVisit(AST::UiInlineComponent *) override;

    bool visit(AST::UiRequired *el) override;

    bool visit(AST::UiAnnotation *el) override;
    void endVisit(AST::UiAnnotation *) override;

    // for Script elements:
    bool visit(AST::BinaryExpression *exp) override;
    void endVisit(AST::BinaryExpression *exp) override;

    bool visit(AST::Block *block) override;
    void endVisit(AST::Block *) override;

    bool visit(AST::ReturnStatement *block) override;
    void endVisit(AST::ReturnStatement *) override;

    bool visit(AST::ForStatement *forStatement) override;
    void endVisit(AST::ForStatement *forStatement) override;

    bool visit(AST::PatternElement *pe) override;
    void endVisit(AST::PatternElement *pe) override;
    void endVisitHelper(AST::PatternElement *pe,
                        const std::shared_ptr<ScriptElements::GenericScriptElement> &element);

    bool visit(AST::IfStatement *) override;
    void endVisit(AST::IfStatement *) override;

    bool visit(AST::FieldMemberExpression *) override;
    void endVisit(AST::FieldMemberExpression *) override;

    bool visit(AST::ArrayMemberExpression *) override;
    void endVisit(AST::ArrayMemberExpression *) override;

    bool visit(AST::CallExpression *) override;
    void endVisit(AST::CallExpression *) override;

    bool visit(AST::ArrayPattern *) override;
    void endVisit(AST::ArrayPattern *) override;

    bool visit(AST::ObjectPattern *) override;
    void endVisit(AST::ObjectPattern *) override;

    bool visit(AST::PatternProperty *) override;
    void endVisit(AST::PatternProperty *) override;

    bool visit(AST::VariableStatement *) override;
    void endVisit(AST::VariableStatement *) override;

    bool visit(AST::Type *expression) override;
    void endVisit(AST::Type *expression) override;

    bool visit(AST::ClassExpression *) override;
    void endVisit(AST::ClassExpression *) override;

    // lists of stuff whose children do not need a qqmljsscope: visitation order can be custom
    bool visit(AST::ArgumentList *) override;
    bool visit(AST::UiParameterList *) override;
    bool visit(AST::PatternElementList *) override;
    bool visit(AST::PatternPropertyList *) override;
    bool visit(AST::VariableDeclarationList *vdl) override;
    bool visit(AST::Elision *elision) override;

    // lists of stuff whose children need a qqmljsscope: visitation order cannot be custom
    bool visit(AST::StatementList *list) override;
    void endVisit(AST::StatementList *list) override;

    // literals and ids
    bool visit(AST::IdentifierExpression *expression) override;
    bool visit(AST::NumericLiteral *expression) override;
    bool visit(AST::StringLiteral *expression) override;
    bool visit(AST::NullExpression *expression) override;
    bool visit(AST::TrueLiteral *expression) override;
    bool visit(AST::FalseLiteral *expression) override;
    bool visit(AST::ComputedPropertyName *expression) override;
    bool visit(AST::IdentifierPropertyName *expression) override;
    bool visit(AST::NumericLiteralPropertyName *expression) override;
    bool visit(AST::StringLiteralPropertyName *expression) override;
    bool visit(AST::TypeAnnotation *expression) override;

    void throwRecursionDepthError() override;

public:
    friend class QQmlDomAstCreatorWithQQmlJSScope;
};

class QQmlDomAstCreatorWithQQmlJSScope : public AST::Visitor
{
public:
    QQmlDomAstCreatorWithQQmlJSScope(MutableDomItem &qmlFile, QQmlJSLogger *logger);

#define X(name)                       \
    bool visit(AST::name *) override; \
    void endVisit(AST::name *) override;
    QQmlJSASTClassListToVisit
#undef X

    virtual void throwRecursionDepthError() override;
    /*!
       \internal
       Disable the DOM for scriptexpressions, as not yet unimplemented script elements might crash
       the construction.
     */
    void enableScriptExpressions(bool enable = true)
    {
        m_enableScriptExpressions = enable;
        m_domCreator.enableScriptExpressions(enable);
    }

    QQmlJSImporter &importer() { return m_importer; }
    QQmlJSImportVisitor &scopeCreator() { return m_scopeCreator; }

private:
    void setScopeInDomAfterEndvisit();
    void setScopeInDomBeforeEndvisit();

    template<typename U, typename... V>
    using IsInList = std::disjunction<std::is_same<U, V>...>;
    template<typename U>
    using RequiresCustomIteration = IsInList<U, AST::PatternElementList, AST::PatternPropertyList,
                                             AST::FormalParameterList>;

    template<typename T>
    void customListIteration(T *t)
    {
        static_assert(RequiresCustomIteration<T>::value);
        for (auto it = t; it; it = it->next) {
            if constexpr (std::is_same_v<T, AST::PatternElementList>) {
                AST::Node::accept(it->elision, this);
                AST::Node::accept(it->element, this);
            } else if constexpr (std::is_same_v<T, AST::PatternPropertyList>) {
                AST::Node::accept(it->property, this);
            } else if constexpr (std::is_same_v<T, AST::FormalParameterList>) {
                AST::Node::accept(it->element, this);
            } else {
                Q_UNREACHABLE();
            }
        }
    }

    template<typename T>
    bool visitT(T *t)
    {
        if (m_marker && m_marker->nodeKind == t->kind) {
            m_marker->count += 1;
        }

        // first case: no marker, both can visit
        if (!m_marker) {
            bool continueForDom = m_domCreator.visit(t);
            bool continueForScope = m_scopeCreator.visit(t);
            if (!continueForDom && !continueForScope)
                return false;
            else if (continueForDom ^ continueForScope) {
                m_marker.emplace();
                m_marker->inactiveVisitor = continueForDom ? ScopeCreator : DomCreator;
                m_marker->count = 1;
                m_marker->nodeKind = AST::Node::Kind(t->kind);

                if constexpr (RequiresCustomIteration<T>::value) {
                    customListIteration(t);
                    return false;
                } else {
                    return true;
                }
            } else {
                Q_ASSERT(continueForDom && continueForScope);

                if constexpr (RequiresCustomIteration<T>::value) {
                    customListIteration(t);
                    return false;
                } else {
                    return true;
                }
            }
            Q_UNREACHABLE();
        }

        // second case: a marker, just one visit
        switch (m_marker->inactiveVisitor) {
        case DomCreator: {
            const bool continueForScope = m_scopeCreator.visit(t);
            if (continueForScope) {
                if constexpr (RequiresCustomIteration<T>::value) {
                    customListIteration(t);
                    return false;
                }
            }
            return continueForScope;
        }
        case ScopeCreator: {
            const bool continueForDom = m_domCreator.visit(t);
            if (continueForDom) {
                if constexpr (RequiresCustomIteration<T>::value) {
                    customListIteration(t);
                    return false;
                }
            }
            return continueForDom;
        }
        };
        Q_UNREACHABLE();
    }

    template<typename T>
    void endVisitT(T *t)
    {
        if (m_marker && m_marker->nodeKind == t->kind) {
            m_marker->count -= 1;
            if (m_marker->count == 0)
                m_marker.reset();
        }

        if (m_marker) {
            switch (m_marker->inactiveVisitor) {
            case DomCreator: {
                m_scopeCreator.endVisit(t);
                return;
            }
            case ScopeCreator: {
                m_domCreator.endVisit(t);
                return;
            }
            };
            Q_UNREACHABLE();
        }

        setScopeInDomBeforeEndvisit();
        m_domCreator.endVisit(t);
        setScopeInDomAfterEndvisit();
        m_scopeCreator.endVisit(t);
    }

    QQmlJSScope::Ptr m_root;
    QQmlJSLogger *m_logger;
    QQmlJSImporter m_importer;
    QString m_implicitImportDirectory;
    QQmlJSImportVisitor m_scopeCreator;
    QQmlDomAstCreator m_domCreator;

    enum InactiveVisitor : bool { DomCreator, ScopeCreator };
    struct Marker
    {
        qsizetype count;
        AST::Node::Kind nodeKind;
        InactiveVisitor inactiveVisitor;
    };
    std::optional<Marker> m_marker;
    bool m_enableScriptExpressions;
};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QQMLDOMASTCREATOR_P_H
