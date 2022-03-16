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

#include "qxmlstream.h"
#include "qdocindexfiles.h"
#include "qdoctagfiles.h"
#include "config.h"
#include "qdocdatabase.h"
#include "location.h"
#include "atom.h"
#include "generator.h"
#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static Node* top = 0;

/*!
  \class QDocIndexFiles

  This class handles qdoc index files.
 */

QDocIndexFiles* QDocIndexFiles::qdocIndexFiles_ = NULL;

/*!
  Constructs the singleton QDocIndexFiles.
 */
QDocIndexFiles::QDocIndexFiles()
    : gen_( 0 )
{
    qdb_ = QDocDatabase::qdocDB();
}

/*!
  Destroys the singleton QDocIndexFiles.
 */
QDocIndexFiles::~QDocIndexFiles()
{
    qdb_ = 0;
    gen_ = 0;
}

/*!
  Creates the singleton. Allows only one instance of the class
  to be created. Returns a pointer to the singleton.
 */
QDocIndexFiles* QDocIndexFiles::qdocIndexFiles()
{
   if (!qdocIndexFiles_)
      qdocIndexFiles_ = new QDocIndexFiles;
   return qdocIndexFiles_;
}

/*!
  Destroys the singleton.
 */
void QDocIndexFiles::destroyQDocIndexFiles()
{
    if (qdocIndexFiles_) {
        delete qdocIndexFiles_;
        qdocIndexFiles_ = 0;
    }
}

/*!
  Reads and parses the list of index files in \a indexFiles.
 */
void QDocIndexFiles::readIndexes(const QStringList& indexFiles)
{
    relatedList_.clear();
    foreach (const QString& indexFile, indexFiles) {
        QString msg = "Loading index file: " + indexFile;
        Location::logToStdErr(msg);
        //qDebug() << msg;
        readIndexFile(indexFile);
    }
}

static bool readingRoot = true;

/*!
  Reads and parses the index file at \a path.
 */
void QDocIndexFiles::readIndexFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Could not read index file" << path;
        return;
    }

    QXmlStreamReader reader(&file);
    reader.setNamespaceProcessing(false);

    if (!reader.readNextStartElement())
        return;

    if (reader.name() != QLatin1String("INDEX"))
        return;

    QXmlStreamAttributes attrs = reader.attributes();

    // Generate a relative URL between the install dir and the index file
    // when the -installdir command line option is set.
    QString indexUrl;
    if (Config::installDir.isEmpty()) {
        indexUrl = attrs.value(QLatin1String("url")).toString();
    }
    else {
        // Use a fake directory, since we will copy the output to a sub directory of
        // installDir when using "make install". This is just for a proper relative path.
        //QDir installDir(path.section('/', 0, -3) + "/outputdir");
        QDir installDir(path.section('/', 0, -3) + '/' + Generator::outputSubdir());
        indexUrl = installDir.relativeFilePath(path).section('/', 0, -2);
    }
    project_ = attrs.value(QLatin1String("project")).toString();
    QString indexTitle = attrs.value(QLatin1String("indexTitle")).toString();
    basesList_.clear();

    NamespaceNode* root = qdb_->newIndexTree(project_);
    root->tree()->setIndexTitle(indexTitle);

    // Scan all elements in the XML file, constructing a map that contains
    // base classes for each class found.
    while (reader.readNextStartElement()) {
        readingRoot = true;
        readIndexSection(reader, root, indexUrl);
    }

    // Now that all the base classes have been found for this index,
    // arrange them into an inheritance hierarchy.
    resolveIndex();
}

/*!
  Read a <section> element from the index file and create the
  appropriate node(s).
 */
