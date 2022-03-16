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

#include <qcryptographichash.h>
#include <qdebug.h>
#include <qhash.h>
#include <qmap.h>

#include "atom.h"
#include "helpprojectwriter.h"
#include "htmlgenerator.h"
#include "config.h"
#include "node.h"
#include "qdocdatabase.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

HelpProjectWriter::HelpProjectWriter(const Config &config,
                                     const QString &defaultFileName,
                                     Generator* g)
{
    reset(config, defaultFileName, g);
}

void HelpProjectWriter::reset(const Config &config,
                         const QString &defaultFileName,
                         Generator* g)
{
    projects.clear();
    gen_ = g;
    /*
      Get the pointer to the singleton for the qdoc database and
      store it locally. This replaces all the local accesses to
      the node tree, which are now private.
     */
    qdb_ = QDocDatabase::qdocDB();

    // The output directory should already have been checked by the calling
    // generator.
    outputDir = config.getOutputDir();

    QStringList names = config.getStringList(CONFIG_QHP + Config::dot + "projects");

    foreach (const QString &projectName, names) {
        HelpProject project;
        project.name = projectName;

        QString prefix = CONFIG_QHP + Config::dot + projectName + Config::dot;
        project.helpNamespace = config.getString(prefix + "namespace");
        project.virtualFolder = config.getString(prefix + "virtualFolder");
        project.version = config.getString(CONFIG_VERSION);
        project.fileName = config.getString(prefix + "file");
        if (project.fileName.isEmpty())
            project.fileName = defaultFileName;
        project.extraFiles = config.getStringSet(prefix + "extraFiles");
        project.extraFiles += config.getStringSet(CONFIG_QHP + Config::dot + "extraFiles");
        project.indexTitle = config.getString(prefix + "indexTitle");
        project.indexRoot = config.getString(prefix + "indexRoot");
        project.filterAttributes = config.getStringList(prefix + "filterAttributes").toSet();
        project.includeIndexNodes = config.getBool(prefix + "includeIndexNodes");
        QSet<QString> customFilterNames = config.subVars(prefix + "customFilters");
        foreach (const QString &filterName, customFilterNames) {
            QString name = config.getString(prefix + "customFilters" + Config::dot + filterName + Config::dot + "name");
            QSet<QString> filters = config.getStringList(prefix + "customFilters" + Config::dot + filterName + Config::dot + "filterAttributes").toSet();
            project.customFilters[name] = filters;
        }
        //customFilters = config.defs.

        foreach (QString name, config.getStringSet(prefix + "excluded"))
            project.excluded.insert(name.replace(QLatin1Char('\\'), QLatin1Char('/')));

        foreach (const QString &name, config.getStringList(prefix + "subprojects")) {
            SubProject subproject;
            QString subprefix = prefix + "subprojects" + Config::dot + name + Config::dot;
            subproject.title = config.getString(subprefix + "title");
            subproject.indexTitle = config.getString(subprefix + "indexTitle");
            subproject.sortPages = config.getBool(subprefix + "sortPages");
            subproject.type = config.getString(subprefix + "type");
            readSelectors(subproject, config.getStringList(subprefix + "selectors"));
            project.subprojects.append(subproject);
        }

        if (project.subprojects.isEmpty()) {
            SubProject subproject;
            readSelectors(subproject, config.getStringList(prefix + "selectors"));
            project.subprojects.insert(0, subproject);
        }

        projects.append(project);
    }
}

