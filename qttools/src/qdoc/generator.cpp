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
  generator.cpp
*/
#include <qdir.h>
#include <qdebug.h>
#include "codemarker.h"
#include "config.h"
#include "doc.h"
#include "editdistance.h"
#include "generator.h"
#include "loggingcategory.h"
#include "openedlist.h"
#include "quoter.h"
#include "separator.h"
#include "tokenizer.h"
#include "qdocdatabase.h"

QT_BEGIN_NAMESPACE

Generator* Generator::currentGenerator_;
QStringList Generator::exampleDirs;
QStringList Generator::exampleImgExts;
QMap<QString, QMap<QString, QString> > Generator::fmtLeftMaps;
QMap<QString, QMap<QString, QString> > Generator::fmtRightMaps;
QList<Generator *> Generator::generators;
QStringList Generator::imageDirs;
QStringList Generator::imageFiles;
QMap<QString, QStringList> Generator::imgFileExts;
QString Generator::outDir_;
QString Generator::outSubdir_;
QStringList Generator::outFileNames_;
QSet<QString> Generator::outputFormats;
QHash<QString, QString> Generator::outputPrefixes;
QHash<QString, QString> Generator::outputSuffixes;
QString Generator::project_;
QStringList Generator::scriptDirs;
QStringList Generator::scriptFiles;
QStringList Generator::styleDirs;
QStringList Generator::styleFiles;
bool Generator::noLinkErrors_ = false;
bool Generator::autolinkErrors_ = false;
bool Generator::redirectDocumentationToDevNull_ = false;
Generator::QDocPass Generator::qdocPass_ = Generator::Neither;
bool Generator::qdocSingleExec_ = false;
bool Generator::qdocWriteQaPages_ = false;
bool Generator::useOutputSubdirs_ = true;
QmlTypeNode* Generator::qmlTypeContext_ = 0;

static QRegExp tag("</?@[^>]*>");
static QLatin1String amp("&amp;");
static QLatin1String gt("&gt;");
static QLatin1String lt("&lt;");
static QLatin1String quot("&quot;");

static inline void setDebugEnabled(bool v)
{
    const_cast<QLoggingCategory &>(lcQdoc()).setEnabled(QtDebugMsg, v);
}

void Generator::startDebugging(const QString& message)
{
    setDebugEnabled(true);
    qCDebug(lcQdoc, "START DEBUGGING: %s", qPrintable(message));
}

void Generator::stopDebugging(const QString& message)
{
    qCDebug(lcQdoc, "STOP DEBUGGING: %s", qPrintable(message));
    setDebugEnabled(false);
}

bool Generator::debugging()
{
    return lcQdoc().isEnabled(QtDebugMsg);
}

/*!
  Constructs the generator base class. Prepends the newly
  constructed generator to the list of output generators.
  Sets a pointer to the QDoc database singleton, which is
  available to the generator subclasses.
 */
Generator::Generator()
    : inLink_(false),
      inContents_(false),
      inSectionHeading_(false),
      inTableHeader_(false),
      threeColumnEnumValueTable_(true),
      showInternal_(false),
      singleExec_(false),
      numTableRows_(0)
{
    qdb_ = QDocDatabase::qdocDB();
    generators.prepend(this);
}

/*!
  Destroys the generator after removing it from the list of
  output generators.
 */
Generator::~Generator()
{
    generators.removeAll(this);
}

void Generator::appendFullName(Text& text,
                               const Node *apparentNode,
                               const Node *relative,
                               const Node *actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    text << Atom(Atom::LinkNode, CodeMarker::stringForNode(actualNode))
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
         << Atom(Atom::String, apparentNode->plainFullName(relative))
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
}

void Generator::appendFullName(Text& text,
                               const Node *apparentNode,
                               const QString& fullName,
                               const Node *actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    text << Atom(Atom::LinkNode, CodeMarker::stringForNode(actualNode))
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
         << Atom(Atom::String, fullName)
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
}

void Generator::appendFullNames(Text& text, const NodeList& nodes, const Node* relative)
{
    NodeList::ConstIterator n = nodes.constBegin();
    int index = 0;
    while (n != nodes.constEnd()) {
        appendFullName(text,*n,relative);
        text << comma(index++,nodes.count());
        ++n;
    }
}

/*!
  Append the signature for the function named in \a node to
  \a text, so that is is a link to the documentation for that
  function.
 */
void Generator::appendSignature(Text& text, const Node* node)
{
    text << Atom(Atom::LinkNode, CodeMarker::stringForNode(node))
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
         << Atom(Atom::String, node->signature(false, true))
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
}

/*!
  Generate a bullet list of function signatures. The function
  nodes are in \a nodes. It uses the \a relative node and the
  \a marker for the generation.
 */
void Generator::signatureList(const NodeList& nodes, const Node* relative, CodeMarker* marker)
{
    Text text;
    int count = 0;
    text << Atom(Atom::ListLeft, QString("bullet"));
    NodeList::ConstIterator n = nodes.constBegin();
    while (n != nodes.constEnd()) {
        text << Atom(Atom::ListItemNumber, QString::number(++count));
        text << Atom(Atom::ListItemLeft, QString("bullet"));
        appendSignature(text, *n);
        text << Atom(Atom::ListItemRight, QString("bullet"));
        ++n;
    }
    text << Atom(Atom::ListRight, QString("bullet"));
    generateText(text, relative, marker);
}

int Generator::appendSortedNames(Text& text, const ClassNode* cn, const QList<RelatedClass>& rc)
{
    QList<RelatedClass>::ConstIterator r;
    QMap<QString,Text> classMap;
    int index = 0;

    r = rc.constBegin();
    while (r != rc.constEnd()) {
        ClassNode* rcn = (*r).node_;
        if (rcn && rcn->access() == Node::Public &&
              rcn->status() != Node::Internal &&
              !rcn->doc().isEmpty()) {
            Text className;
            appendFullName(className, rcn, cn);
            classMap[className.toString().toLower()] = className;
        }
        ++r;
    }

    QStringList classNames = classMap.keys();
    classNames.sort();

    foreach (const QString &className, classNames) {
        text << classMap[className];
        text << comma(index++, classNames.count());
    }
    return index;
}

int Generator::appendSortedQmlNames(Text& text, const Node* base, const NodeList& subs)
{
    QMap<QString,Text> classMap;
    int index = 0;

    for (int i = 0; i < subs.size(); ++i) {
        Text t;
        if (!base->isQtQuickNode() || !subs[i]->isQtQuickNode() ||
                (base->logicalModuleName() == subs[i]->logicalModuleName())) {
            appendFullName(t, subs[i], base);
            classMap[t.toString().toLower()] = t;
        }
    }

    QStringList names = classMap.keys();
    names.sort();

    foreach (const QString &name, names) {
        text << classMap[name];
        text << comma(index++, names.count());
    }
    return index;
}

/*!
  For debugging qdoc.
 */
void Generator::writeOutFileNames()
{
    QFile files("outputlist.txt");
    if (!files.open(QFile::WriteOnly))
        return;
    QTextStream filesout(&files);
    foreach (const QString &file, outFileNames_) {
        filesout << file << "\n";
    }
}

/*!
  Creates the file named \a fileName in the output directory.
  Attaches a QTextStream to the created file, which is written
  to all over the place using out().
 */
void Generator::beginSubPage(const Node* node, const QString& fileName)
{
    QString path = outputDir() + QLatin1Char('/');
    if (Generator::useOutputSubdirs() && !node->outputSubdirectory().isEmpty() &&
        !outputDir().endsWith(node->outputSubdirectory()))
        path += node->outputSubdirectory() + QLatin1Char('/');
    path += fileName;

    QFile* outFile = new QFile(redirectDocumentationToDevNull_ ? QStringLiteral("/dev/null") : path);
    if (!redirectDocumentationToDevNull_ && outFile->exists())
        node->location().error(tr("Output file already exists; overwriting %1").arg(outFile->fileName()));
    if (!outFile->open(QFile::WriteOnly))
        node->location().fatal(tr("Cannot open output file '%1'").arg(outFile->fileName()));
    qCDebug(lcQdoc, "Writing: %s", qPrintable(path));
    outFileNames_ << fileName;
    QTextStream* out = new QTextStream(outFile);

#ifndef QT_NO_TEXTCODEC
    if (outputCodec)
        out->setCodec(outputCodec);
#endif
    outStreamStack.push(out);
    const_cast<Node*>(node)->setOutputFileName(fileName);
}