void QDocIndexFiles::readIndexSection(QXmlStreamReader& reader,
                                      Node* current,
                                      const QString& indexUrl)
{
    QXmlStreamAttributes attributes = reader.attributes();
    QStringRef elementName = reader.name();

    QString name = attributes.value(QLatin1String("name")).toString();
    QString href = attributes.value(QLatin1String("href")).toString();
    Node* node;
    Location location;
    Aggregate* parent = 0;

    bool hasReadChildren = false;

    if (current->isAggregate())
        parent = static_cast<Aggregate*>(current);

    QString filePath;
    int lineNo = 0;
    if (attributes.hasAttribute(QLatin1String("filepath"))) {
        filePath = attributes.value(QLatin1String("filepath")).toString();
        lineNo = attributes.value("lineno").toInt();
    }
    if (elementName == QLatin1String("namespace")) {
        NamespaceNode* ns = new NamespaceNode(parent, name);
        node = ns;
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");
        if (attributes.hasAttribute(QLatin1String("documented"))) {
            if (attributes.value(QLatin1String("documented")) == QLatin1String("true"))
                ns->setDocumented();
        }

    }
    else if (elementName == QLatin1String("class")) {
        node = new ClassNode(parent, name);
        if (attributes.hasAttribute(QLatin1String("bases"))) {
            QString bases = attributes.value(QLatin1String("bases")).toString();
            if (!bases.isEmpty())
                basesList_.append(QPair<ClassNode*,QString>(static_cast<ClassNode*>(node), bases));
        }
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name.toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(name.toLower() + ".html");
        bool abstract = false;
        if (attributes.value(QLatin1String("abstract")) == QLatin1String("true"))
            abstract = true;
        node->setAbstract(abstract);
    }
    else if (elementName == QLatin1String("qmlclass")) {
        QmlTypeNode* qcn = new QmlTypeNode(parent, name);
        qcn->setTitle(attributes.value(QLatin1String("title")).toString());
        QString logicalModuleName = attributes.value(QLatin1String("qml-module-name")).toString();
        if (!logicalModuleName.isEmpty())
            qdb_->addToQmlModule(logicalModuleName, qcn);
        bool abstract = false;
        if (attributes.value(QLatin1String("abstract")) == QLatin1String("true"))
            abstract = true;
        qcn->setAbstract(abstract);
        QString qmlFullBaseName = attributes.value(QLatin1String("qml-base-type")).toString();
        if (!qmlFullBaseName.isEmpty()) {
            qcn->setQmlBaseName(qmlFullBaseName);
        }
        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value("location").toString();
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qcn;
    }
    else if (elementName == QLatin1String("jstype")) {
        QmlTypeNode* qcn = new QmlTypeNode(parent, name);
        qcn->setGenus(Node::JS);
        qcn->setTitle(attributes.value(QLatin1String("title")).toString());
        QString logicalModuleName = attributes.value(QLatin1String("js-module-name")).toString();
        if (!logicalModuleName.isEmpty())
            qdb_->addToQmlModule(logicalModuleName, qcn);
        bool abstract = false;
        if (attributes.value(QLatin1String("abstract")) == QLatin1String("true"))
            abstract = true;
        qcn->setAbstract(abstract);
        QString qmlFullBaseName = attributes.value(QLatin1String("js-base-type")).toString();
        if (!qmlFullBaseName.isEmpty()) {
            qcn->setQmlBaseName(qmlFullBaseName);
        }
        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value("location").toString();
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qcn;
    }
    else if (elementName == QLatin1String("qmlbasictype")) {
        QmlBasicTypeNode* qbtn = new QmlBasicTypeNode(parent, name);
        qbtn->setTitle(attributes.value(QLatin1String("title")).toString());
        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value("location").toString();
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qbtn;
    }
    else if (elementName == QLatin1String("jsbasictype")) {
        QmlBasicTypeNode* qbtn = new QmlBasicTypeNode(parent, name);
        qbtn->setGenus(Node::JS);
        qbtn->setTitle(attributes.value(QLatin1String("title")).toString());
        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value("location").toString();
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qbtn;
    }
    else if (elementName == QLatin1String("qmlpropertygroup")) {
        QmlTypeNode* qcn = static_cast<QmlTypeNode*>(parent);
        QmlPropertyGroupNode* qpgn = new QmlPropertyGroupNode(qcn, name);
        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value("location").toString();
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qpgn;
    }
    else if (elementName == QLatin1String("jspropertygroup")) {
        QmlTypeNode* qcn = static_cast<QmlTypeNode*>(parent);
        QmlPropertyGroupNode* qpgn = new QmlPropertyGroupNode(qcn, name);
        qpgn->setGenus(Node::JS);
        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value("location").toString();
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);
        node = qpgn;
    }
    else if (elementName == QLatin1String("qmlproperty")) {
        QString type = attributes.value(QLatin1String("type")).toString();
        bool attached = false;
        if (attributes.value(QLatin1String("attached")) == QLatin1String("true"))
            attached = true;
        bool readonly = false;
        if (attributes.value(QLatin1String("writable")) == QLatin1String("false"))
            readonly = true;
        QmlPropertyNode* qpn = new QmlPropertyNode(parent, name, type, attached);
        qpn->setReadOnly(readonly);
        node = qpn;
    }
    else if (elementName == QLatin1String("jsproperty")) {
        QString type = attributes.value(QLatin1String("type")).toString();
        bool attached = false;
        if (attributes.value(QLatin1String("attached")) == QLatin1String("true"))
            attached = true;
        bool readonly = false;
        if (attributes.value(QLatin1String("writable")) == QLatin1String("false"))
            readonly = true;
        QmlPropertyNode* qpn = new QmlPropertyNode(parent, name, type, attached);
        qpn->setGenus(Node::JS);
        qpn->setReadOnly(readonly);
        node = qpn;
    }
    else if ((elementName == QLatin1String("qmlmethod")) ||
             (elementName == QLatin1String("qmlsignal")) ||
             (elementName == QLatin1String("qmlsignalhandler"))) {
        Node::NodeType t = Node::QmlMethod;
        if (elementName == QLatin1String("qmlsignal"))
            t = Node::QmlSignal;
        else if (elementName == QLatin1String("qmlsignalhandler"))
            t = Node::QmlSignalHandler;
        bool attached = false;
        FunctionNode* fn = new FunctionNode(t, parent, name, attached);
        node = fn;
    }
    else if ((elementName == QLatin1String("jsmethod")) ||
             (elementName == QLatin1String("jssignal")) ||
             (elementName == QLatin1String("jssignalhandler"))) {
        Node::NodeType t = Node::QmlMethod;
        if (elementName == QLatin1String("jssignal"))
            t = Node::QmlSignal;
        else if (elementName == QLatin1String("jssignalhandler"))
            t = Node::QmlSignalHandler;
        bool attached = false;
        FunctionNode* fn = new FunctionNode(t, parent, name, attached);
        fn->setGenus(Node::JS);
        node = fn;
    }
    else if (elementName == QLatin1String("group")) {
        CollectionNode* cn = qdb_->addGroup(name);
        cn->setTitle(attributes.value(QLatin1String("title")).toString());
        cn->setSubTitle(attributes.value(QLatin1String("subtitle")).toString());
        if (attributes.value(QLatin1String("seen")) == QLatin1String("true"))
            cn->markSeen();
        node = cn;
    }
    else if (elementName == QLatin1String("module")) {
        CollectionNode* cn = qdb_->addModule(name);
        cn->setTitle(attributes.value(QLatin1String("title")).toString());
        cn->setSubTitle(attributes.value(QLatin1String("subtitle")).toString());
        if (attributes.value(QLatin1String("seen")) == QLatin1String("true"))
            cn->markSeen();
        node = cn;
    }
    else if (elementName == QLatin1String("qmlmodule")) {
        QString t = attributes.value(QLatin1String("qml-module-name")).toString();
        CollectionNode* cn = qdb_->addQmlModule(t);
        QStringList info;
        info << t << attributes.value(QLatin1String("qml-module-version")).toString();
        cn->setLogicalModuleInfo(info);
        cn->setTitle(attributes.value(QLatin1String("title")).toString());
        cn->setSubTitle(attributes.value(QLatin1String("subtitle")).toString());
        if (attributes.value(QLatin1String("seen")) == QLatin1String("true"))
            cn->markSeen();
        node = cn;
    }
    else if (elementName == QLatin1String("jsmodule")) {
        QString t = attributes.value(QLatin1String("js-module-name")).toString();
        CollectionNode* cn = qdb_->addJsModule(t);
        QStringList info;
        info << t << attributes.value(QLatin1String("js-module-version")).toString();
        cn->setLogicalModuleInfo(info);
        cn->setTitle(attributes.value(QLatin1String("title")).toString());
        cn->setSubTitle(attributes.value(QLatin1String("subtitle")).toString());
        if (attributes.value(QLatin1String("seen")) == QLatin1String("true"))
            cn->markSeen();
        node = cn;
    }
    else if (elementName == QLatin1String("page")) {
        Node::DocSubtype subtype;
        Node::PageType ptype = Node::NoPageType;
        QString attr = attributes.value(QLatin1String("subtype")).toString();
        if (attr == QLatin1String("attribution")) {
            subtype = Node::Page;
            ptype = Node::AttributionPage;
        }
        else if (attr == QLatin1String("example")) {
            subtype = Node::Example;
            ptype = Node::ExamplePage;
        }
        else if (attr == QLatin1String("header")) {
            subtype = Node::HeaderFile;
            ptype = Node::ApiPage;
        }
        else if (attr == QLatin1String("file")) {
            subtype = Node::File;
            ptype = Node::NoPageType;
        }
        else if (attr == QLatin1String("page")) {
            subtype = Node::Page;
            ptype = Node::ArticlePage;
        }
        else if (attr == QLatin1String("externalpage")) {
            subtype = Node::ExternalPage;
            ptype = Node::ArticlePage;
        }
        else
            goto done;

        DocumentNode* docNode = new DocumentNode(parent, name, subtype, ptype);
        docNode->setTitle(attributes.value(QLatin1String("title")).toString());

        if (attributes.hasAttribute(QLatin1String("location")))
            name = attributes.value(QLatin1String("location")).toString();

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + name);
        else if (!indexUrl.isNull())
            location = Location(name);

        node = docNode;

    }
    else if (elementName == QLatin1String("enum")) {
        EnumNode* enumNode = new EnumNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

        while (reader.readNextStartElement()) {
            QXmlStreamAttributes childAttributes = reader.attributes();
            if (reader.name() == QLatin1String("value")) {

                EnumItem item(childAttributes.value(QLatin1String("name")).toString(), childAttributes.value(QLatin1String("value")).toString());
                enumNode->addItem(item);
            } else if (reader.name() == QLatin1String("keyword")) {
                insertTarget(TargetRec::Keyword, childAttributes, enumNode);
            } else if (reader.name() == QLatin1String("target")) {
                insertTarget(TargetRec::Target, childAttributes, enumNode);
            }
            reader.skipCurrentElement();
        }

        node = enumNode;

        hasReadChildren = true;
    }
    else if (elementName == QLatin1String("typedef")) {
        node = new TypedefNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (elementName == QLatin1String("property")) {
        node = new PropertyNode(parent, name);

        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

    }
    else if (elementName == QLatin1String("function")) {
        FunctionNode* functionNode = new FunctionNode(parent, name);
        functionNode->setReturnType(attributes.value(QLatin1String("return")).toString());
        functionNode->setVirtualness(attributes.value(QLatin1String("virtual")).toString());
        functionNode->setMetaness(attributes.value(QLatin1String("meta")).toString());
        functionNode->setConst(attributes.value(QLatin1String("const")) == QLatin1String("true"));
        functionNode->setStatic(attributes.value(QLatin1String("static")) == QLatin1String("true"));
        functionNode->setIsDeleted(attributes.value(QLatin1String("delete")) == QLatin1String("true"));
        functionNode->setIsDefaulted(attributes.value(QLatin1String("default")) == QLatin1String("true"));
        functionNode->setFinal(attributes.value(QLatin1String("final")) == QLatin1String("true"));
        functionNode->setOverride(attributes.value(QLatin1String("override")) == QLatin1String("true"));
        int refness = attributes.value(QLatin1String("refness")).toUInt();
        if (refness == 1)
            functionNode->setRef(true);
        else if (refness == 2)
            functionNode->setRefRef(true);
        if (attributes.value(QLatin1String("overload")) == QLatin1String("true")) {
            functionNode->setOverloadFlag(true);
            functionNode->setOverloadNumber(attributes.value(QLatin1String("overload-number")).toUInt());
        }
        else {
            functionNode->setOverloadFlag(false);
            functionNode->setOverloadNumber(0);
        }
        if (attributes.hasAttribute(QLatin1String("relates"))
                && attributes.value(QLatin1String("relates")) != parent->name()) {
            relatedList_.append(
                        QPair<FunctionNode*,QString>(functionNode,
                                                     attributes.value(QLatin1String("relates")).toString()));
        }
        /*
          Note: The "signature" attribute was written to the
          index file, but it is not read back in. Is that ok?
         */

        while (reader.readNextStartElement()) {
            QXmlStreamAttributes childAttributes = reader.attributes();
            if (reader.name() == QLatin1String("parameter")) {
                // Do not use the default value for the parameter; it is not
                // required, and has been known to cause problems.
                Parameter parameter(childAttributes.value(QLatin1String("type")).toString(),
                                    childAttributes.value(QLatin1String("name")).toString(),
                                    QString()); // childAttributes.value(QLatin1String("default"))
                functionNode->addParameter(parameter);
            } else if (reader.name() == QLatin1String("keyword")) {
                insertTarget(TargetRec::Keyword, childAttributes, functionNode);
            } else if (reader.name() == QLatin1String("target")) {
                insertTarget(TargetRec::Target, childAttributes, functionNode);
            }
            reader.skipCurrentElement();
        }

        node = functionNode;
        if (!indexUrl.isEmpty())
            location =  Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");

        hasReadChildren = true;
    }
    else if (elementName == QLatin1String("variable")) {
        node = new VariableNode(parent, name);
        if (!indexUrl.isEmpty())
            location = Location(indexUrl + QLatin1Char('/') + parent->name().toLower() + ".html");
        else if (!indexUrl.isNull())
            location = Location(parent->name().toLower() + ".html");
    }
    else if (elementName == QLatin1String("keyword")) {
        insertTarget(TargetRec::Keyword, attributes, current);
        goto done;
    }
    else if (elementName == QLatin1String("target")) {
        insertTarget(TargetRec::Target, attributes, current);
        goto done;
    }
    else if (elementName == QLatin1String("contents")) {
        insertTarget(TargetRec::Contents, attributes, current);
        goto done;
    }
    else
        goto done;

    {
        QString access = attributes.value(QLatin1String("access")).toString();
        if (access == "public")
            node->setAccess(Node::Public);
        else if (access == "protected")
            node->setAccess(Node::Protected);
        else if ((access == "private") || (access == "internal"))
            node->setAccess(Node::Private);
        else
            node->setAccess(Node::Public);

        if ((elementName != QLatin1String("page")) &&
                (elementName != QLatin1String("qmlclass")) &&
                (elementName != QLatin1String("qmlbasictype")) &&
                (elementName != QLatin1String("jstype")) &&
                (elementName != QLatin1String("jsbasictype"))) {
            QString threadSafety = attributes.value(QLatin1String("threadsafety")).toString();
            if (threadSafety == QLatin1String("non-reentrant"))
                node->setThreadSafeness(Node::NonReentrant);
            else if (threadSafety == QLatin1String("reentrant"))
                node->setThreadSafeness(Node::Reentrant);
            else if (threadSafety == QLatin1String("thread safe"))
                node->setThreadSafeness(Node::ThreadSafe);
            else
                node->setThreadSafeness(Node::UnspecifiedSafeness);
        }
        else
            node->setThreadSafeness(Node::UnspecifiedSafeness);

        QString status = attributes.value(QLatin1String("status")).toString();
        if (status == QLatin1String("obsolete"))
            node->setStatus(Node::Obsolete);
        else if (status == QLatin1String("deprecated"))
            node->setStatus(Node::Obsolete);
        else if (status == QLatin1String("preliminary"))
            node->setStatus(Node::Preliminary);
        else if (status == QLatin1String("active"))
            node->setStatus(Node::Active);
        else if (status == QLatin1String("internal"))
            node->setStatus(Node::Internal);
        else
            node->setStatus(Node::Active);

        QString physicalModuleName = attributes.value(QLatin1String("module")).toString();
        if (!physicalModuleName.isEmpty())
            qdb_->addToModule(physicalModuleName, node);
        if (!href.isEmpty()) {
            node->setUrl(href);
            // Include the index URL if it exists
            if (!node->isExternalPage() && !indexUrl.isEmpty())
                node->setUrl(indexUrl + QLatin1Char('/') + href);
        }

        QString since = attributes.value(QLatin1String("since")).toString();
        if (!since.isEmpty()) {
            node->setSince(since);
        }

        QString groupsAttr = attributes.value(QLatin1String("groups")).toString();
        if (!groupsAttr.isEmpty()) {
            QStringList groupNames = groupsAttr.split(QLatin1Char(','));
            foreach (const QString &name, groupNames) {
                qdb_->addToGroup(name, node);
            }
        }

        // Create some content for the node.
        QSet<QString> emptySet;
        Location t(filePath);
        if (!filePath.isEmpty()) {
            t.setLineNo(lineNo);
            node->setLocation(t);
            location = t;
        }
        Doc doc(location, location, QString(), emptySet, emptySet); // placeholder
        node->setDoc(doc);
        node->setIndexNodeFlag();
        node->setOutputSubdirectory(project_.toLower());
        QString briefAttr = attributes.value(QLatin1String("brief")).toString();
        if (!briefAttr.isEmpty()) {
            node->setReconstitutedBrief(briefAttr);
        }

        if (!hasReadChildren) {
            bool useParent = (elementName == QLatin1String("namespace") && name.isEmpty());
            while (reader.readNextStartElement()) {
                if (useParent)
                    readIndexSection(reader, parent, indexUrl);
                else
                    readIndexSection(reader, node, indexUrl);
            }
        }
    }

  done:
    while (!reader.isEndElement()) {
        if (reader.readNext() == QXmlStreamReader::Invalid) {
            break;
        }
    }
}