void HelpProjectWriter::readSelectors(SubProject &subproject, const QStringList &selectors)
{
    QHash<QString, Node::NodeType> typeHash;
    typeHash["namespace"] = Node::Namespace;
    typeHash["class"] = Node::Class;
    typeHash["doc"] = Node::Document;
    typeHash["fake"] = Node::Document; // Legacy alias for 'doc'
    typeHash["enum"] = Node::Enum;
    typeHash["typedef"] = Node::Typedef;
    typeHash["function"] = Node::Function;
    typeHash["property"] = Node::Property;
    typeHash["variable"] = Node::Variable;
    typeHash["group"] = Node::Group;
    typeHash["module"] = Node::Module;
    typeHash["qmlmodule"] = Node::QmlModule;
    typeHash["qmlproperty"] = Node::QmlProperty;
    typeHash["qmlsignal"] = Node::QmlSignal;
    typeHash["qmlsignalhandler"] = Node::QmlSignalHandler;
    typeHash["qmlmethod"] = Node::QmlMethod;
    typeHash["qmlpropertygroup"] = Node::QmlPropertyGroup;
    typeHash["qmlclass"] = Node::QmlType; // Legacy alias for 'qmltype'
    typeHash["qmltype"] = Node::QmlType;
    typeHash["qmlbasictype"] = Node::QmlBasicType;

    QHash<QString, Node::DocSubtype> docSubtypeHash;
    docSubtypeHash["example"] = Node::Example;
    docSubtypeHash["headerfile"] = Node::HeaderFile;
    docSubtypeHash["file"] = Node::File;
    docSubtypeHash["page"] = Node::Page;
    docSubtypeHash["externalpage"] = Node::ExternalPage;

    QSet<Node::DocSubtype> allSubTypes = QSet<Node::DocSubtype>::fromList(docSubtypeHash.values());

    foreach (const QString &selector, selectors) {
        QStringList pieces = selector.split(QLatin1Char(':'));
        if (pieces.size() == 1) {
            QString lower = selector.toLower();
            if (typeHash.contains(lower))
                subproject.selectors[typeHash[lower]] = allSubTypes;
        } else if (pieces.size() >= 2) {
            QString docType = pieces[0].toLower();
            pieces = pieces[1].split(QLatin1Char(','));
            if (typeHash.contains(docType)) {
                QSet<Node::DocSubtype> docSubtypes;
                for (int i = 0; i < pieces.size(); ++i) {
                    QString piece = pieces[i].toLower();
                    if (typeHash[docType] == Node::Group
                        || typeHash[docType] == Node::Module
                        || typeHash[docType] == Node::QmlModule) {
                        subproject.groups << piece;
                        continue;
                    }
                    if (docSubtypeHash.contains(piece))
                        docSubtypes.insert(docSubtypeHash[piece]);
                }
                subproject.selectors[typeHash[docType]] = docSubtypes;
            }
        }
    }
}

void HelpProjectWriter::addExtraFile(const QString &file)
{
    for (int i = 0; i < projects.size(); ++i)
        projects[i].extraFiles.insert(file);
}

void HelpProjectWriter::addExtraFiles(const QSet<QString> &files)
{
    for (int i = 0; i < projects.size(); ++i)
        projects[i].extraFiles.unite(files);
}

/*
    Returns a list of strings describing the keyword details for a given node.

    The first string is the human-readable name to be shown in Assistant.
    The second string is a unique identifier.
    The third string is the location of the documentation for the keyword.
*/
QStringList HelpProjectWriter::keywordDetails(const Node *node) const
{
    QStringList details;

    if (node->parent() && !node->parent()->name().isEmpty()) {
        // "name"
        if (node->type() == Node::Enum || node->type() == Node::Typedef)
            details << node->parent()->name()+"::"+node->name();
        else
            details << node->name();
        // "id"
        details << node->parent()->name()+"::"+node->name();
    }
    else if (node->isQmlType() || node->isQmlBasicType()) {
        details << node->name();
        details << "QML." + node->name();
    }
    else if (node->isJsType() || node->isJsBasicType()) {
        details << node->name();
        details << "JS." + node->name();
    }
    else if (node->isDocumentNode()) {
        const DocumentNode *fake = static_cast<const DocumentNode *>(node);
        details << fake->fullTitle();
        details << fake->fullTitle();
    }
    else {
        details << node->name();
        details << node->name();
    }
    details << gen_->fullDocumentLocation(node, false);
    return details;
}