/*!
  Flush the text stream associated with the subpage, and
  then pop it off the text stream stack and delete it.
  This terminates output of the subpage.
 */
void Generator::endSubPage()
{
    outStreamStack.top()->flush();
    delete outStreamStack.top()->device();
    delete outStreamStack.pop();
}

QString Generator::fileBase(const Node *node) const
{
    if (node->relates())
        node = node->relates();
    else if (!node->isAggregate() && !node->isCollectionNode())
        node = node->parent();
    if (node->type() == Node::QmlPropertyGroup) {
        node = node->parent();
    }

    if (node->hasFileNameBase())
        return node->fileNameBase();

    QString base;
    if (node->isDocumentNode()) {
        base = node->name();
        if (base.endsWith(".html") && !node->isExampleFile())
            base.truncate(base.length() - 5);

        if (node->isExample() || node->isExampleFile()) {
            QString modPrefix(node->physicalModuleName());
            if (modPrefix.isEmpty()) {
                modPrefix = project_;
            }
            base.prepend(modPrefix.toLower() + QLatin1Char('-'));
        }
        if (node->isExample()) {
            base.append(QLatin1String("-example"));
        }
    }
    else if (node->isQmlType() || node->isQmlBasicType() ||
             node->isJsType() || node->isJsBasicType()) {
        base = node->name();
        /*
          To avoid file name conflicts in the html directory,
          we prepend a prefix (by default, "qml-") and an optional suffix
          to the file name. The suffix, if one exists, is appended to the
          module name.
        */
        if (!node->logicalModuleName().isEmpty()) {
            base.prepend(node->logicalModuleName()
                         + outputSuffix(node)
                         + QLatin1Char('-'));
        }
        base.prepend(outputPrefix(node));
    }
    else if (node->isCollectionNode()) {
        base = node->name() + outputSuffix(node);
        if (base.endsWith(".html"))
            base.truncate(base.length() - 5);

        if (node->isQmlModule()) {
            base.append("-qmlmodule");
        }
        else if (node->isJsModule()) {
            base.append("-jsmodule");
        }
        else if (node->isModule()) {
            base.append("-module");
        }
        // Why not add "-group" for group pages?
    }
    else {
        const Node *p = node;
        forever {
            const Node *pp = p->parent();
            base.prepend(p->name());
            if (!pp || pp->name().isEmpty() || pp->isDocumentNode())
                break;
            base.prepend(QLatin1Char('-'));
            p = pp;
        }
        if (node->isNamespace() && !node->name().isEmpty()) {
            const NamespaceNode* ns = static_cast<const NamespaceNode*>(node);
            if (!ns->isDocumentedHere()) {
                base.append(QLatin1String("-sub-"));
                base.append(ns->tree()->camelCaseModuleName());
            }
        }
    }

    // the code below is effectively equivalent to:
    //   base.replace(QRegExp("[^A-Za-z0-9]+"), " ");
    //   base = base.trimmed();
    //   base.replace(QLatin1Char(' '), QLatin1Char('-'));
    //   base = base.toLower();
    // as this function accounted for ~8% of total running time
    // we optimize a bit...

    QString res;
    // +5 prevents realloc in fileName() below
    res.reserve(base.size() + 5);
    bool begun = false;
    for (int i = 0; i != base.size(); ++i) {
        QChar c = base.at(i);
        uint u = c.unicode();
        if (u >= 'A' && u <= 'Z')
            u += 'a' - 'A';
        if ((u >= 'a' &&  u <= 'z') || (u >= '0' && u <= '9')) {
            res += QLatin1Char(u);
            begun = true;
        }
        else if (begun) {
            res += QLatin1Char('-');
            begun = false;
        }
    }
    while (res.endsWith(QLatin1Char('-')))
        res.chop(1);
    Node* n = const_cast<Node*>(node);
    n->setFileNameBase(res);
    return res;
}

/*!
  If the \a node has a URL, return the URL as the file name.
  Otherwise, construct the file name from the fileBase() and
  either the provided \a extension or fileExtension(), and
  return the constructed name.
 */
QString Generator::fileName(const Node* node, const QString &extension) const
{
    if (!node->url().isEmpty())
        return node->url();

    QString name = fileBase(node) + QLatin1Char('.');
    return extension.isNull() ? name + fileExtension() : name + extension;
}

