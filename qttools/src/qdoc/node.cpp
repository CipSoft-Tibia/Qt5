/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "node.h"
#include "tree.h"
#include "codemarker.h"
#include "cppcodeparser.h"
#include <quuid.h>
#include "qdocdatabase.h"
#include <qdebug.h>
#include "generator.h"
#include "tokenizer.h"
#include "puredocparser.h"

QT_BEGIN_NAMESPACE

int Node::propertyGroupCount_ = 0;
QStringMap Node::operators_;
QMap<QString,Node::NodeType> Node::goals_;

/*!
  Initialize the map of search goals. This is called once
  by QDocDatabase::initializeDB(). The map key is a string
  representing a value in the enum Node::NodeType. The map value
  is the enum value.

  There should be an entry in the map for each value in the
  NodeType enum.
 */
void Node::initialize()
{
    goals_.insert("class", Node::Class);
    goals_.insert("qmltype", Node::QmlType);
    goals_.insert("page", Node::Document);
    goals_.insert("function", Node::Function);
    goals_.insert("property", Node::Property);
    goals_.insert("variable", Node::Variable);
    goals_.insert("group", Node::Group);
    goals_.insert("module", Node::Module);
    goals_.insert("qmlmodule", Node::QmlModule);
    goals_.insert("qmppropertygroup", Node::QmlPropertyGroup);
    goals_.insert("qmlproperty", Node::QmlProperty);
    goals_.insert("qmlsignal", Node::QmlSignal);
    goals_.insert("qmlsignalhandler", Node::QmlSignalHandler);
    goals_.insert("qmlmethod", Node::QmlMethod);
    goals_.insert("qmlbasictype", Node::QmlBasicType);
    goals_.insert("enum", Node::Enum);
    goals_.insert("typealias", Node::Typedef);
    goals_.insert("typedef", Node::Typedef);
    goals_.insert("namespace", Node::Namespace);
}

bool Node::nodeNameLessThan(const Node *n1, const Node *n2)
{
    if (n1->isAggregate() && n2->isAggregate()) {
        const Aggregate* f1 = static_cast<const Aggregate*>(n1);
        const Aggregate* f2 = static_cast<const Aggregate*>(n2);

        if (f1->fullTitle() < f2->fullTitle())
            return true;
        else if (f1->fullTitle() > f2->fullTitle())
            return false;
    }

    if (n1->type() == Node::Function && n2->type() == Node::Function) {
        const FunctionNode* f1 = static_cast<const FunctionNode*>(n1);
        const FunctionNode* f2 = static_cast<const FunctionNode*>(n2);

        if (f1->isConst() < f2->isConst())
            return true;
        else if (f1->isConst() > f2->isConst())
            return false;

        if (f1->signature(false) < f2->signature(false))
            return true;
        else if (f1->signature(false) > f2->signature(false))
            return false;
    }

    if (n1->location().filePath() < n2->location().filePath())
        return true;
    else if (n1->location().filePath() > n2->location().filePath())
        return false;

    if (n1->type() < n2->type())
        return true;
    else if (n1->type() > n2->type())
        return false;

    if (n1->name() < n2->name())
        return true;
    else if (n1->name() > n2->name())
        return false;

    if (n1->access() < n2->access())
        return true;
    else if (n1->access() > n2->access())
        return false;

    return false;
}

/*!
  Increment the number of property groups seen in the current
  file, and return the new value.
 */
int Node::incPropertyGroupCount() { return ++propertyGroupCount_; }

/*!
  Reset the number of property groups seen in the current file
  to 0, because we are starting a new file.
 */
void Node::clearPropertyGroupCount() { propertyGroupCount_ = 0; }

/*!
  \class Node
  \brief The Node class is a node in the Tree.

  A Node represents a class or function or something else
  from the source code..
 */

/*!
  When this Node is destroyed, if it has a parent Node, it
  removes itself from the parent node's child list.
 */
Node::~Node()
{
    if (parent_)
        parent_->removeChild(this);

    if (relatesTo_)
        removeRelates();
}

/*!
    Removes this node from the aggregate's list of related
    nodes, or if this node has created a dummy "relates"
    aggregate, deletes it.
*/
void Node::removeRelates()
{
    if (!relatesTo_)
        return;

    if (relatesTo_->isDocumentNode() && !relatesTo_->parent()) {
        delete relatesTo_;
        relatesTo_ = 0;
    } else {
        relatesTo_->removeRelated(this);
    }
}

/*!
  Returns this node's name member. Appends "()" to the returned
  name if this node is a function node, but not if it is a macro
  because macro names normally appear without parentheses.
 */
QString Node::plainName() const
{
    if (isFunction() && !isMacro())
        return name_ + QLatin1String("()");
    return name_;
}

/*!
  Constructs and returns the node's fully qualified name by
  recursively ascending the parent links and prepending each
  parent name + "::". Breaks out when the parent pointer is
  \a relative. Almost all calls to this function pass 0 for
  \a relative.
 */
QString Node::plainFullName(const Node* relative) const
{
    if (name_.isEmpty())
        return QLatin1String("global");

    QString fullName;
    const Node* node = this;
    while (node) {
        fullName.prepend(node->plainName());
        if (node->parent() == relative || node->parent()->name().isEmpty())
            break;
        fullName.prepend(QLatin1String("::"));
        node = node->parent();
    }
    return fullName;
}

/*!
  Constructs and returns the node's fully qualified signature
  by recursively ascending the parent links and prepending each
  parent name + "::" to the plain signature. The return type is
  not included.
 */
QString Node::plainSignature() const
{
    if (name_.isEmpty())
        return QLatin1String("global");

    QString fullName;
    const Node* node = this;
    while (node) {
        fullName.prepend(node->signature(false, true));
        if (node->parent()->name().isEmpty())
            break;
        fullName.prepend(QLatin1String("::"));
        node = node->parent();
    }
    return fullName;
}

/*!
  Constructs and returns this node's full name.
 */
QString Node::fullName(const Node* relative) const
{
    if ((isDocumentNode() || isGroup()) && !title().isEmpty())
        return title();
    return plainFullName(relative);
}

/*!
  Try to match this node's type and subtype with one of the
  pairs in \a types. If a match is found, return true. If no
  match is found, return false.

  \a types is a list of type/subtype pairs, where the first
  value in the pair is a Node::NodeType, and the second value is
  a Node::DocSubtype. The second value is used in the match if
  this node's type is Node::Document.
 */
bool Node::match(const NodeTypeList& types) const
{
    for (int i=0; i<types.size(); ++i) {
        if (type() == types.at(i).first) {
            if (type() == Node::Document) {
                if (docSubtype() == types.at(i).second)
                    return true;
            }
            else
                return true;
        }
    }
    return false;
}

/*!
  Sets this Node's Doc to \a doc. If \a replace is false and
  this Node already has a Doc, a warning is reported that the
  Doc is being overridden, and it reports where the previous
  Doc was found. If \a replace is true, the Doc is replaced
  silently.
 */
void Node::setDoc(const Doc& doc, bool replace)
{
    if (!doc_.isEmpty() && !replace) {
        doc.location().warning(tr("Overrides a previous doc"));
        doc_.location().warning(tr("(The previous doc is here)"));
    }
    doc_ = doc;
}

/*!
  Construct a node with the given \a type and having the
  given \a parent and \a name. The new node is added to the
  parent's child list.
 */
Node::Node(NodeType type, Aggregate *parent, const QString& name)
    : nodeType_((unsigned char) type),
      access_((unsigned char) Public),
      safeness_((unsigned char) UnspecifiedSafeness),
      pageType_((unsigned char) NoPageType),
      status_((unsigned char) Active),
      indexNodeFlag_(false),
      parent_(parent),
      relatesTo_(0),
      sharedCommentNode_(0),
      name_(name)
{
    if (parent_)
        parent_->addChild(this);
    outSubDir_ = Generator::outputSubdir();
    if (operators_.isEmpty()) {
        operators_.insert("++","inc");
        operators_.insert("--","dec");
        operators_.insert("==","eq");
        operators_.insert("!=","ne");
        operators_.insert("<<","lt-lt");
        operators_.insert(">>","gt-gt");
        operators_.insert("+=","plus-assign");
        operators_.insert("-=","minus-assign");
        operators_.insert("*=","mult-assign");
        operators_.insert("/=","div-assign");
        operators_.insert("%=","mod-assign");
        operators_.insert("&=","bitwise-and-assign");
        operators_.insert("|=","bitwise-or-assign");
        operators_.insert("^=","bitwise-xor-assign");
        operators_.insert("<<=","bitwise-left-shift-assign");
        operators_.insert(">>=","bitwise-right-shift-assign");
        operators_.insert("||","logical-or");
        operators_.insert("&&","logical-and");
        operators_.insert("()","call");
        operators_.insert("[]","subscript");
        operators_.insert("->","pointer");
        operators_.insert("->*","pointer-star");
        operators_.insert("+","plus");
        operators_.insert("-","minus");
        operators_.insert("*","mult");
        operators_.insert("/","div");
        operators_.insert("%","mod");
        operators_.insert("|","bitwise-or");
        operators_.insert("&","bitwise-and");
        operators_.insert("^","bitwise-xor");
        operators_.insert("!","not");
        operators_.insert("~","bitwise-not");
        operators_.insert("<=","lt-eq");
        operators_.insert(">=","gt-eq");
        operators_.insert("<","lt");
        operators_.insert(">","gt");
        operators_.insert("=","assign");
        operators_.insert(",","comma");
        operators_.insert("delete[]","delete-array");
        operators_.insert("delete","delete");
        operators_.insert("new[]","new-array");
        operators_.insert("new","new");
    }
}

/*! \fn QString Node::url() const
  Returns the node's URL.
 */

/*! \fn void Node::setUrl(const QString &url)
  Sets the node's URL to \a url
 */

/*!
  Returns this node's page type as a string, for use as an
  attribute value in XML or HTML.
 */
QString Node::pageTypeString() const
{
    return pageTypeString((PageType) pageType_);
}

/*!
  Returns the page type \a t as a string, for use as an
  attribute value in XML or HTML.
 */
QString Node::pageTypeString(unsigned char t)
{
    switch ((PageType)t) {
    case Node::AttributionPage:
        return "attribution";
    case Node::ApiPage:
        return "api";
    case Node::ArticlePage:
        return "article";
    case Node::ExamplePage:
        return "example";
    case Node::HowToPage:
        return "howto";
    case Node::OverviewPage:
        return "overview";
    case Node::TutorialPage:
        return "tutorial";
    case Node::FAQPage:
        return "faq";
    case Node::DitaMapPage:
        return "ditamap";
    default:
        return "article";
    }
}

/*!
  Returns this node's type as a string for use as an
  attribute value in XML or HTML.
 */
QString Node::nodeTypeString() const
{
    return nodeTypeString(type());
}

/*!
  Returns the node type \a t as a string for use as an
  attribute value in XML or HTML.
 */
