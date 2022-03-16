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

#include "generator.h"
#include "atom.h"
#include "tree.h"
#include "qdocdatabase.h"
#include "qdoctagfiles.h"
#include "qdocindexfiles.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static NodeMap emptyNodeMap_;
static NodeMultiMap emptyNodeMultiMap_;
bool QDocDatabase::debug = false;

/*! \class QDocForest
  This class manages a collection of trees. Each tree is an
  instance of class Tree, which is a private class.

  The forest is populated as each index file is loaded.
  Each index file adds a tree to the forest. Each tree
  is named with the name of the module it represents.

  The search order is created by searchOrder(), if it has
  not already been created. The search order and module
  names arrays have parallel structure, i.e. modulNames_[i]
  is the module name of the Tree at searchOrder_[i].
 */

/*!
  Destroys the qdoc forest. This requires deleting
  each Tree in the forest. Note that the forest has
  been transferred into the search order array, so
  what is really being used to destroy the forest
  is the search order array.
 */
QDocForest::~QDocForest()
{
    for (int i=0; i<searchOrder_.size(); ++i)
        delete searchOrder_.at(i);
    forest_.clear();
    searchOrder_.clear();
    indexSearchOrder_.clear();
    moduleNames_.clear();
    primaryTree_ = 0;
}

/*!
  Initializes the forest prior to a traversal and
  returns a pointer to the root node of the primary
  tree. If the forest is empty, it return 0
 */
NamespaceNode* QDocForest::firstRoot()
{
    currentIndex_ = 0;
    return (!searchOrder().isEmpty() ? searchOrder()[0]->root() : 0);
}

/*!
  Increments the forest's current tree index. If the current
  tree index is still within the forest, the function returns
  the root node of the current tree. Otherwise it returns 0.
 */
NamespaceNode* QDocForest::nextRoot()
{
    ++currentIndex_;
    return (currentIndex_ < searchOrder().size() ? searchOrder()[currentIndex_]->root() : 0);
}

/*!
  Initializes the forest prior to a traversal and
  returns a pointer to the primary tree. If the
  forest is empty, it returns 0.
 */
Tree* QDocForest::firstTree()
{
    currentIndex_ = 0;
    return (!searchOrder().isEmpty() ? searchOrder()[0] : 0);
}

/*!
  Increments the forest's current tree index. If the current
  tree index is still within the forest, the function returns
  the pointer to the current tree. Otherwise it returns 0.
 */
Tree* QDocForest::nextTree()
{
    ++currentIndex_;
    return (currentIndex_ < searchOrder().size() ? searchOrder()[currentIndex_] : 0);
}

/*!
  \fn Tree* QDocForest::primaryTree()

  Returns the pointer to the primary tree.
 */

/*!
  Finds the tree for module \a t in the forest and
  sets the primary tree to be that tree. After the
  primary tree is set, that tree is removed from the
  forest.

  \node It gets re-inserted into the forest after the
  search order is built.
 */
void QDocForest::setPrimaryTree(const QString& t)
{
    QString T = t.toLower();
    primaryTree_ = findTree(T);
    forest_.remove(T);
    if (!primaryTree_)
        qDebug() << "ERROR: Could not set primary tree to:" << t;
}

/*!
  If the search order array is empty, create the search order.
  If the search order array is not empty, do nothing.
 */
void QDocForest::setSearchOrder(QStringList& t)
{
    if (!searchOrder_.isEmpty())
        return;

    /* Allocate space for the search order. */
    searchOrder_.reserve(forest_.size()+1);
    searchOrder_.clear();
    moduleNames_.reserve(forest_.size()+1);
    moduleNames_.clear();

    /* The primary tree is always first in the search order. */
    QString primaryName = primaryTree()->physicalModuleName();
    searchOrder_.append(primaryTree_);
    moduleNames_.append(primaryName);
    forest_.remove(primaryName);

    QMap<QString, Tree*>::iterator i;
    foreach (const QString &m, t) {
        if (primaryName != m) {
            i = forest_.find(m);
            if (i != forest_.end()) {
                searchOrder_.append(i.value());
                moduleNames_.append(m);
                forest_.remove(m);
            }
        }
    }
    /*
      If any trees remain in the forest, just add them
      to the search order sequentially, because we don't
      know any better at this point.
     */
    if (!forest_.isEmpty()) {
        i = forest_.begin();
        while (i != forest_.end()) {
            searchOrder_.append(i.value());
            moduleNames_.append(i.key());
            ++i;
        }
        forest_.clear();
    }

    /*
      Rebuild the forest after constructing the search order.
      It was destroyed during construction of the search order,
      but it is needed for module-specific searches.

      Note that this loop also inserts the primary tree into the
      forrest. That is a requirement.
     */
    for (int i=0; i<searchOrder_.size(); ++i) {
        if (!forest_.contains(moduleNames_.at(i))) {
            forest_.insert(moduleNames_.at(i), searchOrder_.at(i));
        }
    }
#if 0
    qDebug() << "    SEARCH ORDER:";
    for (int i=0; i<moduleNames_.size(); ++i)
        qDebug() << "      " << i+1 << "." << moduleNames_.at(i);
    qDebug() << "    FOREST:" << forest_.keys();
    qDebug() << "SEARCH ORDER:" << moduleNames_;
#endif
}

/*!
  Returns an ordered array of Tree pointers that represents
  the order in which the trees should be searched. The first
  Tree in the array is the tree for the current module, i.e.
  the module for which qdoc is generating documentation.

  The other Tree pointers in the array represent the index
  files that were loaded in preparation for generating this
  module's documentation. Each Tree pointer represents one
  index file. The index file Tree points have been ordered
  heuristically to, hopefully, minimize searching. Thr order
  will probably be changed.

  If the search order array is empty, this function calls
  indexSearchOrder(). The search order array is empty while
  the index files are being loaded, but some searches must
  be performed during this time, notably searches for base
  class nodes. These searches require a temporary search
  order. The temporary order changes throughout the loading
  of the index files, but it is always the tree for the
  current index file first, followed by the trees for the
  index files that have already been loaded. The only
  ordering required in this temporary search order is that
  the current tree must be searched first.
 */
const QVector<Tree*>& QDocForest::searchOrder()
{
    if (searchOrder_.isEmpty())
        return indexSearchOrder();
    return searchOrder_;
}

/*!
  There are two search orders used by qdoc when searching for
  things. The normal search order is returned by searchOrder(),
  but this normal search order is not known until all the index
  files have been read. At that point, setSearchOrder() is
  called.

  During the reading of the index files, the vector holding
  the normal search order remains empty. Whenever the search
  order is requested, if that vector is empty, this function
  is called to return a temporary search order, which includes
  all the index files that have been read so far, plus the
  one being read now. That one is prepended to the front of
  the vector.
 */
const QVector<Tree*>& QDocForest::indexSearchOrder()
{
    if (forest_.size() > indexSearchOrder_.size())
        indexSearchOrder_.prepend(primaryTree_);
    return indexSearchOrder_;
}

/*!
  Create a new Tree for the index file for the specified
  \a module and add it to the forest. Return the pointer
  to its root.
 */
NamespaceNode* QDocForest::newIndexTree(const QString& module)
{
    primaryTree_ = new Tree(module, qdb_);
    forest_.insert(module.toLower(), primaryTree_);
    return primaryTree_->root();
}

/*!
  Create a new Tree for use as the primary tree. This tree
  will represent the primary module. \a module is camel case.
 */