QString Generator::cleanRef(const QString& ref)
{
    QString clean;

    if (ref.isEmpty())
        return clean;

    clean.reserve(ref.size() + 20);
    const QChar c = ref[0];
    const uint u = c.unicode();

    if ((u >= 'a' && u <= 'z') ||
            (u >= 'A' && u <= 'Z') ||
            (u >= '0' && u <= '9')) {
        clean += c;
    } else if (u == '~') {
        clean += "dtor.";
    } else if (u == '_') {
        clean += "underscore.";
    } else {
        clean += QLatin1Char('A');
    }

    for (int i = 1; i < (int) ref.length(); i++) {
        const QChar c = ref[i];
        const uint u = c.unicode();
        if ((u >= 'a' && u <= 'z') ||
                (u >= 'A' && u <= 'Z') ||
                (u >= '0' && u <= '9') || u == '-' ||
                u == '_' || u == ':' || u == '.') {
            clean += c;
        } else if (c.isSpace()) {
            clean += QLatin1Char('-');
        } else if (u == '!') {
            clean += "-not";
        } else if (u == '&') {
            clean += "-and";
        } else if (u == '<') {
            clean += "-lt";
        } else if (u == '=') {
            clean += "-eq";
        } else if (u == '>') {
            clean += "-gt";
        } else if (u == '#') {
            clean += QLatin1Char('#');
        } else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

QMap<QString, QString>& Generator::formattingLeftMap()
{
    return fmtLeftMaps[format()];
}

QMap<QString, QString>& Generator::formattingRightMap()
{
    return fmtRightMaps[format()];
}

/*!
  Returns the full document location.
 */
QString Generator::fullDocumentLocation(const Node *node, bool useSubdir)
{
    if (!node)
        return QString();
    if (!node->url().isEmpty())
        return node->url();

    QString parentName;
    QString anchorRef;
    QString fdl;

    /*
      If the useSubdir parameter is set, then the output is
      being sent to subdirectories of the output directory.
      Prepend the subdirectory name + '/' to the result.
     */
    if (useSubdir) {
        fdl = node->outputSubdirectory();
        if (!fdl.isEmpty())
            fdl.append(QLatin1Char('/'));
    }
    if (node->isNamespace()) {

        // The root namespace has no name - check for this before creating
        // an attribute containing the location of any documentation.

        if (!fileBase(node).isEmpty())
            parentName = fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
        else
            return QString();
    }
    else if (node->isQmlType() || node->isQmlBasicType() ||
             node->isJsType() || node->isJsBasicType()) {
        QString fb = fileBase(node);
        if (fb.startsWith(outputPrefix(node)))
            return fb + QLatin1Char('.') + currentGenerator()->fileExtension();
        else {
            QString mq;
            if (!node->logicalModuleName().isEmpty()) {
                mq = node->logicalModuleName().replace(QChar('.'),QChar('-'));
                mq = mq.toLower() + QLatin1Char('-');
            }
            return fdl + outputPrefix(node) + mq + fileBase(node) +
                QLatin1Char('.') + currentGenerator()->fileExtension();
        }
    }
    else if (node->isDocumentNode() || node->isCollectionNode()) {
        parentName = fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
    }
    else if (fileBase(node).isEmpty())
        return QString();

    Node *parentNode = 0;

    if ((parentNode = node->relates())) {
        parentName = fullDocumentLocation(node->relates());
    }
    else if ((parentNode = node->parent())) {
        if (parentNode->isQmlPropertyGroup() || parentNode->isJsPropertyGroup()) {
            parentNode = parentNode->parent();
            parentName = fullDocumentLocation(parentNode);
        }
        else {
            parentName = fullDocumentLocation(node->parent());
        }
    }

    switch (node->type()) {
    case Node::Class:
    case Node::Namespace:
        parentName = fileBase(node) + QLatin1Char('.') + currentGenerator()->fileExtension();
        break;
    case Node::Function:
    {
        const FunctionNode *fn = static_cast<const FunctionNode *>(node);

        if (fn->isDtor())
            anchorRef = "#dtor." + fn->name().mid(1);

        else if (fn->hasOneAssociatedProperty() && fn->doc().isEmpty())
            return fullDocumentLocation(fn->firstAssociatedProperty());

        else if (fn->overloadNumber() > 0)
            anchorRef = QLatin1Char('#') + cleanRef(fn->name())
                    + QLatin1Char('-') + QString::number(fn->overloadNumber());
        else
            anchorRef = QLatin1Char('#') + cleanRef(fn->name());
        break;
    }
    /*
      Use node->name() instead of fileBase(node) as
      the latter returns the name in lower-case. For
      HTML anchors, we need to preserve the case.
    */
    case Node::Enum:
        anchorRef = QLatin1Char('#') + node->name() + "-enum";
        break;
    case Node::Typedef:
    {
        const TypedefNode *tdef = static_cast<const TypedefNode *>(node);
        if (tdef->associatedEnum()) {
            return fullDocumentLocation(tdef->associatedEnum());
        }
        anchorRef = QLatin1Char('#') + node->name() + "-typedef";
        break;
    }
    case Node::Property:
        anchorRef = QLatin1Char('#') + node->name() + "-prop";
        break;
    case Node::QmlProperty:
        if (node->isAttached())
            anchorRef = QLatin1Char('#') + node->name() + "-attached-prop";
        else
            anchorRef = QLatin1Char('#') + node->name() + "-prop";
        break;
    case Node::QmlSignal:
        anchorRef = QLatin1Char('#') + node->name() + "-signal";
        break;
    case Node::QmlSignalHandler:
        anchorRef = QLatin1Char('#') + node->name() + "-signal-handler";
        break;
    case Node::QmlMethod:
        anchorRef = QLatin1Char('#') + node->name() + "-method";
        break;
    case Node::Variable:
        anchorRef = QLatin1Char('#') + node->name() + "-var";
        break;
    case Node::QmlType:
    case Node::Document:
    case Node::Group:
    case Node::Module:
    case Node::QmlModule:
    {
        parentName = fileBase(node);
        parentName.replace(QLatin1Char('/'), QLatin1Char('-')).replace(QLatin1Char('.'), QLatin1Char('-'));
        parentName += QLatin1Char('.') + currentGenerator()->fileExtension();
    }
        break;
    default:
        break;
    }

    if (!node->isClass() && !node->isNamespace()) {
        if (node->status() == Node::Obsolete)
            parentName.replace(QLatin1Char('.') + currentGenerator()->fileExtension(),
                               "-obsolete." + currentGenerator()->fileExtension());
    }

    return fdl + parentName.toLower() + anchorRef;
}

void Generator::generateAlsoList(const Node *node, CodeMarker *marker)
{
    QList<Text> alsoList = node->doc().alsoList();
    supplementAlsoList(node, alsoList);

    if (!alsoList.isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "See also "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        for (int i = 0; i < alsoList.size(); ++i)
            text << alsoList.at(i) << separator(i, alsoList.size());

        text << Atom::ParaRight;
        generateText(text, node, marker);
    }
}

int Generator::generateAtom(const Atom * /* atom */,
                            const Node * /* relative */,
                            CodeMarker * /* marker */)
{
    return 0;
}

const Atom *Generator::generateAtomList(const Atom *atom,
                                        const Node *relative,
                                        CodeMarker *marker,
                                        bool generate,
                                        int &numAtoms)
{
    while (atom) {
        if (atom->type() == Atom::FormatIf) {
            int numAtoms0 = numAtoms;
            bool rightFormat = canHandleFormat(atom->string());
            atom = generateAtomList(atom->next(),
                                    relative,
                                    marker,
                                    generate && rightFormat,
                                    numAtoms);
            if (!atom)
                return 0;

            if (atom->type() == Atom::FormatElse) {
                ++numAtoms;
                atom = generateAtomList(atom->next(),
                                        relative,
                                        marker,
                                        generate && !rightFormat,
                                        numAtoms);
                if (!atom)
                    return 0;
            }

            if (atom->type() == Atom::FormatEndif) {
                if (generate && numAtoms0 == numAtoms) {
                    relative->location().warning(tr("Output format %1 not handled %2")
                                                 .arg(format()).arg(outFileName()));
                    Atom unhandledFormatAtom(Atom::UnhandledFormat, format());
                    generateAtomList(&unhandledFormatAtom,
                                     relative,
                                     marker,
                                     generate,
                                     numAtoms);
                }
                atom = atom->next();
            }
        }
        else if (atom->type() == Atom::FormatElse ||
                 atom->type() == Atom::FormatEndif) {
            return atom;
        }
        else {
            int n = 1;
            if (generate) {
                n += generateAtom(atom, relative, marker);
                numAtoms += n;
            }
            while (n-- > 0)
                atom = atom->next();
        }
    }
    return 0;
}

/*!
  Generate the body of the documentation from the qdoc comment
  found with the entity represented by the \a node.
 */
void Generator::generateBody(const Node *node, CodeMarker *marker)
{
    bool quiet = false;

    if (node->type() == Node::Document) {
        const DocumentNode *dn = static_cast<const DocumentNode *>(node);
        if ((dn->docSubtype() == Node::File) || (dn->docSubtype() == Node::Image)) {
            quiet = true;
        }
    }
    if (!node->hasDoc() && !node->hasSharedDoc()) {
        /*
          Test for special function, like a destructor or copy constructor,
          that has no documentation.
        */
        if (node->type() == Node::Function) {
            const FunctionNode* func = static_cast<const FunctionNode*>(node);
            if (func->isDtor()) {
                Text text;
                text << "Destroys the instance of ";
                text << func->parent()->name() << ".";
                if (func->isVirtual())
                    text << " The destructor is virtual.";
                out() << "<p>";
                generateText(text, node, marker);
                out() << "</p>";
            }
            else if (func->isCtor()) {
                Text text;
                text << "Default constructs an instance of ";
                text << func->parent()->name() << ".";
                out() << "<p>";
                generateText(text, node, marker);
                out() << "</p>";
            }
            else if (func->isCCtor()) {
                Text text;
                text << "Copy constructor.";
                out() << "<p>";
                generateText(text, node, marker);
                out() << "</p>";
            }
            else if (func->isMCtor()) {
                Text text;
                text << "Move-copy constructor.";
                out() << "<p>";
                generateText(text, node, marker);
                out() << "</p>";
            }
            else if (func->isCAssign()) {
                Text text;
                text << "Copy-assignment operator.";
                out() << "<p>";
                generateText(text, node, marker);
                out() << "</p>";
            }
            else if (func->isMAssign()) {
                Text text;
                text << "Move-assignment operator.";
                out() << "<p>";
                generateText(text, node, marker);
                out() << "</p>";
            }
            else if (!node->isWrapper() && !quiet && !node->isReimplemented()) {
                if (!func->isIgnored()) // undocumented functions added by Q_OBJECT
                    node->location().warning(tr("No documentation for '%1'").arg(node->plainSignature()));
            }
        }
        else if (!node->isWrapper() && !quiet && !node->isReimplemented()) {
            /*
              Don't require documentation of things defined in Q_GADGET
             */
            if (node->name() != QLatin1String("QtGadgetHelper"))
                node->location().warning(tr("No documentation for '%1'").arg(node->plainSignature()));
        }
    }
    else if (!node->isSharingComment()) {
        if (node->type() == Node::Function) {
            const FunctionNode *func = static_cast<const FunctionNode *>(node);
            if (!func->reimplementedFrom().isEmpty())
                generateReimplementedFrom(func, marker);
        }

        if (!generateText(node->doc().body(), node, marker)) {
            if (node->isReimplemented())
                return;
        }

        if (node->type() == Node::Enum) {
            const EnumNode *enume = (const EnumNode *) node;

            QSet<QString> definedItems;
            QList<EnumItem>::ConstIterator it = enume->items().constBegin();
            while (it != enume->items().constEnd()) {
                definedItems.insert((*it).name());
                ++it;
            }

            QSet<QString> documentedItems = enume->doc().enumItemNames().toSet();
            QSet<QString> allItems = definedItems + documentedItems;
            if (allItems.count() > definedItems.count() ||
                    allItems.count() > documentedItems.count()) {
                QSet<QString>::ConstIterator a = allItems.constBegin();
                while (a != allItems.constEnd()) {
                    if (!definedItems.contains(*a)) {
                        QString details;
                        QString best = nearestName(*a, definedItems);
                        if (!best.isEmpty() && !documentedItems.contains(best))
                            details = tr("Maybe you meant '%1'?").arg(best);

                        node->doc().location().warning(tr("No such enum item '%1' in %2")
                                                       .arg(*a).arg(node->plainFullName()), details);
                        if (*a == "Void")
                            qDebug() << "VOID:" << node->name() << definedItems;
                    }
                    else if (!documentedItems.contains(*a)) {
                        node->doc().location().warning(tr("Undocumented enum item '%1' in %2")
                                                       .arg(*a).arg(node->plainFullName()));
                    }
                    ++a;
                }
            }
        }
        else if (node->type() == Node::Function) {
            const FunctionNode *func = static_cast<const FunctionNode *>(node);
            QSet<QString> definedParams;
            QVector<Parameter>::ConstIterator p = func->parameters().constBegin();
            while (p != func->parameters().constEnd()) {
                if (!(*p).name().isEmpty())
                    definedParams.insert((*p).name());
                ++p;
            }

            QSet<QString> documentedParams = func->doc().parameterNames();
            QSet<QString> allParams = definedParams + documentedParams;
            if (allParams.count() > definedParams.count()
                    || allParams.count() > documentedParams.count()) {
                QSet<QString>::ConstIterator a = allParams.constBegin();
                while (a != allParams.constEnd()) {
                    if (!definedParams.contains(*a)) {
                        QString details;
                        QString best = nearestName(*a, definedParams);
                        if (!best.isEmpty())
                            details = tr("Maybe you meant '%1'?").arg(best);

                        node->doc().location().warning(
                                    tr("No such parameter '%1' in %2").arg(*a).arg(node->plainFullName()),
                                    details);
                    }
                    else if (!(*a).isEmpty() && !documentedParams.contains(*a)) {
                        bool needWarning = (func->status() > Node::Obsolete);
                        if (func->overloadNumber() > 0) {
                            FunctionNode *primaryFunc = func->parent()->findFunctionNode(func->name(), QString());
                            if (primaryFunc) {
                                foreach (const Parameter &param,
                                         primaryFunc->parameters()) {
                                    if (param.name() == *a) {
                                        needWarning = false;
                                        break;
                                    }
                                }
                            }
                        }
                        if (needWarning && !func->isReimplemented() && !func->isOverload())
                            node->doc().location().warning(
                                        tr("Undocumented parameter '%1' in %2")
                                        .arg(*a).arg(node->plainFullName()));
                    }
                    ++a;
                }
            }
            /*
              Something like this return value check should
              be implemented at some point.
            */
            if (func->status() > Node::Obsolete && func->returnType() == "bool"
                    && func->reimplementedFrom() == 0 && !func->isOverload()) {
                QString body = func->doc().body().toString();
                if (!body.contains("return", Qt::CaseInsensitive))
                    node->doc().location().warning(tr("Undocumented return value"));
            }
        }
    }

    if (node->isDocumentNode()) {
        const DocumentNode *dn = static_cast<const DocumentNode *>(node);
        if (dn->isExample() && !dn->noAutoList()) {
            generateExampleFiles(dn, marker);
        }
        else if (dn->docSubtype() == Node::File) {
            Text text;
            Quoter quoter;
            Doc::quoteFromFile(dn->doc().location(), quoter, dn->name());
            QString code = quoter.quoteTo(dn->location(), QString(), QString());
            CodeMarker *codeMarker = CodeMarker::markerForFileName(dn->name());
            text << Atom(codeMarker->atomType(), code);
            generateText(text, dn, codeMarker);
        }
    }
}

void Generator::generateCppReferencePage(Node* /* node */, CodeMarker* /* marker */)
{
}

void Generator::generateExampleFiles(const DocumentNode *dn, CodeMarker *marker)
{
    if (dn->childNodes().isEmpty())
        return;
    generateFileList(dn, marker, Node::File);
    generateFileList(dn, marker, Node::Image);
}

void Generator::generateDocumentNode(DocumentNode* /* dn */, CodeMarker* /* marker */)
{
}

void Generator::generateCollectionNode(CollectionNode* , CodeMarker* )
{
}

/*!
  This function is called when the documentation for an
  example is being formatted. It outputs the list of source
  files comprising the example, and the list of images used
  by the example. The images are copied into a subtree of
  \c{...doc/html/images/used-in-examples/...}
 */
void Generator::generateFileList(const DocumentNode* dn,
                                 CodeMarker* marker,
                                 Node::DocSubtype subtype,
                                 const QString& regExp)
{
    int count = 0;
    Text text;
    OpenedList openedList(OpenedList::Bullet);
    QString tag;

    NodeList children(dn->childNodes());
    std::sort(children.begin(), children.end(), Generator::compareNodes);
    if (!regExp.isEmpty()) {
        QRegExp re(regExp);
        QMutableListIterator<Node*> i(children);
        while (i.hasNext()) {
            if (!re.exactMatch(i.next()->name()))
                i.remove();
        }
    }
    if (children.size() > 1) {
        switch (subtype) {
        default:
        case Node::File:
            tag = "Files:";
            break;
        case Node::Image:
            tag = "Images:";
            break;
        }
        text << Atom::ParaLeft << tag << Atom::ParaRight;
    }

    text << Atom(Atom::ListLeft, openedList.styleString());

    foreach (const Node* child, children) {
        if (child->docSubtype() == subtype) {
            ++count;
            QString file = child->name();
            if (subtype == Node::Image) {
                if (!file.isEmpty()) {
                    QDir dirInfo;
                    QString userFriendlyFilePath;
                    const QString prefix("/images/used-in-examples/");
                    QString srcPath = Config::findFile(dn->location(),
                                                       QStringList(),
                                                       exampleDirs,
                                                       file,
                                                       exampleImgExts,
                                                       &userFriendlyFilePath);
                    outFileNames_ << prefix.mid(1) + userFriendlyFilePath;
                    userFriendlyFilePath.truncate(userFriendlyFilePath.lastIndexOf('/'));
                    QString imgOutDir = outDir_ + prefix + userFriendlyFilePath;
                    if (!dirInfo.mkpath(imgOutDir))
                        dn->location().fatal(tr("Cannot create output directory '%1'").arg(imgOutDir));
                    Config::copyFile(dn->location(), srcPath, file, imgOutDir);
                }

            }

            openedList.next();
            text << Atom(Atom::ListItemNumber, openedList.numberString())
                 << Atom(Atom::ListItemLeft, openedList.styleString())
                 << Atom::ParaLeft
                 << Atom(Atom::Link, file)
                 << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << file
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                 << Atom::ParaRight
                 << Atom(Atom::ListItemRight, openedList.styleString());
        }
    }
    text << Atom(Atom::ListRight, openedList.styleString());
    if (count > 0)
        generateText(text, dn, marker);
}

void Generator::generateInheritedBy(const ClassNode *classe, CodeMarker *marker)
{
    if (!classe->derivedClasses().isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Inherited by: "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        appendSortedNames(text, classe, classe->derivedClasses());
        text << Atom::ParaRight;
        generateText(text, classe, marker);
    }
}

void Generator::generateInherits(const ClassNode *classe, CodeMarker *marker)
{
    QList<RelatedClass>::ConstIterator r;
    int index;

    if (!classe->baseClasses().isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Inherits: "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        r = classe->baseClasses().constBegin();
        index = 0;
        while (r != classe->baseClasses().constEnd()) {
            if ((*r).node_) {
                appendFullName(text, (*r).node_, classe);

                if ((*r).access_ == Node::Protected) {
                    text << " (protected)";
                }
                else if ((*r).access_ == Node::Private) {
                    text << " (private)";
                }
                text << separator(index++, classe->baseClasses().count());
            }
            ++r;
        }
        text << Atom::ParaRight;
        generateText(text, classe, marker);
    }
}

/*!
  Recursive writing of HTML files from the root \a node.
 */
void Generator::generateDocumentation(Node* node)
{
    if (!node->url().isNull())
        return;
    if (node->isIndexNode())
        return;
    if (node->isInternal() && !showInternal_)
        return;

    if (node->isDocumentNode()) {
        DocumentNode* docNode = static_cast<DocumentNode*>(node);
        if (docNode->docSubtype() == Node::ExternalPage)
            return;
        if (docNode->docSubtype() == Node::Image)
            return;
        if (docNode->docSubtype() == Node::Page) {
            if (docNode->count() > 0)
                qDebug("PAGE %s HAS CHILDREN", qPrintable(docNode->title()));
        }
    }
    else if (node->isQmlPropertyGroup() || node->isJsPropertyGroup())
        return;

    /*
      Obtain a code marker for the source file.
     */
    CodeMarker *marker = CodeMarker::markerForFileName(node->location().filePath());

    if (node->parent() != 0) {
        if (node->isClass() || (node->isNamespace() && node->docMustBeGenerated())) {
            beginSubPage(node, fileName(node));
            generateCppReferencePage(static_cast<Aggregate*>(node), marker);
            endSubPage();
        }
        else if (node->isQmlType() || node->isJsType()) {
            beginSubPage(node, fileName(node));
            QmlTypeNode* qcn = static_cast<QmlTypeNode*>(node);
            generateQmlTypePage(qcn, marker);
            endSubPage();
        }
        else if (node->isDocumentNode()) {
            beginSubPage(node, fileName(node));
            generateDocumentNode(static_cast<DocumentNode*>(node), marker);
            endSubPage();
        }
        else if (node->isQmlBasicType() || node->isJsBasicType()) {
            beginSubPage(node, fileName(node));
            QmlBasicTypeNode* qbtn = static_cast<QmlBasicTypeNode*>(node);
            generateQmlBasicTypePage(qbtn, marker);
            endSubPage();
        }
    }

    if (node->isAggregate()) {
        Aggregate* aggregate = static_cast<Aggregate*>(node);
        int i = 0;
        while (i < aggregate->childNodes().count()) {
            Node *c = aggregate->childNodes().at(i);
            if (c->isAggregate() && !c->isPrivate()) {
                generateDocumentation((Aggregate*)c);
            }
            else if (c->isCollectionNode()) {
                /*
                  A collection node collects: groups, C++ modules,
                  QML modules or JavaScript modules.

                  Don't output an HTML page for the collection
                  node unless the \group, \module, \qmlmodule or
                  \jsmodule command was actually seen by qdoc in
                  the qdoc comment for the node.

                  A key prerequisite in this case is the call to
                  mergeCollections(cn). We must determine whether
                  this group, module, QML module, or JavaScript
                  module has members in other modules. We know at
                  this point that cn's members list contains only
                  members in the current module. Therefore, before
                  outputting the page for cn, we must search for
                  members of cn in the other modules and add them
                  to the members list.
                */
                CollectionNode* cn = static_cast<CollectionNode*>(c);
                if (cn->wasSeen()) {
                    qdb_->mergeCollections(cn);
                    beginSubPage(c, fileName(c));
                    generateCollectionNode(cn, marker);
                    endSubPage();
                }
            }
            ++i;
        }
    }
}

/*!
  Generate a list of maintainers in the output
 */
void Generator::generateMaintainerList(const Aggregate* node, CodeMarker* marker)
{
    QStringList sl = getMetadataElements(node,"maintainer");

    if (!sl.isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Maintained by: "
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD);

        for (int i = 0; i < sl.size(); ++i)
            text << sl.at(i) << separator(i, sl.size());

        text << Atom::ParaRight;
        generateText(text, node, marker);
    }
}

/*!
  Output the "Inherit by" list for the QML element,
  if it is inherited by any other elements.
 */
void Generator::generateQmlInheritedBy(const QmlTypeNode* qcn,
                                              CodeMarker* marker)
{
    if (qcn) {
        NodeList subs;
        QmlTypeNode::subclasses(qcn, subs);
        if (!subs.isEmpty()) {
            Text text;
            text << Atom::ParaLeft << "Inherited by ";
            appendSortedQmlNames(text, qcn, subs);
            text << Atom::ParaRight;
            generateText(text, qcn, marker);
        }
    }
}

/*!
 */
void Generator::generateQmlInherits(QmlTypeNode* , CodeMarker* )
{
    // stub.
}

/*!
  Extract sections of markup text surrounded by \e qmltext
  and \e endqmltext and output them.
 */
bool Generator::generateQmlText(const Text& text,
                                const Node *relative,
                                CodeMarker *marker,
                                const QString& /* qmlName */ )
{
    const Atom* atom = text.firstAtom();
    bool result = false;

    if (atom != 0) {
        initializeTextOutput();
        while (atom) {
            if (atom->type() != Atom::QmlText)
                atom = atom->next();
            else {
                atom = atom->next();
                while (atom && (atom->type() != Atom::EndQmlText)) {
                    int n = 1 + generateAtom(atom, relative, marker);
                    while (n-- > 0)
                        atom = atom->next();
                }
            }
        }
        result = true;
    }
    return result;
}

void Generator::generateReimplementedFrom(const FunctionNode *fn, CodeMarker *marker)
{
    if (!fn->reimplementedFrom().isEmpty()) {
        if (fn->parent()->isClass()) {
            ClassNode* cn = static_cast<ClassNode*>(fn->parent());
            const FunctionNode *from = cn->findOverriddenFunction(fn);
            if (from && from->access() != Node::Private && from->parent()->access() != Node::Private) {
                Text text;
                text << Atom::ParaLeft << "Reimplemented from ";
                QString fullName =  from->parent()->name() + "::" + from->name() + "()";
                appendFullName(text, from->parent(), fullName, from);
                text << "." << Atom::ParaRight;
                generateText(text, fn, marker);
            }
        }
    }
}

void Generator::generateSince(const Node *node, CodeMarker *marker)
{
    if (!node->since().isEmpty()) {
        Text text;
        text << Atom::ParaLeft
             << "This "
             << typeString(node);
        if (node->type() == Node::Enum)
            text << " was introduced or modified in ";
        else
            text << " was introduced in ";

        QStringList since = node->since().split(QLatin1Char(' '));
        if (since.count() == 1) {
            // If there is only one argument, assume it is the Qt version number.
            text << " Qt " << since[0];
        } else {
            // Otherwise, reconstruct the <project> <version> string.
            text << " " << since.join(' ');
        }

        text << "." << Atom::ParaRight;
        generateText(text, node, marker);
    }
}

void Generator::generateStatus(const Node *node, CodeMarker *marker)
{
    Text text;

    switch (node->status()) {
    case Node::Active:
        // Do nothing.
        break;
    case Node::Preliminary:
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD)
             << "This "
             << typeString(node)
             << " is under development and is subject to change."
             << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
             << Atom::ParaRight;
        break;
    case Node::Deprecated:
        text << Atom::ParaLeft;
        if (node->isAggregate())
            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD);
        text << "This " << typeString(node) << " is deprecated.";
        if (node->isAggregate())
            text << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD);
        text << Atom::ParaRight;
        break;
    case Node::Obsolete:
        text << Atom::ParaLeft;
        if (node->isAggregate())
            text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD);
        text << "This " << typeString(node) << " is obsolete.";
        if (node->isAggregate())
            text << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD);
        text << " It is provided to keep old source code working. "
             << "We strongly advise against "
             << "using it in new code." << Atom::ParaRight;
        break;
    case Node::Internal:
    default:
        break;
    }
    generateText(text, node, marker);
}