QString Node::nodeTypeString(unsigned char t)
{
    switch ((NodeType)t) {
    case Namespace:
        return "namespace";
    case Class:
        return "class";
    case Document:
        return "document";
    case Enum:
        return "enum";
    case Typedef:
        return "typedef";
    case Function:
        return "function";
    case Property:
        return "property";
    case Variable:
        return "variable";
    case Group:
        return "group";
    case Module:
        return "module";
    case QmlType:
        return "QML type";
    case QmlBasicType:
        return "QML basic type";
    case QmlModule:
        return "QML module";
    case QmlProperty:
        return "QML property";
    case QmlPropertyGroup:
        return "QML property group";
    case QmlSignal:
        return "QML signal";
    case QmlSignalHandler:
        return "QML signal handler";
    case QmlMethod:
        return "QML method";
    case SharedComment:
        return "shared comment";
    default:
        break;
    }
    return QString();
}

/*!
  Returns this node's subtype as a string for use as an
  attribute value in XML or HTML. This is only useful
  in the case where the node type is Document.
 */
QString Node::nodeSubtypeString() const
{
    return nodeSubtypeString(docSubtype());
}

/*!
  Returns the node subtype \a t as a string for use as an
  attribute value in XML or HTML. This is only useful
  in the case where the node type is Document.
 */
QString Node::nodeSubtypeString(unsigned char t)
{
    switch ((DocSubtype)t) {
    case Example:
        return "example";
    case HeaderFile:
        return "header file";
    case File:
        return "file";
    case Image:
        return "image";
    case Page:
        return "page";
    case ExternalPage:
        return "external page";
    case DitaMap:
        return "ditamap";
    case NoSubtype:
    default:
        break;
    }
    return QString();
}

/*!
  Set the page type according to the string \a t.
 */
void Node::setPageType(const QString& t)
{
    if ((t == "API") || (t == "api"))
        pageType_ = (unsigned char) ApiPage;
    else if (t == "howto")
        pageType_ = (unsigned char) HowToPage;
    else if (t == "overview")
        pageType_ = (unsigned char) OverviewPage;
    else if (t == "tutorial")
        pageType_ = (unsigned char) TutorialPage;
    else if (t == "faq")
        pageType_ = (unsigned char) FAQPage;
    else if (t == "article")
        pageType_ = (unsigned char) ArticlePage;
    else if (t == "example")
        pageType_ = (unsigned char) ExamplePage;
    else if (t == "ditamap")
        pageType_ = (unsigned char) DitaMapPage;
}

/*! Converts the boolean value \a b to an enum representation
  of the boolean type, which includes an enum value for the
  \e {default value} of the item, i.e. true, false, or default.
 */
Node::FlagValue Node::toFlagValue(bool b)
{
    return b ? FlagValueTrue : FlagValueFalse;
}

/*!
  Converts the enum \a fv back to a boolean value.
  If \a fv is neither the true enum value nor the
  false enum value, the boolean value returned is
  \a defaultValue.

  Note that runtimeDesignabilityFunction() should be called
  first. If that function returns the name of a function, it
  means the function must be called at runtime to determine
  whether the property is Designable.
 */
bool Node::fromFlagValue(FlagValue fv, bool defaultValue)
{
    switch (fv) {
    case FlagValueTrue:
        return true;
    case FlagValueFalse:
        return false;
    default:
        return defaultValue;
    }
}

/*!
  Sets the pointer to the node that this node relates to.
 */
void Node::setRelates(Aggregate *pseudoParent)
{
    if (pseudoParent == parent())
        return;

    removeRelates();
    relatesTo_ = pseudoParent;
    pseudoParent->addRelated(this);
}

/*!
  Sets the (unresolved) entity \a name that this node relates to.
 */
void Node::setRelates(const QString& name)
{
    removeRelates();
    // Create a dummy aggregate for writing the name into the index
    relatesTo_ = new DocumentNode(0, name, Node::NoSubtype, Node::NoPageType);
}

/*!
  This function creates a pair that describes a link.
  The pair is composed from \a link and \a desc. The
  \a linkType is the map index the pair is filed under.
 */
void Node::setLink(LinkType linkType, const QString &link, const QString &desc)
{
    QPair<QString,QString> linkPair;
    linkPair.first = link;
    linkPair.second = desc;
    linkMap_[linkType] = linkPair;
}

/*!
    Sets the information about the project and version a node was introduced
    in. The string is simplified, removing excess whitespace before being
    stored.
*/
void Node::setSince(const QString &since)
{
    since_ = since.simplified();
}

/*!
  Returns a string representing the access specifier.
 */
QString Node::accessString() const
{
    switch ((Access) access_) {
    case Protected:
        return "protected";
    case Private:
        return "private";
    case Public:
    default:
        break;
    }
    return "public";
}

/*!
  Extract a class name from the type \a string and return it.
 */
QString Node::extractClassName(const QString &string) const
{
    QString result;
    for (int i=0; i<=string.size(); ++i) {
        QChar ch;
        if (i != string.size())
            ch = string.at(i);

        QChar lower = ch.toLower();
        if ((lower >= QLatin1Char('a') && lower <= QLatin1Char('z')) ||
                ch.digitValue() >= 0 ||
                ch == QLatin1Char('_') ||
                ch == QLatin1Char(':')) {
            result += ch;
        }
        else if (!result.isEmpty()) {
            if (result != QLatin1String("const"))
                return result;
            result.clear();
        }
    }
    return result;
}

/*!
  Returns a string representing the access specifier.
 */
QString RelatedClass::accessString() const
{
    switch (access_) {
    case Node::Protected:
        return "protected";
    case Node::Private:
        return "private";
    case Node::Public:
    default:
        break;
    }
    return "public";
}

/*!
  Returns the inheritance status.
 */
Node::Status Node::inheritedStatus() const
{
    Status parentStatus = Active;
    if (parent_)
        parentStatus = parent_->inheritedStatus();
    return (Status)qMin((int)status_, (int)parentStatus);
}

/*!
  Returns the thread safeness value for whatever this node
  represents. But if this node has a parent and the thread
  safeness value of the parent is the same as the thread
  safeness value of this node, what is returned is the
  value \c{UnspecifiedSafeness}. Why?
 */
Node::ThreadSafeness Node::threadSafeness() const
{
    if (parent_ && (ThreadSafeness) safeness_ == parent_->inheritedThreadSafeness())
        return UnspecifiedSafeness;
    return (ThreadSafeness) safeness_;
}

/*!
  If this node has a parent, the parent's thread safeness
  value is returned. Otherwise, this node's thread safeness
  value is returned. Why?
 */
Node::ThreadSafeness Node::inheritedThreadSafeness() const
{
    if (parent_ && (ThreadSafeness) safeness_ == UnspecifiedSafeness)
        return parent_->inheritedThreadSafeness();
    return (ThreadSafeness) safeness_;
}


/*!
  If this node is a QML or JS type node, return a pointer to
  it. If it is a child of a QML or JS type node, return the
  pointer to its parent QMLor JS type node. Otherwise return
  0;
 */
QmlTypeNode* Node::qmlTypeNode()
{
    if (isQmlNode() || isJsNode()) {
        Node* n = this;
        while (n && !(n->isQmlType() || n->isJsType()))
            n = n->parent();
        if (n && (n->isQmlType() || n->isJsType()))
            return static_cast<QmlTypeNode*>(n);
    }
    return 0;
}

/*!
  If this node is a QML node, find its QML class node,
  and return a pointer to the C++ class node from the
  QML class node. That pointer will be null if the QML
  class node is a component. It will be non-null if
  the QML class node is a QML element.
 */
ClassNode* Node::declarativeCppNode()
{
    QmlTypeNode* qcn = qmlTypeNode();
    if (qcn)
        return qcn->classNode();
    return 0;
}

/*!
  Returns \c true if the node's status is Internal, or if its
  parent is a class with internal status.
 */
bool Node::isInternal() const
{
    if (status() == Internal)
        return true;
    if (parent() && parent()->status() == Internal)
        return true;
    if (relates() && relates()->status() == Internal)
        return true;
    return false;
}

/*!
  Returns a pointer to the root of the Tree this node is in.
 */
const Node* Node::root() const
{
    return (parent() ? parent()->root() : this);
}

/*!
  Returns a pointer to the Tree this node is in.
 */
Tree* Node::tree() const
{
    return root()->tree();
}

/*!
  Sets the node's declaration location, its definition
  location, or both, depending on the suffix of the file
  name from the file path in location \a t.
 */
void Node::setLocation(const Location& t)
{
    QString suffix = t.fileSuffix();
    if (suffix == "h")
        declLocation_ = t;
    else if (suffix == "cpp")
        defLocation_ = t;
    else {
        declLocation_ = t;
        defLocation_ = t;
    }
}

/*!
  Adds this node to the shared comment node \a t.
 */
void Node::setSharedCommentNode(SharedCommentNode* t)
{
    sharedCommentNode_ = t;
    t->append(this);
}

/*!
  Returns true if this node is sharing a comment and the
  shared comment is not empty.
 */
bool Node::hasSharedDoc() const
{
    return (sharedCommentNode_ && sharedCommentNode_->hasDoc());
}

/*!
  \class Aggregate
 */

/*!
  The inner node destructor deletes the children and removes
  this node from its related nodes.
 */
Aggregate::~Aggregate()
{
    removeFromRelated();
    deleteChildren();
}

/*!
  If \a genus is \c{Node::DontCare}, find the first node in
  this node's child list that has the given \a name. If this
  node is a QML type, be sure to also look in the children
  of its property group nodes. Return the matching node or 0.

  If \a genus is either \c{Node::CPP} or \c {Node::QML}, then
  find all this node's children that have the given \a name,
  and return the one that satisfies the \a genus requirement.
 */
Node *Aggregate::findChildNode(const QString& name, Node::Genus genus, int findFlags) const
{
    if (genus == Node::DontCare) {
        Node *node = childMap_.value(name);
        if (node && !node->isQmlPropertyGroup()) // mws asks: Why not property group?
            return node;
        if (isQmlType() || isJsType()) {
            for (int i=0; i<children_.size(); ++i) {
                Node* n = children_.at(i);
                if (n->isQmlPropertyGroup() || isJsPropertyGroup()) {
                    node = static_cast<Aggregate*>(n)->findChildNode(name, genus);
                    if (node)
                        return node;
                }
            }
        }
    } else {
        NodeList nodes = childMap_.values(name);
        if (!nodes.isEmpty()) {
            for (int i = 0; i < nodes.size(); ++i) {
                Node* node = nodes.at(i);
                if (genus == node->genus()) {
                    if (findFlags & TypesOnly) {
                        if (!node->isTypedef()
                                && !node->isClass()
                                && !node->isQmlType()
                                && !node->isQmlBasicType()
                                && !node->isJsType()
                                && !node->isJsBasicType()
                                && !node->isEnumType())
                            continue;
                    } else if (findFlags & IgnoreModules && node->isModule())
                        continue;
                    return node;
                }
            }
        }
    }
    if (genus != Node::DontCare && this->genus() != genus)
        return nullptr;
    return primaryFunctionMap_.value(name);
}