void QDocForest::newPrimaryTree(const QString& module)
{
    primaryTree_ = new Tree(module, qdb_);
}

/*!
  Searches through the forest for a node named \a targetPath
  and returns a pointer to it if found. The \a relative node
  is the starting point. It only makes sense for the primary
  tree, which is searched first. After the primary tree has
  been searched, \a relative is set to 0 for searching the
  other trees, which are all index trees. With relative set
  to 0, the starting point for each index tree is the root
  of the index tree.
 */
const Node* QDocForest::findNodeForTarget(QStringList& targetPath,
                                          const Node* relative,
                                          Node::Genus genus,
                                          QString& ref)
{
    int flags = SearchBaseClasses | SearchEnumValues;

    QString entity = targetPath.takeFirst();
    QStringList entityPath = entity.split("::");

    QString target;
    if (!targetPath.isEmpty())
        target = targetPath.takeFirst();

    foreach (Tree* t, searchOrder()) {
        const Node* n = t->findNodeForTarget(entityPath, target, relative, flags, genus, ref);
        if (n)
            return n;
        relative = 0;
    }
    return 0;
}

/*!
  Print the list of module names ordered according
  to how many successful searches each tree had.
 */
void QDocForest::printLinkCounts(const QString& project)
{
    Location::null.report(QString("%1: Link Counts").arg(project));
    QMultiMap<int, QString> m;
    foreach (Tree* t, searchOrder()) {
        if (t->linkCount() < 0)
            m.insert(t->linkCount(), t->physicalModuleName());
    }
    QString depends = "depends                 +=";
    QString module = project.toLower();
    QMultiMap<int, QString>::iterator i = m.begin();
    while (i != m.end()) {
        QString line = "  " + i.value();
        if (i.value() != module)
            depends += QLatin1Char(' ') + i.value();
        int pad = 30 - line.length();
        for (int k=0; k<pad; ++k)
            line += QLatin1Char(' ');
        line += "%1";
        Location::null.report(line.arg(-(i.key())));
        ++i;
    }
    Location::null.report("Optimal depends variable:");
    Location::null.report(depends);
}

/*!
  Print the list of module names ordered according
  to how many successful searches each tree had.
 */
QString QDocForest::getLinkCounts(QStringList& strings, QVector<int>& counts)
{
    QMultiMap<int, QString> m;
    foreach (Tree* t, searchOrder()) {
        if (t->linkCount() < 0)
            m.insert(t->linkCount(), t->physicalModuleName());
    }
    QString depends = "depends                 +=";
    QString module = Generator::defaultModuleName().toLower();
    QMultiMap<int, QString>::iterator i = m.begin();
    while (i != m.end()) {
        if (i.value() != module) {
            counts.append(-(i.key()));
            strings.append(i.value());
            depends += QLatin1Char(' ') + i.value();
        }
        ++i;
    }
    return depends;
}

/*!
  Finds the node for the qualified function path in \a target
  that also has the parameters in \a params and returns it.
  \a target is the path name to the function, without a return
  type and withpout the parameters.

  \a relative is the node in the primary tree where the search
  begins. It is not used in the other trees, if the node is not
  found in the primary tree. \a genus can be used to force the
  search to find a C++ function or a QML function.

  The entire forest is searched, but the first match is accepted.
 */
const FunctionNode* QDocForest::findFunctionNode(const QString& target,
                                                 const QString& params,
                                                 const Node* relative,
                                                 Node::Genus genus)
{
    foreach (Tree* t, searchOrder()) {
        const Node* n = t->findFunctionNode(target, params, relative, genus);
        if (n)
            return static_cast<const FunctionNode *>(n);
        relative = 0;
    }
    return 0;
}

/*!
  Finds the node for the qualified function path in \a target
  and returns it. \a target is a complete function signature
  without the return type.

  \a relative is the node in the primary tree where the search
  begins. It is not used in the other trees, if the node is not
  found in the primary tree. \a genus can be used to force the
  search to find a C++ function or a QML function.

  The entire forest is searched, but the first match is accepted.
 */
const Node* QDocForest::findFunctionNode(const QString& target,
                                         const Node* relative,
                                         Node::Genus genus)
{
    QString function, params;
    int length = target.length();
    if (target.endsWith(QChar(')'))) {
        int position = target.lastIndexOf(QChar('('));
        params = target.mid(position+1, length-position-2);
        function = target.left(position);
    }
    else
        function = target;
    return findFunctionNode(function, params, relative, genus);
}

/*! \class QDocDatabase
  This class provides exclusive access to the qdoc database,
  which consists of a forrest of trees and a lot of maps and
  other useful data structures.
 */

QDocDatabase* QDocDatabase::qdocDB_ = NULL;
NodeMap QDocDatabase::typeNodeMap_;

/*!
  Constructs the singleton qdoc database object. The singleton
  constructs the \a forest_ object, which is also a singleton.
  \a showInternal_ is normally false. If it is true, qdoc will
  write documentation for nodes marked \c internal.

  \a singleExec_ is false when qdoc is being used in the standard
  way of running qdoc twices for each module, first with -prepare
  and then with -generate. First the -prepare phase is run for
  each module, then the -generate phase is run for each module.

  When \a singleExec_ is true, qdoc is run only once. During the
  single execution, qdoc processes the qdocconf files for all the
  modules sequentially in a loop. Each source file for each module
  is read exactly once.
 */
QDocDatabase::QDocDatabase()
    : showInternal_(false), singleExec_(false), forest_(this)
{
    // nothing
}

/*!
  Destroys the qdoc database object. This requires destroying
  the forest object, which contains an array of tree pointers.
  Each tree is deleted.
 */
QDocDatabase::~QDocDatabase()
{
    // nothing.
}

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
*/
QDocDatabase* QDocDatabase::qdocDB()
{
    if (!qdocDB_) {
      qdocDB_ = new QDocDatabase;
      initializeDB();
    }
   return qdocDB_;
}

/*!
  Destroys the singleton.
 */
void QDocDatabase::destroyQdocDB()
{
    if (qdocDB_) {
        delete qdocDB_;
        qdocDB_ = 0;
    }
}

/*!
  Initialize data structures in the singleton qdoc database.

  In particular, the type node map is initialized with a lot
  type names that don't refer to documented types. For example,
  many C++ standard types are included. These might be documented
  here at some point, but for now they are not. Other examples
  include \c array and \c data, which are just generic names
  used as place holders in function signatures that appear in
  the documentation.

  \note Do not add QML basic types into this list as it will
        break linking to those types.

  Also calls Node::initialize() to initialize the search goal map.
 */