/*!
  Generates a bold line that says:
  "The signal is private, not emitted by the user.
  The function is public so the user can pass it to connect()."
 */
void Generator::generatePrivateSignalNote(const Node* node, CodeMarker* marker)
{
    Text text;
    text << Atom::ParaLeft
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD)
         << "Note: "
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
         << "This is a private signal. It can be used in signal connections but cannot be emitted by the user."
         << Atom::ParaRight;
    generateText(text, node, marker);
}

/*!
  Generates a bold line that says:
  "This function can be invoked via the meta-object system and from QML. See Q_INVOKABLE."
 */
void Generator::generateInvokableNote(const Node* node, CodeMarker* marker)
{
    Text text;
    text << Atom::ParaLeft
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD)
         << "Note: "
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
         << "This function can be invoked via the meta-object system and from QML. See "
         << Atom(Atom::Link,"Q_INVOKABLE")
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
         << "Q_INVOKABLE"
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
         << "."
         << Atom::ParaRight;
    generateText(text, node, marker);
}

/*!
  Generate the documentation for \a relative. i.e. \a relative
  is the node that reporesentas the entity where a qdoc comment
  was found, and \a text represents the qdoc comment.
 */
bool Generator::generateText(const Text& text,
                             const Node *relative,
                             CodeMarker *marker)
{
    bool result = false;
    if (text.firstAtom() != 0) {
        int numAtoms = 0;
        initializeTextOutput();
        generateAtomList(text.firstAtom(),
                         relative,
                         marker,
                         true,
                         numAtoms);
        result = true;
    }
    return result;
}