void QDocIndexFiles::insertTarget(TargetRec::TargetType type,
                               const QXmlStreamAttributes &attributes,
                               Node *node)
{
    int priority;
    switch (type) {
    case TargetRec::Keyword:
        priority = 1;
        break;
    case TargetRec::Target:
        priority = 2;
        break;
    case TargetRec::Contents:
        priority = 3;
        break;
    default:
        return;
    }

    QString name = attributes.value(QLatin1String("name")).toString();
    QString title = attributes.value(QLatin1String("title")).toString();
    qdb_->insertTarget(name, title, type, node, priority);
}

/*!
  This function tries to resolve class inheritance immediately
  after the index file is read. It is not always possible to
  resolve a class inheritance at this point, because the base
  class might be in an index file that hasn't been read yet, or
  it might be in one of the header files that will be read for
  the current module. These cases will be resolved after all
  the index files and header and source files have been read,
  just prior to beginning the generate phase for the current
  module.

  I don't think this is completely correct because it always
  sets the access to public.
 */
void QDocIndexFiles::resolveIndex()
{
    QPair<ClassNode*,QString> pair;
    foreach (pair, basesList_) {
        foreach (const QString& base, pair.second.split(QLatin1Char(','))) {
            QStringList basePath = base.split(QString("::"));
            Node* n = qdb_->findClassNode(basePath);
            if (n)
                pair.first->addResolvedBaseClass(Node::Public, static_cast<ClassNode*>(n));
            else
                pair.first->addUnresolvedBaseClass(Node::Public, basePath, QString());
        }
    }
    // No longer needed.
    basesList_.clear();
}