/*!
  Find all the child nodes of this node that are named
  \a name and return them in \a nodes.
 */
void Aggregate::findChildren(const QString& name, NodeList& nodes) const
{
    nodes = childMap_.values(name);
    Node* n = primaryFunctionMap_.value(name);
    if (n) {
        nodes.append(n);
        NodeList t = secondaryFunctionMap_.value(name);
        if (!t.isEmpty())
            nodes.append(t);
    }
    if (!nodes.isEmpty() || !(isQmlNode() || isJsNode()))
        return;
    int i = name.indexOf(QChar('.'));
    if (i < 0)
        return;
    QString qmlPropGroup = name.left(i);
    NodeList t = childMap_.values(qmlPropGroup);
    if (t.isEmpty())
        return;
    foreach (Node* n, t) {
        if (n->isQmlPropertyGroup() || n->isJsPropertyGroup()) {
            n->findChildren(name, nodes);
            if (!nodes.isEmpty())
                break;
        }
    }
}

/*!
  This function is like findChildNode(), but if a node
  with the specified \a name is found but it is not of the
  specified \a type, 0 is returned.
 */
Node* Aggregate::findChildNode(const QString& name, NodeType type)
{
    if (type == Function)
        return primaryFunctionMap_.value(name);
    else {
        NodeList nodes = childMap_.values(name);
        for (int i=0; i<nodes.size(); ++i) {
            Node* node = nodes.at(i);
            if (node->type() == type)
                return node;
        }
    }
    return 0;
}

/*!
  Find a function node that is a child of this nose, such
  that the function node has the specified \a name.
 */
FunctionNode *Aggregate::findFunctionNode(const QString& name, const QString& params) const
{
    FunctionNode* pfn = static_cast<FunctionNode*>(primaryFunctionMap_.value(name));
    FunctionNode* fn = pfn;
    if (fn) {
        const QVector<Parameter>* funcParams = &(fn->parameters());
        if (params.isEmpty() && funcParams->isEmpty() && !fn->isInternal())
            return fn;
        bool isQPrivateSignal = false; // Not used in the search
        QVector<Parameter> testParams;
        if (!params.isEmpty()) {
            CppCodeParser* cppParser = PureDocParser::pureDocParser();
            cppParser->parseParameters(params, testParams, isQPrivateSignal);
        }
        NodeList funcs = secondaryFunctionMap_.value(name);
        int i = -1;
        while (fn) {
            if (testParams.size() == funcParams->size()) {
                if (testParams.isEmpty() && !fn->isInternal())
                    return fn;
                bool different = false;
                for (int j=0; j<testParams.size(); j++) {
                    if (testParams.at(j).dataType() != funcParams->at(j).dataType()) {
                        different = true;
                        break;
                    }
                }
                if (!different && !fn->isInternal())
                    return fn;
            }
            if (++i < funcs.size()) {
                fn = static_cast<FunctionNode*>(funcs.at(i));
                funcParams = &(fn->parameters());
            }
            else
                fn = 0;
        }
        /*
          Most \l commands that link to functions don't include
          the parameter declarations in the function signature,
          so if the \l is meant to go to a function that does
          have parameters, the algorithm above won't find it.
          Therefore we must return the pointer to the function
          in the primary function map in the cases where the
          parameters should have been specified in the \l command.
          But if the primary function is marked internal, search
          the secondary list to find one that is not marked internal.
        */
        if (!fn) {
            if (!testParams.empty())
                return 0;
            if (pfn && !pfn->isInternal())
                return pfn;
            foreach (Node* n, funcs) {
                fn = static_cast<FunctionNode*>(n);
                if (!fn->isInternal())
                    return fn;
            }
        }
    }
    return fn;
}

/*!
  Find the function node that is a child of this node, such
  that the function has the same name and signature as the
  \a clone node.
 */
FunctionNode *Aggregate::findFunctionNode(const FunctionNode *clone) const
{
    QMap<QString,Node*>::ConstIterator c = primaryFunctionMap_.constFind(clone->name());
    if (c != primaryFunctionMap_.constEnd()) {
        if (isSameSignature(clone, (FunctionNode *) *c)) {
            return (FunctionNode *) *c;
        }
        else if (secondaryFunctionMap_.contains(clone->name())) {
            const NodeList& secs = secondaryFunctionMap_[clone->name()];
            NodeList::ConstIterator s = secs.constBegin();
            while (s != secs.constEnd()) {
                if (isSameSignature(clone, (FunctionNode *) *s))
                    return (FunctionNode *) *s;
                ++s;
            }
        }
    }
    return 0;
}

/*!
  Returns the list of keys from the primary function map.
 */
QStringList Aggregate::primaryKeys()
{
    QStringList t;
    QMap<QString, Node*>::iterator i = primaryFunctionMap_.begin();
    while (i != primaryFunctionMap_.end()) {
        t.append(i.key());
        ++i;
    }
    return t;
}

/*!
  Returns the list of keys from the secondary function map.
 */
QStringList Aggregate::secondaryKeys()
{
    QStringList t;
    QMap<QString, NodeList>::iterator i = secondaryFunctionMap_.begin();
    while (i != secondaryFunctionMap_.end()) {
        t.append(i.key());
        ++i;
    }
    return t;
}

/*!
  Mark all child nodes that have no documentation as having
  private access and internal status. qdoc will then ignore
  them for documentation purposes.
 */
void Aggregate::makeUndocumentedChildrenInternal()
{
    foreach (Node *child, childNodes()) {
        if (!child->isSharingComment() && !child->hasDoc() && !child->docMustBeGenerated()) {
            child->setAccess(Node::Private);
            child->setStatus(Node::Internal);
        }
    }
}

/*!
  This is where we set the overload numbers for function nodes.
  \note Overload numbers for related non-members are handled
  separately.
 */
void Aggregate::normalizeOverloads()
{
    QMap<QString, Node *>::Iterator p1 = primaryFunctionMap_.begin();
    while (p1 != primaryFunctionMap_.end()) {
        FunctionNode *primaryFunc = (FunctionNode *) *p1;
        if (primaryFunc->status() != Active || primaryFunc->access() == Private) {
            if (secondaryFunctionMap_.contains(primaryFunc->name())) {
                /*
                  Either the primary function is not active or it is private.
                  It therefore can't be the primary function. Search the list
                  of overloads to find one that can be the primary function.
                */
                NodeList& overloads = secondaryFunctionMap_[primaryFunc->name()];
                NodeList::ConstIterator s = overloads.constBegin();
                while (s != overloads.constEnd()) {
                    FunctionNode *overloadFunc = (FunctionNode *) *s;
                    /*
                      Any non-obsolete, non-private function (i.e., visible function)
                      is preferable to the current primary function. Swap the primary
                      and overload functions.
                    */
                    if (overloadFunc->status() == Active && overloadFunc->access() != Private) {
                        primaryFunc->setOverloadNumber(overloadFunc->overloadNumber());
                        overloads.replace(overloads.indexOf(overloadFunc), primaryFunc);
                        *p1 = overloadFunc;
                        overloadFunc->setOverloadFlag(false);
                        overloadFunc->setOverloadNumber(0);
                        break;
                    }
                    ++s;
                }
            }
        }
        ++p1;
    }
    /*
      Ensure that none of the primary functions is marked with \overload.
     */
    QMap<QString, Node *>::Iterator p = primaryFunctionMap_.begin();
    while (p != primaryFunctionMap_.end()) {
        FunctionNode *primaryFunc = (FunctionNode *) *p;
        if (primaryFunc->isOverload()) {
            if (secondaryFunctionMap_.contains(primaryFunc->name())) {
                /*
                  The primary function is marked with \overload. Find an
                  overload in the secondary function map that is not marked
                  with \overload but that is active and not private. Then
                  swap it with the primary function.
                */
                NodeList& overloads = secondaryFunctionMap_[primaryFunc->name()];
                NodeList::ConstIterator s = overloads.constBegin();
                while (s != overloads.constEnd()) {
                    FunctionNode *overloadFunc = (FunctionNode *) *s;
                    if (!overloadFunc->isOverload()) {
                        if (overloadFunc->status() == Active && overloadFunc->access() != Private) {
                            primaryFunc->setOverloadNumber(overloadFunc->overloadNumber());
                            overloads.replace(overloads.indexOf(overloadFunc), primaryFunc);
                            *p = overloadFunc;
                            overloadFunc->setOverloadFlag(false);
                            overloadFunc->setOverloadNumber(0);
                            break;
                        }
                    }
                    ++s;
                }
            }
        }
        ++p;
    }
    /*
      Recursive part.
     */
    NodeList::ConstIterator c = childNodes().constBegin();
    while (c != childNodes().constEnd()) {
        if ((*c)->isAggregate())
            ((Aggregate *) *c)->normalizeOverloads();
        ++c;
    }
}

/*!
 */
void Aggregate::removeFromRelated()
{
    while (!related_.isEmpty()) {
        Node *p = static_cast<Node *>(related_.takeFirst());

        if (p != 0 && p->relates() == this) p->clearRelated();
    }
}

/*!
  Deletes all this node's children.
 */
void Aggregate::deleteChildren()
{
    NodeList childrenCopy = children_;
    // Clear internal collections before deleting child nodes
    children_.clear();
    childMap_.clear();
    enumChildren_.clear();
    primaryFunctionMap_.clear();
    secondaryFunctionMap_.clear();
    qDeleteAll(childrenCopy);
}

/*! \fn bool Aggregate::isAggregate() const
  Returns \c true because this is an inner node.
 */

/*!
  Returns \c true if the node is a class node or a QML type node
  that is marked as being a wrapper class or QML type, or if
  it is a member of a wrapper class or type.
 */
bool Node::isWrapper() const
{
    return (parent_ ? parent_->isWrapper() : false);
}

/*!
  Finds the enum type node that has \a enumValue as one of
  its enum values and returns a pointer to it. Returns 0 if
  no enum type node is found that has \a enumValue as one
  of its values.
 */
const EnumNode *Aggregate::findEnumNodeForValue(const QString &enumValue) const
{
    foreach (const Node *node, enumChildren_) {
        const EnumNode *en = static_cast<const EnumNode *>(node);
        if (en->hasItem(enumValue))
            return en;
    }
    return 0;
}

/*!
  Returns a node list containing all the member functions of
  some class such that the functions overload the name \a funcName.
 */
NodeList Aggregate::overloads(const QString &funcName) const
{
    NodeList result;
    Node *primary = primaryFunctionMap_.value(funcName);
    if (primary) {
        result << primary;
        result += secondaryFunctionMap_[funcName];
    }
    return result;
}

/*!
  Construct an inner node (i.e., not a leaf node) of the
  given \a type and having the given \a parent and \a name.
 */