/*
  The node is an aggregate, typically a class node, which has
  a threadsafeness level. This function checks all the children
  of the node to see if they are exceptions to the node's
  threadsafeness. If there are any exceptions, the exceptions
  are added to the appropriate set (reentrant, threadsafe, and
  nonreentrant, and true is returned. If there are no exceptions,
  the three node lists remain empty and false is returned.
 */
static bool hasExceptions(const Node* node,
                          NodeList& reentrant,
                          NodeList& threadsafe,
                          NodeList& nonreentrant)
{
    bool result = false;
    Node::ThreadSafeness ts = node->threadSafeness();
    const Aggregate* a = static_cast<const Aggregate*>(node);
    NodeList::ConstIterator c = a->childNodes().constBegin();
    while (c != a->childNodes().constEnd()) {
        if (!(*c)->isObsolete()){
            switch ((*c)->threadSafeness()) {
            case Node::Reentrant:
                reentrant.append(*c);
                if (ts == Node::ThreadSafe)
                    result = true;
                break;
            case Node::ThreadSafe:
                threadsafe.append(*c);
                if (ts == Node::Reentrant)
                    result = true;
                break;
            case Node::NonReentrant:
                nonreentrant.append(*c);
                result = true;
                break;
            default:
                break;
            }
        }
        ++c;
    }
    return result;
}