bool HelpProjectWriter::generateSection(HelpProject &project,
                                        QXmlStreamWriter & /* writer */,
                                        const Node *node)
{
    if (!node->url().isEmpty() && !(project.includeIndexNodes && !node->url().startsWith("http")))
        return false;

    if (node->isPrivate() || node->isInternal())
        return false;

    if (node->name().isEmpty())
        return true;

    QString docPath = node->doc().location().filePath();
    if (!docPath.isEmpty() && project.excluded.contains(docPath))
        return false;

    QString objName = node->isDocumentNode() ? node->fullTitle() : node->fullDocumentName();
    // Only add nodes to the set for each subproject if they match a selector.
    // Those that match will be listed in the table of contents.

    for (int i = 0; i < project.subprojects.length(); i++) {
        SubProject subproject = project.subprojects[i];
        // No selectors: accept all nodes.
        if (subproject.selectors.isEmpty()) {
            project.subprojects[i].nodes[objName] = node;
        }
        else if (subproject.selectors.contains(node->type())) {
            // Add all group members for '[group|module|qmlmodule]:name' selector
            if (node->isGroup() || node->isModule() || node->isQmlModule()) {
                if (project.subprojects[i].groups.contains(node->name().toLower())) {
                    const CollectionNode* cn = static_cast<const CollectionNode*>(node);
                    foreach (const Node* m, cn->members()) {
                        QString memberName = m->isDocumentNode()
                                ? m->fullTitle() : m->fullDocumentName();
                        project.subprojects[i].nodes[memberName] = m;
                    }
                }
            }
            // Accept only the node types in the selectors hash.
            else if (!node->isDocumentNode())
                project.subprojects[i].nodes[objName] = node;
            else {
                // Accept only doc nodes with subtypes contained in the selector's
                // mask.
                const DocumentNode *docNode = static_cast<const DocumentNode *>(node);
                if (subproject.selectors[node->type()].contains(docNode->docSubtype()) &&
                    !docNode->isExternalPage() && !docNode->fullTitle().isEmpty()) {

                    project.subprojects[i].nodes[objName] = node;
                }
            }
        }
    }

    switch (node->type()) {

    case Node::Class:
        project.keywords.append(keywordDetails(node));
        break;
    case Node::QmlType:
    case Node::QmlBasicType:
        if (node->doc().hasKeywords()) {
            foreach (const Atom* keyword, node->doc().keywords()) {
                if (!keyword->string().isEmpty()) {
                    QStringList details;
                    details << keyword->string()
                            << keyword->string()
                            << gen_->fullDocumentLocation(node, false);
                    project.keywords.append(details);
                }
                else
                    node->doc().location().warning(tr("Bad keyword in %1").arg(gen_->fullDocumentLocation(node, false)));
            }
        }
        project.keywords.append(keywordDetails(node));
        break;

    case Node::Namespace:
        project.keywords.append(keywordDetails(node));
        break;

    case Node::Enum:
        project.keywords.append(keywordDetails(node));
    {
        const EnumNode *enumNode = static_cast<const EnumNode*>(node);
        foreach (const EnumItem &item, enumNode->items()) {
            QStringList details;

            if (enumNode->itemAccess(item.name()) == Node::Private)
                continue;

            if (!node->parent()->name().isEmpty()) {
                details << node->parent()->name()+"::"+item.name(); // "name"
                details << node->parent()->name()+"::"+item.name(); // "id"
            } else {
                details << item.name(); // "name"
                details << item.name(); // "id"
            }
            details << gen_->fullDocumentLocation(node, false);
            project.keywords.append(details);
        }
    }
        break;

    case Node::Group:
    case Node::Module:
    case Node::QmlModule:
        {
            const CollectionNode* cn = static_cast<const CollectionNode*>(node);
            if (!cn->fullTitle().isEmpty()) {
                if (cn->doc().hasKeywords()) {
                    foreach (const Atom* keyword, cn->doc().keywords()) {
                        if (!keyword->string().isEmpty()) {
                            QStringList details;
                            details << keyword->string()
                                    << keyword->string()
                                    << gen_->fullDocumentLocation(node, false);
                            project.keywords.append(details);
                        }
                        else
                            cn->doc().location().warning(
                                      tr("Bad keyword in %1").arg(gen_->fullDocumentLocation(node, false))
                                     );
                    }
                }
                project.keywords.append(keywordDetails(node));
            }
        }
        break;

    case Node::Property:
    case Node::QmlProperty:
    case Node::QmlSignal:
    case Node::QmlSignalHandler:
    case Node::QmlMethod:
        project.keywords.append(keywordDetails(node));
        break;

    case Node::Function:
    {
        const FunctionNode *funcNode = static_cast<const FunctionNode *>(node);

        // Only insert keywords for non-constructors. Constructors are covered
        // by the classes themselves.

        if (!funcNode->isSomeCtor())
            project.keywords.append(keywordDetails(node));

        // Insert member status flags into the entries for the parent
        // node of the function, or the node it is related to.
        // Since parent nodes should have already been inserted into
        // the set of files, we only need to ensure that related nodes
        // are inserted.

        if (node->relates()) {
            project.memberStatus[node->relates()].insert(node->status());
        } else if (node->parent())
            project.memberStatus[node->parent()].insert(node->status());
    }
        break;

    case Node::Typedef:
    {
        const TypedefNode *typedefNode = static_cast<const TypedefNode *>(node);
        QStringList typedefDetails = keywordDetails(node);
        const EnumNode *enumNode = typedefNode->associatedEnum();
        // Use the location of any associated enum node in preference
        // to that of the typedef.
        if (enumNode)
            typedefDetails[2] = gen_->fullDocumentLocation(enumNode, false);

        project.keywords.append(typedefDetails);
    }
        break;

    case Node::Variable:
    {
        project.keywords.append(keywordDetails(node));
    }
        break;

        // Document nodes (such as manual pages) contain subtypes, titles and other
        // attributes.
    case Node::Document: {
        const DocumentNode *docNode = static_cast<const DocumentNode*>(node);
        if (!docNode->isExternalPage() && docNode->docSubtype() != Node::Image &&
                !docNode->fullTitle().isEmpty()) {

            if (docNode->docSubtype() != Node::File) {
                if (docNode->doc().hasKeywords()) {
                    foreach (const Atom *keyword, docNode->doc().keywords()) {
                        if (!keyword->string().isEmpty()) {
                            QStringList details;
                            details << keyword->string()
                                    << keyword->string()
                                    << gen_->fullDocumentLocation(node, false);
                            project.keywords.append(details);
                        } else
                            docNode->doc().location().warning(
                                        tr("Bad keyword in %1").arg(gen_->fullDocumentLocation(node, false))
                                        );
                    }
                }
                project.keywords.append(keywordDetails(node));
            }
        }
        break;
    }
    default:
        ;
    }

    // Add all images referenced in the page to the set of files to include.
    const Atom *atom = node->doc().body().firstAtom();
    while (atom) {
        if (atom->type() == Atom::Image || atom->type() == Atom::InlineImage) {
            // Images are all placed within a single directory regardless of
            // whether the source images are in a nested directory structure.
            QStringList pieces = atom->string().split(QLatin1Char('/'));
            project.files.insert("images/" + pieces.last());
        }
        atom = atom->next();
    }

    return true;
}