Aggregate::Aggregate(NodeType type, Aggregate *parent, const QString& name)
    : Node(type, parent, name), noAutoList_(false)
{
    switch (type) {
    case Class:
    case QmlType:
    case Namespace:
        setPageType(ApiPage);
        break;
    default:
        break;
    }
}

/*!
  Appends an \a include file to the list of include files.
 */
void Aggregate::addInclude(const QString& include)
{
    includes_.append(include);
}

/*!
  Sets the list of include files to \a includes.
 */
void Aggregate::setIncludes(const QStringList& includes)
{
    includes_ = includes;
}

/*!
  f1 is always the clone
 */
bool Aggregate::isSameSignature(const FunctionNode *f1, const FunctionNode *f2)
{
    if (f1->parameters().size() != f2->parameters().size())
        return false;
    if (f1->isConst() != f2->isConst())
        return false;
    if (f1->isRef() != f2->isRef())
        return false;
    if (f1->isRefRef() != f2->isRefRef())
        return false;

    QVector<Parameter>::ConstIterator p1 = f1->parameters().constBegin();
    QVector<Parameter>::ConstIterator p2 = f2->parameters().constBegin();
    while (p2 != f2->parameters().constEnd()) {
        if ((*p1).hasType() && (*p2).hasType()) {
            QString t1 = p1->dataType();
            QString t2 = p2->dataType();

            if (t1.length() < t2.length())
                qSwap(t1, t2);

            /*
              ### hack for C++ to handle superfluous
              "Foo::" prefixes gracefully
            */
            if (t1 != t2 && t1 != (f2->parent()->name() + "::" + t2)) {
                // Accept a difference in the template parametters of the type if one
                // is omited (eg. "QAtomicInteger" == "QAtomicInteger<T>")
                auto ltLoc = t1.indexOf('<');
                auto gtLoc = t1.indexOf('>', ltLoc);
                if (ltLoc < 0 || gtLoc < ltLoc)
                    return false;
                t1.remove(ltLoc, gtLoc - ltLoc + 1);
                if (t1 != t2)
                    return false;
            }
        }
        ++p1;
        ++p2;
    }
    return true;
}

/*!
  Adds the \a child to this node's child list. It might also
  be necessary to update this node's internal collections and
  the child's parent pointer and output subdirectory.
 */
void Aggregate::addChild(Node *child)
{
    children_.append(child);
    if (child->type() == Function
            || child->type() == QmlMethod
            || child->type() == QmlSignal) {
        FunctionNode *func = static_cast<FunctionNode*>(child);
        QString name = func->name();
        if (!primaryFunctionMap_.contains(name)) {
            primaryFunctionMap_.insert(name, func);
            func->setOverloadNumber(0);
        }
        else {
            NodeList &overloads = secondaryFunctionMap_[name];
            overloads.append(func);
            func->setOverloadNumber(overloads.size());
        }
    }
    else {
        if (child->type() == Enum)
            enumChildren_.append(child);
        childMap_.insertMulti(child->name(), child);
    }
    if (child->parent() == 0) {
        child->setParent(this);
        child->setOutputSubdirectory(this->outputSubdirectory());
        child->setUrl(QString());
        child->setIndexNodeFlag(isIndexNode());
    }
}

/*!
  Adds the \a child to this node's child map using \a title
  as the key. The \a child is not added to the child list
  again, because it is presumed to already be there. We just
  want to be able to find the child by its \a title.
 */
void Aggregate::addChild(Node* child, const QString& title)
{
    childMap_.insertMulti(title, child);
}

/*!
  The \a child is removed from this node's child list and
  from this node's internal collections. The child's parent
  pointer is set to 0, but its output subdirectory is not
  changed.
 */
void Aggregate::removeChild(Node *child)
{
    children_.removeAll(child);
    enumChildren_.removeAll(child);
    if (child->type() == Function
            || child->type() == QmlMethod
            || child->type() == QmlSignal) {
        QMap<QString, Node *>::Iterator primary = primaryFunctionMap_.find(child->name());
        NodeList& overloads = secondaryFunctionMap_[child->name()];
        if (primary != primaryFunctionMap_.end() && *primary == child) {
            primaryFunctionMap_.erase(primary);
            if (!overloads.isEmpty()) {
                FunctionNode* fn = static_cast<FunctionNode*>(overloads.takeFirst());
                fn->setOverloadNumber(0);
                primaryFunctionMap_.insert(child->name(), fn);
            }
        }
        else
            overloads.removeAll(child);
    }
    QMap<QString, Node *>::Iterator ent = childMap_.find(child->name());
    while (ent != childMap_.end() && ent.key() == child->name()) {
        if (*ent == child) {
            childMap_.erase(ent);
            break;
        }
        ++ent;
    }
    if (child->title().isEmpty())
        return;
    ent = childMap_.find(child->title());
    while (ent != childMap_.end() && ent.key() == child->title()) {
        if (*ent == child) {
            childMap_.erase(ent);
            break;
        }
        ++ent;
    }
    child->setParent(0);
}

/*!
 Recursively sets the output subdirectory for children
 */
void Aggregate::setOutputSubdirectory(const QString &t)
{
    Node::setOutputSubdirectory(t);
    for (int i = 0; i < childNodes().size(); ++i)
        childNodes().at(i)->setOutputSubdirectory(t);
}

/*!
  Find the module (Qt Core, Qt GUI, etc.) to which the class belongs.
  We do this by obtaining the full path to the header file's location
  and examine everything between "src/" and the filename.  This is
  semi-dirty because we are assuming a particular directory structure.

  This function is only really useful if the class's module has not
  been defined in the header file with a QT_MODULE macro or with an
  \inmodule command in the documentation.
*/
QString Node::physicalModuleName() const
{
    if (!physicalModuleName_.isEmpty())
        return physicalModuleName_;

    QString path = location().filePath();
    QString pattern = QString("src") + QDir::separator();
    int start = path.lastIndexOf(pattern);

    if (start == -1)
        return QString();

    QString moduleDir = path.mid(start + pattern.size());
    int finish = moduleDir.indexOf(QDir::separator());

    if (finish == -1)
        return QString();

    QString physicalModuleName = moduleDir.left(finish);

    if (physicalModuleName == "corelib")
        return "QtCore";
    else if (physicalModuleName == "uitools")
        return "QtUiTools";
    else if (physicalModuleName == "gui")
        return "QtGui";
    else if (physicalModuleName == "network")
        return "QtNetwork";
    else if (physicalModuleName == "opengl")
        return "QtOpenGL";
    else if (physicalModuleName == "svg")
        return "QtSvg";
    else if (physicalModuleName == "sql")
        return "QtSql";
    else if (physicalModuleName == "qtestlib")
        return "QtTest";
    else if (moduleDir.contains("webkit"))
        return "QtWebKit";
    else if (physicalModuleName == "xml")
        return "QtXml";
    else
        return QString();
}

/*!
  Removes a node from the list of nodes related to this one.
  If it is a function node, also remove from the primary/
  secondary function maps.
 */
void Aggregate::removeRelated(Node *pseudoChild)
{
    related_.removeAll(pseudoChild);

    if (pseudoChild->isFunction()) {
        QMap<QString, Node *>::Iterator p = primaryFunctionMap_.find(pseudoChild->name());
        while (p != primaryFunctionMap_.end()) {
            if (p.value() == pseudoChild) {
                primaryFunctionMap_.erase(p);
                break;
            }
            ++p;
        }
        NodeList& overloads = secondaryFunctionMap_[pseudoChild->name()];
        overloads.removeAll(pseudoChild);
    }
}

/*!
  Adds \a pseudoChild to the list of nodes related to this one. Resolve a correct
  overload number for a related non-member function.
 */
void Aggregate::addRelated(Node *pseudoChild)
{
    related_.append(pseudoChild);

    if (pseudoChild->isFunction()) {
        FunctionNode* fn = static_cast<FunctionNode*>(pseudoChild);
        if (primaryFunctionMap_.contains(pseudoChild->name())) {
            secondaryFunctionMap_[pseudoChild->name()].append(pseudoChild);
            fn->setOverloadNumber(secondaryFunctionMap_[pseudoChild->name()].size());
            fn->setOverloadFlag(true);
        }
        else {
            primaryFunctionMap_.insert(pseudoChild->name(), pseudoChild);
            fn->setOverloadNumber(0);
            fn->setOverloadFlag(false);
        }
    }
}

/*!
  If this node has a child that is a QML property named \a n,
  return the pointer to that child.
 */
QmlPropertyNode* Aggregate::hasQmlProperty(const QString& n) const
{
    foreach (Node* child, childNodes()) {
        if (child->type() == Node::QmlProperty) {
            if (child->name() == n)
                return static_cast<QmlPropertyNode*>(child);
        }
        else if (child->isQmlPropertyGroup()) {
            QmlPropertyNode* t = child->hasQmlProperty(n);
            if (t)
                return t;
        }
    }
    return 0;
}

/*!
  If this node has a child that is a QML property named \a n
  whose type (attached or normal property) matches \a attached,
  return the pointer to that child.
 */
QmlPropertyNode* Aggregate::hasQmlProperty(const QString& n, bool attached) const
{
    foreach (Node* child, childNodes()) {
        if (child->type() == Node::QmlProperty) {
            if (child->name() == n && child->isAttached() == attached)
                return static_cast<QmlPropertyNode*>(child);
        }
        else if (child->isQmlPropertyGroup()) {
            QmlPropertyNode* t = child->hasQmlProperty(n, attached);
            if (t)
                return t;
        }
    }
    return 0;
}

/*!
  \class LeafNode
 */

/*! \fn bool LeafNode::isAggregate() const
  Returns \c false because this is a LeafNode.
 */

/*!
  Constructs a leaf node named \a name of the specified
  \a type. The new leaf node becomes a child of \a parent.
 */
LeafNode::LeafNode(NodeType type, Aggregate *parent, const QString& name)
    : Node(type, parent, name)
{
    switch (type) {
    case Enum:
    case Function:
    case Typedef:
    case Variable:
    case QmlProperty:
    case QmlSignal:
    case QmlSignalHandler:
    case QmlMethod:
    case QmlBasicType:
    case SharedComment:
        setPageType(ApiPage);
        break;
    default:
        break;
    }
}

/*!
  This constructor should only be used when this node's parent
  is meant to be \a parent, but this node is not to be listed
  as a child of \a parent. It is currently only used for the
  documentation case where a \e{qmlproperty} command is used
  to override the QML definition of a QML property.
 */
LeafNode::LeafNode(Aggregate* parent, NodeType type, const QString& name)
    : Node(type, 0, name)
{
    setParent(parent);
    switch (type) {
    case Enum:
    case Function:
    case Typedef:
    case Variable:
    case QmlProperty:
    case QmlSignal:
    case QmlSignalHandler:
    case QmlMethod:
    case SharedComment:
        setPageType(ApiPage);
        break;
    default:
        break;
    }
}


/*!
  \class NamespaceNode
  \brief This class represents a C++ namespace.

  A namespace can be used in multiple C++ modules, so there
  can be a NamespaceNode for namespace Xxx in more than one
  Node tree.
 */