static void startNote(Text& text)
{
    text << Atom::ParaLeft
         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_BOLD)
         << "Note:"
         << Atom(Atom::FormattingRight, ATOM_FORMATTING_BOLD)
         << " ";
}

/*!
  Generates text that explains how threadsafe and/or reentrant
  \a node is.
 */
void Generator::generateThreadSafeness(const Node *node, CodeMarker *marker)
{
    Text text, rlink, tlink;
    NodeList reentrant;
    NodeList threadsafe;
    NodeList nonreentrant;
    Node::ThreadSafeness ts = node->threadSafeness();
    bool exceptions = false;

    rlink << Atom(Atom::Link,"reentrant")
          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
          << "reentrant"
          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);

    tlink << Atom(Atom::Link,"thread-safe")
          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
          << "thread-safe"
          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);

    switch (ts) {
    case Node::UnspecifiedSafeness:
        break;
    case Node::NonReentrant:
        text << Atom::ParaLeft
             << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
             << "Warning:"
             << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD)
             << " This "
             << typeString(node)
             << " is not "
             << rlink
             << "."
             << Atom::ParaRight;
        break;
    case Node::Reentrant:
    case Node::ThreadSafe:
        startNote(text);
        if (node->isAggregate()) {
            exceptions = hasExceptions(node, reentrant, threadsafe, nonreentrant);
            text << "All functions in this " << typeString(node) << " are ";
            if (ts == Node::ThreadSafe)
                text << tlink;
            else
                text << rlink;

            if (!exceptions || (ts == Node::Reentrant && !threadsafe.isEmpty()))
                text << ".";
            else
                text << " with the following exceptions:";
        }
        else {
            text << "This " << typeString(node) << " is ";
            if (ts == Node::ThreadSafe)
                text << tlink;
            else
                text << rlink;
            text << ".";
        }
        text << Atom::ParaRight;
        break;
    default:
        break;
    }
    generateText(text,node,marker);

    if (exceptions) {
        text.clear();
        if (ts == Node::Reentrant) {
            if (!nonreentrant.isEmpty()) {
                startNote(text);
                text << "These functions are not "
                     << rlink
                     << ":"
                     << Atom::ParaRight;
                signatureList(nonreentrant, node, marker);
            }
            if (!threadsafe.isEmpty()) {
                text.clear();
                startNote(text);
                text << "These functions are also "
                     << tlink
                     << ":"
                     << Atom::ParaRight;
                generateText(text, node, marker);
                signatureList(threadsafe, node, marker);
            }
        }
        else { // thread-safe
            if (!reentrant.isEmpty()) {
                startNote(text);
                text << "These functions are only "
                     << rlink
                     << ":"
                     << Atom::ParaRight;
                signatureList(reentrant, node, marker);
            }
            if (!nonreentrant.isEmpty()) {
                text.clear();
                startNote(text);
                text << "These functions are not "
                     << rlink
                     << ":"
                     << Atom::ParaRight;
                signatureList(nonreentrant, node, marker);
            }
        }
    }
}

/*!
    If the node is an overloaded signal, and a node with an example on how to connect to it
 */
void Generator::generateOverloadedSignal(const Node* node, CodeMarker* marker)
{
    if (node->type() != Node::Function)
        return;
    const FunctionNode *func = static_cast<const FunctionNode *>(node);
    if (!func->isSignal())
        return;
    if (node->parent()->overloads(node->name()).count() <= 1)
        return;


    // Compute a friendly name for the object of that instance.
    // e.g:  "QAbstractSocket" -> "abstractSocket"
    QString objectName = node->parent()->name();
    if (objectName.size() >= 2) {
        if (objectName[0] == 'Q')
            objectName = objectName.mid(1);
        objectName[0] = objectName[0].toLower();
    }

    // We have an overloaded signal, show an example. Note, for const
    // overloaded signals one should use Q{Const,NonConst}Overload, but
    // it is very unlikely that we will ever have public API overloading
    // signals by const.
    QString code = "connect(" + objectName + ", QOverload<";
    for (int i = 0; i < func->parameters().size(); ++i) {
        if (i != 0)
            code += ", ";
        code += func->parameters().at(i).dataType();
    }

    code += ">::of(&" + func->parent()->name() + "::" + func->name() + "),\n    [=](";

    for (int i = 0; i < func->parameters().size(); ++i) {
        if (i != 0)
            code += ", ";
        const Parameter &p = func->parameters().at(i);
        code += p.dataType();
        if (code[code.size()-1].isLetterOrNumber())
            code += QLatin1Char(' ');
        code += p.name();
    }

    code += "){ /* ... */ });";

    Text text;
    text << Atom::ParaLeft
         << Atom(Atom::FormattingLeft,ATOM_FORMATTING_BOLD)
         << "Note:"
         << Atom(Atom::FormattingRight,ATOM_FORMATTING_BOLD)
         << " Signal "
         << Atom(Atom::FormattingLeft,ATOM_FORMATTING_ITALIC)
         << node->name()
         << Atom(Atom::FormattingRight,ATOM_FORMATTING_ITALIC)
         << " is overloaded in this class. "
            "To connect to this signal by using the function pointer syntax, Qt "
            "provides a convenient helper for obtaining the function pointer as "
            "shown in this example:"
          << Atom(Atom::Code, marker->markedUpCode(code, node, func->location()));

    generateText(text, node, marker);
}


/*!
  Traverses the database recursivly to generate all the documentation.
 */
void Generator::generateDocs()
{
    currentGenerator_ = this;
    generateDocumentation(qdb_->primaryTreeRoot());
}

Generator *Generator::generatorForFormat(const QString& format)
{
    QList<Generator *>::ConstIterator g = generators.constBegin();
    while (g != generators.constEnd()) {
        if ((*g)->format() == format)
            return *g;
        ++g;
    }
    return 0;
}

/*!
  Looks up the tag \a t in the map of metadata values for the
  current topic in \a inner. If a value for the tag is found,
  the value is returned.

  \note If \a t is found in the metadata map, it is erased.
  i.e. Once you call this function for a particular \a t,
  you consume \a t.
 */
QString Generator::getMetadataElement(const Aggregate* inner, const QString& t)
{
    QString s;
    QStringMultiMap& metaTagMap = const_cast<QStringMultiMap&>(inner->doc().metaTagMap());
    QStringMultiMap::iterator i = metaTagMap.find(t);
    if (i != metaTagMap.end()) {
        s = i.value();
        metaTagMap.erase(i);
    }
    return s;
}

/*!
  Looks up the tag \a t in the map of metadata values for the
  current topic in \a inner. If values for the tag are found,
  they are returned in a string list.

  \note If \a t is found in the metadata map, all the pairs
  having the key \a t are erased. i.e. Once you call this
  function for a particular \a t, you consume \a t.
 */
QStringList Generator::getMetadataElements(const Aggregate* inner, const QString& t)
{
    QStringList s;
    QStringMultiMap& metaTagMap = const_cast<QStringMultiMap&>(inner->doc().metaTagMap());
    s = metaTagMap.values(t);
    if (!s.isEmpty())
        metaTagMap.remove(t);
    return s;
}

/*!
  Returns a relative path name for an image.
 */