void HelpProjectWriter::generateSections(HelpProject &project,
                                         QXmlStreamWriter &writer, const Node *node)
{
    /*
      Don't include index nodes in the help file. Or DITA map nodes.
     */
    if (node->isIndexNode() || node->docSubtype() == Node::DitaMap)
        return;
    if (!generateSection(project, writer, node))
        return;

    if (node->isAggregate()) {
        const Aggregate *inner = static_cast<const Aggregate *>(node);

        // Ensure that we don't visit nodes more than once.
        QSet<const Node*> childSet;
        foreach (const Node *childNode, inner->childNodes()) {
            if (childNode->isIndexNode())
                continue;

            if (childNode->isPrivate())
                continue;

            if (childNode->isDocumentNode()) {
                childSet << childNode;
            }
            else if (childNode->isQmlPropertyGroup() || childNode->isJsPropertyGroup()) {
                /*
                  Don't visit QML/JS property group nodes,
                  but visit their children, which are all
                  QML/JS property nodes.

                  This is probably not correct anymore,
                  because The Qml/Js Property Group is
                  an actual documented thing.
                 */
                const Aggregate* inner = static_cast<const Aggregate*>(childNode);
                foreach (const Node* n, inner->childNodes()) {
                    if (n->isPrivate())
                        continue;
                    childSet << n;
                }
            }
            else {
                // Store member status of children
                project.memberStatus[node].insert(childNode->status());
                if (childNode->relates()) {
                    project.memberStatus[childNode->relates()].insert(childNode->status());
                }

                if (childNode->isFunction()) {
                    const FunctionNode *funcNode = static_cast<const FunctionNode *>(childNode);
                    if (funcNode->isOverload())
                        continue;
                }
                childSet << childNode;
            }
        }
        foreach (const Node *child, childSet)
            generateSections(project, writer, child);
    }
}