/*!
  Constructs a namespace node for a namespace named \a name.
  The namespace node has the specified \a parent.
 */
NamespaceNode::NamespaceNode(Aggregate *parent, const QString& name)
    : Aggregate(Namespace, parent, name), seen_(false), documented_(false), tree_(0), docNode_(0)
{
    setGenus(Node::CPP);
    setPageType(ApiPage);
}

/*!
  Returns true if this namespace is to be documented in the
  current module. There can be elements declared in this
  namespace spread over multiple modules. Those elements are
  documented in the modules where they are declared, but they
  are linked to from the namespace page in the module where
  the namespace itself is documented.
 */
bool NamespaceNode::isDocumentedHere() const
{
    return whereDocumented_ == tree()->camelCaseModuleName();
}

/*!
  Returns true if this namespace node contains at least one
  child that has documentation and is not private or internal.
 */
bool NamespaceNode::hasDocumentedChildren() const
{
    foreach (Node* n, childNodes()) {
        if (n->hasDoc() && !n->isPrivate() && !n->isInternal())
            return true;
    }
    return false;
}

/*!
  Report qdoc warning for each documented child in a namespace
  that is not documented. This function should only be called
  when the namespace is not documented.
 */
void NamespaceNode::reportDocumentedChildrenInUndocumentedNamespace() const
{
    foreach (Node* n, childNodes()) {
        if (n->hasDoc() && !n->isPrivate() && !n->isInternal()) {
            QString msg1 = n->name();
            if (n->isFunction())
                msg1 += "()";
            msg1 += tr(" is documented, but namespace %1 is not documented in any module.").arg(name());
            QString msg2 = tr("Add /*! '\\%1 %2' ... */ or remove the qdoc comment marker (!) at that line number.").arg(COMMAND_NAMESPACE).arg(name());

            n->doc().location().warning(msg1, msg2);
        }
    }
}

/*!
  Returns true if this namespace node is not private and
  contains at least one public child node with documentation.
 */
bool NamespaceNode::docMustBeGenerated() const
{
    if (hasDoc() && !isInternal() && !isPrivate())
        return true;
    return (hasDocumentedChildren() ? true : false);
}

/*!
  \class ClassNode
  \brief This class represents a C++ class.
 */

/*!
  Constructs a class node. A class node will generate an API page.
 */
ClassNode::ClassNode(Aggregate *parent, const QString& name)
    : Aggregate(Class, parent, name)
{
    abstract_ = false;
    wrapper_ = false;
    qmlelement = 0;
    setGenus(Node::CPP);
    setPageType(ApiPage);
}

/*!
  Adds the base class \a node to this class's list of base
  classes. The base class has the specified \a access. This
  is a resolved base class.
 */
void ClassNode::addResolvedBaseClass(Access access, ClassNode* node)
{
    bases_.append(RelatedClass(access, node));
    node->derived_.append(RelatedClass(access, this));
}

/*!
  Adds the derived class \a node to this class's list of derived
  classes. The derived class inherits this class with \a access.
 */
void ClassNode::addDerivedClass(Access access, ClassNode* node)
{
    derived_.append(RelatedClass(access, node));
}

/*!
  Add an unresolved base class to this class node's list of
  base classes. The unresolved base class will be resolved
  before the generate phase of qdoc. In an unresolved base
  class, the pointer to the base class node is 0.
 */
void ClassNode::addUnresolvedBaseClass(Access access,
                                       const QStringList& path,
                                       const QString& signature)
{
    bases_.append(RelatedClass(access, path, signature));
}

/*!
  Add an unresolved \c using clause to this class node's list
  of \c using clauses. The unresolved \c using clause will be
  resolved before the generate phase of qdoc. In an unresolved
  \c using clause, the pointer to the function node is 0.
 */
void ClassNode::addUnresolvedUsingClause(const QString& signature)
{
    usingClauses_.append(UsingClause(signature));
}

/*!
 */
void ClassNode::fixBaseClasses()
{
    int i;
    i = 0;
    QSet<ClassNode *> found;

    // Remove private and duplicate base classes.
    while (i < bases_.size()) {
        ClassNode* bc = bases_.at(i).node_;
        if (!bc)
            bc = QDocDatabase::qdocDB()->findClassNode(bases_.at(i).path_);
        if (bc && (bc->access() == Node::Private || found.contains(bc))) {
            RelatedClass rc = bases_.at(i);
            bases_.removeAt(i);
            ignoredBases_.append(rc);
            const QList<RelatedClass> &bb = bc->baseClasses();
            for (int j = bb.size() - 1; j >= 0; --j)
                bases_.insert(i, bb.at(j));
        }
        else {
            ++i;
        }
        found.insert(bc);
    }

    i = 0;
    while (i < derived_.size()) {
        ClassNode* dc = derived_.at(i).node_;
        if (dc && dc->access() == Node::Private) {
            derived_.removeAt(i);
            const QList<RelatedClass> &dd = dc->derivedClasses();
            for (int j = dd.size() - 1; j >= 0; --j)
                derived_.insert(i, dd.at(j));
        }
        else {
            ++i;
        }
    }
}

/*!
  Not sure why this is needed.
 */
void ClassNode::fixPropertyUsingBaseClasses(PropertyNode* pn)
{
    QList<RelatedClass>::const_iterator bc = baseClasses().constBegin();
    while (bc != baseClasses().constEnd()) {
        ClassNode* cn = bc->node_;
        if (cn) {
            Node* n = cn->findChildNode(pn->name(), Node::Property);
            if (n) {
                PropertyNode* baseProperty = static_cast<PropertyNode*>(n);
                cn->fixPropertyUsingBaseClasses(baseProperty);
                pn->setOverriddenFrom(baseProperty);
            }
            else
                cn->fixPropertyUsingBaseClasses(pn);
        }
        ++bc;
    }
}

/*!
  Search the child list to find the property node with the
  specified \a name.
 */
PropertyNode* ClassNode::findPropertyNode(const QString& name)
{
    Node* n = findChildNode(name, Node::Property);

    if (n)
        return static_cast<PropertyNode*>(n);

    PropertyNode* pn = 0;

    const QList<RelatedClass> &bases = baseClasses();
    if (!bases.isEmpty()) {
        for (int i = 0; i < bases.size(); ++i) {
            ClassNode* cn = bases[i].node_;
            if (cn) {
                pn = cn->findPropertyNode(name);
                if (pn)
                    break;
            }
        }
    }
    const QList<RelatedClass>& ignoredBases = ignoredBaseClasses();
    if (!ignoredBases.isEmpty()) {
        for (int i = 0; i < ignoredBases.size(); ++i) {
            ClassNode* cn = ignoredBases[i].node_;
            if (cn) {
                pn = cn->findPropertyNode(name);
                if (pn)
                    break;
            }
        }
    }

    return pn;
}

/*!
  This function does a recursive search of this class node's
  base classes looking for one that has a QML element. If it
  finds one, it returns the pointer to that QML element. If
  it doesn't find one, it returns null.
 */
QmlTypeNode* ClassNode::findQmlBaseNode()
{
    QmlTypeNode* result = 0;
    const QList<RelatedClass>& bases = baseClasses();

    if (!bases.isEmpty()) {
        for (int i = 0; i < bases.size(); ++i) {
            ClassNode* cn = bases[i].node_;
            if (cn && cn->qmlElement()) {
                return cn->qmlElement();
            }
        }
        for (int i = 0; i < bases.size(); ++i) {
            ClassNode* cn = bases[i].node_;
            if (cn) {
                result = cn->findQmlBaseNode();
                if (result != 0) {
                    return result;
                }
            }
        }
    }
    return result;
}

/*!
  \a fn is an overriding function in this class or in a class
  derived from this class. Find the node for the function that
  \a fn overrides in this class's children or in one of this
  class's base classes. Return a pointer to the overridden
  function or return 0.
 */
FunctionNode* ClassNode::findOverriddenFunction(const FunctionNode* fn)
{
    QList<RelatedClass>::Iterator bc = bases_.begin();
    while (bc != bases_.end()) {
        ClassNode *cn = bc->node_;
        if (!cn) {
            cn = QDocDatabase::qdocDB()->findClassNode(bc->path_);
            bc->node_ = cn;
        }
        if (cn) {
            FunctionNode* result = cn->findFunctionNode(fn);
            if (result && !result->isNonvirtual())
                return result;
            result = cn->findOverriddenFunction(fn);
            if (result && !result->isNonvirtual())
                return result;
        }
        ++bc;
    }
    return 0;
}

/*!
  \class DocumentNode
 */

/*!
  The type of a DocumentNode is Document, and it has a \a subtype,
  which specifies the type of DocumentNode. The page type for
  the page index is set here.
 */
DocumentNode::DocumentNode(Aggregate* parent, const QString& name, DocSubtype subtype, Node::PageType ptype)
    : Aggregate(Document, parent, name), nodeSubtype_(subtype)
{
    setGenus(Node::DOC);
    switch (subtype) {
    case Page:
        setPageType(ptype);
        break;
    case DitaMap:
        setPageType(ptype);
        break;
    case Example:
        setPageType(ExamplePage);
        break;
    default:
        break;
    }
}

/*! \fn QString DocumentNode::title() const
  Returns the document node's title. This is used for the page title.
*/

/*!
  Sets the document node's \a title. This is used for the page title.
 */
void DocumentNode::setTitle(const QString &title)
{
    title_ = title;
    parent()->addChild(this, title);
}

/*!
  Returns the document node's full title, which is usually
  just title(), but for some DocSubtype values is different
  from title()
 */
QString DocumentNode::fullTitle() const
{
    if (nodeSubtype_ == File) {
        if (title().isEmpty())
            return name().mid(name().lastIndexOf('/') + 1) + " Example File";
        else
            return title();
    }
    else if (nodeSubtype_ == Image) {
        if (title().isEmpty())
            return name().mid(name().lastIndexOf('/') + 1) + " Image File";
        else
            return title();
    }
    else if (nodeSubtype_ == HeaderFile) {
        if (title().isEmpty())
            return name();
        else
            return name() + " - " + title();
    }
    else {
        return title();
    }
}

/*!
  Returns the subtitle.
 */
QString DocumentNode::subTitle() const
{
    if (!subtitle_.isEmpty())
        return subtitle_;

    if ((nodeSubtype_ == File) || (nodeSubtype_ == Image)) {
        if (title().isEmpty() && name().contains(QLatin1Char('/')))
            return name();
    }
    return QString();
}

/*!
  \class EnumNode
 */

/*!
  The constructor for the node representing an enum type
  has a \a parent class and an enum type \a name.
 */
EnumNode::EnumNode(Aggregate *parent, const QString& name)
    : LeafNode(Enum, parent, name), flagsType_(0)
{
    setGenus(Node::CPP);
}

/*!
  Add \a item to the enum type's item list.
 */
