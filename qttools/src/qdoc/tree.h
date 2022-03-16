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

/*
  tree.h
*/

#ifndef TREE_H
#define TREE_H

#include <QtCore/qstack.h>
#include "node.h"

QT_BEGIN_NAMESPACE

class QStringList;
class QDocDatabase;

struct TargetRec
{
  public:
    enum TargetType { Unknown, Target, Keyword, Contents, Class, Function, Page, Subtitle };

    TargetRec(const QString& name,
              const QString& title,
              TargetRec::TargetType type,
              Node* node,
              int priority)
    : node_(node), ref_(name), title_(title), priority_(priority), type_(type) {
        // Discard the dedicated ref for keywords - they always
        // link to the top of the QDoc comment they appear in
        if (type == Keyword)
            ref_.clear();
    }

    bool isEmpty() const { return ref_.isEmpty(); }
    Node::Genus genus() { return (node_ ? node_->genus() : Node::DontCare); }

    Node* node_;
    QString ref_;
    QString title_;
    int priority_;
    TargetType type_;
};

struct TargetLoc
{
  public:
  TargetLoc(const Node* loc, const QString& t, const QString& fileName, const QString& text, bool broken)
  : loc_(loc), target_(t), fileName_(fileName), text_(text), broken_(broken) { }
    const Node*   loc_;
    QString target_;
    QString fileName_;
    QString text_;
    bool    broken_;
};

typedef QMultiMap<QString, TargetRec*> TargetMap;
typedef QMultiMap<QString, DocumentNode*> DocumentNodeMultiMap;
typedef QMap<QString, QmlTypeNode*> QmlTypeMap;
typedef QMultiMap<QString, const ExampleNode*> ExampleNodeMap;
typedef QVector<TargetLoc*> TargetList;
typedef QMap<QString, TargetList*> TargetListMap;

class Tree
{
 private:
    friend class QDocForest;
    friend class QDocDatabase;

    typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
    typedef QMap<PropertyNode*, RoleMap> PropertyMap;

    Tree(const QString& camelCaseModuleName, QDocDatabase* qdb);
    ~Tree();