void QDocDatabase::initializeDB()
{
    Node::initialize();
    typeNodeMap_.insert( "accepted", 0);
    typeNodeMap_.insert( "actionPerformed", 0);
    typeNodeMap_.insert( "activated", 0);
    typeNodeMap_.insert( "alias", 0);
    typeNodeMap_.insert( "anchors", 0);
    typeNodeMap_.insert( "any", 0);
    typeNodeMap_.insert( "array", 0);
    typeNodeMap_.insert( "autoSearch", 0);
    typeNodeMap_.insert( "axis", 0);
    typeNodeMap_.insert( "backClicked", 0);
    typeNodeMap_.insert( "boomTime", 0);
    typeNodeMap_.insert( "border", 0);
    typeNodeMap_.insert( "buttonClicked", 0);
    typeNodeMap_.insert( "callback", 0);
    typeNodeMap_.insert( "char", 0);
    typeNodeMap_.insert( "clicked", 0);
    typeNodeMap_.insert( "close", 0);
    typeNodeMap_.insert( "closed", 0);
    typeNodeMap_.insert( "cond", 0);
    typeNodeMap_.insert( "data", 0);
    typeNodeMap_.insert( "dataReady", 0);
    typeNodeMap_.insert( "dateString", 0);
    typeNodeMap_.insert( "dateTimeString", 0);
    typeNodeMap_.insert( "datetime", 0);
    typeNodeMap_.insert( "day", 0);
    typeNodeMap_.insert( "deactivated", 0);
    typeNodeMap_.insert( "drag", 0);
    typeNodeMap_.insert( "easing", 0);
    typeNodeMap_.insert( "error", 0);
    typeNodeMap_.insert( "exposure", 0);
    typeNodeMap_.insert( "fatalError", 0);
    typeNodeMap_.insert( "fileSelected", 0);
    typeNodeMap_.insert( "flags", 0);
    typeNodeMap_.insert( "float", 0);
    typeNodeMap_.insert( "focus", 0);
    typeNodeMap_.insert( "focusZone", 0);
    typeNodeMap_.insert( "format", 0);
    typeNodeMap_.insert( "framePainted", 0);
    typeNodeMap_.insert( "from", 0);
    typeNodeMap_.insert( "frontClicked", 0);
    typeNodeMap_.insert( "function", 0);
    typeNodeMap_.insert( "hasOpened", 0);
    typeNodeMap_.insert( "hovered", 0);
    typeNodeMap_.insert( "hoveredTitle", 0);
    typeNodeMap_.insert( "hoveredUrl", 0);
    typeNodeMap_.insert( "imageCapture", 0);
    typeNodeMap_.insert( "imageProcessing", 0);
    typeNodeMap_.insert( "index", 0);
    typeNodeMap_.insert( "initialized", 0);
    typeNodeMap_.insert( "isLoaded", 0);
    typeNodeMap_.insert( "item", 0);
    typeNodeMap_.insert( "jsdict", 0);
    typeNodeMap_.insert( "jsobject", 0);
    typeNodeMap_.insert( "key", 0);
    typeNodeMap_.insert( "keysequence", 0);
    typeNodeMap_.insert( "listViewClicked", 0);
    typeNodeMap_.insert( "loadRequest", 0);
    typeNodeMap_.insert( "locale", 0);
    typeNodeMap_.insert( "location", 0);
    typeNodeMap_.insert( "long", 0);
    typeNodeMap_.insert( "message", 0);
    typeNodeMap_.insert( "messageReceived", 0);
    typeNodeMap_.insert( "mode", 0);
    typeNodeMap_.insert( "month", 0);
    typeNodeMap_.insert( "name", 0);
    typeNodeMap_.insert( "number", 0);
    typeNodeMap_.insert( "object", 0);
    typeNodeMap_.insert( "offset", 0);
    typeNodeMap_.insert( "ok", 0);
    typeNodeMap_.insert( "openCamera", 0);
    typeNodeMap_.insert( "openImage", 0);
    typeNodeMap_.insert( "openVideo", 0);
    typeNodeMap_.insert( "padding", 0);
    typeNodeMap_.insert( "parent", 0);
    typeNodeMap_.insert( "path", 0);
    typeNodeMap_.insert( "photoModeSelected", 0);
    typeNodeMap_.insert( "position", 0);
    typeNodeMap_.insert( "precision", 0);
    typeNodeMap_.insert( "presetClicked", 0);
    typeNodeMap_.insert( "preview", 0);
    typeNodeMap_.insert( "previewSelected", 0);
    typeNodeMap_.insert( "progress", 0);
    typeNodeMap_.insert( "puzzleLost", 0);
    typeNodeMap_.insert( "qmlSignal", 0);
    typeNodeMap_.insert( "rectangle", 0);
    typeNodeMap_.insert( "request", 0);
    typeNodeMap_.insert( "requestId", 0);
    typeNodeMap_.insert( "section", 0);
    typeNodeMap_.insert( "selected", 0);
    typeNodeMap_.insert( "send", 0);
    typeNodeMap_.insert( "settingsClicked", 0);
    typeNodeMap_.insert( "shoe", 0);
    typeNodeMap_.insert( "short", 0);
    typeNodeMap_.insert( "signed", 0);
    typeNodeMap_.insert( "sizeChanged", 0);
    typeNodeMap_.insert( "size_t", 0);
    typeNodeMap_.insert( "sockaddr", 0);
    typeNodeMap_.insert( "someOtherSignal", 0);
    typeNodeMap_.insert( "sourceSize", 0);
    typeNodeMap_.insert( "startButtonClicked", 0);
    typeNodeMap_.insert( "state", 0);
    typeNodeMap_.insert( "std::initializer_list", 0);
    typeNodeMap_.insert( "std::list", 0);
    typeNodeMap_.insert( "std::map", 0);
    typeNodeMap_.insert( "std::pair", 0);
    typeNodeMap_.insert( "std::string", 0);
    typeNodeMap_.insert( "std::vector", 0);
    typeNodeMap_.insert( "stringlist", 0);
    typeNodeMap_.insert( "swapPlayers", 0);
    typeNodeMap_.insert( "symbol", 0);
    typeNodeMap_.insert( "t", 0);
    typeNodeMap_.insert( "T", 0);
    typeNodeMap_.insert( "tagChanged", 0);
    typeNodeMap_.insert( "timeString", 0);
    typeNodeMap_.insert( "timeout", 0);
    typeNodeMap_.insert( "to", 0);
    typeNodeMap_.insert( "toggled", 0);
    typeNodeMap_.insert( "type", 0);
    typeNodeMap_.insert( "unsigned", 0);
    typeNodeMap_.insert( "urllist", 0);
    typeNodeMap_.insert( "va_list", 0);
    typeNodeMap_.insert( "value", 0);
    typeNodeMap_.insert( "valueEmitted", 0);
    typeNodeMap_.insert( "videoFramePainted", 0);
    typeNodeMap_.insert( "videoModeSelected", 0);
    typeNodeMap_.insert( "videoRecorder", 0);
    typeNodeMap_.insert( "void", 0);
    typeNodeMap_.insert( "volatile", 0);
    typeNodeMap_.insert( "wchar_t", 0);
    typeNodeMap_.insert( "x", 0);
    typeNodeMap_.insert( "y", 0);
    typeNodeMap_.insert( "zoom", 0);
    typeNodeMap_.insert( "zoomTo", 0);
}

/*! \fn NamespaceNode* QDocDatabase::primaryTreeRoot()
  Returns a pointer to the root node of the primary tree.
 */

/*!
  \fn const CNMap& QDocDatabase::groups()
  Returns a const reference to the collection of all
  group nodes in the primary tree.
*/

/*!
  \fn const CNMap& QDocDatabase::modules()
  Returns a const reference to the collection of all
  module nodes in the primary tree.
*/

/*!
  \fn const CNMap& QDocDatabase::qmlModules()
  Returns a const reference to the collection of all
  QML module nodes in the primary tree.
*/

/*!
  \fn const CNMap& QDocDatabase::jsModules()
  Returns a const reference to the collection of all
  JovaScript module nodes in the primary tree.
*/

/*! \fn CollectionNode* QDocDatabase::findGroup(const QString& name)
  Find the group node named \a name and return a pointer
  to it. If a matching node is not found, add a new group
  node named \a name and return a pointer to that one.

  If a new group node is added, its parent is the tree root,
  and the new group node is marked \e{not seen}.
 */