void EnumNode::addItem(const EnumItem& item)
{
    items_.append(item);
    names_.insert(item.name());
}

/*!
  Returns the access level of the enumeration item named \a name.
  Apparently it is private if it has been omitted by qdoc's
  omitvalue command. Otherwise it is public.
 */
Node::Access EnumNode::itemAccess(const QString &name) const
{
    if (doc().omitEnumItemNames().contains(name))
        return Private;
    return Public;
}

/*!
  Returns the enum value associated with the enum \a name.
 */
QString EnumNode::itemValue(const QString &name) const
{
    foreach (const EnumItem &item, items_) {
        if (item.name() == name)
            return item.value();
    }
    return QString();
}

/*!
  \class TypedefNode
 */

/*!
 */
TypedefNode::TypedefNode(Aggregate *parent, const QString& name)
    : LeafNode(Typedef, parent, name), associatedEnum_(0)
{
    setGenus(Node::CPP);
}

/*!
 */
void TypedefNode::setAssociatedEnum(const EnumNode *enume)
{
    associatedEnum_ = enume;
}

/*!
  \class TypeAliasNode
 */

/*!
  Constructs a TypeAliasNode for the \a aliasedType with the
  specified \a name and \a parent.
 */
TypeAliasNode::TypeAliasNode(Aggregate *parent, const QString& name, const QString& aliasedType)
    : TypedefNode(parent, name), aliasedType_(aliasedType)
{
    // nothing.
}

/*!
  \class Parameter
  \brief The class Parameter contains one parameter.

  A parameter can be a function parameter or a macro
  parameter.
 */

/*!
  Constructs this parameter from the \a dataType, the \a name,
  and the \a defaultValue.
 */
Parameter::Parameter(const QString& dataType, const QString& name, const QString& defaultValue)
    : dataType_(dataType),
      name_(name),
      defaultValue_(defaultValue)
{
    // nothing.
}

/*!
  Standard copy constructor copies \p.
 */
Parameter::Parameter(const Parameter& p)
    : dataType_(p.dataType_),
      name_(p.name_),
      defaultValue_(p.defaultValue_)
{
    // nothing.
}

/*!
  standard assignment operator assigns \p.
 */
Parameter& Parameter::operator=(const Parameter& p)
{
    dataType_ = p.dataType_;
    name_ = p.name_;
    defaultValue_ = p.defaultValue_;
    return *this;
}

/*!
  Reconstructs the text describing the parameter and
  returns it. If \a value is true, the default value
  will be included, if there is one.
 */
QString Parameter::reconstruct(bool value) const
{
    QString p = dataType_;
    if (!p.endsWith(QChar('*')) && !p.endsWith(QChar('&')) && !p.endsWith(QChar(' ')))
        p += QLatin1Char(' ');
    p += name_;
    if (value && !defaultValue_.isEmpty())
        p += " = " + defaultValue_;
    return p;
}

/*!
  \class FunctionNode
 */

/*!
  Construct a function node for a C++ function. It's parent
  is \a parent, and it's name is \a name.

  Do not set overloadNumber_ in the initializer list because it
  is set by addChild() in the Node base class.
 */
FunctionNode::FunctionNode(Aggregate *parent, const QString& name)
    : LeafNode(Function, parent, name),
      metaness_(Plain),
      virtualness_(NonVirtual),
      const_(false),
      static_(false),
      reimplemented_(false),
      attached_(false),
      privateSignal_(false),
      overload_(false),
      isDeleted_(false),
      isDefaulted_(false),
      isFinal_(false),
      isOverride_(false),
      isImplicit_(false),
      isRef_(false),
      isRefRef_(false),
      isInvokable_(false)
{
    setGenus(Node::CPP);
}

/*!
  Construct a function node for a QML method or signal, specified
  by \a type. It's parent is \a parent, and it's name is \a name.
  If \a attached is true, it is an attached method or signal.

  Do not set overloadNumber_ in the initializer list because it
  is set by addChild() in the Node base class.
 */
FunctionNode::FunctionNode(NodeType type, Aggregate *parent, const QString& name, bool attached)
    : LeafNode(type, parent, name),
      metaness_(Plain),
      virtualness_(NonVirtual),
      const_(false),
      static_(false),
      reimplemented_(false),
      attached_(attached),
      privateSignal_(false),
      overload_(false),
      isDeleted_(false),
      isDefaulted_(false),
      isFinal_(false),
      isOverride_(false),
      isImplicit_(false),
      isRef_(false),
      isRefRef_(false),
      isInvokable_(false)
{
    setGenus(Node::QML);
    if (type == QmlMethod || type == QmlSignal) {
        if (name.startsWith("__"))
            setStatus(Internal);
    }
    else if (type == Function)
        setGenus(Node::CPP);
}

/*!
  Returns this function's virtualness value as a string
  for use as an attribute value in index files.
 */
QString FunctionNode::virtualness() const
{
    switch (virtualness_) {
    case FunctionNode::NormalVirtual:
        return "virtual";
    case FunctionNode::PureVirtual:
        return "pure";
    case FunctionNode::NonVirtual:
    default:
        break;
    }
    return "non";
}

/*!
  Sets the function node's virtualness value based on the value
  of string \a t, which is the value of the function's \e{virtual}
  attribute in an index file. If \a t is \e{pure}, and if the
  parent() is a C++ class, set the parent's \e abstract flag to
  \c {true}.
 */
void FunctionNode::setVirtualness(const QString& t)
{
    if (t == QLatin1String("non"))
        virtualness_ = NonVirtual;
    else if (t == QLatin1String("virtual"))
        virtualness_ = NormalVirtual;
    else if (t == QLatin1String("pure")) {
        virtualness_ = PureVirtual;
        if (parent() && parent()->isClass())
            parent()->setAbstract(true);
    }
}

/*!
  Sets the function node's Metaness value based on the value
  of string \a t, which is the value of the function's "meta"
  attribute in an index file.
 */
void FunctionNode::setMetaness(const QString& t)
{
    if (t == QLatin1String("plain"))
        metaness_ = Plain;
    else if (t == QLatin1String("signal"))
        metaness_ = Signal;
    else if (t == QLatin1String("slot"))
        metaness_ = Slot;
    else if (t == QLatin1String("constructor"))
        metaness_ = Ctor;
    else if (t == QLatin1String("copy-constructor"))
        metaness_ = CCtor;
    else if (t == QLatin1String("move-constructor"))
        metaness_ = MCtor;
    else if (t == QLatin1String("destructor"))
        metaness_ = Dtor;
    else if (t == QLatin1String("macro"))
        metaness_ = MacroWithParams;
    else if (t == QLatin1String("macrowithparams"))
        metaness_ = MacroWithParams;
    else if (t == QLatin1String("macrowithoutparams"))
        metaness_ = MacroWithoutParams;
    else if (t == QLatin1String("copy-assign"))
        metaness_ = CAssign;
    else if (t == QLatin1String("move-assign"))
        metaness_ = MAssign;
    else if (t == QLatin1String("native"))
        metaness_ = Native;
    else
        metaness_ = Plain;
}

/*! \fn void FunctionNode::setOverloadFlag(bool b)
  Sets this function node's overload flag to \a b.
  It does not set the overload number.
 */

/*! \fn void FunctionNode::setOverloadNumber(unsigned char n)
  Sets this function node's overload number to \a n.
  It does not set the overload flag.
 */

/*!
  Sets the function node's reimplementation flag to \a b.
  When \a b is true, it is supposed to mean that this function
  is a reimplementation of a virtual function in a base class,
  but it really just means the \e {\\reimp} command was seen in
  the qdoc comment.
 */
void FunctionNode::setReimplemented(bool b)
{
    reimplemented_ = b;
}

/*!
  Returns a string representing the Metaness enum value for
  this function. It is used in index files.
 */
QString FunctionNode::metaness() const
{
    switch (metaness_) {
    case FunctionNode::Plain:
        return "plain";
    case FunctionNode::Signal:
        return "signal";
    case FunctionNode::Slot:
        return "slot";
    case FunctionNode::Ctor:
        return "constructor";
    case FunctionNode::CCtor:
        return "copy-constructor";
    case FunctionNode::MCtor:
        return "move-constructor";
    case FunctionNode::Dtor:
        return "destructor";
    case FunctionNode::MacroWithParams:
        return "macrowithparams";
    case FunctionNode::MacroWithoutParams:
        return "macrowithoutparams";
    case FunctionNode::Native:
        return "native";
    case FunctionNode::CAssign:
        return "copy-assign";
    case FunctionNode::MAssign:
        return "move-assign";
    default:
        return "plain";
    }
    return QString();
}

/*!
  Append \a parameter to the parameter list.
 */
void FunctionNode::addParameter(const Parameter& parameter)
{
    parameters_.append(parameter);
}

/*!
  Split the parameters \a t and store them in this function's
  Parameter vector.
 */
void FunctionNode::setParameters(const QString &t)
{
    clearParams();
    if (!t.isEmpty()) {
        QStringList commaSplit = t.split(',');
        foreach (QString s, commaSplit) {
            QStringList blankSplit = s.split(' ');
            QString pName = blankSplit.last();
            blankSplit.removeLast();
            QString pType = blankSplit.join(' ');
            int i = 0;
            while (i < pName.length() && !pName.at(i).isLetter())
                i++;
            if (i > 0) {
                pType += QChar(' ') + pName.left(i);
                pName = pName.mid(i);
            }
            addParameter(Parameter(pType, pName));
        }
    }
}

void FunctionNode::borrowParameterNames(const FunctionNode *source)
{
    QVector<Parameter>::Iterator t = parameters_.begin();
    QVector<Parameter>::ConstIterator s = source->parameters_.constBegin();
    while (s != source->parameters_.constEnd() && t != parameters_.end()) {
        if (!(*s).name().isEmpty())
            (*t).setName((*s).name());
        ++s;
        ++t;
    }
}

/*!
  Adds the "associated" property \a p to this function node.
  The function might be the setter or getter for a property,
  for example.
 */
void FunctionNode::addAssociatedProperty(PropertyNode *p)
{
    associatedProperties_.append(p);
}

/*!
  Returns true if this function has at least one property
  that is active, i.e. at least one property that is not
  obsolete.
 */
bool FunctionNode::hasActiveAssociatedProperty() const
{
    if (associatedProperties_.isEmpty())
        return false;
    foreach (const PropertyNode* p, associatedProperties_) {
        if (!p->isObsolete())
            return true;
    }
    return false;
}

/*! \fn unsigned char FunctionNode::overloadNumber() const
  Returns the overload number for this function.
 */

/*!
  Returns the list of parameter names.
 */
QStringList FunctionNode::parameterNames() const
{
    QStringList names;
    QVector<Parameter>::ConstIterator p = parameters().constBegin();
    while (p != parameters().constEnd()) {
        names << (*p).name();
        ++p;
    }
    return names;
}

/*!
  Returns a raw list of parameters. If \a names is true, the
  names are included. If \a values is true, the default values
  are included, if any are present.
 */