void HelpProjectWriter::generate()
{
    for (int i = 0; i < projects.size(); ++i)
        generateProject(projects[i]);
}

void HelpProjectWriter::writeHashFile(QFile &file)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);

    QFile hashFile(file.fileName() + ".sha1");
    if (!hashFile.open(QFile::WriteOnly | QFile::Text))
        return;

    hashFile.write(hash.result().toHex());
    hashFile.close();
}

void HelpProjectWriter::writeSection(QXmlStreamWriter &writer, const QString &path,
                                           const QString &value)
{
    writer.writeStartElement(QStringLiteral("section"));
    writer.writeAttribute(QStringLiteral("ref"), path);
    writer.writeAttribute(QStringLiteral("title"), value);
    writer.writeEndElement(); // section
}

/*
    Write subsections for all members, compatibility members and obsolete members.
*/
void HelpProjectWriter::addMembers(HelpProject &project, QXmlStreamWriter &writer,
                                          const Node *node)
{
    QString href = gen_->fullDocumentLocation(node, false);
    href = href.left(href.size()-5);
    if (href.isEmpty())
        return;

    bool derivedClass = false;
    if (node->type() == Node::Class)
        derivedClass = !(static_cast<const ClassNode *>(node)->baseClasses().isEmpty());

    // Do not generate a 'List of all members' for namespaces or header files,
    // but always generate it for derived classes and QML classes
    if (!node->isNamespace() && !node->isHeaderFile() &&
        (derivedClass || node->isQmlType() || node->isJsType() ||
         !project.memberStatus[node].isEmpty())) {
        QString membersPath = href + QStringLiteral("-members.html");
        writeSection(writer, membersPath, tr("List of all members"));
    }
    if (project.memberStatus[node].contains(Node::Obsolete)) {
        QString obsoletePath = href + QStringLiteral("-obsolete.html");
        writeSection(writer, obsoletePath, tr("Obsolete members"));
    }
}

void HelpProjectWriter::writeNode(HelpProject &project, QXmlStreamWriter &writer,
                                  const Node *node)
{
    QString href = gen_->fullDocumentLocation(node, false);
    QString objName = node->name();

    switch (node->type()) {

    case Node::Class:
        writer.writeStartElement("section");
        writer.writeAttribute("ref", href);
        if (node->parent() && !node->parent()->name().isEmpty())
            writer.writeAttribute("title", tr("%1::%2 Class Reference").arg(node->parent()->name()).arg(objName));
        else
            writer.writeAttribute("title", tr("%1 Class Reference").arg(objName));

        addMembers(project, writer, node);
        writer.writeEndElement(); // section
        break;

    case Node::Namespace:
        writeSection(writer, href, objName);
        break;

    case Node::QmlType:
        writer.writeStartElement("section");
        writer.writeAttribute("ref", href);
        writer.writeAttribute("title", tr("%1 Type Reference").arg(node->fullTitle()));
        addMembers(project, writer, node);
        writer.writeEndElement(); // section
        break;

    case Node::Document: {
        // Document nodes (such as manual pages) contain subtypes, titles and other
        // attributes.
        const DocumentNode *docNode = static_cast<const DocumentNode*>(node);

        writer.writeStartElement("section");
        writer.writeAttribute("ref", href);
        writer.writeAttribute("title", docNode->fullTitle());

        if (docNode->docSubtype() == Node::HeaderFile)
            addMembers(project, writer, node);

        writer.writeEndElement(); // section
    }
        break;
    case Node::Group:
    case Node::Module:
    case Node::QmlModule:
        {
            const CollectionNode* cn = static_cast<const CollectionNode*>(node);
            writer.writeStartElement("section");
            writer.writeAttribute("ref", href);
            writer.writeAttribute("title", cn->fullTitle());
            writer.writeEndElement(); // section
        }
        break;
    default:
        ;
    }
}