    Node* findNodeForInclude(const QStringList& path) const;
    ClassNode* findClassNode(const QStringList& path, const Node* start = 0) const;
    NamespaceNode* findNamespaceNode(const QStringList& path) const;
    FunctionNode* findFunctionNode(const QStringList& parentPath, const FunctionNode* clone);
    const Node* findFunctionNode(const QString& target,
                                 const QString& params,
                                 const Node* relative,
                                 Node::Genus genus) const;

    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            const Node* start,
                            Node::NodeType type) const;
    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            Node* start,
                            const NodeTypeList& types) const;

    const Node* findNodeForTarget(const QStringList& path,
                                  const QString& target,
                                  const Node* node,
                                  int flags,
                                  Node::Genus genus,
                                  QString& ref) const;
    const Node* matchPathAndTarget(const QStringList& path,
                                   int idx,
                                   const QString& target,
                                   const Node* node,
                                   int flags,
                                   Node::Genus genus,
                                   QString& ref) const;

    const Node* findNode(const QStringList &path,
                         const Node* relative,     // = 0,
                         int findFlags,            // = 0,
                         Node::Genus genus) const; // = Node::DontCare) const;

    QmlTypeNode* findQmlTypeNode(const QStringList& path);

    Node* findNodeByNameAndType(const QStringList& path, Node::NodeType type) const;
    Aggregate* findRelatesNode(const QStringList& path);
    QString getRef(const QString& target, const Node* node) const;
    void insertTarget(const QString& name,
                      const QString& title,
                      TargetRec::TargetType type,
                      Node* node,
                      int priority);
    void resolveTargets(Aggregate* root);
    const Node* findUnambiguousTarget(const QString& target, Node::Genus genus, QString& ref) const;
    const DocumentNode* findDocumentNodeByTitle(const QString& title) const;

    void addPropertyFunction(PropertyNode *property,
                             const QString &funcName,
                             PropertyNode::FunctionRole funcRole);
    void resolveInheritance(Aggregate* n = 0);
    void resolveInheritanceHelper(int pass, ClassNode* cn);
    void resolveProperties();
    void resolveCppToQmlLinks();
    void resolveUsingClauses();
    void fixInheritance(NamespaceNode *rootNode = 0);
    NamespaceNode *root() { return &root_; }

    const FunctionNode *findFunctionNode(const QStringList &path,
                                         const QString& params,
                                         const Node *relative = 0,
                                         int findFlags = 0,
                                         Node::Genus genus = Node::DontCare) const;
    const NamespaceNode *root() const { return &root_; }

    NodeList allBaseClasses(const ClassNode *classe) const;
    QString refForAtom(const Atom* atom);

    CNMap* getCollectionMap(Node::Genus genus);
    const CNMap& groups() const { return groups_; }
    const CNMap& modules() const { return modules_; }
    const CNMap& qmlModules() const { return qmlModules_; }
    const CNMap& jsModules() const { return jsModules_; }

    CollectionNode* getCollection(const QString& name, Node::Genus genus);
    CollectionNode* findCollection(const QString& name, Node::Genus genus);

    CollectionNode* findGroup(const QString& name) { return findCollection(name, Node::DOC); }
    CollectionNode* findModule(const QString& name) { return findCollection(name, Node::CPP); }
    CollectionNode* findQmlModule(const QString& name) { return findCollection(name, Node::QML); }
    CollectionNode* findJsModule(const QString& name) { return findCollection(name, Node::JS); }

    CollectionNode* addGroup(const QString& name) { return findGroup(name); }
    CollectionNode* addModule(const QString& name) { return findModule(name); }
    CollectionNode* addQmlModule(const QString& name) { return findQmlModule(name); }
    CollectionNode* addJsModule(const QString& name) { return findJsModule(name); }

    CollectionNode* addToGroup(const QString& name, Node* node);
    CollectionNode* addToModule(const QString& name, Node* node);
    CollectionNode* addToQmlModule(const QString& name, Node* node);
    CollectionNode* addToJsModule(const QString& name, Node* node);

    QmlTypeNode* lookupQmlType(const QString& name) const { return qmlTypeMap_.value(name); }
    Aggregate* lookupQmlBasicType(const QString& name) const { return qmlTypeMap_.value(name); }
    void insertQmlType(const QString& key, QmlTypeNode* n);
    void addExampleNode(ExampleNode* n) { exampleNodeMap_.insert(n->title(), n); }
    ExampleNodeMap& exampleNodeMap() { return exampleNodeMap_; }
    const Node* checkForCollision(const QString& name);
    void setIndexFileName(const QString& t) { indexFileName_ = t; }

    bool treeHasBeenAnalyzed() const { return treeHasBeenAnalyzed_; }
    bool docsHaveBeenGenerated() const { return docsHaveBeenGenerated_; }
    void setTreeHasBeenAnalyzed() { treeHasBeenAnalyzed_ = true; }
    void setdocsHaveBeenGenerated() { docsHaveBeenGenerated_ = true; }
    QString getNewLinkTarget(const Node* locNode,
                             const Node* t,
                             const QString& fileName,
                             QString& text,
                             bool broken);
    TargetList* getTargetList(const QString& module);
    QStringList getTargetListKeys() { return targetListMap_->keys(); }
    Node* findFunctionNodeForTag(const QString &tag, Aggregate* parent = 0);
    Node *findMacroNode(const QString &t, const Aggregate *parent = 0);

 public:
    const QString& camelCaseModuleName() const { return camelCaseModuleName_; }
    const QString& physicalModuleName() const { return physicalModuleName_; }
    const QString& indexFileName() const { return indexFileName_; }
    long incrementLinkCount() { return --linkCount_; }
    void clearLinkCount() { linkCount_ = 0; }
    long linkCount() const { return linkCount_; }
    const QString& indexTitle() const { return indexTitle_; }
    void setIndexTitle(const QString &t) { indexTitle_ = t; }

private:
    bool treeHasBeenAnalyzed_;
    bool docsHaveBeenGenerated_;
    long linkCount_;
    QString camelCaseModuleName_;
    QString physicalModuleName_;
    QString indexFileName_;
    QString indexTitle_;
    QDocDatabase* qdb_;
    NamespaceNode root_;
    PropertyMap unresolvedPropertyMap;
    DocumentNodeMultiMap    docNodesByTitle_;
    TargetMap               nodesByTargetRef_;
    TargetMap               nodesByTargetTitle_;
    CNMap                   groups_;
    CNMap                   modules_;
    CNMap                   qmlModules_;
    CNMap                   jsModules_;
    QmlTypeMap              qmlTypeMap_;
    ExampleNodeMap          exampleNodeMap_;
    TargetListMap*          targetListMap_;
};

QT_END_NAMESPACE

#endif