QString FunctionNode::rawParameters(bool names, bool values) const
{
    QString raw;
    foreach (const Parameter &parameter, parameters()) {
        raw += parameter.dataType();
        if (names)
            raw += parameter.name();
        if (values)
            raw += parameter.defaultValue();
    }
    return raw;
}

/*!
  Returns the list of reconstructed parameters. If \a values
  is true, the default values are included, if any are present.
 */
QStringList FunctionNode::reconstructParameters(bool values) const
{
    QStringList reconstructedParameters;
    QVector<Parameter>::ConstIterator p = parameters().constBegin();
    while (p != parameters().constEnd()) {
        reconstructedParameters << (*p).reconstruct(values);
        ++p;
    }
    return reconstructedParameters;
}

/*!
  Reconstructs and returns the function's signature. If \a values
  is true, the default values of the parameters are included, if
  present.
 */
QString FunctionNode::signature(bool values, bool noReturnType) const
{
    QString s;
    if (!noReturnType && !returnType().isEmpty())
        s = returnType() + QLatin1Char(' ');
    s += name();
    if (!isMacroWithoutParams()) {
        s += QLatin1Char('(');
        QStringList reconstructedParameters = reconstructParameters(values);
        int p = reconstructedParameters.size();
        if (p > 0) {
            for (int i=0; i<p; i++) {
                s += reconstructedParameters[i];
                if (i < (p-1))
                    s += ", ";
            }
        }
        s += QLatin1Char(')');
        if (isMacro())
            return s;
    }
    if (isConst())
        s += " const";
    if (isRef())
        s += " &";
    else if (isRefRef())
        s += " &&";
    if (isImplicit())
        s += " = default";
    return s;
}

/*!
  Returns true if function \a fn has role \a r for this
  property.
 */
PropertyNode::FunctionRole PropertyNode::role(const FunctionNode* fn) const
{
    for (int i=0; i<4; i++) {
        if (functions_[i].contains(const_cast<FunctionNode*>(fn)))
            return (FunctionRole) i;
    }
    return Notifier;
}

/*!
  Print some debugging stuff.
 */
void FunctionNode::debug() const
{
    qDebug("QML METHOD %s returnType_ %s parentPath_ %s",
           qPrintable(name()), qPrintable(returnType_), qPrintable(parentPath_.join(' ')));
}

/*!
  Compares this FunctionNode to the FunctionNode pointed to
  by \a fn. Returns true if they describe the same function.
 */
bool FunctionNode::compare(const FunctionNode *fn) const
{
    if (!fn)
        return false;
    if (type() != fn->type())
        return false;
    if (parent() != fn->parent())
        return false;
    if (returnType_ != fn->returnType())
        return false;
    if (isConst() != fn->isConst())
        return false;
    if (isAttached() != fn->isAttached())
        return false;
    const QVector<Parameter>& p = fn->parameters();
    if (parameters().size() != p.size())
        return false;
    if (!p.isEmpty()) {
        for (int i = 0; i < p.size(); ++i) {
            if (parameters()[i].dataType() != p[i].dataType())
                return false;
        }
    }
    return true;
}

/*!
  In some cases, it is ok for a public function to be not documented.
  For example, the macro Q_OBJECT adds several functions to the API of
  a class, but these functions are normally not meant to be documented.
  So if a function node doesn't have documentation, then if its name is
  in the list of functions that it is ok not to document, this function
  returns true. Otherwise, it returns false.

  These are the member function names added by macros.  Usually they
  are not documented, but they can be documented, so this test avoids
  reporting an error if they are not documented.

  But maybe we should generate a standard text for each of them?
 */
bool FunctionNode::isIgnored() const
{
    if (!hasDoc() && !hasSharedDoc()) {
        if (name().startsWith(QLatin1String("qt_")) ||
            name() == QLatin1String("metaObject") ||
            name() == QLatin1String("tr") ||
            name() == QLatin1String("trUtf8") ||
            name() == QLatin1String("d_func")) {
            return true;
        }
    }
    return false;
}

/*!
  \class PropertyNode

  This class describes one instance of using the Q_PROPERTY macro.
 */

/*!
  The constructor sets the \a parent and the \a name, but
  everything else is set to default values.
 */
PropertyNode::PropertyNode(Aggregate *parent, const QString& name)
    : LeafNode(Property, parent, name),
      stored_(FlagValueDefault),
      designable_(FlagValueDefault),
      scriptable_(FlagValueDefault),
      writable_(FlagValueDefault),
      user_(FlagValueDefault),
      const_(false),
      final_(false),
      revision_(-1),
      overrides_(0)
{
    setGenus(Node::CPP);
}

/*!
  Sets this property's \e {overridden from} property to
  \a baseProperty, which indicates that this property
  overrides \a baseProperty. To begin with, all the values
  in this property are set to the corresponding values in
  \a baseProperty.

  We probably should ensure that the constant and final
  attributes are not being overridden improperly.
 */
void PropertyNode::setOverriddenFrom(const PropertyNode* baseProperty)
{
    for (int i = 0; i < NumFunctionRoles; ++i) {
        if (functions_[i].isEmpty())
            functions_[i] = baseProperty->functions_[i];
    }
    if (stored_ == FlagValueDefault)
        stored_ = baseProperty->stored_;
    if (designable_ == FlagValueDefault)
        designable_ = baseProperty->designable_;
    if (scriptable_ == FlagValueDefault)
        scriptable_ = baseProperty->scriptable_;
    if (writable_ == FlagValueDefault)
        writable_ = baseProperty->writable_;
    if (user_ == FlagValueDefault)
        user_ = baseProperty->user_;
    overrides_ = baseProperty;
}

/*!
 */
QString PropertyNode::qualifiedDataType() const
{
    if (setters().isEmpty() && resetters().isEmpty()) {
        if (type_.contains(QLatin1Char('*')) || type_.contains(QLatin1Char('&'))) {
            // 'QWidget *' becomes 'QWidget *' const
            return type_ + " const";
        }
        else {
            /*
              'int' becomes 'const int' ('int const' is
              correct C++, but looks wrong)
            */
            return "const " + type_;
        }
    }
    else {
        return type_;
    }
}

bool QmlTypeNode::qmlOnly = false;
QMultiMap<const Node*, Node*> QmlTypeNode::inheritedBy;

/*!
  Constructs a Qml class node. The new node has the given
  \a parent and \a name.
 */
QmlTypeNode::QmlTypeNode(Aggregate *parent, const QString& name)
    : Aggregate(QmlType, parent, name),
      abstract_(false),
      cnodeRequired_(false),
      wrapper_(false),
      cnode_(0),
      logicalModule_(0),
      qmlBaseNode_(0)
{
    int i = 0;
    if (name.startsWith("QML:")) {
        qDebug() << "BOGUS QML qualifier:" << name;
        i = 4;
    }
    setTitle(name.mid(i));
    setPageType(Node::ApiPage);
    setGenus(Node::QML);
}

/*!
  Needed for printing a debug messages.
 */
QmlTypeNode::~QmlTypeNode()
{
    // nothing.
}

/*!
  Clear the static maps so that subsequent runs don't try to use
  contents from a previous run.
 */
void QmlTypeNode::terminate()
{
    inheritedBy.clear();
}

/*!
  Record the fact that QML class \a base is inherited by
  QML class \a sub.
 */
void QmlTypeNode::addInheritedBy(const Node *base, Node* sub)
{
    if (sub->isInternal())
        return;
    if (!inheritedBy.contains(base, sub))
        inheritedBy.insert(base, sub);
}

/*!
  Loads the list \a subs with the nodes of all the subclasses of \a base.
 */
void QmlTypeNode::subclasses(const Node *base, NodeList &subs)
{
    subs.clear();
    if (inheritedBy.count(base) > 0) {
        subs = inheritedBy.values(base);
    }
}


/*!
  If this QML type node has a base type node,
  return the fully qualified name of that QML
  type, i.e. <QML-module-name>::<QML-type-name>.
 */