/*! \fn CollectionNode* QDocDatabase::findModule(const QString& name)
  Find the module node named \a name and return a pointer
  to it. If a matching node is not found, add a new module
  node named \a name and return a pointer to that one.

  If a new module node is added, its parent is the tree root,
  and the new module node is marked \e{not seen}.
 */

/*! \fn CollectionNode* QDocDatabase::findQmlModule(const QString& name, bool javaScript)
  Find the QML module node named \a name and return a pointer
  to it. If a matching node is not found, add a new QML module
  node named \a name and return a pointer to that one.

  If \a javaScript is set, the return collection must be a
  JavaScript module.

  If a new QML or JavaScript module node is added, its parent
  is the tree root, and the new node is marked \e{not seen}.
 */

/*! \fn CollectionNode* QDocDatabase::addGroup(const QString& name)
  Looks up the group named \a name in the primary tree. If
  a match is found, a pointer to the node is returned.
  Otherwise, a new group node named \a name is created and
  inserted into the collection, and the pointer to that node
  is returned.
 */

/*! \fn CollectionNode* QDocDatabase::addModule(const QString& name)
  Looks up the module named \a name in the primary tree. If
  a match is found, a pointer to the node is returned.
  Otherwise, a new module node named \a name is created and
  inserted into the collection, and the pointer to that node
  is returned.
 */

/*! \fn CollectionNode* QDocDatabase::addQmlModule(const QString& name)
  Looks up the QML module named \a name in the primary tree.
  If a match is found, a pointer to the node is returned.
  Otherwise, a new QML module node named \a name is created
  and inserted into the collection, and the pointer to that
  node is returned.
 */

/*! \fn CollectionNode* QDocDatabase::addJsModule(const QString& name)
  Looks up the JavaScript module named \a name in the primary
  tree. If a match is found, a pointer to the node is returned.
  Otherwise, a new JavaScript module node named \a name is
  created and inserted into the collection, and the pointer to
  that node is returned.
 */

/*! \fn CollectionNode* QDocDatabase::addToGroup(const QString& name, Node* node)
  Looks up the group node named \a name in the collection
  of all group nodes. If a match is not found, a new group
  node named \a name is created and inserted into the collection.
  Then append \a node to the group's members list, and append the
  group node to the member list of the \a node. The parent of the
  \a node is not changed by this function. Returns a pointer to
  the group node.
 */

/*! \fn CollectionNode* QDocDatabase::addToModule(const QString& name, Node* node)
  Looks up the module node named \a name in the collection
  of all module nodes. If a match is not found, a new module
  node named \a name is created and inserted into the collection.
  Then append \a node to the module's members list. The parent of
  \a node is not changed by this function. Returns the module node.
 */

/*! \fn Collection* QDocDatabase::addToQmlModule(const QString& name, Node* node)
  Looks up the QML module named \a name. If it isn't there,
  create it. Then append \a node to the QML module's member
  list. The parent of \a node is not changed by this function.
 */

/*! \fn Collection* QDocDatabase::addToJsModule(const QString& name, Node* node)
  Looks up the JavaScript module named \a name. If it isn't there,
  create it. Then append \a node to the JavaScript module's member
  list. The parent of \a node is not changed by this function.
 */

/*!
  Looks up the QML type node identified by the qualified Qml
  type \a name and returns a pointer to the QML type node.
 */
QmlTypeNode* QDocDatabase::findQmlType(const QString& name)
{
    QmlTypeNode* qcn = forest_.lookupQmlType(name);
    if (qcn)
        return qcn;
    return 0;
}

/*!
  Looks up the QML type node identified by the Qml module id
  \a qmid and QML type \a name and returns a pointer to the
  QML type node. The key is \a qmid + "::" + \a name.

  If the QML module id is empty, it looks up the QML type by
  \a name only.
 */
QmlTypeNode* QDocDatabase::findQmlType(const QString& qmid, const QString& name)
{
    if (!qmid.isEmpty()) {
        QString t = qmid + "::" + name;
        QmlTypeNode* qcn = forest_.lookupQmlType(t);
        if (qcn)
            return qcn;
    }

    QStringList path(name);
    Node* n = forest_.findNodeByNameAndType(path, Node::QmlType);
    if (n && (n->isQmlType() || n->isJsType()))
        return static_cast<QmlTypeNode*>(n);
    return 0;
}

/*!
  Looks up the QML basic type node identified by the Qml module id
  \a qmid and QML basic type \a name and returns a pointer to the
  QML basic type node. The key is \a qmid + "::" + \a name.

  If the QML module id is empty, it looks up the QML basic type by
  \a name only.
 */
Aggregate* QDocDatabase::findQmlBasicType(const QString& qmid, const QString& name)
{
    if (!qmid.isEmpty()) {
        QString t = qmid + "::" + name;
        Aggregate* a = forest_.lookupQmlBasicType(t);
        if (a)
            return a;
    }

    QStringList path(name);
    Node* n = forest_.findNodeByNameAndType(path, Node::QmlBasicType);
    if (n && n->isQmlBasicType())
        return static_cast<Aggregate*>(n);
    return 0;
}

/*!
  Looks up the QML type node identified by the Qml module id
  constructed from the strings in the \a import record and the
  QML type \a name and returns a pointer to the QML type node.
  If a QML type node is not found, 0 is returned.
 */
QmlTypeNode* QDocDatabase::findQmlType(const ImportRec& import, const QString& name)
{
    if (!import.isEmpty()) {
        QStringList dotSplit;
        dotSplit = name.split(QLatin1Char('.'));
        QString qmName;
        if (import.importUri_.isEmpty())
            qmName = import.name_;
        else
            qmName = import.importUri_;
        for (int i=0; i<dotSplit.size(); ++i) {
            QString qualifiedName = qmName + "::" + dotSplit[i];
            QmlTypeNode* qcn = forest_.lookupQmlType(qualifiedName);
            if (qcn)
                return qcn;
        }
    }
    return 0;
}

/*!
  This function calls a set of functions for each tree in the
  forest that has not already been analyzed. In this way, when
  running qdoc in \e singleExec mode, each tree is analyzed in
  turn, and its classes and types are added to the appropriate
  node maps.
 */
void QDocDatabase::processForest()
{
    Tree* t = forest_.firstTree();
    while (t) {
        findAllClasses(t->root());
        findAllFunctions(t->root());
        findAllObsoleteThings(t->root());
        findAllLegaleseTexts(t->root());
        findAllSince(t->root());
        findAllAttributions(t->root());
        t->setTreeHasBeenAnalyzed();
        t = forest_.nextTree();
    }
    resolveNamespaces();
}

/*!
  This function calls \a func for each tree in the forest,
  but only if Tree::treeHasBeenAnalyzed() returns false for
  the tree. In this way, when running qdoc in \e singleExec
  mode, each tree is analyzed in turn, and its classes and
  types are added to the appropriate node maps.
 */
void QDocDatabase::processForest(void (QDocDatabase::*func) (Aggregate*))
{
    Tree* t = forest_.firstTree();
    while (t) {
        if (!t->treeHasBeenAnalyzed()) {
            (this->*(func))(t->root());
        }
        t = forest_.nextTree();
    }
}

/*!
  Constructs the collection of legalese texts, if it has not
  already been constructed and returns a reference to it.
 */