QString Generator::imageFileName(const Node *relative, const QString& fileBase)
{
    QString userFriendlyFilePath;
    QString filePath = Config::findFile(relative->doc().location(),
                                        imageFiles,
                                        imageDirs,
                                        fileBase,
                                        imgFileExts[format()],
                                        &userFriendlyFilePath);

    if (filePath.isEmpty())
        return QString();

    QString path = Config::copyFile(relative->doc().location(),
                                    filePath,
                                    userFriendlyFilePath,
                                    outputDir() + QLatin1String("/images"));
    int images_slash = path.lastIndexOf("images/");
    QString relImagePath;
    if (images_slash != -1)
        relImagePath = path.mid(images_slash);
    return relImagePath;
}

QString Generator::indent(int level, const QString& markedCode)
{
    if (level == 0)
        return markedCode;

    QString t;
    int column = 0;

    int i = 0;
    while (i < (int) markedCode.length()) {
        if (markedCode.at(i) == QLatin1Char('\n')) {
            column = 0;
        }
        else {
            if (column == 0) {
                for (int j = 0; j < level; j++)
                    t += QLatin1Char(' ');
            }
            column++;
        }
        t += markedCode.at(i++);
    }
    return t;
}



void Generator::initialize(const Config &config)
{
    outputFormats = config.getOutputFormats();
    redirectDocumentationToDevNull_ = config.getBool(CONFIG_REDIRECTDOCUMENTATIONTODEVNULL);

    imageFiles = config.getCanonicalPathList(CONFIG_IMAGES);
    imageDirs = config.getCanonicalPathList(CONFIG_IMAGEDIRS);
    scriptFiles = config.getCanonicalPathList(CONFIG_SCRIPTS);
    scriptDirs = config.getCanonicalPathList(CONFIG_SCRIPTDIRS);
    styleFiles = config.getCanonicalPathList(CONFIG_STYLES);
    styleDirs = config.getCanonicalPathList(CONFIG_STYLEDIRS);
    exampleDirs = config.getCanonicalPathList(CONFIG_EXAMPLEDIRS);
    exampleImgExts = config.getStringList(CONFIG_EXAMPLES + Config::dot + CONFIG_IMAGEEXTENSIONS);

    QString imagesDotFileExtensions = CONFIG_IMAGES + Config::dot + CONFIG_FILEEXTENSIONS;
    for (const auto &ext : config.subVars(imagesDotFileExtensions))
        imgFileExts[ext] = config.getStringList(imagesDotFileExtensions + Config::dot + ext);

    for (auto &g : generators) {
        if (outputFormats.contains(g->format())) {
            currentGenerator_ = g;
            g->initializeGenerator(config);
        }
    }

    for (const auto &n : config.subVars(CONFIG_FORMATTING)) {
        QString formattingDotName = CONFIG_FORMATTING + Config::dot + n;
        for (const auto &f : config.subVars(formattingDotName)) {
            QString def = config.getString(formattingDotName + Config::dot + f);
            if (!def.isEmpty()) {
                int numParams = Config::numParams(def);
                int numOccs = def.count("\1");
                if (numParams != 1) {
                    config.lastLocation().warning(tr("Formatting '%1' must "
                                                     "have exactly one "
                                                     "parameter (found %2)")
                                                  .arg(n).arg(numParams));
                } else if (numOccs > 1) {
                    config.lastLocation().fatal(tr("Formatting '%1' must "
                                                   "contain exactly one "
                                                   "occurrence of '\\1' "
                                                   "(found %2)")
                                                .arg(n).arg(numOccs));
                } else {
                    int paramPos = def.indexOf("\1");
                    fmtLeftMaps[f].insert(n, def.left(paramPos));
                    fmtRightMaps[f].insert(n, def.mid(paramPos + 1));
                }
            }
        }
    }

    project_ = config.getString(CONFIG_PROJECT);
    outDir_ = config.getOutputDir();
    outSubdir_ = outDir_.mid(outDir_.lastIndexOf('/') + 1);

    outputPrefixes.clear();
    QStringList items = config.getStringList(CONFIG_OUTPUTPREFIXES);
    if (!items.isEmpty()) {
        for (const auto &prefix : items)
            outputPrefixes[prefix] = config.getString(CONFIG_OUTPUTPREFIXES + Config::dot + prefix);
    } else {
        outputPrefixes[QLatin1String("QML")] = QLatin1String("qml-");
        outputPrefixes[QLatin1String("JS")] = QLatin1String("js-");
    }

    outputSuffixes.clear();
    for (const auto &suffix : config.getStringList(CONFIG_OUTPUTSUFFIXES))
        outputSuffixes[suffix] = config.getString(CONFIG_OUTPUTSUFFIXES + Config::dot + suffix);

    noLinkErrors_ = config.getBool(CONFIG_NOLINKERRORS);
    autolinkErrors_ = config.getBool(CONFIG_AUTOLINKERRORS);
}

/*!
  Creates template-specific subdirs (e.g. /styles and /scripts for HTML)
  and copies the files to them.
  */
void Generator::copyTemplateFiles(const Config &config, const QString &configVar, const QString &subDir)
{
    QStringList files = config.getCanonicalPathList(configVar, true);
    if (!files.isEmpty()) {
        QDir dirInfo;
        QString templateDir = outDir_ + QLatin1Char('/') + subDir;
        if (!dirInfo.exists(templateDir) && !dirInfo.mkdir(templateDir)) {
            config.lastLocation().fatal(tr("Cannot create %1 directory '%2'")
                                        .arg(subDir, templateDir));
        } else {
            for (const auto &file : files) {
                if (!file.isEmpty())
                    Config::copyFile(config.lastLocation(), file, file, templateDir);
            }
        }
    }
}

/*!
    Reads format-specific variables from \a config, sets output
    (sub)directories, creates them on the filesystem and copies the
    template-specific files.
 */
void Generator::initializeFormat(const Config &config)
{
    outFileNames_.clear();
    useOutputSubdirs_ = true;
    if (config.getBool(format() + Config::dot + "nosubdirs"))
        resetUseOutputSubdirs();

    if (outputFormats.isEmpty())
        return;

    outDir_ = config.getOutputDir(format());
    if (outDir_.isEmpty()) {
        config.lastLocation().fatal(tr("No output directory specified in "
                                       "configuration file or on the command line"));
    } else {
        outSubdir_ = outDir_.mid(outDir_.lastIndexOf('/') + 1);
    }

    QDir dirInfo;
    if (dirInfo.exists(outDir_)) {
        if (!generating() && Generator::useOutputSubdirs()) {
            if (!Config::removeDirContents(outDir_))
                config.lastLocation().error(tr("Cannot empty output directory '%1'").arg(outDir_));
        }
    } else if (!dirInfo.mkpath(outDir_)) {
        config.lastLocation().fatal(tr("Cannot create output directory '%1'").arg(outDir_));
    }

    // Output directory exists, which is enough for prepare phase.
    if (preparing())
        return;

    if (!dirInfo.exists(outDir_ + "/images") && !dirInfo.mkdir(outDir_ + "/images"))
        config.lastLocation().fatal(tr("Cannot create images directory '%1'").arg(outDir_ + "/images"));

    copyTemplateFiles(config, format() + Config::dot + CONFIG_STYLESHEETS, "style");
    copyTemplateFiles(config, format() + Config::dot + CONFIG_SCRIPTS, "scripts");
    copyTemplateFiles(config, format() + Config::dot + CONFIG_EXTRAIMAGES, "images");

    // Use a format-specific .quotinginformation if defined, otherwise a global value
    if (config.subVars(format()).contains(CONFIG_QUOTINGINFORMATION))
        quoting_ = config.getBool(format() + Config::dot + CONFIG_QUOTINGINFORMATION);
    else
        quoting_ = config.getBool(CONFIG_QUOTINGINFORMATION);
}

/*!
  Appends each directory path in \a moreImageDirs to the
  list of image directories.
 */
void Generator::augmentImageDirs(QSet<QString>& moreImageDirs)
{
    if (moreImageDirs.isEmpty())
        return;
    QSet<QString>::const_iterator i = moreImageDirs.begin();
    while (i != moreImageDirs.end()) {
        imageDirs.append(*i);
        ++i;
    }
}

/*!
  Sets the generator's pointer to the Config instance.
 */
void Generator::initializeGenerator(const Config& config)
{
    config_ = &config;
    showInternal_ = config.getBool(CONFIG_SHOWINTERNAL);
    singleExec_ = config.getBool(CONFIG_SINGLEEXEC);
}

bool Generator::matchAhead(const Atom *atom, Atom::AtomType expectedAtomType)
{
    return atom->next() != 0 && atom->next()->type() == expectedAtomType;
}