QString QmlTypeNode::qmlFullBaseName() const
{
    QString result;
    if (qmlBaseNode_) {
        result = qmlBaseNode_->logicalModuleName() + "::" + qmlBaseNode_->name();
    }
    return result;
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module name from the QML module node. Otherwise, return the
  empty string.
 */
QString QmlTypeNode::logicalModuleName() const
{
    return (logicalModule_ ? logicalModule_->logicalModuleName() : QString());
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module version from the QML module node. Otherwise, return
  the empty string.
 */
QString QmlTypeNode::logicalModuleVersion() const
{
    return (logicalModule_ ? logicalModule_->logicalModuleVersion() : QString());
}

/*!
  If the QML type's QML module pointer is set, return the QML
  module identifier from the QML module node. Otherwise, return
  the empty string.
 */
QString QmlTypeNode::logicalModuleIdentifier() const
{
    return (logicalModule_ ? logicalModule_->logicalModuleIdentifier() : QString());
}

/*!
  Returns true if this QML type inherits \a type.
 */
bool QmlTypeNode::inherits(Aggregate* type)
{
    QmlTypeNode* qtn = qmlBaseNode();
    while (qtn != 0) {
        if (qtn == type)
            return true;
        qtn = qtn->qmlBaseNode();
    }
    return false;
}

/*!
  Constructs a Qml basic type node. The new node has the given
  \a parent and \a name.
 */
QmlBasicTypeNode::QmlBasicTypeNode(Aggregate *parent,
                                   const QString& name)
    : Aggregate(QmlBasicType, parent, name)
{
    setTitle(name);
    setGenus(Node::QML);
}

/*!
  Constructor for the Qml property group node. \a parent is
  always a QmlTypeNode.
 */
QmlPropertyGroupNode::QmlPropertyGroupNode(QmlTypeNode* parent, const QString& name)
    : Aggregate(QmlPropertyGroup, parent, name)
{
    idNumber_ = -1;
    setGenus(Node::QML);
}

/*!
  Return the property group node's id number for use in
  constructing an id attribute for the property group.
  If the id number is currently -1, increment the global
  property group count and set the id number to the new
  value.
 */
QString QmlPropertyGroupNode::idNumber()
{
    if (idNumber_ == -1)
        idNumber_ = incPropertyGroupCount();
    return QString().setNum(idNumber_);
}

/*!
  Constructor for the QML property node.
 */
QmlPropertyNode::QmlPropertyNode(Aggregate* parent,
                                 const QString& name,
                                 const QString& type,
                                 bool attached)
    : LeafNode(QmlProperty, parent, name),
      type_(type),
      stored_(FlagValueDefault),
      designable_(FlagValueDefault),
      isAlias_(false),
      isdefault_(false),
      attached_(attached),
      readOnly_(FlagValueDefault)
{
    setPageType(ApiPage);
    if (type_ == QString("alias"))
        isAlias_ = true;
    if (name.startsWith("__"))
        setStatus(Internal);
    setGenus(Node::QML);
}

/*!
  Returns \c true if a QML property or attached property is
  not read-only. The algorithm for figuring this out is long
  amd tedious and almost certainly will break. It currently
  doesn't work for the qmlproperty:

  \code
      bool PropertyChanges::explicit,
  \endcode

  ...because the tokenizer gets confused on \e{explicit}.
 */
bool QmlPropertyNode::isWritable()
{
    if (readOnly_ != FlagValueDefault)
        return !fromFlagValue(readOnly_, false);

    QmlTypeNode* qcn = qmlTypeNode();
    if (qcn) {
        if (qcn->cppClassRequired()) {
            if (qcn->classNode()) {
                PropertyNode* pn = findCorrespondingCppProperty();
                if (pn)
                    return pn->isWritable();
                else
                    defLocation().warning(tr("No Q_PROPERTY for QML property %1::%2::%3 "
                                             "in C++ class documented as QML type: "
                                             "(property not found in the C++ class or its base classes)")
                                          .arg(logicalModuleName()).arg(qmlTypeName()).arg(name()));
            }
            else
                defLocation().warning(tr("No Q_PROPERTY for QML property %1::%2::%3 "
                                         "in C++ class documented as QML type: "
                                         "(C++ class not specified or not found).")
                                      .arg(logicalModuleName()).arg(qmlTypeName()).arg(name()));
        }
    }
    return true;
}

/*!
  Returns a pointer this QML property's corresponding C++
  property, if it has one.
 */
PropertyNode* QmlPropertyNode::findCorrespondingCppProperty()
{
    PropertyNode* pn;
    Node* n = parent();
    while (n && !(n->isQmlType() || n->isJsType()))
        n = n->parent();
    if (n) {
        QmlTypeNode* qcn = static_cast<QmlTypeNode*>(n);
        ClassNode* cn = qcn->classNode();
        if (cn) {
            /*
              If there is a dot in the property name, first
              find the C++ property corresponding to the QML
              property group.
             */
            QStringList dotSplit = name().split(QChar('.'));
            pn = cn->findPropertyNode(dotSplit[0]);
            if (pn) {
                /*
                  Now find the C++ property corresponding to
                  the QML property in the QML property group,
                  <group>.<property>.
                */
                if (dotSplit.size() > 1) {
                    QStringList path(extractClassName(pn->qualifiedDataType()));
                    Node* nn = QDocDatabase::qdocDB()->findClassNode(path);
                    if (nn) {
                        ClassNode* cn = static_cast<ClassNode*>(nn);
                        PropertyNode *pn2 = cn->findPropertyNode(dotSplit[1]);
                        /*
                          If found, return the C++ property
                          corresponding to the QML property.
                          Otherwise, return the C++ property
                          corresponding to the QML property
                          group.
                        */
                        return (pn2 ? pn2 : pn);
                    }
                }
                else
                    return pn;
            }
        }
    }
    return 0;
}

/*!
  This returns the name of the owning QML type.
 */
QString QmlPropertyNode::element() const
{
    if (parent()->isQmlPropertyGroup())
        return parent()->element();
    return parent()->name();
}

/*!
  Construct the full document name for this node and return it.
 */
QString Node::fullDocumentName() const
{
    QStringList pieces;
    const Node* n = this;

    do {
        if (!n->name().isEmpty() && !n->isQmlPropertyGroup())
            pieces.insert(0, n->name());

        if ((n->isQmlType() || n->isJsType()) && !n->logicalModuleName().isEmpty()) {
            pieces.insert(0, n->logicalModuleName());
            break;
        }

        if (n->isDocumentNode())
            break;

        // Examine the parent node if one exists.
        if (n->parent())
            n = n->parent();
        else
            break;
    } while (true);

    // Create a name based on the type of the ancestor node.
    QString concatenator = "::";
    if (n->isQmlType() || n->isJsType())
        concatenator = QLatin1Char('.');

    if (n->isDocumentNode())
        concatenator = QLatin1Char('#');

    return pieces.join(concatenator);
}

/*!
  Returns the \a str as an NCName, which means the name can
  be used as the value of an \e id attribute. Search for NCName
  on the internet for details of what can be an NCName.
 */
QString Node::cleanId(const QString &str)
{
    QString clean;
    QString name = str.simplified();

    if (name.isEmpty())
        return clean;

    name = name.replace("::","-");
    name = name.replace(QLatin1Char(' '), QLatin1Char('-'));
    name = name.replace("()","-call");

    clean.reserve(name.size() + 20);
    if (!str.startsWith("id-"))
        clean = "id-";
    const QChar c = name[0];
    const uint u = c.unicode();

    if ((u >= 'a' && u <= 'z') ||
            (u >= 'A' && u <= 'Z') ||
            (u >= '0' && u <= '9')) {
        clean += c;
    }
    else if (u == '~') {
        clean += "dtor.";
    }
    else if (u == '_') {
        clean += "underscore.";
    }
    else {
        clean += QLatin1Char('a');
    }

    for (int i = 1; i < (int) name.length(); i++) {
        const QChar c = name[i];
        const uint u = c.unicode();
        if ((u >= 'a' && u <= 'z') ||
                (u >= 'A' && u <= 'Z') ||
                (u >= '0' && u <= '9') || u == '-' ||
                u == '_' || u == '.') {
            clean += c;
        }
        else if (c.isSpace() || u == ':' ) {
            clean += QLatin1Char('-');
        }
        else if (u == '!') {
            clean += "-not";
        }
        else if (u == '&') {
            clean += "-and";
        }
        else if (u == '<') {
            clean += "-lt";
        }
        else if (u == '=') {
            clean += "-eq";
        }
        else if (u == '>') {
            clean += "-gt";
        }
        else if (u == '#') {
            clean += "-hash";
        }
        else if (u == '(') {
            clean += QLatin1Char('-');
        }
        else if (u == ')') {
            clean += QLatin1Char('-');
        }
        else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

/*!
  Prints the inner node's list of children.
  For debugging only.
 */
void Aggregate::printChildren(const QString& title)
{
    qDebug() << title << name() << children_.size();
    if (children_.size() > 0) {
        for (int i=0; i<children_.size(); ++i) {
            Node* n = children_.at(i);
            qDebug() << "  CHILD:" << n->name() << n->nodeTypeString() << n->nodeSubtypeString();
        }
    }
}

/*!
  Returns \c true if the collection node's member list is
  not empty.
 */
bool CollectionNode::hasMembers() const
{
    return !members_.isEmpty();
}

/*!
  Appends \a node to the collection node's member list, if
  and only if it isn't already in the member list.
 */
void CollectionNode::addMember(Node* node)
{
    if (!members_.contains(node))
        members_.append(node);
}

/*!
  Returns \c true if this collection node contains at least
  one namespace node.
 */
bool CollectionNode::hasNamespaces() const
{
    if (!members_.isEmpty()) {
        NodeList::const_iterator i = members_.begin();
        while (i != members_.end()) {
            if ((*i)->isNamespace())
                return true;
            ++i;
        }
    }
    return false;
}

/*!
  Returns \c true if this collection node contains at least
  one class node.
 */
bool CollectionNode::hasClasses() const
{
    if (!members_.isEmpty()) {
        NodeList::const_iterator i = members_.cbegin();
        while (i != members_.cend()) {
            if ((*i)->isClass())
                return true;
            ++i;
        }
    }
    return false;
}

/*!
  Loads \a out with all this collection node's members that
  are namespace nodes.
 */
void CollectionNode::getMemberNamespaces(NodeMap& out)
{
    out.clear();
    NodeList::const_iterator i = members_.cbegin();
    while (i != members_.cend()) {
        if ((*i)->isNamespace())
            out.insert((*i)->name(),(*i));
        ++i;
    }
}

/*!
  Loads \a out with all this collection node's members that
  are class nodes.
 */
void CollectionNode::getMemberClasses(NodeMap& out) const
{
    out.clear();
    NodeList::const_iterator i = members_.cbegin();
    while (i != members_.cend()) {
        if ((*i)->isClass())
            out.insert((*i)->name(),(*i));
        ++i;
    }
}

/*!
  Prints the collection node's list of members.
  For debugging only.
 */
void CollectionNode::printMembers(const QString& title)
{
    qDebug() << title << name() << members_.size();
    if (members_.size() > 0) {
        for (int i=0; i<members_.size(); ++i) {
            Node* n = members_.at(i);
            qDebug() << "  MEMBER:" << n->name() << n->nodeTypeString() << n->nodeSubtypeString();
        }
    }
}

/*!
  Sets the document node's \a title. This is used for the page title.
 */
void CollectionNode::setTitle(const QString& title)
{
    title_ = title;
    parent()->addChild(this, title);
}

/*!
  This function splits \a arg on the blank character to get a
  logical module name and version number. If the version number
  is present, it spilts the version number on the '.' character
  to get a major version number and a minor vrsion number. If
  the version number is present, both the major and minor version
  numbers should be there, but the minor version number is not
  absolutely necessary.
 */
void CollectionNode::setLogicalModuleInfo(const QString& arg)
{
    QStringList blankSplit = arg.split(QLatin1Char(' '));
    logicalModuleName_ = blankSplit[0];
    if (blankSplit.size() > 1) {
        QStringList dotSplit = blankSplit[1].split(QLatin1Char('.'));
        logicalModuleVersionMajor_ = dotSplit[0];
        if (dotSplit.size() > 1)
            logicalModuleVersionMinor_ = dotSplit[1];
        else
            logicalModuleVersionMinor_ = "0";
    }
}

/*!
  This function accepts the logical module \a info as a string
  list. If the logical module info contains the version number,
  it spilts the version number on the '.' character to get the
  major and minor vrsion numbers. Both major and minor version
  numbers should be provided, but the minor version number is
  not strictly necessary.
 */
void CollectionNode::setLogicalModuleInfo(const QStringList& info)
{
    logicalModuleName_ = info[0];
    if (info.size() > 1) {
        QStringList dotSplit = info[1].split(QLatin1Char('.'));
        logicalModuleVersionMajor_ = dotSplit[0];
        if (dotSplit.size() > 1)
            logicalModuleVersionMinor_ = dotSplit[1];
        else
            logicalModuleVersionMinor_ = "0";
    }
}

/*!
  Sets the overload flag to \b in each node in the collection.
 */
void SharedCommentNode::setOverloadFlag(bool b)
{
    for (Node *n : collective_)
        n->setOverloadFlag(b);
}

/*!
  Sets the pointer to the node that this node relates to
  in each of the nodes in this shared comment node's collective.
 */
void SharedCommentNode::setRelates(Aggregate *pseudoParent)
{
    Node::setRelates(pseudoParent);
    for (Node *n : collective_)
        n->setRelates(pseudoParent);
}

/*!
  Sets the (unresolved) entity \a name that this node relates to
  in each of the nodes in this shared comment node's collective.
 */
void SharedCommentNode::setRelates(const QString& name)
{
    Node::setRelates(name);
    for (Node *n : collective_)
        n->setRelates(name);
}

QT_END_NAMESPACE