/*
    Goes though the list of nodes that are related to other aggregates
    that were read from all index files, and tries to find the aggregate
    nodes from the database. Calls the node's setRelates() for each
    aggregate that is found in the local module (primary tree).

    This function is meant to be called before starting the doc generation,
    after all the index files are read.
 */
void QDocIndexFiles::resolveRelates()
{
    if (relatedList_.isEmpty())
        return;

    // Restrict searching only to the local (primary) tree
    QVector<Tree*> searchOrder = qdb_->searchOrder();
    qdb_->setLocalSearch();

    QPair<FunctionNode*,QString> relatedPair;
    foreach (relatedPair, relatedList_) {
        QStringList path = relatedPair.second.split("::");
        Node* n = qdb_->findRelatesNode(path);
        if (n)
            relatedPair.first->setRelates(static_cast<Aggregate*>(n));
    }
    // Restore original search order
    qdb_->setSearchOrder(searchOrder);
    relatedList_.clear();
}

/*!
  Generate the index section with the given \a writer for the \a node
  specified, returning true if an element was written; otherwise returns
  false.
 */
bool QDocIndexFiles::generateIndexSection(QXmlStreamWriter& writer,
                                          Node* node,
                                          bool generateInternalNodes)
{
    if (!gen_)
        gen_ = Generator::currentGenerator();

    Q_ASSERT(gen_);
    /*
      Don't include index nodes in a new index file. Or DITA map nodes.
     */
    if (node->isIndexNode() || node->docSubtype() == Node::DitaMap)
        return false;

    QString nodeName;
    QString logicalModuleName;
    QString logicalModuleVersion;
    QString qmlFullBaseName;
    switch (node->type()) {
    case Node::Namespace:
        nodeName = "namespace";
        break;
    case Node::Class:
        nodeName = "class";
        break;
    case Node::QmlType:
        {
            if (node->isQmlNode())
                nodeName = "qmlclass";
            else
                nodeName = "jstype";
            CollectionNode* cn = node->logicalModule();
            if (cn)
                logicalModuleName = cn->logicalModuleName();
            qmlFullBaseName = node->qmlFullBaseName();
        }
        break;
    case Node::QmlBasicType:
        if (node->isQmlNode())
            nodeName = "qmlbasictype";
        else
            nodeName = "jsbasictype";
        break;
    case Node::Document:
        nodeName = "page";
        break;
    case Node::Group:
        nodeName = "group";
        break;
    case Node::Module:
        nodeName = "module";
        break;
    case Node::QmlModule:
        if (node->isQmlNode())
            nodeName = "qmlmodule";
        else
            nodeName = "jsmodule";
        break;
    case Node::Enum:
        nodeName = "enum";
        break;
    case Node::Typedef:
        nodeName = "typedef";
        break;
    case Node::Property:
        nodeName = "property";
        break;
    case Node::Function:
        nodeName = "function";
        break;
    case Node::Variable:
        nodeName = "variable";
        break;
    case Node::QmlProperty:
        if (node->isQmlNode())
            nodeName = "qmlproperty";
        else
            nodeName = "jsProperty";
        break;
    case Node::QmlPropertyGroup:
        if (node->isQmlNode())
            nodeName = "qmlpropertygroup";
        else
            nodeName = "jspropertygroup";
        break;
    case Node::QmlSignal:
        if (node->isQmlNode())
            nodeName = "qmlsignal";
        else
            nodeName = "jssignal";
        break;
    case Node::QmlSignalHandler:
        if (node->isQmlNode())
            nodeName = "qmlsignalhandler";
        else
            nodeName = "jssignalhandler";
        break;
    case Node::QmlMethod:
        if (node->isQmlNode())
            nodeName = "qmlmethod";
        else
            nodeName = "jsmethod";
        break;
    default:
        return false;
    }

    QString access;
    switch (node->access()) {
    case Node::Public:
        access = "public";
        break;
    case Node::Protected:
        access = "protected";
        break;
    case Node::Private:
        {
            access = "private";
            bool b = generateInternalNodes;
            if (b)
                b = false;
        }
        break;
    default:
        return false;
    }

    QString objName = node->name();
    // Special case: only the root node should have an empty name.
    if (objName.isEmpty() && node != qdb_->primaryTreeRoot())
        return false;

    writer.writeStartElement(nodeName);

    QXmlStreamAttributes attributes;

    if (!node->isDocumentNode() && !node->isCollectionNode()) {
        QString threadSafety;
        switch (node->threadSafeness()) {
        case Node::NonReentrant:
            threadSafety = "non-reentrant";
            break;
        case Node::Reentrant:
            threadSafety = "reentrant";
            break;
        case Node::ThreadSafe:
            threadSafety = "thread safe";
            break;
        case Node::UnspecifiedSafeness:
        default:
            threadSafety = "unspecified";
            break;
        }
        writer.writeAttribute("threadsafety", threadSafety);
    }

    QString status;
    switch (node->status()) {
    case Node::Obsolete:
        status = "obsolete";
        break;
    case Node::Deprecated:
        status = "obsolete";
        break;
    case Node::Preliminary:
        status = "preliminary";
        break;
    case Node::Active:
        status = "active";
        break;
    case Node::Internal:
        status = "internal";
        break;
    default:
        status = "main";
        break;
    }

    writer.writeAttribute("name", objName);

    // Write module and base type info for QML/JS types
    if (node->type() == Node::QmlType || node->type() == Node::QmlModule) {
        QString baseNameAttr("qml-base-type");
        QString moduleNameAttr("qml-module-name");
        QString moduleVerAttr("qml-module-version");
        if (node->isJsNode()) {
            baseNameAttr = "js-base-type";
            moduleNameAttr = "js-module-name";
            moduleVerAttr = "js-module-version";
        }
        if (node->type() == Node::QmlModule) {
            logicalModuleName = node->logicalModuleName();
            logicalModuleVersion = node->logicalModuleVersion();
        }
        if (!logicalModuleName.isEmpty())
            writer.writeAttribute(moduleNameAttr, logicalModuleName);
        else
            writer.writeAttribute(moduleNameAttr, node->name());
        if (!logicalModuleVersion.isEmpty())
            writer.writeAttribute(moduleVerAttr, logicalModuleVersion);
        if (!qmlFullBaseName.isEmpty())
            writer.writeAttribute(baseNameAttr, qmlFullBaseName);
    }

    QString href;
    if (!node->isExternalPage()) {
        QString fullName = node->fullDocumentName();
        if (fullName != objName)
            writer.writeAttribute("fullname", fullName);
#if 0
        if (Generator::useOutputSubdirs())
            href = node->outputSubdirectory();
        if (!href.isEmpty())
            href.append(QLatin1Char('/'));
        href.append(gen_->fullDocumentLocation(node));
#endif
        href = gen_->fullDocumentLocation(node);
    }
    else
        href = node->name();
    if (node->isQmlNode() || node->isJsNode()) {
        Aggregate* p = node->parent();
        if (p) {
            if (p->isQmlPropertyGroup() || p->isJsPropertyGroup())
                p = p->parent();
            if (p && (p->isQmlType() || p->isJsType()) && p->isAbstract())
                href.clear();
        }
    }
    if (!href.isEmpty())
        writer.writeAttribute("href", href);

    writer.writeAttribute("status", status);
    if (!node->isDocumentNode() && !node->isCollectionNode()) {
        writer.writeAttribute("access", access);
        if (node->isAbstract())
            writer.writeAttribute("abstract", "true");
    }
    const Location& declLocation = node->declLocation();
    if (!declLocation.fileName().isEmpty())
        writer.writeAttribute("location", declLocation.fileName());
    if (!declLocation.filePath().isEmpty()) {
        writer.writeAttribute("filepath", declLocation.filePath());
        writer.writeAttribute("lineno", QString("%1").arg(declLocation.lineNo()));
    }

    if (!node->since().isEmpty()) {
        writer.writeAttribute("since", node->since());
    }

    QString brief = node->doc().trimmedBriefText(node->name()).toString();
    switch (node->type()) {
    case Node::Class:
        {
            // Classes contain information about their base classes.
            const ClassNode* classNode = static_cast<const ClassNode*>(node);
            QList<RelatedClass> bases = classNode->baseClasses();
            QSet<QString> baseStrings;
            foreach (const RelatedClass& related, bases) {
                ClassNode* n = related.node_;
                if (n)
                    baseStrings.insert(n->fullName());
                else if (!related.path_.isEmpty())
                    baseStrings.insert(related.path_.join(QLatin1String("::")));
            }
            if (!baseStrings.isEmpty())
            {
                QStringList baseStringsAsList = baseStrings.toList();
                baseStringsAsList.sort();
                writer.writeAttribute("bases", baseStringsAsList.join(QLatin1Char(',')));
            }
            if (!node->physicalModuleName().isEmpty())
                writer.writeAttribute("module", node->physicalModuleName());
            if (!classNode->groupNames().isEmpty())
                writer.writeAttribute("groups", classNode->groupNames().join(QLatin1Char(',')));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Namespace:
        {
            const NamespaceNode* ns = static_cast<const NamespaceNode*>(node);
            writer.writeAttribute("documented", ns->hasDoc() ? "true" : "false");
            if (!ns->physicalModuleName().isEmpty())
                writer.writeAttribute("module", ns->physicalModuleName());
            if (!ns->groupNames().isEmpty())
                writer.writeAttribute("groups", ns->groupNames().join(QLatin1Char(',')));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlType:
        {
            const QmlTypeNode* qcn = static_cast<const QmlTypeNode*>(node);
            writer.writeAttribute("title", qcn->title());
            writer.writeAttribute("fulltitle", qcn->fullTitle());
            writer.writeAttribute("subtitle", qcn->subTitle());
            if (!qcn->groupNames().isEmpty())
                writer.writeAttribute("groups", qcn->groupNames().join(QLatin1Char(',')));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Document:
        {
            /*
              Document nodes (such as manual pages) have a subtype,
              a title, and other attributes.
            */
            bool writeModuleName = false;
            const DocumentNode* docNode = static_cast<const DocumentNode*>(node);
            switch (docNode->docSubtype()) {
            case Node::Example:
                writer.writeAttribute("subtype", "example");
                writeModuleName = true;
                break;
            case Node::HeaderFile:
                writer.writeAttribute("subtype", "header");
                writeModuleName = true;
                break;
            case Node::File:
                writer.writeAttribute("subtype", "file");
                break;
            case Node::Page:
                if (docNode->pageType() == Node::AttributionPage)
                    writer.writeAttribute("subtype", "attribution");
                else
                    writer.writeAttribute("subtype", "page");

                writeModuleName = true;
                break;
            case Node::ExternalPage:
                writer.writeAttribute("subtype", "externalpage");
                break;
            default:
                break;
            }
            writer.writeAttribute("title", docNode->title());
            writer.writeAttribute("fulltitle", docNode->fullTitle());
            writer.writeAttribute("subtitle", docNode->subTitle());
            if (!node->physicalModuleName().isEmpty() && writeModuleName) {
                writer.writeAttribute("module", node->physicalModuleName());
            }
            if (!docNode->groupNames().isEmpty())
                writer.writeAttribute("groups", docNode->groupNames().join(QLatin1Char(',')));
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Group:
        {
            const CollectionNode* cn = static_cast<const CollectionNode*>(node);
            writer.writeAttribute("seen", cn->wasSeen() ? "true" : "false");
            writer.writeAttribute("title", cn->title());
            if (!cn->subTitle().isEmpty())
                writer.writeAttribute("subtitle", cn->subTitle());
            if (!cn->physicalModuleName().isEmpty())
                writer.writeAttribute("module", cn->physicalModuleName());
            if (!cn->groupNames().isEmpty())
                writer.writeAttribute("groups", cn->groupNames().join(QLatin1Char(',')));
            /*
              This is not read back in, so it probably
              shouldn't be written out in the first place.
            */
            if (!cn->members().isEmpty()) {
                QStringList names;
                foreach (const Node* member, cn->members())
                    names.append(member->name());
                writer.writeAttribute("members", names.join(QLatin1Char(',')));
            }
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Module:
        {
            const CollectionNode* cn = static_cast<const CollectionNode*>(node);
            writer.writeAttribute("seen", cn->wasSeen() ? "true" : "false");
            writer.writeAttribute("title", cn->title());
            if (!cn->subTitle().isEmpty())
                writer.writeAttribute("subtitle", cn->subTitle());
            if (!cn->physicalModuleName().isEmpty())
                writer.writeAttribute("module", cn->physicalModuleName());
            if (!cn->groupNames().isEmpty())
                writer.writeAttribute("groups", cn->groupNames().join(QLatin1Char(',')));
            /*
              This is not read back in, so it probably
              shouldn't be written out in the first place.
            */
            if (!cn->members().isEmpty()) {
                QStringList names;
                foreach (const Node* member, cn->members())
                    names.append(member->name());
                writer.writeAttribute("members", names.join(QLatin1Char(',')));
            }
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlModule:
        {
            const CollectionNode* cn = static_cast<const CollectionNode*>(node);
            writer.writeAttribute("seen", cn->wasSeen() ? "true" : "false");
            writer.writeAttribute("title", cn->title());
            if (!cn->subTitle().isEmpty())
                writer.writeAttribute("subtitle", cn->subTitle());
            if (!cn->physicalModuleName().isEmpty())
                writer.writeAttribute("module", cn->physicalModuleName());
            if (!cn->groupNames().isEmpty())
                writer.writeAttribute("groups", cn->groupNames().join(QLatin1Char(',')));
            /*
              This is not read back in, so it probably
              shouldn't be written out in the first place.
            */
            if (!cn->members().isEmpty()) {
                QStringList names;
                foreach (const Node* member, cn->members())
                    names.append(member->name());
                writer.writeAttribute("members", names.join(QLatin1Char(',')));
            }
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Function:
        {
            /*
              Function nodes contain information about the type of
              function being described.
            */
            const FunctionNode* functionNode = static_cast<const FunctionNode*>(node);
            writer.writeAttribute("virtual", functionNode->virtualness());
            writer.writeAttribute("meta", functionNode->metaness());
            writer.writeAttribute("const", functionNode->isConst()?"true":"false");
            writer.writeAttribute("static", functionNode->isStatic()?"true":"false");
            writer.writeAttribute("overload", functionNode->isOverload()?"true":"false");
            writer.writeAttribute("delete", functionNode->isDeleted() ? "true" : "false");
            writer.writeAttribute("default", functionNode->isDefaulted() ? "true" : "false");
            writer.writeAttribute("final", functionNode->isFinal() ? "true" : "false");
            writer.writeAttribute("override", functionNode->isOverride() ? "true" : "false");
            if (functionNode->isRef())
                writer.writeAttribute("refness", QString::number(1));
            else if (functionNode->isRefRef())
                writer.writeAttribute("refness", QString::number(2));
            if (functionNode->isOverload())
                writer.writeAttribute("overload-number", QString::number(functionNode->overloadNumber()));
            if (functionNode->relates()) {
                writer.writeAttribute("relates", functionNode->relates()->name());
            }
            if (functionNode->hasAssociatedProperties()) {
                QStringList associatedProperties;
                foreach (PropertyNode* pn, functionNode->associatedProperties()) {
                    associatedProperties << pn->name();
                }
                associatedProperties.sort();
                writer.writeAttribute("associated-property", associatedProperties.join(QLatin1Char(',')));
            }
            writer.writeAttribute("type", functionNode->returnType());
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);

            /*
              Note: The "signature" attribute is written to the
              index file, but it is not read back in by qdoc. However,
              we need it for the webxml generator.
            */
            QString signature = functionNode->signature(false);
            // 'const' is already part of FunctionNode::signature()
            if (functionNode->isFinal())
                signature += " final";
            if (functionNode->isOverride())
                signature += " override";
            if (functionNode->isPureVirtual())
                signature += " = 0";
            else if (functionNode->isDeleted())
                signature += " = delete";
            else if (functionNode->isDefaulted())
                signature += " = default";
            writer.writeAttribute("signature", signature);

            for (int i = 0; i < functionNode->parameters().size(); ++i) {
                Parameter parameter = functionNode->parameters()[i];
                writer.writeStartElement("parameter");
                writer.writeAttribute("type", parameter.dataType());
                writer.writeAttribute("name", parameter.name());
                writer.writeAttribute("default", parameter.defaultValue());
                writer.writeEndElement(); // parameter
            }
        }
        break;
    case Node::QmlProperty:
        {
            QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
            writer.writeAttribute("type", qpn->dataType());
            writer.writeAttribute("attached", qpn->isAttached() ? "true" : "false");
            writer.writeAttribute("writable", qpn->isWritable() ? "true" : "false");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::QmlPropertyGroup:
        {
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Property:
        {
            const PropertyNode* propertyNode = static_cast<const PropertyNode*>(node);
            writer.writeAttribute("type", propertyNode->dataType());
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
            foreach (const Node* fnNode, propertyNode->getters()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("getter");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // getter
                }
            }
            foreach (const Node* fnNode, propertyNode->setters()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("setter");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // setter
                }
            }
            foreach (const Node* fnNode, propertyNode->resetters()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("resetter");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // resetter
                }
            }
            foreach (const Node* fnNode, propertyNode->notifiers()) {
                if (fnNode) {
                    const FunctionNode* functionNode = static_cast<const FunctionNode*>(fnNode);
                    writer.writeStartElement("notifier");
                    writer.writeAttribute("name", functionNode->name());
                    writer.writeEndElement(); // notifier
                }
            }
        }
        break;
    case Node::Variable:
        {
            const VariableNode* variableNode = static_cast<const VariableNode*>(node);
            writer.writeAttribute("type", variableNode->dataType());
            writer.writeAttribute("static", variableNode->isStatic() ? "true" : "false");
            if (!brief.isEmpty())
                writer.writeAttribute("brief", brief);
        }
        break;
    case Node::Enum:
        {
            const EnumNode* enumNode = static_cast<const EnumNode*>(node);
            if (enumNode->flagsType()) {
                writer.writeAttribute("typedef",enumNode->flagsType()->fullDocumentName());
            }
            foreach (const EnumItem& item, enumNode->items()) {
                writer.writeStartElement("value");
                writer.writeAttribute("name", item.name());
                writer.writeAttribute("value", item.value());
                writer.writeEndElement(); // value
            }
        }
        break;
    case Node::Typedef:
        {
            const TypedefNode* typedefNode = static_cast<const TypedefNode*>(node);
            if (typedefNode->associatedEnum()) {
                writer.writeAttribute("enum",typedefNode->associatedEnum()->fullDocumentName());
            }
        }
        break;
    default:
        break;
    }

    /*
      For our pages, we canonicalize the target, keyword and content
      item names so that they can be used by qdoc for other sets of
      documentation.

      The reason we do this here is that we don't want to ruin
      externally composed indexes, containing non-qdoc-style target names
      when reading in indexes.

      targets and keywords are now allowed in any node, not just inner nodes.
    */

    if (node->doc().hasTargets()) {
        bool external = false;
        if (node->type() == Node::Document) {
            const DocumentNode* docNode = static_cast<const DocumentNode*>(node);
            if (docNode->docSubtype() == Node::ExternalPage)
                external = true;
        }
        foreach (const Atom* target, node->doc().targets()) {
            QString title = target->string();
            QString name =  Doc::canonicalTitle(title);
            writer.writeStartElement("target");
            if (!external)
                writer.writeAttribute("name", name);
            else
                writer.writeAttribute("name", title);
            if (name != title)
                writer.writeAttribute("title", title);
            writer.writeEndElement(); // target
        }
    }
    if (node->doc().hasKeywords()) {
        foreach (const Atom* keyword, node->doc().keywords()) {
            QString title = keyword->string();
            QString name =  Doc::canonicalTitle(title);
            writer.writeStartElement("keyword");
            writer.writeAttribute("name", name);
            if (name != title)
                writer.writeAttribute("title", title);
            writer.writeEndElement(); // keyword
        }
    }

    /*
      Some nodes have a table of contents. For these, we close
      the opening tag, create sub-elements for the items in the
      table of contents, and then add a closing tag for the
      element. Elements for all other nodes are closed in the
      opening tag.
    */
    if (node->isAggregate() || node->isCollectionNode()) {
        if (node->doc().hasTableOfContents()) {
            for (int i = 0; i < node->doc().tableOfContents().size(); ++i) {
                Atom* item = node->doc().tableOfContents()[i];
                int level = node->doc().tableOfContentsLevels()[i];
                QString title = Text::sectionHeading(item).toString();
                writer.writeStartElement("contents");
                writer.writeAttribute("name", Doc::canonicalTitle(title));
                writer.writeAttribute("title", title);
                writer.writeAttribute("level", QString::number(level));
                writer.writeEndElement(); // contents
            }
        }
    }
    return true;
}

/*!
  Generate index sections for the child nodes of the given \a node
  using the \a writer specified. If \a generateInternalNodes is true,
  nodes marked as internal will be included in the index; otherwise,
  they will be omitted.
*/
void QDocIndexFiles::generateIndexSections(QXmlStreamWriter& writer,
                                           Node* node,
                                           bool generateInternalNodes)
{
    /*
      Note that groups, modules, and QML modules are written
      after all the other nodes.
     */
    if (node->isGroup() || node->isModule() || node->isQmlModule() || node->isJsModule())
        return;

    if (generateIndexSection(writer, node, generateInternalNodes)) {
        if (node->isAggregate()) {
            const Aggregate* inner = static_cast<const Aggregate*>(node);

            NodeList cnodes = inner->childNodes();
            std::sort(cnodes.begin(), cnodes.end(), Node::nodeNameLessThan);

            foreach (Node* child, cnodes) {
                generateIndexSections(writer, child, generateInternalNodes);
            }
        }

        if (node == top) {
            /*
              We wait until the end of the index file to output the group, module,
              and QML module elements. By outputting them at the end, when we read
              the index file back in, all the group, module, and QML module member
              elements will have already been created. It is then only necessary to
              create the group, module, or QML module element and add each member to
              its member list.
            */
            const CNMap& groups = qdb_->groups();
            if (!groups.isEmpty()) {
                CNMap::ConstIterator g = groups.constBegin();
                while (g != groups.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
            }

            const CNMap& modules = qdb_->modules();
            if (!modules.isEmpty()) {
                CNMap::ConstIterator g = modules.constBegin();
                while (g != modules.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
            }

            const CNMap& qmlModules = qdb_->qmlModules();
            if (!qmlModules.isEmpty()) {
                CNMap::ConstIterator g = qmlModules.constBegin();
                while (g != qmlModules.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
            }

            const CNMap& jsModules = qdb_->jsModules();
            if (!jsModules.isEmpty()) {
                CNMap::ConstIterator g = jsModules.constBegin();
                while (g != jsModules.constEnd()) {
                    if (generateIndexSection(writer, g.value(), generateInternalNodes))
                        writer.writeEndElement();
                    ++g;
                }
            }
        }

        writer.writeEndElement();
    }
}

/*!
  Outputs an index file.
 */
void QDocIndexFiles::generateIndex(const QString& fileName,
                                   const QString& url,
                                   const QString& title,
                                   Generator* g,
                                   bool generateInternalNodes)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;

    QString msg = "Writing index file: " + fileName;
    Location::logToStdErr(msg);

    gen_ = g;
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE QDOCINDEX>");

    writer.writeStartElement("INDEX");
    writer.writeAttribute("url", url);
    writer.writeAttribute("title", title);
    writer.writeAttribute("version", qdb_->version());
    writer.writeAttribute("project", g->config()->getString(CONFIG_PROJECT));

    top = qdb_->primaryTreeRoot();
    if (!top->tree()->indexTitle().isEmpty())
        writer.writeAttribute("indexTitle", top->tree()->indexTitle());

    generateIndexSections(writer, top, generateInternalNodes);

    writer.writeEndElement(); // INDEX
    writer.writeEndElement(); // QDOCINDEX
    writer.writeEndDocument();
    file.close();
}

QT_END_NAMESPACE