/*!
  Used for writing to the current output stream. Returns a
  reference to the current output stream, which is then used
  with the \c {<<} operator for writing.
 */
QTextStream &Generator::out()
{
    return *outStreamStack.top();
}

QString Generator::outFileName()
{
    return QFileInfo(static_cast<QFile*>(out().device())->fileName()).fileName();
}

QString Generator::outputPrefix(const Node *node)
{
    // Prefix is applied to QML and JS types
    if (node->isQmlType() || node->isQmlBasicType())
        return outputPrefixes[QLatin1String("QML")];
    if (node->isJsType() || node->isJsBasicType())
        return outputPrefixes[QLatin1String("JS")];
    return QString();
}

QString Generator::outputSuffix(const Node *node)
{
    // Suffix is applied to QML and JS types, as
    // well as module pages.
    if (node->isQmlModule() || node->isQmlType() || node->isQmlBasicType())
        return outputSuffixes[QLatin1String("QML")];
    if (node->isJsModule() || node->isJsType() || node->isJsBasicType())
        return outputSuffixes[QLatin1String("JS")];
    return QString();
}

bool Generator::parseArg(const QString& src,
                             const QString& tag,
                             int* pos,
                             int n,
                             QStringRef* contents,
                             QStringRef* par1,
                             bool debug)
{
#define SKIP_CHAR(c) \
    if (debug) \
    qDebug() << "looking for " << c << " at " << QString(src.data() + i, n - i); \
    if (i >= n || src[i] != c) { \
    if (debug) \
    qDebug() << " char '" << c << "' not found"; \
    return false; \
} \
    ++i;


#define SKIP_SPACE \
    while (i < n && src[i] == ' ') \
    ++i;

    int i = *pos;
    int j = i;

    // assume "<@" has been parsed outside
    //SKIP_CHAR('<');
    //SKIP_CHAR('@');

    if (tag != QStringRef(&src, i, tag.length())) {
        if (0 && debug)
            qDebug() << "tag " << tag << " not found at " << i;
        return false;
    }

    if (debug)
        qDebug() << "haystack:" << src << "needle:" << tag << "i:" <<i;

    // skip tag
    i += tag.length();

    // parse stuff like:  linkTag("(<@link node=\"([^\"]+)\">).*(</@link>)");
    if (par1) {
        SKIP_SPACE;
        // read parameter name
        j = i;
        while (i < n && src[i].isLetter())
            ++i;
        if (src[i] == '=') {
            if (debug)
                qDebug() << "read parameter" << QString(src.data() + j, i - j);
            SKIP_CHAR('=');
            SKIP_CHAR('"');
            // skip parameter name
            j = i;
            while (i < n && src[i] != '"')
                ++i;
            *par1 = QStringRef(&src, j, i - j);
            SKIP_CHAR('"');
            SKIP_SPACE;
        } else {
            if (debug)
                qDebug() << "no optional parameter found";
        }
    }
    SKIP_SPACE;
    SKIP_CHAR('>');

    // find contents up to closing "</@tag>
    j = i;
    for (; true; ++i) {
        if (i + 4 + tag.length() > n)
            return false;
        if (src[i] != '<')
            continue;
        if (src[i + 1] != '/')
            continue;
        if (src[i + 2] != '@')
            continue;
        if (tag != QStringRef(&src, i + 3, tag.length()))
            continue;
        if (src[i + 3 + tag.length()] != '>')
            continue;
        break;
    }

    *contents = QStringRef(&src, j, i - j);

    i += tag.length() + 4;

    *pos = i;
    if (debug)
        qDebug() << " tag " << tag << " found: pos now: " << i;
    return true;
#undef SKIP_CHAR
}

QString Generator::plainCode(const QString& markedCode)
{
    QString t = markedCode;
    t.replace(tag, QString());
    t.replace(quot, QLatin1String("\""));
    t.replace(gt, QLatin1String(">"));
    t.replace(lt, QLatin1String("<"));
    t.replace(amp, QLatin1String("&"));
    return t;
}

void Generator::setImageFileExtensions(const QStringList& extensions)
{
    imgFileExts[format()] = extensions;
}

void Generator::singularPlural(Text& text, const NodeList& nodes)
{
    if (nodes.count() == 1)
        text << " is";
    else
        text << " are";
}

int Generator::skipAtoms(const Atom *atom, Atom::AtomType type) const
{
    int skipAhead = 0;
    atom = atom->next();
    while (atom != 0 && atom->type() != type) {
        skipAhead++;
        atom = atom->next();
    }
    return skipAhead;
}

/*!
  Resets the variables used during text output.
 */
void Generator::initializeTextOutput()
{
    inLink_ = false;
    inContents_ = false;
    inSectionHeading_ = false;
    inTableHeader_ = false;
    numTableRows_ = 0;
    threeColumnEnumValueTable_ = true;
    link_.clear();
    sectionNumber_.clear();
}

void Generator::supplementAlsoList(const Node *node, QList<Text> &alsoList)
{
    if (node->isFunction() && !node->isMacro()) {
        const FunctionNode *func = static_cast<const FunctionNode *>(node);
        if (func->overloadNumber() == 0) {
            QString alternateName;
            const FunctionNode *alternateFunc = 0;

            if (func->name().startsWith("set") && func->name().size() >= 4) {
                alternateName = func->name()[3].toLower();
                alternateName += func->name().mid(4);
                alternateFunc = func->parent()->findFunctionNode(alternateName, QString());

                if (!alternateFunc) {
                    alternateName = "is" + func->name().mid(3);
                    alternateFunc = func->parent()->findFunctionNode(alternateName, QString());
                    if (!alternateFunc) {
                        alternateName = "has" + func->name().mid(3);
                        alternateFunc = func->parent()->findFunctionNode(alternateName, QString());
                    }
                }
            }
            else if (!func->name().isEmpty()) {
                alternateName = "set";
                alternateName += func->name()[0].toUpper();
                alternateName += func->name().mid(1);
                alternateFunc = func->parent()->findFunctionNode(alternateName, QString());
            }

            if (alternateFunc && alternateFunc->access() != Node::Private) {
                int i;
                for (i = 0; i < alsoList.size(); ++i) {
                    if (alsoList.at(i).toString().contains(alternateName))
                        break;
                }

                if (i == alsoList.size()) {
                    alternateName += "()";

                    Text also;
                    also << Atom(Atom::Link, alternateName)
                         << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                         << alternateName
                         << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                    alsoList.prepend(also);
                }
            }
        }
    }
}

void Generator::terminate()
{
    QList<Generator *>::ConstIterator g = generators.constBegin();
    while (g != generators.constEnd()) {
        if (outputFormats.contains((*g)->format()))
            (*g)->terminateGenerator();
        ++g;
    }

    fmtLeftMaps.clear();
    fmtRightMaps.clear();
    imgFileExts.clear();
    imageFiles.clear();
    imageDirs.clear();
    outDir_.clear();
}

void Generator::terminateGenerator()
{
}

/*!
  Trims trailing whitespace off the \a string and returns
  the trimmed string.
 */
QString Generator::trimmedTrailing(const QString& string, const QString &prefix, const QString &suffix)
{
    QString trimmed = string;
    while (trimmed.length() > 0 && trimmed[trimmed.length() - 1].isSpace())
        trimmed.truncate(trimmed.length() - 1);

    trimmed.append(suffix);
    trimmed.prepend(prefix);
    return trimmed;
}

QString Generator::typeString(const Node *node)
{
    switch (node->type()) {
    case Node::Namespace:
        return "namespace";
    case Node::Class:
        return "class";
    case Node::QmlType:
        return "type";
    case Node::QmlBasicType:
        return "type";
    case Node::Document:
        return "documentation";
    case Node::Enum:
        return "enum";
    case Node::Typedef:
        return "typedef";
    case Node::Function:
        return "function";
    case Node::Property:
    case Node::QmlProperty:
        return "property";
    case Node::QmlPropertyGroup:
        return "property group";
    case Node::QmlSignal:
        return "signal";
    case Node::QmlSignalHandler:
        return "signal handler";
    case Node::QmlMethod:
        return "method";
    case Node::Module:
    case Node::QmlModule:
        return "module";
    default:
        return "documentation";
    }
}

void Generator::unknownAtom(const Atom *atom)
{
    Location::internalError(tr("unknown atom type '%1' in %2 generator")
                            .arg(atom->typeString()).arg(format()));
}

QT_END_NAMESPACE