void HelpProjectWriter::generateProject(HelpProject &project)
{
    const Node *rootNode;

    // Restrict searching only to the local (primary) tree
    QVector<Tree*> searchOrder = qdb_->searchOrder();
    qdb_->setLocalSearch();

    if (!project.indexRoot.isEmpty())
        rootNode = qdb_->findDocumentNodeByTitle(project.indexRoot);
    else
        rootNode = qdb_->primaryTreeRoot();

    if (!rootNode)
        return;

    project.files.clear();
    project.keywords.clear();

    QFile file(outputDir + QDir::separator() + project.fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("QtHelpProject");
    writer.writeAttribute("version", "1.0");

    // Write metaData, virtualFolder and namespace elements.
    writer.writeTextElement("namespace", project.helpNamespace);
    writer.writeTextElement("virtualFolder", project.virtualFolder);
    writer.writeStartElement("metaData");
    writer.writeAttribute("name", "version");
    writer.writeAttribute("value", project.version);
    writer.writeEndElement();

    // Write customFilter elements.
    QHash<QString, QSet<QString> >::ConstIterator it;
    for (it = project.customFilters.constBegin(); it != project.customFilters.constEnd(); ++it) {
        writer.writeStartElement("customFilter");
        writer.writeAttribute("name", it.key());
        QStringList sortedAttributes = it.value().toList();
        sortedAttributes.sort();
        foreach (const QString &filter, sortedAttributes)
            writer.writeTextElement("filterAttribute", filter);
        writer.writeEndElement(); // customFilter
    }

    // Start the filterSection.
    writer.writeStartElement("filterSection");

    // Write filterAttribute elements.
    QStringList sortedFilterAttributes = project.filterAttributes.toList();
    sortedFilterAttributes.sort();
    foreach (const QString &filterName, sortedFilterAttributes)
        writer.writeTextElement("filterAttribute", filterName);

    writer.writeStartElement("toc");
    writer.writeStartElement("section");
    const Node* node = qdb_->findDocumentNodeByTitle(project.indexTitle);
    if (node == 0)
        node = qdb_->findNodeByNameAndType(QStringList("index.html"), Node::Document);
    QString indexPath;
    if (node)
        indexPath = gen_->fullDocumentLocation(node, false);
    else
        indexPath = "index.html";
    writer.writeAttribute("ref", indexPath);
    writer.writeAttribute("title", project.indexTitle);

    generateSections(project, writer, rootNode);

    for (int i = 0; i < project.subprojects.length(); i++) {
        SubProject subproject = project.subprojects[i];

        if (subproject.type == QLatin1String("manual")) {

            const Node *indexPage = qdb_->findNodeForTarget(subproject.indexTitle, 0);
            if (indexPage) {
                Text indexBody = indexPage->doc().body();
                const Atom *atom = indexBody.firstAtom();
                QStack<int> sectionStack;
                bool inItem = false;

                while (atom) {
                    switch (atom->type()) {
                    case Atom::ListLeft:
                        sectionStack.push(0);
                        break;
                    case Atom::ListRight:
                        if (sectionStack.pop() > 0)
                            writer.writeEndElement(); // section
                        break;
                    case Atom::ListItemLeft:
                        inItem = true;
                        break;
                    case Atom::ListItemRight:
                        inItem = false;
                        break;
                    case Atom::Link:
                        if (inItem) {
                            if (sectionStack.top() > 0)
                                writer.writeEndElement(); // section

                            const Node *page = qdb_->findNodeForTarget(atom->string(), 0);
                            writer.writeStartElement("section");
                            QString indexPath = gen_->fullDocumentLocation(page, false);
                            writer.writeAttribute("ref", indexPath);
                            QString title = atom->string();
                            if (atom->next() && atom->next()->string() == ATOM_FORMATTING_LINK)
                                if (atom->next()->next())
                                    title = atom->next()->next()->string();
                            writer.writeAttribute("title", title);

                            sectionStack.top() += 1;
                        }
                        break;
                    default:
                        ;
                    }

                    if (atom == indexBody.lastAtom())
                        break;
                    atom = atom->next();
                }
            } else
                rootNode->doc().location().warning(
                            tr("Failed to find index: %1").arg(subproject.indexTitle)
                            );

        } else {

            writer.writeStartElement("section");
            QString indexPath = gen_->fullDocumentLocation(qdb_->findNodeForTarget(subproject.indexTitle, 0),
                                                               false);
            writer.writeAttribute("ref", indexPath);
            writer.writeAttribute("title", subproject.title);

            if (subproject.sortPages) {
                QStringList titles = subproject.nodes.keys();
                titles.sort();
                foreach (const QString &title, titles) {
                    writeNode(project, writer, subproject.nodes[title]);
                }
            } else {
                // Find a contents node and navigate from there, using the NextLink values.
                QSet<QString> visited;
                bool contentsFound = false;
                foreach (const Node *node, subproject.nodes) {
                    QString nextTitle = node->links().value(Node::NextLink).first;
                    if (!nextTitle.isEmpty() &&
                            node->links().value(Node::ContentsLink).first.isEmpty()) {

                        const Node *nextPage = qdb_->findNodeForTarget(nextTitle, 0);

                        // Write the contents node.
                        writeNode(project, writer, node);
                        contentsFound = true;

                        while (nextPage) {
                            writeNode(project, writer, nextPage);
                            nextTitle = nextPage->links().value(Node::NextLink).first;
                            if (nextTitle.isEmpty() || visited.contains(nextTitle))
                                break;
                            nextPage = qdb_->findNodeForTarget(nextTitle, 0);
                            visited.insert(nextTitle);
                        }
                        break;
                    }
                }
                // No contents/nextpage links found, write all nodes unsorted
                if (!contentsFound) {
                    QList<const Node*> subnodes = subproject.nodes.values();

                    std::sort(subnodes.begin(), subnodes.end(), Node::nodeNameLessThan);

                    foreach (const Node *node, subnodes)
                        writeNode(project, writer, node);
                }
            }

            writer.writeEndElement(); // section
        }
    }

    // Restore original search order
    qdb_->setSearchOrder(searchOrder);

    writer.writeEndElement(); // section
    writer.writeEndElement(); // toc

    writer.writeStartElement("keywords");
    std::sort(project.keywords.begin(), project.keywords.end());
    foreach (const QStringList &details, project.keywords) {
        writer.writeStartElement("keyword");
        writer.writeAttribute("name", details[0]);
        writer.writeAttribute("id", details[1]);
        writer.writeAttribute("ref", details[2]);
        writer.writeEndElement(); //keyword
    }
    writer.writeEndElement(); // keywords

    writer.writeStartElement("files");

    // The list of files to write is the union of generated files and
    // other files (images and extras) included in the project
    QSet<QString> files = QSet<QString>::fromList(gen_->outputFileNames());
    files.unite(project.files);
    files.unite(project.extraFiles);
    QStringList sortedFiles = files.toList();
    sortedFiles.sort();
    foreach (const QString &usedFile, sortedFiles) {
        if (!usedFile.isEmpty())
            writer.writeTextElement("file", usedFile);
    }
    writer.writeEndElement(); // files

    writer.writeEndElement(); // filterSection
    writer.writeEndElement(); // QtHelpProject
    writer.writeEndDocument();
    writeHashFile(file);
    file.close();
}

QT_END_NAMESPACE