TextToNodeMap& QDocDatabase::getLegaleseTexts()
{
    if (legaleseTexts_.isEmpty())
        processForest(&QDocDatabase::findAllLegaleseTexts);
    return legaleseTexts_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of C++ classes with obsolete members.
 */
NodeMultiMap& QDocDatabase::getClassesWithObsoleteMembers()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return classesWithObsoleteMembers_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of obsolete QML types.
 */
NodeMultiMap& QDocDatabase::getObsoleteQmlTypes()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return obsoleteQmlTypes_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of QML types with obsolete members.
 */
NodeMultiMap& QDocDatabase::getQmlTypesWithObsoleteMembers()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return qmlTypesWithObsoleteMembers_;
}

/*! \fn NodeMultiMap& QDocDatabase::getNamespaces()
  Returns a reference to the map of all namespace nodes.
  This function must not be called in the -prepare phase.
 */

/*!
  Construct the data structures for QML basic types, if they
  have not already been constructed. Returns a reference to
  the map of QML basic types.
 */
NodeMultiMap& QDocDatabase::getQmlBasicTypes()
{
    if (cppClasses_.isEmpty() && qmlBasicTypes_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return qmlBasicTypes_;
}

/*!
  Construct the data structures for QML types, if they
  have not already been constructed. Returns a reference to
  the multimap of QML types.
 */
NodeMultiMap& QDocDatabase::getQmlTypes()
{
    if (cppClasses_.isEmpty() && qmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return qmlTypes_;
}

/*!
  Construct the data structures for examples, if they
  have not already been constructed. Returns a reference to
  the multimap of example nodes.
 */
NodeMultiMap& QDocDatabase::getExamples()
{
    if (cppClasses_.isEmpty() && examples_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return examples_;
}

/*!
  Construct the data structures for attributions, if they
  have not already been constructed. Returns a reference to
  the multimap of attribution nodes.
 */
NodeMultiMap& QDocDatabase::getAttributions()
{
    if (attributions_.isEmpty())
        processForest(&QDocDatabase::findAllAttributions);
    return attributions_;
}

/*!
  Construct the data structures for obsolete things, if they
  have not already been constructed. Returns a reference to
  the map of obsolete C++ clases.
 */
NodeMultiMap& QDocDatabase::getObsoleteClasses()
{
    if (obsoleteClasses_.isEmpty() && obsoleteQmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllObsoleteThings);
    return obsoleteClasses_;
}

/*!
  Construct the C++ class data structures, if they have not
  already been constructed. Returns a reference to the map
  of all C++ classes.
 */
NodeMultiMap& QDocDatabase::getCppClasses()
{
    if (cppClasses_.isEmpty() && qmlTypes_.isEmpty())
        processForest(&QDocDatabase::findAllClasses);
    return cppClasses_;
}

/*!
  Finds all the C++ class nodes and QML type nodes and
  sorts them into maps.
 */
void QDocDatabase::findAllClasses(Aggregate* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private && (!(*c)->isInternal() || showInternal_) &&
            (*c)->tree()->camelCaseModuleName() != QString("QDoc")) {

            if ((*c)->type() == Node::Class) {
                QString className = (*c)->name();
                if ((*c)->parent() &&
                        (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                    className = (*c)->parent()->name()+"::"+className;

                cppClasses_.insert(className.toLower(), *c);
            }
            else if ((*c)->isQmlType() || (*c)->isQmlBasicType() ||
                      (*c)->isJsType() || (*c)->isJsBasicType()) {
                QString qmlTypeName = (*c)->name().toLower();
                if (qmlTypeName.startsWith(QLatin1String("QML:"), Qt::CaseInsensitive))
                    qmlTypes_.insert(qmlTypeName.mid(4),*c);
                else
                    qmlTypes_.insert(qmlTypeName,*c);

                //also add to the QML basic type map
                if ((*c)->isQmlBasicType() || (*c)->isJsBasicType())
                    qmlBasicTypes_.insert(qmlTypeName,*c);
            }
            else if ((*c)->isExample()) {
                // use the module index title as key for the example map
                QString title = (*c)->tree()->indexTitle();
                if (!examples_.contains(title, *c))
                    examples_.insert(title, *c);
            }
            else if ((*c)->isAggregate()) {
                findAllClasses(static_cast<Aggregate*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Construct the function index data structure and return it.
  This data structure is used to output the function index page.
 */
NodeMapMap& QDocDatabase::getFunctionIndex()
{
    processForest(&QDocDatabase::findAllFunctions);
    return funcIndex_;
}

/*!
  Finds all the function nodes
 */
void QDocDatabase::findAllFunctions(Aggregate* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->isAggregate()) {
                findAllFunctions(static_cast<Aggregate*>(*c));
            }
            else if ((*c)->type() == Node::Function) {
                const FunctionNode* func = static_cast<const FunctionNode*>(*c);
                if ((func->status() > Node::Obsolete) && !func->isInternal() &&
                    !func->isSomeCtor() && !func->isDtor()) {
                    funcIndex_[(*c)->name()].insert((*c)->parent()->fullDocumentName(), *c);
                }
            }
        }
        ++c;
    }
}

/*!
  Finds all the attribution pages and collects them per module
 */
void QDocDatabase::findAllAttributions(Aggregate* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if ((*c)->docSubtype() == Node::Page
                     && (*c)->pageType() == Node::AttributionPage) {
                attributions_.insertMulti((*c)->tree()->indexTitle(), *c);
            } else if ((*c)->isAggregate()) {
                findAllAttributions(static_cast<Aggregate*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Finds all the nodes containing legalese text and puts them
  in a map.
 */
void QDocDatabase::findAllLegaleseTexts(Aggregate* node)
{
    NodeList::ConstIterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            if (!(*c)->doc().legaleseText().isEmpty())
                legaleseTexts_.insertMulti((*c)->doc().legaleseText(), *c);
            if ((*c)->isAggregate())
                findAllLegaleseTexts(static_cast<Aggregate *>(*c));
        }
        ++c;
    }
}

/*!
  Finds all the namespace nodes in the tree beginning at
  \a node and puts them in a map to be used later as an
  index.

  Ensure each namespace node has a name before inserting
  it into the map, because the root namespace node has no
  name, and we are not interested in it.
 */
void QDocDatabase::findAllNamespaces(Aggregate* node)
{
    foreach (Node* n, node->childNodes()) {
        if (n->isNamespace() && !n->name().isEmpty())
            nmm_.insert(n->name(), n);
        if (n->isNamespace() || (n->isAggregate() && n->access() != Node::Private))
            findAllNamespaces(static_cast<Aggregate *>(n));
    }
}

/*!
  Finds all nodes with status = Obsolete and sorts them into
  maps. They can be C++ classes, QML types, or they can be
  functions, enum types, typedefs, methods, etc.
 */
void QDocDatabase::findAllObsoleteThings(Aggregate* node)
{
    NodeList::const_iterator c = node->childNodes().constBegin();
    while (c != node->childNodes().constEnd()) {
        if ((*c)->access() != Node::Private) {
            QString name = (*c)->name();
            if ((*c)->status() == Node::Obsolete) {
                if ((*c)->type() == Node::Class) {
                    if ((*c)->parent() && (*c)->parent()->type() == Node::Namespace &&
                        !(*c)->parent()->name().isEmpty())
                        name = (*c)->parent()->name() + "::" + name;
                    obsoleteClasses_.insert(name, *c);
                }
                else if ((*c)->isQmlType() || (*c)->isJsType()) {
                    if (name.startsWith(QLatin1String("QML:")))
                        name = name.mid(4);
                    name = (*c)->logicalModuleName() + "::" + name;
                    obsoleteQmlTypes_.insert(name,*c);
                }
            }
            else if ((*c)->type() == Node::Class) {
                Aggregate* n = static_cast<Aggregate*>(*c);
                bool inserted = false;
                NodeList::const_iterator p = n->childNodes().constBegin();
                while (p != n->childNodes().constEnd()) {
                    if ((*p)->access() != Node::Private) {
                        switch ((*p)->type()) {
                        case Node::Enum:
                        case Node::Typedef:
                        case Node::Function:
                        case Node::Property:
                        case Node::Variable:
                            if ((*p)->status() == Node::Obsolete) {
                                if ((*c)->parent() && (*c)->parent()->type() == Node::Namespace &&
                                    !(*c)->parent()->name().isEmpty())
                                    name = (*c)->parent()->name() + "::" + name;
                                classesWithObsoleteMembers_.insert(name, *c);
                                inserted = true;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    if (inserted)
                        break;
                    ++p;
                }
            }
            else if ((*c)->isQmlType() || (*c)->isJsType()) {
                Aggregate* n = static_cast<Aggregate*>(*c);
                bool inserted = false;
                NodeList::const_iterator p = n->childNodes().constBegin();
                while (p != n->childNodes().constEnd()) {
                    if ((*p)->access() != Node::Private) {
                        switch ((*c)->type()) {
                        case Node::QmlProperty:
                        case Node::QmlSignal:
                        case Node::QmlSignalHandler:
                        case Node::QmlMethod:
                            if ((*c)->parent()) {
                                Node* parent = (*c)->parent();
                                if ((parent->isQmlPropertyGroup() ||
                                     parent->isJsPropertyGroup()) && parent->parent())
                                    parent = parent->parent();
                                if (parent && (parent->isQmlType() || parent->isJsType()) &&
                                    !parent->name().isEmpty())
                                    name = parent->name() + "::" + name;
                            }
                            qmlTypesWithObsoleteMembers_.insert(name,*c);
                            inserted = true;
                            break;
                        default:
                            break;
                        }
                    }
                    if (inserted)
                        break;
                    ++p;
                }
            }
            else if ((*c)->isAggregate()) {
                findAllObsoleteThings(static_cast<Aggregate*>(*c));
            }
        }
        ++c;
    }
}

/*!
  Finds all the nodes where a \e{since} command appeared in the
  qdoc comment and sorts them into maps according to the kind of
  node.

  This function is used for generating the "New Classes... in x.y"
  section on the \e{What's New in Qt x.y} page.
 */
void QDocDatabase::findAllSince(Aggregate* node)
{
    NodeList::const_iterator child = node->childNodes().constBegin();
    while (child != node->childNodes().constEnd()) {
        QString sinceString = (*child)->since();
        // Insert a new entry into each map for each new since string found.
        if (((*child)->access() != Node::Private) && !sinceString.isEmpty()) {
            NodeMultiMapMap::iterator nsmap = newSinceMaps_.find(sinceString);
            if (nsmap == newSinceMaps_.end())
                nsmap = newSinceMaps_.insert(sinceString,NodeMultiMap());

            NodeMapMap::iterator ncmap = newClassMaps_.find(sinceString);
            if (ncmap == newClassMaps_.end())
                ncmap = newClassMaps_.insert(sinceString,NodeMap());

            NodeMapMap::iterator nqcmap = newQmlTypeMaps_.find(sinceString);
            if (nqcmap == newQmlTypeMaps_.end())
                nqcmap = newQmlTypeMaps_.insert(sinceString,NodeMap());

            if ((*child)->type() == Node::Function) {
                // Insert functions into the general since map.
                FunctionNode *func = static_cast<FunctionNode *>(*child);
                if ((func->status() > Node::Obsolete) && !func->isSomeCtor() && !func->isDtor()) {
                    nsmap.value().insert(func->name(),(*child));
                }
            }
            else {
                if ((*child)->type() == Node::Class) {
                    // Insert classes into the since and class maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    ncmap.value().insert(className,(*child));
                }
                else if ((*child)->isQmlType() || (*child)->isJsType()) {
                    // Insert QML elements into the since and element maps.
                    QString className = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        className = (*child)->parent()->name()+"::"+className;
                    }
                    nsmap.value().insert(className,(*child));
                    nqcmap.value().insert(className,(*child));
                }
                else if ((*child)->isQmlProperty() || (*child)->isJsProperty()) {
                    // Insert QML properties into the since map.
                    QString propertyName = (*child)->name();
                    nsmap.value().insert(propertyName,(*child));
                }
                else {
                    // Insert external documents into the general since map.
                    QString name = (*child)->name();
                    if ((*child)->parent() && !(*child)->parent()->name().isEmpty()) {
                        name = (*child)->parent()->name()+"::"+name;
                    }
                    nsmap.value().insert(name,(*child));
                }
            }
        }
        // Recursively find child nodes with since commands.
        if ((*child)->isAggregate())
            findAllSince(static_cast<Aggregate *>(*child));

        ++child;
    }
}

/*!
  Find the \a key in the map of new class maps, and return a
  reference to the value, which is a NodeMap. If \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getClassMap(const QString& key)
{
    if (newSinceMaps_.isEmpty() && newClassMaps_.isEmpty() && newQmlTypeMaps_.isEmpty())
        processForest(&QDocDatabase::findAllSince);
    NodeMapMap::const_iterator i = newClassMaps_.constFind(key);
    if (i != newClassMaps_.constEnd())
        return i.value();
    return emptyNodeMap_;
}

/*!
  Find the \a key in the map of new QML type maps, and return a
  reference to the value, which is a NodeMap. If the \a key is not
  found, return a reference to an empty NodeMap.
 */
const NodeMap& QDocDatabase::getQmlTypeMap(const QString& key)
{
    if (newSinceMaps_.isEmpty() && newClassMaps_.isEmpty() && newQmlTypeMaps_.isEmpty())
        processForest(&QDocDatabase::findAllSince);
    NodeMapMap::const_iterator i = newQmlTypeMaps_.constFind(key);
    if (i != newQmlTypeMaps_.constEnd())
        return i.value();
    return emptyNodeMap_;
}

/*!
  Find the \a key in the map of new \e {since} maps, and return
  a reference to the value, which is a NodeMultiMap. If \a key
  is not found, return a reference to an empty NodeMultiMap.
 */
const NodeMap& QDocDatabase::getSinceMap(const QString& key)
{
    if (newSinceMaps_.isEmpty() && newClassMaps_.isEmpty() && newQmlTypeMaps_.isEmpty())
        processForest(&QDocDatabase::findAllSince);
    NodeMultiMapMap::const_iterator i = newSinceMaps_.constFind(key);
    if (i != newSinceMaps_.constEnd())
        return i.value();
    return emptyNodeMultiMap_;
}

/*!
  Performs several housekeeping algorithms that create
  certain data structures and resolve lots of links, prior
  to generating documentation.
 */
void QDocDatabase::resolveIssues() {
    primaryTreeRoot()->normalizeOverloads();
    fixInheritance();
    resolveProperties();
    primaryTreeRoot()->makeUndocumentedChildrenInternal();
    resolveQmlInheritance(primaryTreeRoot());
    primaryTree()->resolveTargets(primaryTreeRoot());
    primaryTree()->resolveCppToQmlLinks();
    if (!Generator::singleExec()) {
        QDocIndexFiles::qdocIndexFiles()->resolveRelates();
        QDocIndexFiles::destroyQDocIndexFiles();
    }
    if (Generator::generating())
        resolveNamespaces();
}

void QDocDatabase::resolveStuff()
{
    primaryTree()->resolveInheritance();
    resolveQmlInheritance(primaryTreeRoot());
    //primaryTree()->resolveTargets(primaryTreeRoot());
    primaryTree()->resolveCppToQmlLinks();
    primaryTree()->resolveUsingClauses();
    resolveNamespaces();
}

/*!
  Multiple namespace nodes for a particular namespace can be
  created in multiple places. This function first finds all
  namespace nodes and inserts them into a multimap. Then it
  combines all the namespace nodes with the same name into a
  single node and inserts that combined namespace node into
  a namespace index.
 */
void QDocDatabase::resolveNamespaces()
{
    if (!namespaceIndex_.isEmpty())
        return;
    Tree* t = forest_.firstTree();
    while (t) {
        findAllNamespaces(t->root());
        t = forest_.nextTree();
    }
    QList<QString> keys = nmm_.uniqueKeys();
    foreach (const QString &s, keys) {
        NamespaceNode* ns = 0;
        NamespaceNode* somewhere = 0;
        QList<Node*> nodes = nmm_.values(s);
        int count = nmm_.remove(s);
        if (count > 0) {
            foreach (Node* n, nodes) {
                ns = static_cast<NamespaceNode*>(n);
                if (ns->isDocumentedHere())
                    break;
                else if (ns->wasDocumented())
                    somewhere = ns;
                ns = 0;
            }
            if (ns) {
                foreach (Node* n, nodes) {
                    NamespaceNode* NS = static_cast<NamespaceNode*>(n);
                    if (NS->wasDocumented() && NS != ns) {
                        ns->doc().location().warning(tr("Namespace %1 documented more than once").arg(NS->name()));
                        NS->doc().location().warning(tr("...also seen here"));
                    }
                }

            } else if (somewhere == 0) {
                foreach (Node* n, nodes) {
                    NamespaceNode* NS = static_cast<NamespaceNode*>(n);
                    NS->reportDocumentedChildrenInUndocumentedNamespace();
                }
            }
            if (somewhere) {
                foreach (Node* n, nodes) {
                    NamespaceNode* NS = static_cast<NamespaceNode*>(n);
                    if (NS != somewhere)
                        NS->setDocNode(somewhere);
                }
            }
        }
        if (ns && count > 1) {
            foreach (Node* n, nodes) {
                NamespaceNode* NS = static_cast<NamespaceNode*>(n);
                if ((NS != ns) && !NS->childNodes().isEmpty()) {
                    const NodeList& children = NS->childNodes();
                    int i = children.size() - 1;
                    while (i >= 0) {
                        Node* child = children.at(i--);
                        if (child && child->isPublic() && !child->isInternal())
                            ns->addOrphan(child);
                    }
                }
            }
        }
        if (ns == 0)
            ns = static_cast<NamespaceNode*>(nodes.at(0));
        namespaceIndex_.insert(ns->name(), ns);
    }
}

/*!
  This function is called for autolinking to a \a type,
  which could be a function return type or a parameter
  type. The tree node that represents the \a type is
  returned. All the trees are searched until a match is
  found. When searching the primary tree, the search
  begins at \a relative and proceeds up the parent chain.
  When searching the index trees, the search begins at the
  root.
 */
const Node* QDocDatabase::findTypeNode(const QString& type, const Node* relative, Node::Genus genus)
{
    QStringList path = type.split("::");
    if ((path.size() == 1) && (path.at(0)[0].isLower() || path.at(0) == QString("T"))) {
        NodeMap::iterator i = typeNodeMap_.find(path.at(0));
        if (i != typeNodeMap_.end())
            return i.value();
    }
    return forest_.findTypeNode(path, relative, genus);
}

/*!
  Finds the node that will generate the documentation that
  contains the \a target and returns a pointer to it.

  Can this be improved by using the target map in Tree?
 */
const Node* QDocDatabase::findNodeForTarget(const QString& target, const Node* relative)
{
    const Node* node = 0;
    if (target.isEmpty())
        node = relative;
    else if (target.endsWith(".html"))
        node = findNodeByNameAndType(QStringList(target), Node::Document);
    else {
        QStringList path = target.split("::");
        int flags = SearchBaseClasses | SearchEnumValues;
        foreach (Tree* t, searchOrder()) {
            const Node* n = t->findNode(path, relative, flags, Node::DontCare);
            if (n)
                return n;
            relative = 0;
        }
        node = findDocumentNodeByTitle(target);
    }
    return node;
}

/*!
  For each QML Type node in the tree beginning at \a root,
  if it has a QML base type name but its QML base type node
  pointer is 0, use the QML base type name to look up the
  base type node. If the node is found in the tree, set the
  node's QML base type node pointer.
 */
void QDocDatabase::resolveQmlInheritance(Aggregate* root)
{
    NodeMap previousSearches;
    // Do we need recursion?
    foreach (Node* child, root->childNodes()) {
        if (child->isQmlType() || child->isJsType()) {
            QmlTypeNode* qcn = static_cast<QmlTypeNode*>(child);
            if (qcn->qmlBaseNodeNotSet() && !qcn->qmlBaseName().isEmpty()) {
                QmlTypeNode* bqcn = static_cast<QmlTypeNode*>(previousSearches.value(qcn->qmlBaseName()));
                if (bqcn && (bqcn != qcn)) {
                    qcn->setQmlBaseNode(bqcn);
                    QmlTypeNode::addInheritedBy(bqcn, qcn);
                }
                else {
                    if (!qcn->importList().isEmpty()) {
                        const ImportList& imports = qcn->importList();
                        for (int i=0; i<imports.size(); ++i) {
                            bqcn = findQmlType(imports[i], qcn->qmlBaseName());
                            if (bqcn && (bqcn != qcn)) {
                                if (bqcn->logicalModuleVersion()[0] != imports[i].version_[0])
                                    bqcn = 0; // Safeguard for QTBUG-53529
                                break;
                            }
                        }
                    }
                    if (bqcn == 0) {
                        bqcn = findQmlType(QString(), qcn->qmlBaseName());
                    }
                    if (bqcn && (bqcn != qcn)) {
                        qcn->setQmlBaseNode(bqcn);
                        QmlTypeNode::addInheritedBy(bqcn, qcn);
                        previousSearches.insert(qcn->qmlBaseName(), bqcn);
                    }
#if 0
                    else {
                        qDebug() << "Temporary error message (ignore): UNABLE to resolve QML base type:"
                                 << qcn->qmlBaseName() << "for QML type:" << qcn->name();
                    }
#endif
                }
            }
        }
    }
}

/*!
  Generates a tag file and writes it to \a name.
 */
void QDocDatabase::generateTagFile(const QString& name, Generator* g)
{
    if (!name.isEmpty()) {
        QDocTagFiles::qdocTagFiles()->generateTagFile(name, g);
        QDocTagFiles::destroyQDocTagFiles();
    }
}

/*!
  Reads and parses the qdoc index files listed in \a t.
 */
void QDocDatabase::readIndexes(const QStringList& t)
{
    QStringList indexFiles;
    foreach (const QString& f, t) {
        QString fn = f.mid(f.lastIndexOf(QChar('/'))+1);
        if (!isLoaded(fn))
            indexFiles << f;
        else
            qDebug() << "This index file is already in memory:" << f;
    }
    QDocIndexFiles::qdocIndexFiles()->readIndexes(indexFiles);
}

/*!
  Generates a qdoc index file and write it to \a fileName. The
  index file is generated with the parameters \a url, \a title,
  \a g, and \a generateInternalNodes.
 */
void QDocDatabase::generateIndex(const QString& fileName,
                                 const QString& url,
                                 const QString& title,
                                 Generator* g,
                                 bool generateInternalNodes)
{
    QString t = fileName.mid(fileName.lastIndexOf(QChar('/'))+1);
    primaryTree()->setIndexFileName(t);
    QDocIndexFiles::qdocIndexFiles()->generateIndex(fileName, url, title, g, generateInternalNodes);
    QDocIndexFiles::destroyQDocIndexFiles();
}

/*!
  If there are open namespaces, search for the function node
  having the same function name as the \a clone node in each
  open namespace. The \a parentPath is a portion of the path
  name provided with the function name at the point of
  reference. \a parentPath is usually a class name. Return
  the pointer to the function node if one is found in an
  open namespace. Otherwise return 0.

  This open namespace concept is of dubious value and might
  be removed.
 */
FunctionNode* QDocDatabase::findNodeInOpenNamespace(const QStringList& parentPath,
                                                    const FunctionNode* clone)
{
    FunctionNode* fn = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList path = t.split("::") + parentPath;
            fn = findFunctionNode(path, clone);
            if (fn)
                break;
        }
    }
    return fn;
}

/*!
  Find a node of the specified \a type that is reached with
  the specified \a path qualified with the name of one of the
  open namespaces (might not be any open ones). If the node
  is found in an open namespace, prefix \a path with the name
  of the open namespace and "::" and return a pointer to the
  node. Othewrwise return 0.

  This function only searches in the current primary tree.
 */
Node* QDocDatabase::findNodeInOpenNamespace(QStringList& path, Node::NodeType type)
{
    if (path.isEmpty())
        return 0;
    Node* n = 0;
    if (!openNamespaces_.isEmpty()) {
        foreach (const QString& t, openNamespaces_) {
            QStringList p;
            if (t != path[0])
                p = t.split("::") + path;
            else
                p = path;
            n = primaryTree()->findNodeByNameAndType(p, type);
            if (n) {
                path = p;
                break;
            }
        }
    }
    return n;
}

/*!
  Finds all the collection nodes of the specified \a genus
  into the collection node map \a cnm. Nodes that match the
  \a relative node are not included.
 */
void QDocDatabase::mergeCollections(Node::Genus genus, CNMap& cnm, const Node* relative)
{
    cnm.clear();
    CNMultiMap cnmm;
    foreach (Tree* t, searchOrder()) {
        CNMap* m = t->getCollectionMap(genus);
        if (m && !m->isEmpty()) {
            CNMap::const_iterator i = m->cbegin();
            while (i != m->cend()) {
                if (!i.value()->isInternal())
                    cnmm.insert(i.key(), i.value());
                ++i;
            }
        }
    }
    if (cnmm.isEmpty())
        return;
    QRegExp singleDigit("\\b([0-9])\\b");
    QStringList keys = cnmm.uniqueKeys();
    foreach (const QString &key, keys) {
        QList<CollectionNode*> values = cnmm.values(key);
        CollectionNode* n = 0;
        foreach (CollectionNode* v, values) {
            if (v && v->wasSeen() && (v != relative)) {
                n = v;
                break;
            }
        }
        if (n) {
            if (values.size() > 1) {
                foreach (CollectionNode* v, values) {
                    if (v != n) {
                        // Allow multiple (major) versions of QML/JS modules
                        if (n->type() == Node::QmlModule
                                && n->logicalModuleIdentifier() != v->logicalModuleIdentifier()) {
                            if (v->wasSeen() && v != relative && !v->members().isEmpty())
                                cnm.insert(v->fullTitle().toLower(), v);
                            continue;
                        }
                        foreach (Node* t, v->members())
                            n->addMember(t);
                    }
                }
            }
            if (!n->members().isEmpty()) {
                QString sortKey = n->fullTitle().toLower();
                if (sortKey.startsWith("the "))
                    sortKey.remove(0, 4);
                sortKey.replace(singleDigit, "0\\1");
                cnm.insert(sortKey, n);
            }
        }
    }
}

/*!
  Finds all the collection nodes with the same name
  and genus as \a c and merges their members into the
  members list of \a c.

  For QML and JS modules, the merge is done only if
  the module identifier matches between the nodes, to avoid
  merging modules with different (major) versions.
 */
void QDocDatabase::mergeCollections(CollectionNode* c)
{
    foreach (Tree* t, searchOrder()) {
        CollectionNode* cn = t->getCollection(c->name(), c->genus());
        if (cn && cn != c) {
            if (cn->type() == Node::QmlModule
                    && cn->logicalModuleIdentifier() != c->logicalModuleIdentifier())
                continue;
            foreach (Node* n, cn->members())
                c->addMember(n);
        }
    }
}

/*!
  Searches for the node that matches the path in \a atom. The
  \a relative node is used if the first leg of the path is
  empty, i.e. if the path begins with a hashtag. The function
  also sets \a ref if there remains an unused leg in the path
  after the node is found. The node is returned as well as the
  \a ref. If the returned node pointer is null, \a ref is not
  valid.
 */
const Node* QDocDatabase::findNodeForAtom(const Atom* a, const Node* relative, QString& ref)
{
    const Node* node = 0;

    Atom* atom = const_cast<Atom*>(a);
    QStringList targetPath = atom->string().split(QLatin1Char('#'));
    QString first = targetPath.first().trimmed();

    Tree* domain = 0;
    Node::Genus genus = Node::DontCare;
    // Reserved for future use
    //Node::NodeType goal = Node::NoType;

    if (atom->isLinkAtom()) {
        domain = atom->domain();
        genus = atom->genus();
        // Reserved for future use
        //goal = atom->goal();
    }

    if (first.isEmpty())
        node = relative; // search for a target on the current page.
    else if (domain) {
        if (first.endsWith(".html"))
            node = domain->findNodeByNameAndType(QStringList(first), Node::Document);
        else if (first.endsWith(QChar(')'))) {
            QString function, params;
            int length = first.length();
            int position = first.lastIndexOf(QChar('('));
            params = first.mid(position+1, length-position-2);
            function = first.left(position);
            node = domain->findFunctionNode(function, params, 0, genus);
        }
        if (!node) {
            int flags = SearchBaseClasses | SearchEnumValues;
            QStringList nodePath = first.split("::");
            QString target;
            targetPath.removeFirst();
            if (!targetPath.isEmpty())
                target = targetPath.takeFirst();
            if (relative && relative->tree()->physicalModuleName() != domain->physicalModuleName())
                relative = 0;
            return domain->findNodeForTarget(nodePath, target, relative, flags, genus, ref);
        }
    }
    else {
        if (first.endsWith(".html"))
            node = findNodeByNameAndType(QStringList(first), Node::Document);
        else if (first.endsWith(QChar(')'))) {
            node = findFunctionNode(first, relative, genus);
        }
        if (!node)
            return findNodeForTarget(targetPath, relative, genus, ref);
    }

    if (node && ref.isEmpty()) {
        if (!node->url().isEmpty())
            return node;
        targetPath.removeFirst();
        if (!targetPath.isEmpty()) {
            ref = node->root()->tree()->getRef(targetPath.first(), node);
            if (ref.isEmpty())
                node = 0;
        }
    }
    return node;
}

QT_END_NAMESPACE
