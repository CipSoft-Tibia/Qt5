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
  htmlgenerator.cpp
*/

#include "codemarker.h"
#include "codeparser.h"
#include "helpprojectwriter.h"
#include "htmlgenerator.h"
#include "node.h"
#include "qdocdatabase.h"
#include "separator.h"
#include "tree.h"
#include <ctype.h>
#include <qdebug.h>
#include <qlist.h>
#include <qiterator.h>
#include <qtextcodec.h>
#include <quuid.h>
#include <qmap.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

#define COMMAND_VERSION                 Doc::alias("version")
int HtmlGenerator::id = 0;
bool HtmlGenerator::debugging_on = false;

QString HtmlGenerator::divNavTop;

static bool showBrokenLinks = false;

static QRegExp linkTag("(<@link node=\"([^\"]+)\">).*(</@link>)");
static QRegExp funcTag("(<@func target=\"([^\"]*)\">)(.*)(</@func>)");
static QRegExp typeTag("(<@(type|headerfile|func)(?: +[^>]*)?>)(.*)(</@\\2>)");
static QRegExp spanTag("</@(?:comment|preprocessor|string|char|number|op|type|name|keyword)>");
static QRegExp unknownTag("</?@[^>]*>");

static void addLink(const QString &linkTarget,
                    const QStringRef &nestedStuff,
                    QString *res)
{
    if (!linkTarget.isEmpty()) {
        *res += QLatin1String("<a href=\"");
        *res += linkTarget;
        *res += QLatin1String("\">");
        *res += nestedStuff;
        *res += QLatin1String("</a>");
    }
    else {
        *res += nestedStuff;
    }
}

/*!
  Constructs the HTML output generator.
 */
HtmlGenerator::HtmlGenerator()
    : codeIndent(0),
      helpProjectWriter(0),
      inObsoleteLink(false),
      funcLeftParen("\\S(\\()"),
      obsoleteLinks(false)
{
}

/*!
  Destroys the HTML output generator. Deletes the singleton
  instance of HelpProjectWriter.
 */
HtmlGenerator::~HtmlGenerator()
{
    if (helpProjectWriter) {
        delete helpProjectWriter;
        helpProjectWriter = 0;
    }
}

/*!
  Initializes the HTML output generator's data structures
  from the configuration class \a config.
 */
void HtmlGenerator::initializeGenerator(const Config &config)
{
    static const struct {
        const char *key;
        const char *left;
        const char *right;
    } defaults[] = {
        { ATOM_FORMATTING_BOLD, "<b>", "</b>" },
        { ATOM_FORMATTING_INDEX, "<!--", "-->" },
        { ATOM_FORMATTING_ITALIC, "<i>", "</i>" },
        { ATOM_FORMATTING_PARAMETER, "<i>", "</i>" },
        { ATOM_FORMATTING_SUBSCRIPT, "<sub>", "</sub>" },
        { ATOM_FORMATTING_SUPERSCRIPT, "<sup>", "</sup>" },
        { ATOM_FORMATTING_TELETYPE, "<code>", "</code>" }, // <tt> tag is not supported in HTML5
        { ATOM_FORMATTING_UICONTROL, "<b>", "</b>" },
        { ATOM_FORMATTING_UNDERLINE, "<u>", "</u>" },
        { 0, 0, 0 }
    };

    Generator::initializeGenerator(config);
    obsoleteLinks = config.getBool(CONFIG_OBSOLETELINKS);
    setImageFileExtensions(QStringList() << "png" << "jpg" << "jpeg" << "gif");

    /*
      The formatting maps are owned by Generator. They are cleared in
      Generator::terminate().
     */
    int i = 0;
    while (defaults[i].key) {
        formattingLeftMap().insert(QLatin1String(defaults[i].key), QLatin1String(defaults[i].left));
        formattingRightMap().insert(QLatin1String(defaults[i].key), QLatin1String(defaults[i].right));
        i++;
    }

    style = config.getString(HtmlGenerator::format() +
                             Config::dot +
                             CONFIG_STYLE);
    endHeader = config.getString(HtmlGenerator::format() +
                                 Config::dot +
                                 CONFIG_ENDHEADER);
    postHeader = config.getString(HtmlGenerator::format() +
                                  Config::dot +
                                  HTMLGENERATOR_POSTHEADER);
    postPostHeader = config.getString(HtmlGenerator::format() +
                                      Config::dot +
                                      HTMLGENERATOR_POSTPOSTHEADER);
    prologue = config.getString(HtmlGenerator::format() +
                                      Config::dot +
                                      HTMLGENERATOR_PROLOGUE);

    footer = config.getString(HtmlGenerator::format() +
                              Config::dot +
                              HTMLGENERATOR_FOOTER);
    address = config.getString(HtmlGenerator::format() +
                               Config::dot +
                               HTMLGENERATOR_ADDRESS);
    pleaseGenerateMacRef = config.getBool(HtmlGenerator::format() +
                                          Config::dot +
                                          HTMLGENERATOR_GENERATEMACREFS);
    noNavigationBar = config.getBool(HtmlGenerator::format() +
                                   Config::dot +
                                   HTMLGENERATOR_NONAVIGATIONBAR);
    navigationSeparator = config.getString(HtmlGenerator::format() +
                                           Config::dot +
                                           HTMLGENERATOR_NAVIGATIONSEPARATOR);
    tocDepth = config.getInt(HtmlGenerator::format() +
                              Config::dot +
                              HTMLGENERATOR_TOCDEPTH);

    project = config.getString(CONFIG_PROJECT);

    projectDescription = config.getString(CONFIG_DESCRIPTION);
    if (projectDescription.isEmpty() && !project.isEmpty())
        projectDescription = project + QLatin1String(" Reference Documentation");

    projectUrl = config.getString(CONFIG_URL);
    tagFile_ = config.getString(CONFIG_TAGFILE);

#ifndef QT_NO_TEXTCODEC
    outputEncoding = config.getString(CONFIG_OUTPUTENCODING);
    if (outputEncoding.isEmpty())
        outputEncoding = QLatin1String("UTF-8");
    outputCodec = QTextCodec::codecForName(outputEncoding.toLocal8Bit());
#endif

    naturalLanguage = config.getString(CONFIG_NATURALLANGUAGE);
    if (naturalLanguage.isEmpty())
        naturalLanguage = QLatin1String("en");

    QSet<QString> editionNames = config.subVars(CONFIG_EDITION);
    QSet<QString>::ConstIterator edition = editionNames.constBegin();
    while (edition != editionNames.constEnd()) {
        QString editionName = *edition;
        QStringList editionModules = config.getStringList(CONFIG_EDITION +
                                                          Config::dot +
                                                          editionName +
                                                          Config::dot +
                                                          "modules");
        QStringList editionGroups = config.getStringList(CONFIG_EDITION +
                                                         Config::dot +
                                                         editionName +
                                                         Config::dot +
                                                         "groups");

        if (!editionModules.isEmpty())
            editionModuleMap[editionName] = editionModules;
        if (!editionGroups.isEmpty())
            editionGroupMap[editionName] = editionGroups;

        ++edition;
    }

    codeIndent = config.getInt(CONFIG_CODEINDENT); // QTBUG-27798
    codePrefix = config.getString(CONFIG_CODEPREFIX);
    codeSuffix = config.getString(CONFIG_CODESUFFIX);

    /*
      The help file write should be allocated once and only once
      per qdoc execution.
     */
    if (helpProjectWriter)
        helpProjectWriter->reset(config, project.toLower() + ".qhp", this);
    else
        helpProjectWriter = new HelpProjectWriter(config, project.toLower() + ".qhp", this);

    // Documentation template handling
    headerScripts = config.getString(HtmlGenerator::format() + Config::dot + CONFIG_HEADERSCRIPTS);
    headerStyles = config.getString(HtmlGenerator::format() + Config::dot + CONFIG_HEADERSTYLES);

    QString prefix = CONFIG_QHP + Config::dot + project + Config::dot;
    manifestDir = QLatin1String("qthelp://") + config.getString(prefix + QLatin1String("namespace"));
    manifestDir += QLatin1Char('/') + config.getString(prefix + QLatin1String("virtualFolder")) + QLatin1Char('/');
    readManifestMetaContent(config);
    examplesPath = config.getString(CONFIG_EXAMPLESINSTALLPATH);
    if (!examplesPath.isEmpty())
        examplesPath += QLatin1Char('/');

    // Retrieve the config for the navigation bar
    homepage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_HOMEPAGE);

    hometitle = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_HOMETITLE, homepage);

    landingpage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_LANDINGPAGE);

    landingtitle = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_LANDINGTITLE, landingpage);

    cppclassespage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_CPPCLASSESPAGE);

    cppclassestitle = config.getString(CONFIG_NAVIGATION
                                       + Config::dot
                                       + CONFIG_CPPCLASSESTITLE,
                                       QLatin1String("C++ Classes"));

    qmltypespage = config.getString(CONFIG_NAVIGATION
                                    + Config::dot
                                    + CONFIG_QMLTYPESPAGE);

    qmltypestitle = config.getString(CONFIG_NAVIGATION
                                     + Config::dot
                                     + CONFIG_QMLTYPESTITLE,
                                     QLatin1String("QML Types"));

    buildversion = config.getString(CONFIG_BUILDVERSION);
}

/*!
  Gracefully terminates the HTML output generator.
 */
void HtmlGenerator::terminateGenerator()
{
    Generator::terminateGenerator();
}

QString HtmlGenerator::format()
{
    return "HTML";
}

/*!
  Generate targets for any \keyword commands that were seen
  in the qdoc comment for the \a node.
 */
void HtmlGenerator::generateKeywordAnchors(const Node* node)
{
    Q_UNUSED(node);
    // Disabled: keywords always link to the top of the QDoc
    // comment they appear in, and do not use a dedicated anchor.
#if 0
    if (!node->doc().isEmpty()) {
        const QList<Atom*>& keywords = node->doc().keywords();
        foreach (Atom* a, keywords) {
            out() << QLatin1String("<a name=\"") << Doc::canonicalTitle(a->string()) << QLatin1String("\"></a>");
        }
    }
#endif
}

/*!
  If qdoc is in the \c {-prepare} phase, traverse the primary
  tree to generate the index file for the current module.

  If qdoc is in the \c {-generate} phase, traverse the primary
  tree to generate all the HTML documentation for the current
  module. Then generate the help file and the tag file.
 */
void HtmlGenerator::generateDocs()
{
    Node* qflags = qdb_->findClassNode(QStringList("QFlags"));
    if (qflags)
        qflagsHref_ = linkForNode(qflags,0);
    if (!preparing())
        Generator::generateDocs();
    if (Generator::generating() && Generator::writeQaPages())
        generateQAPage();

    if (!generating()) {
        QString fileBase = project.toLower().simplified().replace(QLatin1Char(' '), QLatin1Char('-'));
        qdb_->generateIndex(outputDir() + QLatin1Char('/') + fileBase + ".index",
                            projectUrl,
                            projectDescription,
                            this,
                            true);
    }

    if (!preparing()) {
        helpProjectWriter->generate();
        generateManifestFiles();
        /*
          Generate the XML tag file, if it was requested.
        */
        qdb_->generateTagFile(tagFile_, this);
    }
}

/*!
  Output the module's Quality Assurance page.
 */
void HtmlGenerator::generateQAPage()
{
    NamespaceNode* node = qdb_->primaryTreeRoot();
    beginSubPage(node, "aaa-" + defaultModuleName().toLower() + "-qa-page.html");
    CodeMarker* marker = CodeMarker::markerForFileName(node->location().filePath());
    QString title = "Quality Assurance Page for " + defaultModuleName();
    QString t = "Quality assurance information for checking the " + defaultModuleName() + " documentation.";
    generateHeader(title, node, marker);
    generateTitle(title, Text() << t, LargeSubTitle, node, marker);

    QStringList strings;
    QVector<int> counts;
    QString depends = qdb_->getLinkCounts(strings, counts);
    if (!strings.isEmpty()) {
        t = "Intermodule Link Counts";
        QString ref = registerRef(t);
        out() << "<a name=\"" << ref << "\"></a>" << divNavTop << '\n';
        out() << "<h2 id=\"" << ref << "\">" << protectEnc(t) << "</h2>\n";
        out() << "<table class=\"valuelist\"><tr valign=\"top\" "
              << "class=\"even\"><th class=\"tblConst\">Destination Module</th>"
              << "<th class=\"tblval\">Link Count</th></tr>\n";
        QString fileName;
        for (int i = 0; i< strings.size(); ++i) {
            fileName = generateLinksToLinksPage(strings.at(i), marker);
            out() << "<tr><td class=\"topAlign\"><tt>"
                  << "<a href=\"" << fileName << "\">"
                  << strings.at(i) << "</a>"
                  << "</tt></td><td class=\"topAlign\"><tt>" << counts.at(i)
                  << "</tt></td></tr>\n";
        }
        int count = 0;
        fileName = generateLinksToBrokenLinksPage(marker, count);
        if (count != 0) {
            out() << "<tr><td class=\"topAlign\"><tt>"
                  << "<a href=\"" << fileName << "\">"
                  << "Broken Links" << "</a>"
                  << "</tt></td><td class=\"topAlign\"><tt>" << count
                  << "</tt></td></tr>\n";

        }

        out() << "</table>\n";
        t = "The Optimal \"depends\" Variable";
        out() << "<h2>" << protectEnc(t) << "</h2>\n";
        t = "Consider replacing the depends variable in " + defaultModuleName().toLower() +
            ".qdocconf with this one, if the two are not identical:";
        out() << "<p>" << protectEnc(t) << "</p>\n";
        out() << "<p>" << protectEnc(depends) << "</p>\n";
    }
    generateFooter();
    endSubPage();
}

/*!
  This function writes an html file containing a list of
  links to links that originate in the current module and
  go to targets in the specified \a module. The \a marker
  is used for the same thing the marker is always used for.
 */
QString HtmlGenerator::generateLinksToLinksPage(const QString& module, CodeMarker* marker)
{
    NamespaceNode* node = qdb_->primaryTreeRoot();
    QString fileName = "aaa-links-to-" + module + ".html";
    beginSubPage(node, fileName);
    QString title = "Links from " + defaultModuleName() + " to " + module;
    generateHeader(title, node, marker);
    generateTitle(title, Text(), SmallSubTitle, node, marker);
    out() << "<p>This is a list of links from " << defaultModuleName()
          << " to " << module << ".  ";
    out() << "Click on a link to go to the location of the link. The link is marked ";
    out() << "with red asterisks. ";
    out() << "Click on the marked link to see if it goes to the right place.</p>\n";
    TargetList* tlist = qdb_->getTargetList(module);
    if (tlist) {
        out() << "<table class=\"valuelist\"><tr valign=\"top\" class=\"odd\"><th class=\"tblConst\">Link to  link...</th><th class=\"tblval\">In file...</th><th class=\"tbldscr\">Somewhere after line number...</th></tr>\n";
        foreach (TargetLoc* t, *tlist) {
            // e.g.: <a name="link-8421"></a><a href="layout.html">Layout Management</a>
            out() << "<tr><td class=\"topAlign\">";
            out() << "<a href=\"" << t->fileName_ << "#" << t->target_ << "\">";
            out() << t->text_ << "</a></td>";
            out() << "<td class=\"topAlign\">";
            QString f = t->loc_->doc().location().filePath();
            out() << f << "</td>";
            out() << "<td class=\"topAlign\">";
            out() << t->loc_->doc().location().lineNo() << "</td></tr>\n";
        }
        out() << "</table>\n";
    }
    generateFooter();
    endSubPage();
    return fileName;
}

/*!
  This function writes an html file containing a list of
  links to broken links that originate in the current
  module and go nowwhere. It returns the name of the file
  it generates, and it sets \a count to the number of
  broken links that were found. The \a marker is used for
  the same thing the marker is always used for.
 */
QString HtmlGenerator::generateLinksToBrokenLinksPage(CodeMarker* marker, int& count)
{
    QString fileName;
    NamespaceNode* node = qdb_->primaryTreeRoot();
    TargetList* tlist = qdb_->getTargetList("broken");
    if (tlist && !tlist->isEmpty()) {
        count = tlist->size();
        fileName = "aaa-links-to-broken-links.html";
        beginSubPage(node, fileName);
        QString title = "Broken links in " + defaultModuleName();
        generateHeader(title, node, marker);
        generateTitle(title, Text(), SmallSubTitle, node, marker);
        out() << "<p>This is a list of broken links in " << defaultModuleName() << ".  ";
        out() << "Click on a link to go to the broken link.  ";
        out() << "The link's target could not be found.</p>\n";
        out() << "<table class=\"valuelist\"><tr valign=\"top\" class=\"odd\"><th class=\"tblConst\">Link to broken link...</th><th class=\"tblval\">In file...</th><th class=\"tbldscr\">Somewhere after line number...</th></tr>\n";
        foreach (TargetLoc* t, *tlist) {
            // e.g.: <a name="link-8421"></a><a href="layout.html">Layout Management</a>
            out() << "<tr><td class=\"topAlign\">";
            out() << "<a href=\"" << t->fileName_ << "#" << t->target_ << "\">";
            out() << t->text_ << "</a></td>";
            out() << "<td class=\"topAlign\">";
            QString f = t->loc_->doc().location().filePath();
            out() << f << "</td>";
            out() << "<td class=\"topAlign\">";
            out() << t->loc_->doc().location().lineNo() << "</td></tr>\n";
        }
        out() << "</table>\n";
        generateFooter();
        endSubPage();
    }
    return fileName;
}

/*!
  Generate html from an instance of Atom.
 */
int HtmlGenerator::generateAtom(const Atom *atom, const Node *relative, CodeMarker *marker)
{
    int idx, skipAhead = 0;
    static bool in_para = false;

    switch (atom->type()) {
    case Atom::AutoLink:
    case Atom::NavAutoLink:
        if (!inLink_ && !inContents_ && !inSectionHeading_) {
            const Node *node = 0;
            QString link = getAutoLink(atom, relative, &node);
            if (link.isEmpty()) {
                if (autolinkErrors())
                    relative->doc().location().warning(tr("Can't autolink to '%1'").arg(atom->string()));
            }
            else if (node && node->status() == Node::Obsolete) {
                if ((relative->parent() != node) && !relative->isObsolete())
                    link.clear();
            }
            if (link.isEmpty()) {
                out() << protectEnc(atom->string());
            }
            else {
                if (Generator::writeQaPages() && node && (atom->type() != Atom::NavAutoLink)) {
                    QString text = atom->string();
                    QString target = qdb_->getNewLinkTarget(relative, node, outFileName(), text);
                    out() << "<a id=\"" << Doc::canonicalTitle(target) << "\" class=\"qa-mark\"></a>";
                }
                beginLink(link, node, relative);
                generateLink(atom, marker);
                endLink();
            }
        }
        else {
            out() << protectEnc(atom->string());
        }
        break;
    case Atom::BaseName:
        break;
    case Atom::BriefLeft:
        if (!hasBrief(relative)) {
            skipAhead = skipAtoms(atom, Atom::BriefRight);
            break;
        }
        out() << "<p>";
        if (relative->type() == Node::Property ||
                relative->type() == Node::Variable) {
            atom = atom->next();
            if (atom != 0 && atom->type() == Atom::String) {
                QString firstWord = atom->string().toLower().section(' ', 0, 0, QString::SectionSkipEmpty);
                if (firstWord == QLatin1String("the")
                      || firstWord == QLatin1String("a")
                      || firstWord == QLatin1String("an")
                      || firstWord == QLatin1String("whether")
                      || firstWord == QLatin1String("which")) {
                    QString str = "This ";
                    if (relative->type() == Node::Property)
                        str += "property holds ";
                    else
                        str += "variable holds ";
                    str += atom->string().left(1).toLower() + atom->string().mid(1);
                    const_cast<Atom *>(atom)->setString(str);
                }
            }
        }
        break;
    case Atom::BriefRight:
        if (hasBrief(relative))
            out() << "</p>\n";
        break;
    case Atom::C:
        // This may at one time have been used to mark up C++ code but it is
        // now widely used to write teletype text. As a result, text marked
        // with the \c command is not passed to a code marker.
        out() << formattingLeftMap()[ATOM_FORMATTING_TELETYPE];
        if (inLink_) {
            out() << protectEnc(plainCode(atom->string()));
        }
        else {
            out() << protectEnc(plainCode(atom->string()));
        }
        out() << formattingRightMap()[ATOM_FORMATTING_TELETYPE];
        break;
    case Atom::CaptionLeft:
        out() << "<p class=\"figCaption\">";
        in_para = true;
        break;
    case Atom::CaptionRight:
        endLink();
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        break;
    case Atom::Qml:
        out() << "<pre class=\"qml\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative, false, Node::QML), codePrefix, codeSuffix)
              << "</pre>\n";
        break;
    case Atom::JavaScript:
        out() << "<pre class=\"js\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative, false, Node::JS), codePrefix, codeSuffix)
              << "</pre>\n";
        break;
    case Atom::CodeNew:
        out() << "<p>you can rewrite it as</p>\n";
        Q_FALLTHROUGH();
    case Atom::Code:
        out() << "<pre class=\"cpp\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,atom->string()),relative), codePrefix, codeSuffix)
              << "</pre>\n";
        break;
    case Atom::CodeOld:
        out() << "<p>For example, if you have code like</p>\n";
        Q_FALLTHROUGH();
    case Atom::CodeBad:
        out() << "<pre class=\"cpp plain\">"
              << trimmedTrailing(protectEnc(plainCode(indent(codeIndent,atom->string()))), codePrefix, codeSuffix)
              << "</pre>\n";
        break;
    case Atom::DivLeft:
        out() << "<div";
        if (!atom->string().isEmpty())
            out() << ' ' << atom->string();
        out() << '>';
        break;
    case Atom::DivRight:
        out() << "</div>";
        break;
    case Atom::FootnoteLeft:
        // ### For now
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        out() << "<!-- ";
        break;
    case Atom::FootnoteRight:
        // ### For now
        out() << "-->";
        break;
    case Atom::FormatElse:
    case Atom::FormatEndif:
    case Atom::FormatIf:
        break;
    case Atom::FormattingLeft:
        if (atom->string().startsWith("span ")) {
            out() << '<' + atom->string() << '>';
        }
        else
            out() << formattingLeftMap()[atom->string()];
        if (atom->string() == ATOM_FORMATTING_PARAMETER) {
            if (atom->next() != 0 && atom->next()->type() == Atom::String) {
                QRegExp subscriptRegExp("([a-z]+)_([0-9n])");
                if (subscriptRegExp.exactMatch(atom->next()->string())) {
                    out() << subscriptRegExp.cap(1) << "<sub>"
                          << subscriptRegExp.cap(2) << "</sub>";
                    skipAhead = 1;
                }
            }
        }
        break;
    case Atom::FormattingRight:
        if (atom->string() == ATOM_FORMATTING_LINK) {
            endLink();
        }
        else if (atom->string().startsWith("span ")) {
            out() << "</span>";
        }
        else {
            out() << formattingRightMap()[atom->string()];
        }
        break;
    case Atom::AnnotatedList:
        {
            const CollectionNode* cn = qdb_->getCollectionNode(atom->string(), Node::DOC);
            if (cn)
                generateList(cn, marker, atom->string());
        }
        break;
    case Atom::GeneratedList:
        if (atom->string() == QLatin1String("annotatedclasses")) {
            generateAnnotatedList(relative, marker, qdb_->getCppClasses());
        }
        else if (atom->string() == QLatin1String("annotatedexamples")) {
            generateAnnotatedLists(relative, marker, qdb_->getExamples());
        }
        else if (atom->string() == QLatin1String("annotatedattributions")) {
            generateAnnotatedLists(relative, marker, qdb_->getAttributions());
        }
        else if (atom->string() == QLatin1String("classes")) {
            generateCompactList(Generic, relative, qdb_->getCppClasses(), true, QStringLiteral(""));
        }
        else if (atom->string().contains("classes ")) {
            QString rootName = atom->string().mid(atom->string().indexOf("classes") + 7).trimmed();
            generateCompactList(Generic, relative, qdb_->getCppClasses(), true, rootName);
        }
        else if (atom->string() == QLatin1String("qmlbasictypes")) {
            generateCompactList(Generic, relative, qdb_->getQmlBasicTypes(), true, QStringLiteral(""));
        }
        else if (atom->string() == QLatin1String("qmltypes")) {
            generateCompactList(Generic, relative, qdb_->getQmlTypes(), true, QStringLiteral(""));
        }
        else if ((idx = atom->string().indexOf(QStringLiteral("bymodule"))) != -1) {
            QString moduleName = atom->string().mid(idx + 8).trimmed();
            Node::Genus genus = Node::CPP;
            if (atom->string().startsWith(QLatin1String("qml")))
                genus = Node::QML;
            else if (atom->string().startsWith(QLatin1String("js")))
                genus = Node::JS;
            else if (atom->string().startsWith(QLatin1String("groups")))
                genus = Node::DOC;
            QDocDatabase* qdb = QDocDatabase::qdocDB();
            const CollectionNode* cn = qdb->getCollectionNode(moduleName, genus);
            if (cn) {
                if (genus == Node::CPP) {
                    NodeMap m;
                    cn->getMemberClasses(m);
                    if (!m.isEmpty()) {
                        generateAnnotatedList(relative, marker, m);
                    }
                }
                else
                    generateAnnotatedList(relative, marker, cn->members());
            }
        }
        else if (atom->string().startsWith("examplefiles") ||
                 atom->string().startsWith("exampleimages")) {
            if (relative->isExample()) {
                Node::DocSubtype subType = (atom->string().mid(7,5) == "image") ? Node::Image : Node::File;
                QString regExp;
                int secondArg = atom->string().indexOf(" ");
                if (secondArg != -1)
                    regExp = atom->string().mid(++secondArg);
                generateFileList(static_cast<const DocumentNode*>(relative), marker, subType, regExp);
            }
            else
                relative->location().warning(QString("'\\generatelist \1' can only be used with '\\example' topic command").arg(atom->string()));
        }
        else if (atom->string() == QLatin1String("classhierarchy")) {
            generateClassHierarchy(relative, qdb_->getCppClasses());
        }
        else if (atom->string() == QLatin1String("obsoleteclasses")) {
            generateCompactList(Generic, relative, qdb_->getObsoleteClasses(), false, QStringLiteral("Q"));
        }
        else if (atom->string() == QLatin1String("obsoleteqmltypes")) {
            generateCompactList(Generic, relative, qdb_->getObsoleteQmlTypes(), false, QStringLiteral(""));
        }
        else if (atom->string() == QLatin1String("obsoletecppmembers")) {
            generateCompactList(Obsolete, relative, qdb_->getClassesWithObsoleteMembers(), false, QStringLiteral("Q"));
        }
        else if (atom->string() == QLatin1String("obsoleteqmlmembers")) {
            generateCompactList(Obsolete, relative, qdb_->getQmlTypesWithObsoleteMembers(), false, QStringLiteral(""));
        }
        else if (atom->string() == QLatin1String("functionindex")) {
            generateFunctionIndex(relative);
        }
        else if (atom->string() == QLatin1String("attributions")) {
            generateAnnotatedList(relative, marker, qdb_->getAttributions());
        }
        else if (atom->string() == QLatin1String("legalese")) {
            generateLegaleseList(relative, marker);
        }
        else if (atom->string() == QLatin1String("overviews")) {
            generateList(relative, marker, "overviews");
        }
        else if (atom->string() == QLatin1String("cpp-modules")) {
            generateList(relative, marker, "cpp-modules");
        }
        else if (atom->string() == QLatin1String("qml-modules")) {
            generateList(relative, marker, "qml-modules");
        }
        else if (atom->string() == QLatin1String("namespaces")) {
            generateAnnotatedList(relative, marker, qdb_->getNamespaces());
        }
        else if (atom->string() == QLatin1String("related")) {
            generateList(relative, marker, "related");
        }
#if 0
        /*
          This is not used in Qt5, as of 10/02/2014
          Remove permanently if it is not missed.
         */
        else if (atom->string() == QLatin1String("relatedinline")) {
            const DocumentNode *dn = static_cast<const DocumentNode *>(relative);
            if (dn && !dn->members().isEmpty()) {
                // Reverse the list into the original scan order.
                // Should be sorted.  But on what?  It may not be a
                // regular class or page definition.
                QList<const Node *> list;
                foreach (const Node *node, dn->members())
                    list.prepend(node);
                foreach (const Node *node, list)
                    generateBody(node, marker);
            }
        }
#endif
        break;
    case Atom::SinceList:
        {
            const NodeMultiMap& nsmap = qdb_->getSinceMap(atom->string());
            if (nsmap.isEmpty())
                break;

            const NodeMap& ncmap = qdb_->getClassMap(atom->string());
            const NodeMap& nqcmap = qdb_->getQmlTypeMap(atom->string());

            Sections sections(nsmap);
            out() << "<ul>\n";
            QVector<Section>::ConstIterator s = sections.sinceSections().constBegin();
            while (s != sections.sinceSections().constEnd()) {
                if (!s->members().isEmpty()) {
                    out() << "<li>"
                          << "<a href=\"#"
                          << Doc::canonicalTitle(s->title())
                          << "\">"
                          << s->title()
                          << "</a></li>\n";
                }
                ++s;
            }
            out() << "</ul>\n";

            int idx = 0;
            s = sections.sinceSections().constBegin();
            while (s != sections.sinceSections().constEnd()) {
                if (!s->members().isEmpty()) {
                    out() << "<a name=\""
                          << Doc::canonicalTitle(s->title())
                          << "\"></a>\n";
                    out() << "<h3>" << protectEnc(s->title()) << "</h3>\n";
                    if (idx == Sections::SinceClasses)
                        generateCompactList(Generic, 0, ncmap, false, QStringLiteral("Q"));
                    else if (idx == Sections::SinceQmlTypes)
                        generateCompactList(Generic, 0, nqcmap, false, QStringLiteral(""));
                    else if (idx == Sections::SinceMemberFunctions) {
                        ParentMaps parentmaps;
                        ParentMaps::iterator pmap;
                        NodeVector::const_iterator i = s->members().constBegin();
                        while (i != s->members().constEnd()) {
                            Node* p = (*i)->parent();
                            pmap = parentmaps.find(p);
                            if (pmap == parentmaps.end())
                                pmap = parentmaps.insert(p,NodeMultiMap());
                            pmap->insert((*i)->name(),(*i));
                            ++i;
                        }
                        pmap = parentmaps.begin();
                        while (pmap != parentmaps.end()) {
                            NodeVector nv = pmap->values().toVector();
                            out() << "<p>Class ";

                            out() << "<a href=\""
                                  << linkForNode(pmap.key(), 0)
                                  << "\">";
                            QStringList pieces = pmap.key()->fullName().split("::");
                            out() << protectEnc(pieces.last());
                            out() << "</a>"  << ":</p>\n";

                            generateSection(nv, 0, marker);
                            out() << "<br/>";
                            ++pmap;
                        }
                    }
                    else
                        generateSection(s->members(), 0, marker);
                }
                ++idx;
                ++s;
            }
        }
        break;
    case Atom::BR:
        out() << "<br />\n";
        break;
    case Atom::HR:
        out() << "<hr />\n";
        break;
    case Atom::Image:
    case Atom::InlineImage:
    {
        QString fileName = imageFileName(relative, atom->string());
        QString text;
        if (atom->next() != 0)
            text = atom->next()->string();
        if (atom->type() == Atom::Image)
            out() << "<p class=\"centerAlign\">";
        if (fileName.isEmpty()) {
            relative->location().warning(tr("Missing image: %1").arg(protectEnc(atom->string())));
            out() << "<font color=\"red\">[Missing image "
                  << protectEnc(atom->string()) << "]</font>";
        }
        else {
            QString prefix;
            out() << "<img src=\"" << protectEnc(prefix + fileName) << '"';
            if (!text.isEmpty())
                out() << " alt=\"" << protectEnc(text) << '"';
            else
                out() << " alt=\"\"";
            out() << " />";
            helpProjectWriter->addExtraFile(fileName);
            if (relative->isExample()) {
                const ExampleNode* cen = static_cast<const ExampleNode*>(relative);
                if (cen->imageFileName().isEmpty()) {
                    ExampleNode* en = const_cast<ExampleNode*>(cen);
                    en->setImageFileName(fileName);
                }
            }
        }
        if (atom->type() == Atom::Image)
            out() << "</p>";
    }
        break;
    case Atom::ImageText:
        break;
    case Atom::ImportantLeft:
        out() << "<p>";
        out() << formattingLeftMap()[ATOM_FORMATTING_BOLD];
        out() << "Important: ";
        out() << formattingRightMap()[ATOM_FORMATTING_BOLD];
        break;
    case Atom::ImportantRight:
        out() << "</p>";
        break;
    case Atom::NoteLeft:
        out() << "<p>";
        out() << formattingLeftMap()[ATOM_FORMATTING_BOLD];
        out() << "Note: ";
        out() << formattingRightMap()[ATOM_FORMATTING_BOLD];
        break;
    case Atom::NoteRight:
        out() << "</p>";
        break;
    case Atom::LegaleseLeft:
        out() << "<div class=\"LegaleseLeft\">";
        break;
    case Atom::LegaleseRight:
        out() << "</div>";
        break;
    case Atom::LineBreak:
        out() << "<br/>";
        break;
    case Atom::Link:
    case Atom::NavLink:
    {
        inObsoleteLink = false;
        const Node *node = 0;
        QString link = getLink(atom, relative, &node);
        if (link.isEmpty() && (node != relative) && !noLinkErrors()) {
            relative->doc().location().warning(tr("Can't link to '%1'").arg(atom->string()));
            if (Generator::writeQaPages() && (atom->type() != Atom::NavAutoLink)) {
                QString text = atom->next()->next()->string();
                QString target = qdb_->getNewLinkTarget(relative, node, outFileName(), text, true);
                out() << "<a id=\"" << Doc::canonicalTitle(target) << "\" class=\"qa-mark\"></a>";
            }
        }
        else {
            if (Generator::writeQaPages() && node && (atom->type() != Atom::NavLink)) {
                QString text = atom->next()->next()->string();
                QString target = qdb_->getNewLinkTarget(relative, node, outFileName(), text);
                out() << "<a id=\"" << Doc::canonicalTitle(target) << "\" class=\"qa-mark\"></a>";
            }
            /*
              mws saw this on 17/10/2014.
              Is this correct? Setting node to 0 means the
              following test always fails. Did we decide to
              no longer warn about linking to obsolete things?
             */
            node = 0;
            if (node && node->status() == Node::Obsolete) {
                if ((relative->parent() != node) && !relative->isObsolete()) {
                    inObsoleteLink = true;
                    if (obsoleteLinks) {
                        relative->doc().location().warning(tr("Link to obsolete item '%1' in %2")
                                                           .arg(atom->string())
                                                           .arg(relative->plainFullName()));
                    }
                }
            }
        }
        beginLink(link, node, relative);
        skipAhead = 1;
    }
        break;
    case Atom::LinkNode:
    {
        const Node *node = CodeMarker::nodeForString(atom->string());
        beginLink(linkForNode(node, relative), node, relative);
        skipAhead = 1;
    }
        break;
    case Atom::ListLeft:
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        if (atom->string() == ATOM_LIST_BULLET) {
            out() << "<ul>\n";
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            out() << "<dl>\n";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            out() << "<div class=\"table\"><table class=\"valuelist\">";
            threeColumnEnumValueTable_ = isThreeColumnEnumValueTable(atom);
            if (threeColumnEnumValueTable_) {
                if (++numTableRows_ % 2 == 1)
                    out() << "<tr valign=\"top\" class=\"odd\">";
                else
                    out() << "<tr valign=\"top\" class=\"even\">";

                out() << "<th class=\"tblConst\">Constant</th>";

                // If not in \enum topic, skip the value column
                if (relative->type() == Node::Enum)
                    out() << "<th class=\"tblval\">Value</th>";

                out() << "<th class=\"tbldscr\">Description</th></tr>\n";
            }
            else {
                out() << "<tr><th class=\"tblConst\">Constant</th><th class=\"tblVal\">Value</th></tr>\n";
            }
        }
        else {
            QString olType;
            if (atom->string() == ATOM_LIST_UPPERALPHA) {
                olType = "A";
            }
            else if (atom->string() == ATOM_LIST_LOWERALPHA) {
                olType = "a";
            }
            else if (atom->string() == ATOM_LIST_UPPERROMAN) {
                olType = "I";
            }
            else if (atom->string() == ATOM_LIST_LOWERROMAN) {
                olType = "i";
            }
            else { // (atom->string() == ATOM_LIST_NUMERIC)
                olType = "1";
            }

            if (atom->next() != 0 && atom->next()->string().toInt() > 1) {
                out() << QString("<ol class=\"%1\" type=\"%1\" start=\"%2\">").arg(olType)
                         .arg(atom->next()->string());
            }
            else
                out() << QString("<ol class=\"%1\" type=\"%1\">").arg(olType);
        }
        break;
    case Atom::ListItemNumber:
        break;
    case Atom::ListTagLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            out() << "<dt>";
        }
        else { // (atom->string() == ATOM_LIST_VALUE)
            const Atom* lookAhead = atom->next();
            QString t = lookAhead->string();
            lookAhead = lookAhead->next();
            Q_ASSERT(lookAhead->type() == Atom::ListTagRight);
            lookAhead = lookAhead->next();
            if (lookAhead && lookAhead->type() == Atom::SinceTagLeft) {
                lookAhead = lookAhead->next();
                Q_ASSERT(lookAhead && lookAhead->type() == Atom::String);
                t = t + QLatin1String(" (since ");
                if (lookAhead->string().at(0).isDigit())
                    t = t + QLatin1String("Qt ");
                t = t + lookAhead->string() + QLatin1String(")");
                skipAhead = 4;
            }
            else {
                skipAhead = 1;
            }
            t = protectEnc(plainCode(marker->markedUpEnumValue(t, relative)));
            out() << "<tr><td class=\"topAlign\"><code>" << t << "</code>";

            if (relative->type() == Node::Enum) {
                out() << "</td><td class=\"topAlign tblval\">";
                const EnumNode *enume = static_cast<const EnumNode *>(relative);
                QString itemValue = enume->itemValue(atom->next()->string());
                if (itemValue.isEmpty())
                    out() << '?';
                else
                    out() << "<code>" << protectEnc(itemValue) << "</code>";
            }
        }
        break;
    case Atom::SinceTagRight:
    case Atom::ListTagRight:
        if (atom->string() == ATOM_LIST_TAG)
            out() << "</dt>\n";
        break;
    case Atom::ListItemLeft:
        if (atom->string() == ATOM_LIST_TAG) {
            out() << "<dd>";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            if (threeColumnEnumValueTable_) {
                out() << "</td><td class=\"topAlign\">";
                if (matchAhead(atom, Atom::ListItemRight))
                    out() << "&nbsp;";
            }
        }
        else {
            out() << "<li>";
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::ListItemRight:
        if (atom->string() == ATOM_LIST_TAG) {
            out() << "</dd>\n";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            out() << "</td></tr>\n";
        }
        else {
            out() << "</li>\n";
        }
        break;
    case Atom::ListRight:
        if (atom->string() == ATOM_LIST_BULLET) {
            out() << "</ul>\n";
        }
        else if (atom->string() == ATOM_LIST_TAG) {
            out() << "</dl>\n";
        }
        else if (atom->string() == ATOM_LIST_VALUE) {
            out() << "</table></div>\n";
        }
        else {
            out() << "</ol>\n";
        }
        break;
    case Atom::Nop:
        break;
    case Atom::ParaLeft:
        out() << "<p>";
        in_para = true;
        break;
    case Atom::ParaRight:
        endLink();
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        //if (!matchAhead(atom, Atom::ListItemRight) && !matchAhead(atom, Atom::TableItemRight))
        //    out() << "</p>\n";
        break;
    case Atom::QuotationLeft:
        out() << "<blockquote>";
        break;
    case Atom::QuotationRight:
        out() << "</blockquote>\n";
        break;
    case Atom::RawString:
        out() << atom->string();
        break;
    case Atom::SectionLeft:
        out() << "<a name=\"" << Doc::canonicalTitle(Text::sectionHeading(atom).toString())
              << "\"></a>" << divNavTop << '\n';
        break;
    case Atom::SectionRight:
        break;
    case Atom::SectionHeadingLeft: {
        int unit = atom->string().toInt() + hOffset(relative);
        out() << "<h" + QString::number(unit) + QLatin1Char(' ')
              << "id=\"" << Doc::canonicalTitle(Text::sectionHeading(atom).toString()) << "\">";
        inSectionHeading_ = true;
        break;
    }
    case Atom::SectionHeadingRight:
        out() << "</h" + QString::number(atom->string().toInt() + hOffset(relative)) + ">\n";
        inSectionHeading_ = false;
        break;
    case Atom::SidebarLeft:
        break;
    case Atom::SidebarRight:
        break;
    case Atom::String:
        if (inLink_ && !inContents_ && !inSectionHeading_) {
            generateLink(atom, marker);
        }
        else {
            out() << protectEnc(atom->string());
        }
        break;
    case Atom::TableLeft:
    {
        QString p1, p2;
        QString attr = "generic";
        QString width;
        if (in_para) {
            out() << "</p>\n";
            in_para = false;
        }
        if (atom->count() > 0) {
            p1 = atom->string(0);
            if (atom->count() > 1)
                p2 = atom->string(1);
        }
        if (!p1.isEmpty()) {
            if (p1 == QLatin1String("borderless"))
                attr = p1;
            else if (p1.contains(QLatin1Char('%')))
                width = p1;
        }
        if (!p2.isEmpty()) {
            if (p2 == QLatin1String("borderless"))
                attr = p2;
            else if (p2.contains(QLatin1Char('%')))
                width = p2;
        }
        out() << "<div class=\"table\"><table class=\"" << attr << '"';
        if (!width.isEmpty())
            out() << " width=\"" << width << '"';
        out() << ">\n ";
        numTableRows_ = 0;
    }
        break;
    case Atom::TableRight:
        out() << "</table></div>\n";
        break;
    case Atom::TableHeaderLeft:
        out() << "<thead><tr class=\"qt-style\">";
        inTableHeader_ = true;
        break;
    case Atom::TableHeaderRight:
        out() << "</tr>";
        if (matchAhead(atom, Atom::TableHeaderLeft)) {
            skipAhead = 1;
            out() << "\n<tr class=\"qt-style\">";
        }
        else {
            out() << "</thead>\n";
            inTableHeader_ = false;
        }
        break;
    case Atom::TableRowLeft:
        if (!atom->string().isEmpty())
            out() << "<tr " << atom->string() << '>';
        else if (++numTableRows_ % 2 == 1)
            out() << "<tr valign=\"top\" class=\"odd\">";
        else
            out() << "<tr valign=\"top\" class=\"even\">";
        break;
    case Atom::TableRowRight:
        out() << "</tr>\n";
        break;
    case Atom::TableItemLeft:
    {
        if (inTableHeader_)
            out() << "<th ";
        else
            out() << "<td ";

        for (int i=0; i<atom->count(); ++i) {
            if (i > 0)
                out() << ' ';
            QString p = atom->string(i);
            if (p.contains('=')) {
                out() << p;
            }
            else {
                QStringList spans = p.split(QLatin1Char(','));
                if (spans.size() == 2) {
                    if (spans.at(0) != "1")
                        out() << " colspan=\"" << spans.at(0) << '"';
                    if (spans.at(1) != "1")
                        out() << " rowspan=\"" << spans.at(1) << '"';
                }
            }
        }
        if (inTableHeader_)
            out() << '>';
        else {
            out() << '>';
            //out() << "><p>";
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
    }
        break;
    case Atom::TableItemRight:
        if (inTableHeader_)
            out() << "</th>";
        else {
            out() << "</td>";
            //out() << "</p></td>";
        }
        if (matchAhead(atom, Atom::ParaLeft))
            skipAhead = 1;
        break;
    case Atom::TableOfContents:
        break;
    case Atom::Keyword:
        break;
    case Atom::Target:
        out() << "<a name=\"" << Doc::canonicalTitle(atom->string()) << "\"></a>";
        break;
    case Atom::UnhandledFormat:
        out() << "<b class=\"redFont\">&lt;Missing HTML&gt;</b>";
        break;
    case Atom::UnknownCommand:
        out() << "<b class=\"redFont\"><code>\\" << protectEnc(atom->string())
              << "</code></b>";
        break;
    case Atom::QmlText:
    case Atom::EndQmlText:
        // don't do anything with these. They are just tags.
        break;
    case Atom::CodeQuoteArgument:
    case Atom::CodeQuoteCommand:
    case Atom::SnippetCommand:
    case Atom::SnippetIdentifier:
    case Atom::SnippetLocation:
        // no HTML output (ignore)
        break;
    default:
        unknownAtom(atom);
    }
    return skipAhead;
}

/*!
  Generate a reference page for the C++ class or C++ namespace
  documented in \a node using the \a marker.
 */
void HtmlGenerator::generateCppReferencePage(Node* node, CodeMarker* marker)
{
    Q_ASSERT(node->isAggregate());
    Aggregate* aggregate = static_cast<Aggregate*>(node);

    QString title;
    QString rawTitle;
    QString fullTitle;
    NamespaceNode* ns = 0;
    if (aggregate->isNamespace()) {
        rawTitle = aggregate->plainName();
        fullTitle = aggregate->plainFullName();
        title = rawTitle + " Namespace";
        ns = static_cast<NamespaceNode*>(aggregate);
    }
    else if (aggregate->isClass()) {
        rawTitle = aggregate->plainName();
        fullTitle = aggregate->plainFullName();
        title = rawTitle + " Class";
    }

    Text subtitleText;
    if (rawTitle != fullTitle)
        subtitleText << "(" << Atom(Atom::AutoLink, fullTitle) << ")" << Atom(Atom::LineBreak);

    generateHeader(title, aggregate, marker);

    Sections sections(aggregate);
    SectionVector *sectionVector = ns ? &sections.stdSummarySections() : &sections.stdCppClassSummarySections();
    generateTableOfContents(aggregate, marker, sectionVector);
    generateKeywordAnchors(aggregate);
    generateTitle(title, subtitleText, SmallSubTitle, aggregate, marker);
    if (ns && !ns->hasDoc() && ns->docNode()) {
        NamespaceNode* NS = ns->docNode();
        Text brief;
        brief << "The " << ns->name() << " namespace includes the following elements from module "
              << ns->tree()->camelCaseModuleName() << ". The full namespace is "
              << "documented in module " << NS->tree()->camelCaseModuleName()
              << Atom(Atom::LinkNode, CodeMarker::stringForNode(NS))
              << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
              << Atom(Atom::String, " here.")
              << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        out() << "<p>";
        generateText(brief, ns, marker);
        out() << "</p>\n";
    }
    else
        generateBrief(aggregate, marker);
    generateRequisites(aggregate, marker);
    generateStatus(aggregate, marker);

    out() << "<ul>\n";

    QString membersLink = generateAllMembersFile(Sections::allMembersSection(), marker);
    if (!membersLink.isEmpty())
        out() << "<li><a href=\"" << membersLink << "\">"
              << "List of all members, including inherited members</a></li>\n";

    QString obsoleteLink = generateObsoleteMembersFile(sections, marker);
    if (!obsoleteLink.isEmpty()) {
        out() << "<li><a href=\"" << obsoleteLink << "\">"
              << "Obsolete members</a></li>\n";
    }

    out() << "</ul>\n";
    generateThreadSafeness(aggregate, marker);

    bool needOtherSection = false;

    SectionVector::ConstIterator s = sectionVector->constBegin();
    while (s != sectionVector->constEnd()) {
        if (s->members().isEmpty() && s->reimplementedMembers().isEmpty()) {
            if (!s->inheritedMembers().isEmpty())
                needOtherSection = true;
        }
        else {
            if (!s->members().isEmpty()) {
                QString ref = registerRef(s->title().toLower());
                out() << "<a name=\"" << ref << "\"></a>" << divNavTop << "\n";
                out() << "<h2 id=\"" << ref << "\">" << protectEnc(s->title()) << "</h2>\n";
                generateSection(s->members(), aggregate, marker);
            }
            if (!s->reimplementedMembers().isEmpty()) {
                QString name = QString("Reimplemented ") + s->title();
                QString ref = registerRef(name.toLower());
                out() << "<a name=\"" << ref << "\"></a>" << divNavTop << "\n";
                out() << "<h2 id=\"" << ref << "\">" << protectEnc(name) << "</h2>\n";
                generateSection(s->reimplementedMembers(), aggregate, marker);
            }

            if (!s->inheritedMembers().isEmpty()) {
                out() << "<ul>\n";
                generateSectionInheritedList(*s, aggregate);
                out() << "</ul>\n";
            }
        }
        ++s;
    }

    if (needOtherSection) {
        out() << "<h3>Additional Inherited Members</h3>\n"
                 "<ul>\n";

        s = sectionVector->constBegin();
        while (s != sectionVector->constEnd()) {
            if (s->members().isEmpty() && !s->inheritedMembers().isEmpty())
                generateSectionInheritedList(*s, aggregate);
            ++s;
        }
        out() << "</ul>\n";
    }

    QString detailsRef = registerRef("details");
    out() << "<a name=\"" << detailsRef << "\"></a>" << divNavTop << '\n';

    if (!aggregate->doc().isEmpty()) {
        generateExtractionMark(aggregate, DetailedDescriptionMark);
        out() << "<div class=\"descr\">\n" // QTBUG-9504
              << "<h2 id=\"" << detailsRef << "\">" << "Detailed Description" << "</h2>\n";
        generateBody(aggregate, marker);
        out() << "</div>\n"; // QTBUG-9504
        generateAlsoList(aggregate, marker);
        generateMaintainerList(aggregate, marker);
        generateExtractionMark(aggregate, EndMark);
    }

    sectionVector = ns ? &sections.stdDetailsSections() : &sections.stdCppClassDetailsSections();
    s = sectionVector->constBegin();
    while (s != sectionVector->constEnd()) {
        bool headerGenerated = false;
        NodeVector::ConstIterator m = s->members().constBegin();
        while (m != s->members().constEnd()) {
            if ((*m)->access() == Node::Private) { // ### check necessary?
                ++m;
                continue;
            }
            if (!headerGenerated) {
                if (!s->divClass().isEmpty())
                    out() << "<div class=\"" << s->divClass() << "\">\n"; // QTBUG-9504
                out() << "<h2>" << protectEnc(s->title()) << "</h2>\n";
                headerGenerated = true;
            }
            if ((*m)->type() != Node::Class)
                generateDetailedMember(*m, aggregate, marker);
            else {
                out() << "<h3> class ";
                generateFullName(*m, aggregate);
                out() << "</h3>";
                generateBrief(*m, marker, aggregate);
            }
            QStringList names;
            names << (*m)->name();
            if ((*m)->type() == Node::Function) {
                const FunctionNode *func = reinterpret_cast<const FunctionNode *>(*m);
                if (func->isSomeCtor() || func->isDtor() || func->overloadNumber() != 0)
                    names.clear();
            } else if ((*m)->type() == Node::Property) {
                const PropertyNode *prop = reinterpret_cast<const PropertyNode *>(*m);
                if (!prop->getters().isEmpty() &&
                        !names.contains(prop->getters().first()->name()))
                    names << prop->getters().first()->name();
                if (!prop->setters().isEmpty())
                    names << prop->setters().first()->name();
                if (!prop->resetters().isEmpty())
                    names << prop->resetters().first()->name();
                if (!prop->notifiers().isEmpty())
                    names << prop->notifiers().first()->name();
            } else if ((*m)->type() == Node::Enum) {
                const EnumNode *enume = reinterpret_cast<const EnumNode*>(*m);
                if (enume->flagsType())
                    names << enume->flagsType()->name();

                foreach (const QString &enumName,
                        enume->doc().enumItemNames().toSet() -
                        enume->doc().omitEnumItemNames().toSet())
                    names << plainCode(marker->markedUpEnumValue(enumName,
                                                                 enume));
            }
            ++m;
        }
        if (headerGenerated && !s->divClass().isEmpty())
            out() << "</div>\n"; // QTBUG-9504
        ++s;
    }
    generateFooter(aggregate);
}

/*!
  Generate the HTML page for a QML type. \qcn is the QML type.
  \marker is the code markeup object.
 */
void HtmlGenerator::generateQmlTypePage(QmlTypeNode* qcn, CodeMarker* marker)
{
    Generator::setQmlTypeContext(qcn);
    SubTitleSize subTitleSize = LargeSubTitle;
    QString htmlTitle = qcn->fullTitle();
    if (qcn->isJsType())
        htmlTitle += " JavaScript Type";
    else
        htmlTitle += " QML Type";

    generateHeader(htmlTitle, qcn, marker);
    Sections sections(qcn);
    generateTableOfContents(qcn, marker, &sections.stdQmlTypeSummarySections());
    marker = CodeMarker::markerForLanguage(QLatin1String("QML"));
    generateKeywordAnchors(qcn);
    generateTitle(htmlTitle, Text() << qcn->subTitle(), subTitleSize, qcn, marker);
    generateBrief(qcn, marker);
    generateQmlRequisites(qcn, marker);

    QString allQmlMembersLink = generateAllQmlMembersFile(sections, marker);
    QString obsoleteLink = generateObsoleteQmlMembersFile(sections, marker);
    if (!allQmlMembersLink.isEmpty() || !obsoleteLink.isEmpty()) {
        out() << "<ul>\n";
        if (!allQmlMembersLink.isEmpty()) {
            out() << "<li><a href=\"" << allQmlMembersLink << "\">"
                  << "List of all members, including inherited members</a></li>\n";
        }
        if (!obsoleteLink.isEmpty()) {
            out() << "<li><a href=\"" << obsoleteLink << "\">"
                  << "Obsolete members</a></li>\n";
        }
        out() << "</ul>\n";
    }

    SectionVector::ConstIterator s = sections.stdQmlTypeSummarySections().constBegin();
    while (s != sections.stdQmlTypeSummarySections().constEnd()) {
        if (!s->isEmpty()) {
            QString ref = registerRef(s->title().toLower());
            out() << "<a name=\"" << ref
                  << "\"></a>" << divNavTop << '\n';
            out() << "<h2 id=\"" << ref << "\">" << protectEnc(s->title()) << "</h2>\n";
            generateQmlSummary(*s, qcn, marker);
        }
        ++s;
    }

    generateExtractionMark(qcn, DetailedDescriptionMark);
    QString detailsRef = registerRef("details");
    out() << "<a name=\"" << detailsRef << "\"></a>" << divNavTop << '\n';
    out() << "<h2 id=\"" << detailsRef << "\">" << "Detailed Description" << "</h2>\n";
    generateBody(qcn, marker);
    ClassNode* cn = qcn->classNode();
    if (cn)
        generateQmlText(cn->doc().body(), cn, marker, qcn->name());
    generateAlsoList(qcn, marker);
    generateExtractionMark(qcn, EndMark);
    //out() << "<hr />\n";

    s = sections.stdQmlTypeDetailsSections().constBegin();
    while (s != sections.stdQmlTypeDetailsSections().constEnd()) {
        if (!s->isEmpty()) {
            out() << "<h2>" << protectEnc(s->title()) << "</h2>\n";
            NodeVector::ConstIterator m = s->members().constBegin();
            while (m != s->members().constEnd()) {
                generateDetailedQmlMember(*m, qcn, marker);
                out() << "<br/>\n";
                ++m;
            }
        }
        ++s;
    }
    generateFooter(qcn);
    Generator::setQmlTypeContext(0);
}

/*!
  Generate the HTML page for the QML basic type represented
  by the QML basic type node \a qbtn.
 */
void HtmlGenerator::generateQmlBasicTypePage(QmlBasicTypeNode* qbtn, CodeMarker* marker)
{
    SubTitleSize subTitleSize = LargeSubTitle;
    QString htmlTitle = qbtn->fullTitle();
    if (qbtn->isJsType())
        htmlTitle += " JavaScript Basic Type";
    else
        htmlTitle += " QML Basic Type";

    marker = CodeMarker::markerForLanguage(QLatin1String("QML"));

    generateHeader(htmlTitle, qbtn, marker);
    Sections sections(qbtn);
    generateTableOfContents(qbtn,marker,&sections.stdQmlTypeSummarySections());
    generateKeywordAnchors(qbtn);
    generateTitle(htmlTitle,
                  Text() << qbtn->subTitle(),
                  subTitleSize,
                  qbtn,
                  marker);

    SectionVector::const_iterator s = sections.stdQmlTypeSummarySections().constBegin();
    while (s != sections.stdQmlTypeSummarySections().constEnd()) {
        if (!s->isEmpty()) {
            QString ref = registerRef(s->title().toLower());
            out() << "<a name=\"" << ref
                  << "\"></a>" << divNavTop << '\n';
            out() << "<h2 id=\"" << ref << "\">" << protectEnc(s->title()) << "</h2>\n";
            generateQmlSummary(*s, qbtn, marker);
        }
        ++s;
    }

    generateExtractionMark(qbtn, DetailedDescriptionMark);
    out() << "<div class=\"descr\"> <a name=\"" << registerRef("details") << "\"></a>\n"; // QTBUG-9504

    generateBody(qbtn, marker);
    out() << "</div>\n"; // QTBUG-9504
    generateAlsoList(qbtn, marker);
    generateExtractionMark(qbtn, EndMark);

    s = sections.stdQmlTypeDetailsSections().constBegin();
    while (s != sections.stdQmlTypeDetailsSections().constEnd()) {
        if (!s->isEmpty()) {
            out() << "<h2>" << protectEnc(s->title()) << "</h2>\n";
            NodeVector::ConstIterator m = s->members().constBegin();
            while (m != s->members().constEnd()) {
                generateDetailedQmlMember(*m, qbtn, marker);
                out() << "<br/>\n";
                ++m;
            }
        }
        ++s;
    }
    generateFooter(qbtn);
}

/*!
  Generate the HTML page for an entity that doesn't map
  to any underlying parsable C++ class or QML component.
 */
void HtmlGenerator::generateDocumentNode(DocumentNode* dn, CodeMarker* marker)
{
    /*
      If the document node is a page node, and if the page type
      is DITA map page, write the node's contents as a dita
      map and return without doing anything else.
     */
    if (dn->docSubtype() == Node::Page && dn->pageType() == Node::DitaMapPage) {
        const DitaMapNode* dmn = static_cast<const DitaMapNode*>(dn);
        writeDitaMap(dmn);
        return;
    }

    SubTitleSize subTitleSize = LargeSubTitle;
    Sections sections(dn);
    QString fullTitle = dn->fullTitle();

    generateHeader(fullTitle, dn, marker);
    /*
      Generate the TOC for the new doc format.
      Don't generate a TOC for the home page.
    */
    if ((dn->name() != QLatin1String("index.html")))
        generateTableOfContents(dn,marker,0);

    generateKeywordAnchors(dn);
    generateTitle(fullTitle,
                  Text() << dn->subTitle(),
                  subTitleSize,
                  dn,
                  marker);
    if (dn->isExample()) {
        generateBrief(dn, marker, 0, false);
    } else if (dn->docSubtype() == Node::HeaderFile) {
        // Generate brief text and status for modules.
        generateBrief(dn, marker);
        generateStatus(dn, marker);
        generateSince(dn, marker);

        out() << "<ul>\n";

        QString membersLink = generateAllMembersFile(Sections::allMembersSection(), marker);
        if (!membersLink.isEmpty())
            out() << "<li><a href=\"" << membersLink << "\">"
                  << "List of all members, including inherited members</a></li>\n";

        QString obsoleteLink = generateObsoleteMembersFile(sections, marker);
        if (!obsoleteLink.isEmpty()) {
            out() << "<li><a href=\"" << obsoleteLink << "\">"
                  << "Obsolete members</a></li>\n";
        }

        out() << "</ul>\n";
    }

    SectionVector::const_iterator s = sections.stdSummarySections().constBegin();
    while (s != sections.stdSummarySections().constEnd()) {
        if (!s->isEmpty()) {
            QString ref = registerRef(s->title());
            out() << "<a name=\"" << ref << "\"></a>" << divNavTop << '\n';
            out() << "<h2 id=\"" << ref << "\">" << protectEnc(s->title()) << "</h2>\n";
            generateSectionList(*s, dn, marker);
        }
        ++s;
    }

    generateExtractionMark(dn, DetailedDescriptionMark);
    out() << "<div class=\"descr\"> <a name=\"" << registerRef("details") << "\"></a>\n"; // QTBUG-9504

    generateBody(dn, marker);
    out() << "</div>\n"; // QTBUG-9504
    generateAlsoList(dn, marker);
    generateExtractionMark(dn, EndMark);

    s = sections.stdDetailsSections().constBegin();
    while (s != sections.stdDetailsSections().constEnd()) {
        //out() << "<hr />\n";
        if (!s->isEmpty()) {
            out() << "<h2>" << protectEnc(s->title()) << "</h2>\n";

            NodeVector::ConstIterator m = s->members().constBegin();
            while (m != s->members().constEnd()) {
                generateDetailedMember(*m, dn, marker);
                ++m;
            }
        }
        ++s;
    }
    generateFooter(dn);
}

/*!
  Generate the HTML page for a group, module, or QML module.
 */
void HtmlGenerator::generateCollectionNode(CollectionNode* cn, CodeMarker* marker)
{
    SubTitleSize subTitleSize = LargeSubTitle;
    QString fullTitle = cn->fullTitle();
    QString ref;

    generateHeader(fullTitle, cn, marker);
    generateTableOfContents(cn,marker,0);
    generateKeywordAnchors(cn);
    generateTitle(fullTitle, Text() << cn->subTitle(), subTitleSize, cn, marker);

    if (cn->isModule()) {
        // Generate brief text and status for modules.
        generateBrief(cn, marker);
        generateStatus(cn, marker);
        generateSince(cn, marker);

        NodeMultiMap nmm;
        cn->getMemberNamespaces(nmm);
        if (!nmm.isEmpty()) {
            ref = registerRef("namespaces");
            out() << "<a name=\"" << ref << "\"></a>" << divNavTop << '\n';
            out() << "<h2 id=\"" << ref << "\">Namespaces</h2>\n";
            generateAnnotatedList(cn, marker, nmm);
        }
        nmm.clear();
        cn->getMemberClasses(nmm);
        if (!nmm.isEmpty()) {
            ref = registerRef("classes");
            out() << "<a name=\"" << ref << "\"></a>" << divNavTop << '\n';
            out() << "<h2 id=\"" << ref << "\">Classes</h2>\n";
            generateAnnotatedList(cn, marker, nmm);
        }
        nmm.clear();
    }

    Text brief = cn->doc().briefText();
    if (cn->isModule() && !brief.isEmpty()) {
        generateExtractionMark(cn, DetailedDescriptionMark);
        ref = registerRef("details");
        out() << "<a name=\"" << ref << "\"></a>" << divNavTop << '\n';
        out() << "<div class=\"descr\">\n"; // QTBUG-9504
        out() << "<h2 id=\"" << ref << "\">" << "Detailed Description" << "</h2>\n";
    }
    else {
        generateExtractionMark(cn, DetailedDescriptionMark);
        out() << "<div class=\"descr\"> <a name=\"" << registerRef("details") << "\"></a>\n"; // QTBUG-9504
    }

    generateBody(cn, marker);
    out() << "</div>\n"; // QTBUG-9504
    generateAlsoList(cn, marker);
    generateExtractionMark(cn, EndMark);

    if (!cn->noAutoList()) {
        if (cn->isGroup())
            generateAnnotatedList(cn, marker, cn->members());
        else if (cn->isQmlModule() || cn->isJsModule())
            generateAnnotatedList(cn, marker, cn->members());
    }
    generateFooter(cn);
}

/*!
  Returns "html" for this subclass of Generator.
 */
QString HtmlGenerator::fileExtension() const
{
    return "html";
}

/*!
  Output navigation list in the html file.
 */
void HtmlGenerator::generateNavigationBar(const QString &title,
                                          const Node *node,
                                          CodeMarker *marker,
                                          const QString &buildversion,
                                          bool tableItems)
{
    if (noNavigationBar)
        return;

    Text navigationbar;

    // Set list item types based on the navigation bar type
    Atom::AtomType itemLeft = tableItems ?
                Atom::TableItemLeft : Atom::ListItemLeft;
    Atom::AtomType itemRight = tableItems ?
                Atom::TableItemRight : Atom::ListItemRight;

    if (hometitle == title)
        return;
    if (!homepage.isEmpty())
        navigationbar << Atom(itemLeft)
                      << Atom(Atom::NavLink, homepage)
                      << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                      << Atom(Atom::String, hometitle)
                      << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                      << Atom(itemRight);
    if (!landingpage.isEmpty() && landingtitle != title)
        navigationbar << Atom(itemLeft)
                    << Atom(Atom::NavLink, landingpage)
                    << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                    << Atom(Atom::String, landingtitle)
                    << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                    << Atom(itemRight);

    if (node->isClass()) {
        if (!cppclassespage.isEmpty() && !cppclassestitle.isEmpty())
            navigationbar << Atom(itemLeft)
                        << Atom(Atom::NavLink, cppclassespage)
                        << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                        << Atom(Atom::String, cppclassestitle)
                        << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                        << Atom(itemRight);

        if (!node->name().isEmpty())
            navigationbar << Atom(itemLeft)
                        << Atom(Atom::String, node->name())
                        << Atom(itemRight);
    }
    else if (node->isQmlType() || node->isQmlBasicType() ||
             node->isJsType() || node->isJsBasicType()) {
        if (!qmltypespage.isEmpty() && !qmltypestitle.isEmpty())
            navigationbar << Atom(itemLeft)
                          << Atom(Atom::NavLink, qmltypespage)
                          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                          << Atom(Atom::String, qmltypestitle)
                          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                          << Atom(itemRight)
                          << Atom(itemLeft)
                          << Atom(Atom::String, title)
                          << Atom(itemRight);
    }
    else {
        if (node->isExampleFile()) {
            navigationbar << Atom(itemLeft)
                          << Atom(Atom::NavLink, node->parent()->name())
                          << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                          << Atom(Atom::String, node->parent()->title())
                          << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                          << Atom(itemRight);

        } else if (node->isAggregate()) {
            QStringList groups = static_cast<const Aggregate*>(node)->groupNames();
            if (groups.length() == 1) {
                const Node *groupNode = qdb_->findNodeByNameAndType(QStringList(groups[0]), Node::Group);
                if (groupNode && !groupNode->title().isEmpty()) {
                    navigationbar << Atom(itemLeft)
                                  << Atom(Atom::NavLink, groupNode->name())
                                  << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                                  << Atom(Atom::String, groupNode->title())
                                  << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
                                  << Atom(itemRight);
                }
            }
        }
        navigationbar << Atom(itemLeft)
                      << Atom(Atom::String, title)
                      << Atom(itemRight);
    }

    generateText(navigationbar, node, marker);

    if (buildversion.isEmpty())
        return;

    navigationbar.clear();

     if (tableItems) {
        out() << "</tr></table><table class=\"buildversion\"><tr>\n"
              << "<td id=\"buildversion\" width=\"100%\" align=\"right\">";
    } else {
        out() << "<li id=\"buildversion\">";
    }

    // Link buildversion string to navigation.landingpage
    if (!landingpage.isEmpty() && landingtitle != title) {
        navigationbar << Atom(Atom::NavLink, landingpage)
                      << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                      << Atom(Atom::String, buildversion)
                      << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        generateText(navigationbar, node, marker);
    } else {
        out() << buildversion;
    }
    if (tableItems)
        out() << "</td>\n";
    else
        out() << "</li>\n";
}

void HtmlGenerator::generateHeader(const QString& title,
                                   const Node *node,
                                   CodeMarker *marker)
{
#ifndef QT_NO_TEXTCODEC
    out() << QString("<?xml version=\"1.0\" encoding=\"%1\"?>\n").arg(outputEncoding);
#else
    out() << QString("<?xml version=\"1.0\"?>\n");
#endif
    out() << "<!DOCTYPE html>\n";
    out() << QString("<html lang=\"%1\">\n").arg(naturalLanguage);
    out() << "<head>\n";
    out() << "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
    if (node && !node->doc().location().isEmpty())
        out() << "<!-- " << node->doc().location().fileName() << " -->\n";

    //determine the rest of the <title> element content: "title | titleSuffix version"
    QString titleSuffix;
    if (!landingtitle.isEmpty()) {
        //for normal pages: "title | landingtitle version"
        titleSuffix = landingtitle;
    }
    else if (!hometitle.isEmpty()) {
        // for pages that set the homepage title but not landing page title:
        // "title | hometitle version"
        if (title != hometitle)
            titleSuffix = hometitle;
    }
    else if (!project.isEmpty()) {
        //for projects outside of Qt or Qt 5: "title | project version"
        if (title != project)
            titleSuffix = project;
    }
    else
        //default: "title | Qt version"
        titleSuffix = QLatin1String("Qt ");

    if (title == titleSuffix)
        titleSuffix.clear();

    QString divider;
    if (!titleSuffix.isEmpty() && !title.isEmpty())
        divider = QLatin1String(" | ");

    // Generating page title
    out() << "  <title>"
          << protectEnc(title)
          << divider
          << titleSuffix;

    // append a full version to the suffix if neither suffix nor title
    // include (a prefix of) version information
    QVersionNumber projectVersion = QVersionNumber::fromString(qdb_->version());
    if (!projectVersion.isNull()) {
        QVersionNumber titleVersion;
        QRegExp re("\\d+\\.\\d+");
        const QString &versionedTitle = titleSuffix.isEmpty() ? title : titleSuffix;
        if (versionedTitle.contains(re))
            titleVersion = QVersionNumber::fromString(re.cap());
        if (titleVersion.isNull() || !titleVersion.isPrefixOf(projectVersion))
            out() << QLatin1Char(' ') << projectVersion.toString();
    }
    out() << "</title>\n";

    // Include style sheet and script links.
    out() << headerStyles;
    out() << headerScripts;
    if (endHeader.isEmpty())
        out() << "</head>\n<body>\n";
    else
        out() << endHeader;

#ifdef GENERATE_MAC_REFS
    if (mainPage)
        generateMacRef(node, marker);
#endif

    out() << QString(postHeader).replace("\\" + COMMAND_VERSION, qdb_->version());
    bool usingTable = postHeader.trimmed().endsWith(QLatin1String("<tr>"));
    generateNavigationBar(title, node, marker, buildversion, usingTable);
    out() << QString(postPostHeader).replace("\\" + COMMAND_VERSION, qdb_->version());

    navigationLinks.clear();
    refMap.clear();

    if (node && !node->links().empty()) {
        QPair<QString,QString> linkPair;
        QPair<QString,QString> anchorPair;
        const Node *linkNode;
        bool useSeparator = false;

        if (node->links().contains(Node::PreviousLink)) {
            linkPair = node->links()[Node::PreviousLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (!linkNode || linkNode == node)
                anchorPair = linkPair;
            else
                anchorPair = anchorForNode(linkNode);

            out() << "  <link rel=\"prev\" href=\""
                  << anchorPair.first << "\" />\n";

            navigationLinks += "<a class=\"prevPage\" href=\"" + anchorPair.first + "\">";
            if (linkPair.first == linkPair.second && !anchorPair.second.isEmpty())
                navigationLinks += protect(anchorPair.second);
            else
                navigationLinks += protect(linkPair.second);
            navigationLinks += "</a>\n";
            useSeparator = !navigationSeparator.isEmpty();
        }
        if (node->links().contains(Node::NextLink)) {
            linkPair = node->links()[Node::NextLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (!linkNode || linkNode == node)
                anchorPair = linkPair;
            else
                anchorPair = anchorForNode(linkNode);

            out() << "  <link rel=\"next\" href=\""
                  << anchorPair.first << "\" />\n";

            if (useSeparator)
                navigationLinks += navigationSeparator;

            navigationLinks += "<a class=\"nextPage\" href=\"" + anchorPair.first + "\">";
            if (linkPair.first == linkPair.second && !anchorPair.second.isEmpty())
                navigationLinks += protect(anchorPair.second);
            else
                navigationLinks += protect(linkPair.second);
            navigationLinks += "</a>\n";
        }
        if (node->links().contains(Node::StartLink)) {
            linkPair = node->links()[Node::StartLink];
            linkNode = qdb_->findNodeForTarget(linkPair.first, node);
            if (!linkNode)
                node->doc().location().warning(tr("Cannot link to '%1'").arg(linkPair.first));
            if (!linkNode || linkNode == node)
                anchorPair = linkPair;
            else
                anchorPair = anchorForNode(linkNode);
            out() << "  <link rel=\"start\" href=\""
                  << anchorPair.first << "\" />\n";
        }
    }

    if (node && !node->links().empty())
        out() << "<p class=\"naviNextPrevious headerNavi\">\n" << navigationLinks << "</p><p/>\n";
}

void HtmlGenerator::generateTitle(const QString& title,
                                  const Text &subTitle,
                                  SubTitleSize subTitleSize,
                                  const Node *relative,
                                  CodeMarker *marker)
{
    out() << QString(prologue).replace("\\" + COMMAND_VERSION, qdb_->version());
    if (!title.isEmpty())
        out() << "<h1 class=\"title\">" << protectEnc(title) << "</h1>\n";
    if (!subTitle.isEmpty()) {
        out() << "<span";
        if (subTitleSize == SmallSubTitle)
            out() << " class=\"small-subtitle\">";
        else
            out() << " class=\"subtitle\">";
        generateText(subTitle, relative, marker);
        out() << "</span>\n";
    }
}

void HtmlGenerator::generateFooter(const Node *node)
{
    if (node && !node->links().empty())
        out() << "<p class=\"naviNextPrevious footerNavi\">\n" << navigationLinks << "</p>\n";

    out() << QString(footer).replace("\\" + COMMAND_VERSION, qdb_->version())
          << QString(address).replace("\\" + COMMAND_VERSION, qdb_->version());

    out() << "</body>\n";
    out() << "</html>\n";
}

/*!
Lists the required imports and includes in a table.
The number of rows is known.
*/
void HtmlGenerator::generateRequisites(Aggregate *aggregate, CodeMarker *marker)
{
    QMap<QString, Text> requisites;
    Text text;

    const QString headerText = "Header";
    const QString sinceText = "Since";
    const QString inheritedBytext = "Inherited By";
    const QString inheritsText = "Inherits";
    const QString instantiatedByText = "Instantiated By";
    const QString qtVariableText = "qmake";

    //add the includes to the map
    if (!aggregate->includes().isEmpty()) {
        text.clear();
        text << highlightedCode(indent(codeIndent,
                                       marker->markedUpIncludes(aggregate->includes())),
                                       aggregate);
        requisites.insert(headerText, text);
    }

    //The order of the requisites matter
    QStringList requisiteorder;
    requisiteorder << headerText
                   << qtVariableText
                   << sinceText
                   << instantiatedByText
                   << inheritsText
                   << inheritedBytext;

    //add the since and project into the map
    if (!aggregate->since().isEmpty()) {
        text.clear();
        QStringList since = aggregate->since().split(QLatin1Char(' '));
        if (since.count() == 1) {
            // If there is only one argument, assume it is the Qt version number.
            text << " Qt " << since[0];
        }
        else {
                //Otherwise, reconstruct the <project> <version> string.
                text << " " << since.join(' ');
        }
        text << Atom::ParaRight;
        requisites.insert(sinceText, text);
    }

    if (aggregate->type() == Node::Class || aggregate->type() == Node::Namespace) {
        //add the QT variable to the map
        if (!aggregate->physicalModuleName().isEmpty()) {
            const CollectionNode* cn = qdb_->getCollectionNode(aggregate->physicalModuleName(), Node::CPP);
            if (cn && !cn->qtVariable().isEmpty()) {
                text.clear();
                text << "QT += " + cn->qtVariable();
                requisites.insert(qtVariableText, text);
            }
        }
    }

    if (aggregate->type() == Node::Class) {
        ClassNode* classe = static_cast<ClassNode*>(aggregate);
        if (classe->qmlElement() != 0 && classe->status() != Node::Internal) {
            text.clear();
            text << Atom(Atom::LinkNode, CodeMarker::stringForNode(classe->qmlElement()))
                 << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << Atom(Atom::String, classe->qmlElement()->name())
                 << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
            requisites.insert(instantiatedByText, text);

        }

        //add the inherits to the map
        QList<RelatedClass>::ConstIterator r;
        int index;
        if (!classe->baseClasses().isEmpty()) {
            text.clear();
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
                    text << comma(index++, classe->baseClasses().count());
                }
                ++r;
            }
            text << Atom::ParaRight;
            if (index > 0)
                requisites.insert(inheritsText, text);
        }

        //add the inherited-by to the map
        if (!classe->derivedClasses().isEmpty()) {
            text.clear();
            text << Atom::ParaLeft;
            int count = appendSortedNames(text, classe, classe->derivedClasses());
            text << Atom::ParaRight;
            if (count > 0)
                requisites.insert(inheritedBytext, text);
        }
    }

    if (!requisites.isEmpty()) {
        //generate the table
        out() << "<div class=\"table\"><table class=\"alignedsummary\">\n";

        QStringList::ConstIterator i;
        for (i = requisiteorder.constBegin(); i != requisiteorder.constEnd(); ++i) {

            if (requisites.contains(*i)) {
                out() << "<tr>"
                    << "<td class=\"memItemLeft rightAlign topAlign\"> "
                    << *i << ":"
                    "</td><td class=\"memItemRight bottomAlign\"> ";

                if (*i == headerText)
                    out() << requisites.value(*i).toString();
                else
                    generateText(requisites.value(*i), aggregate, marker);
                out() << "</td></tr>";
            }
        }
        out() << "</table></div>";
    }
}

/*!
Lists the required imports and includes in a table.
The number of rows is known.
*/
void HtmlGenerator::generateQmlRequisites(QmlTypeNode *qcn, CodeMarker *marker)
{
    if (!qcn)
        return;
    QMap<QString, Text> requisites;
    Text text;

    const QString importText = "Import Statement:";
    const QString sinceText = "Since:";
    const QString inheritedBytext = "Inherited By:";
    const QString inheritsText = "Inherits:";
    const QString instantiatesText = "Instantiates:";

    //The order of the requisites matter
    QStringList requisiteorder;
    requisiteorder << importText
                   << sinceText
                   << instantiatesText
                   << inheritsText
                   << inheritedBytext;

    //add the module name and version to the map
    QString logicalModuleVersion;
    const CollectionNode* collection = qdb_->getCollectionNode(qcn->logicalModuleName(), qcn->genus());
    if (collection)
        logicalModuleVersion = collection->logicalModuleVersion();
    else
        logicalModuleVersion = qcn->logicalModuleVersion();
    text.clear();
    text << "import " + qcn->logicalModuleName() + QLatin1Char(' ') + logicalModuleVersion;
    requisites.insert(importText, text);

    //add the since and project into the map
    if (!qcn->since().isEmpty()) {
        text.clear();
        QStringList since = qcn->since().split(QLatin1Char(' '));
        if (since.count() == 1) {
            // If there is only one argument, assume it is the Qt version number.
            text << " Qt " << since[0];
        }
        else {
                //Otherwise, reconstruct the <project> <version> string.
                text << " " << since.join(' ');
        }
        text << Atom::ParaRight;
        requisites.insert(sinceText, text);
    }

    //add the instantiates to the map
    ClassNode* cn = qcn->classNode();
    if (cn && (cn->status() != Node::Internal)) {
        text.clear();
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        requisites.insert(instantiatesText, text);
    }

    //add the inherits to the map
    QmlTypeNode* base = qcn->qmlBaseNode();
    while (base && base->isInternal()) {
        base = base->qmlBaseNode();
    }
    if (base) {
        text.clear();
        text << Atom::ParaLeft
             << Atom(Atom::LinkNode,CodeMarker::stringForNode(base))
             << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
             << Atom(Atom::String, base->name())
             << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK)
             << Atom::ParaRight;
        requisites.insert(inheritsText, text);
    }

    //add the inherited-by to the map
    NodeList subs;
    QmlTypeNode::subclasses(qcn, subs);
    if (!subs.isEmpty()) {
        text.clear();
        text << Atom::ParaLeft;
        int count = appendSortedQmlNames(text, qcn, subs);
        text << Atom::ParaRight;
        if (count > 0)
            requisites.insert(inheritedBytext, text);
    }

    if (!requisites.isEmpty()) {
        //generate the table
        out() << "<div class=\"table\"><table class=\"alignedsummary\">\n";

        QStringList::ConstIterator i;
        for (i = requisiteorder.constBegin(); i != requisiteorder.constEnd(); ++i) {

            if (requisites.contains(*i)) {
                out() << "<tr>"
                    << "<td class=\"memItemLeft rightAlign topAlign\"> "
                    << *i
                    << "</td><td class=\"memItemRight bottomAlign\"> ";

                if (*i == importText)
                    out()<<requisites.value(*i).toString();
                else
                    generateText(requisites.value(*i), qcn, marker);
                out() << "</td></tr>";
            }
        }
        out() << "</table></div>";
    }
}

void HtmlGenerator::generateBrief(const Node *node, CodeMarker *marker,
                                  const Node *relative, bool addLink)
{
    Text brief = node->doc().briefText();

    if (!brief.isEmpty()) {
        if (!brief.lastAtom()->string().endsWith('.')) {
            brief << Atom(Atom::String, ".");
            node->doc().location().warning(tr("'\\brief' statement does not end with a full stop."));
        }
        generateExtractionMark(node, BriefMark);
        out() << "<p>";
        generateText(brief, node, marker);

        if (addLink) {
            if (!relative || node == relative)
                out() << " <a href=\"#";
            else
                out() << " <a href=\"" << linkForNode(node, relative) << '#';
            out() << registerRef("details") << "\">More...</a>";
        }

        out() << "</p>\n";
        generateExtractionMark(node, EndMark);
    }
}

void HtmlGenerator::generateIncludes(const Aggregate *aggregate, CodeMarker *marker)
{
    if (!aggregate->includes().isEmpty()) {
        out() << "<pre class=\"cpp\">"
              << trimmedTrailing(highlightedCode(indent(codeIndent,
                                                        marker->markedUpIncludes(aggregate->includes())),
                                                 aggregate), codePrefix, codeSuffix)
              << "</pre>";
    }
}

/*!
  Revised for the new doc format.
  Generates a table of contents beginning at \a node.
 */
void HtmlGenerator::generateTableOfContents(const Node *node,
                                            CodeMarker *marker,
                                            QVector<Section>* sections)
{
    QList<Atom*> toc;
    if (node->doc().hasTableOfContents())
        toc = node->doc().tableOfContents();
    if (tocDepth == 0 || (toc.isEmpty() && !sections && !node->isModule())) {
        generateSidebar();
        return;
    }

    int sectionNumber = 1;
    int detailsBase = 0;

    // disable nested links in table of contents
    inContents_ = true;
    inLink_ = true;

    out() << "<div class=\"sidebar\">\n";
    out() << "<div class=\"toc\">\n";
    out() << "<h3><a name=\"toc\">Contents</a></h3>\n";
    out() << "<ul>\n";

    if (node->isModule()) {
        if (node->hasNamespaces()) {
            out() << "<li class=\"level"
                  << sectionNumber
                  << "\"><a href=\"#"
                  << registerRef("namespaces")
                  << "\">Namespaces</a></li>\n";
        }
        if (node->hasClasses()) {
            out() << "<li class=\"level"
                  << sectionNumber
                  << "\"><a href=\"#"
                  << registerRef("classes")
                  << "\">Classes</a></li>\n";
        }
        out() << "<li class=\"level"
              << sectionNumber
              << "\"><a href=\"#"
              << registerRef("details")
              << "\">Detailed Description</a></li>\n";
        for (int i = 0; i < toc.size(); ++i) {
            if (toc.at(i)->string().toInt() == 1) {
                detailsBase = 1;
                break;
            }
        }
    }
    else if (sections && (node->isClass() ||
                          node->isNamespace() ||
                          node->isQmlType() ||
                          node->isJsType())) {
        SectionVector::ConstIterator s = sections->constBegin();
        while (s != sections->constEnd()) {
            if (!s->members().isEmpty()) {
                out() << "<li class=\"level"
                      << sectionNumber
                      << "\"><a href=\"#"
                      << registerRef(s->plural())
                      << "\">" << s->title()
                      << "</a></li>\n";
            }
            if (!s->reimplementedMembers().isEmpty()) {
                QString ref = QString("Reimplemented ") + s->plural();
                out() << "<li class=\"level"
                      << sectionNumber
                      << "\"><a href=\"#"
                      << registerRef(ref.toLower())
                      << "\">" << QString("Reimplemented ") + s->title()
                      << "</a></li>\n";
            }
            ++s;
        }
        if (!node->isNamespace() || node->hasDoc()) {
            out() << "<li class=\"level"
                  << sectionNumber
                  << "\"><a href=\"#"
                  << registerRef("details")
                  << "\">Detailed Description</a></li>\n";
        }
        for (int i = 0; i < toc.size(); ++i) {
            if (toc.at(i)->string().toInt() == 1) {
                detailsBase = 1;
                break;
            }
        }
    }

    for (int i = 0; i < toc.size(); ++i) {
        const Atom *atom = toc.at(i);
        sectionNumber = atom->string().toInt() + detailsBase;
        //restrict the ToC depth to the one set by the HTML.tocdepth variable or
        //print all levels if tocDepth is not set.
        if (sectionNumber <= tocDepth || tocDepth < 0) {
            int numAtoms;
            Text headingText = Text::sectionHeading(atom);
            QString s = headingText.toString();
            out() << "<li class=\"level"
                  << sectionNumber
                  << "\">";
            out() << "<a href=\""
                  << '#'
                  << Doc::canonicalTitle(s)
                  << "\">";
            generateAtomList(headingText.firstAtom(), node, marker, true, numAtoms);
            out() << "</a></li>\n";
        }
    }
    out() << "</ul>\n";
    out() << "</div>\n";
    out() << "<div class=\"sidebar-content\" id=\"sidebar-content\"></div>";
    out() << "</div>\n";
    inContents_ = false;
    inLink_ = false;
}

/*!
  Outputs a placeholder div where the style can add customized sidebar content.
 */
void HtmlGenerator::generateSidebar() {
    out() << "<div class=\"sidebar\">";
    out() << "<div class=\"sidebar-content\" id=\"sidebar-content\"></div>";
    out() << "</div>\n";
}

QString HtmlGenerator::generateAllMembersFile(const Section &section, CodeMarker *marker)
{
    if (section.isEmpty())
        return QString();

    const Aggregate *aggregate = section.aggregate();
    QString fileName = fileBase(aggregate) + "-members." + fileExtension();
    beginSubPage(aggregate, fileName);
    QString title = "List of All Members for " + aggregate->name();
    generateHeader(title, aggregate, marker);
    generateSidebar();
    generateTitle(title, Text(), SmallSubTitle, aggregate, marker);
    out() << "<p>This is the complete list of members for ";
    generateFullName(aggregate, 0);
    out() << ", including inherited members.</p>\n";

    generateSectionList(section, aggregate, marker);

    generateFooter();
    endSubPage();
    return fileName;
}

/*!
  This function creates an html page on which are listed all
  the members of the QML class used to generte the \a sections,
  including the inherited members. The \a marker is used for
  formatting stuff.
 */
QString HtmlGenerator::generateAllQmlMembersFile(const Sections &sections, CodeMarker* marker)
{

    if (sections.allMembersSection().isEmpty())
        return QString();

    const Aggregate *aggregate = sections.aggregate();
    QString fileName = fileBase(aggregate) + "-members." + fileExtension();
    beginSubPage(aggregate, fileName);
    QString title = "List of All Members for " + aggregate->name();
    generateHeader(title, aggregate, marker);
    generateSidebar();
    generateTitle(title, Text(), SmallSubTitle, aggregate, marker);
    out() << "<p>This is the complete list of members for ";
    generateFullName(aggregate, 0);
    out() << ", including inherited members.</p>\n";

    ClassKeysNodesList& cknl = sections.allMembersSection().classKeysNodesList();
    if (!cknl.isEmpty()) {
        for (int i=0; i<cknl.size(); i++) {
            ClassKeysNodes* ckn = cknl[i];
            const QmlTypeNode* qcn = ckn->first;
            KeysAndNodes& kn = ckn->second;
            QStringList& keys = kn.first;
            NodeVector& nodes = kn.second;
            if (nodes.isEmpty())
                continue;
            if (i != 0) {
                out() << "<p>The following members are inherited from ";
                generateFullName(qcn,0);
                out() << ".</p>\n";
            }
            out() << "<ul>\n";
            for (int j=0; j<keys.size(); j++) {
                if (nodes[j]->access() == Node::Private || nodes[j]->status() == Node::Internal) {
                    continue;
                }
                out() << "<li class=\"fn\">";
                QString prefix;
                if (!keys.isEmpty()) {
                    prefix = keys.at(j).mid(1);
                    prefix = prefix.left(keys.at(j).indexOf("::")+1);
                }
                generateQmlItem(nodes[j], aggregate, marker, true);
                if (nodes[j]->isAttached())
                    out() << " [attached]";
                //generateSynopsis(nodes[j], qcn, marker, Section::AllMembers, false, &prefix);
                out() << "</li>\n";
            }
            out() << "</ul>\n";
        }
    }

    generateFooter();
    endSubPage();
    return fileName;
}

QString HtmlGenerator::generateObsoleteMembersFile(const Sections &sections, CodeMarker *marker)
{
    SectionPtrVector summary_spv;
    SectionPtrVector details_spv;
    if (!sections.hasObsoleteMembers(&summary_spv, &details_spv))
        return QString();

    Aggregate *aggregate = sections.aggregate();
    QString title = "Obsolete Members for " + aggregate->name();
    QString fileName = fileBase(aggregate) + "-obsolete." + fileExtension();
    QString link;
    if (useOutputSubdirs() && !Generator::outputSubdir().isEmpty())
        link = QString("../" + Generator::outputSubdir() + QLatin1Char('/'));
    link += fileName;
    aggregate->setObsoleteLink(link);

    beginSubPage(aggregate, fileName);
    generateHeader(title, aggregate, marker);
    generateSidebar();
    generateTitle(title, Text(), SmallSubTitle, aggregate, marker);

    out() << "<p><b>The following members of class "
          << "<a href=\"" << linkForNode(aggregate, 0) << "\">"
          << protectEnc(aggregate->name()) << "</a>"
          << " are obsolete.</b> "
          << "They are provided to keep old source code working. "
          << "We strongly advise against using them in new code.</p>\n";

    for (int i = 0; i < summary_spv.size(); ++i) {
        out() << "<h2>" << protectEnc(summary_spv.at(i)->title()) << "</h2>\n";
        const Section &section = *summary_spv.at(i);
        generateSectionList(section, aggregate, marker, Section::Obsolete);
    }

    for (int i = 0; i < details_spv.size(); ++i) {
        //out() << "<hr />\n";
        out() << "<h2>" << protectEnc(details_spv.at(i)->title()) << "</h2>\n";

        const NodeVector &members = details_spv.at(i)->obsoleteMembers();
        NodeVector::ConstIterator m = members.constBegin();
        while (m != members.constEnd()) {
            if ((*m)->access() != Node::Private)
                generateDetailedMember(*m, aggregate, marker);
            ++m;
        }
    }

    generateFooter();
    endSubPage();
    return fileName;
}

/*!
  Generates a separate file where obsolete members of the QML
  type \a qcn are listed. The \a marker is used to generate
  the section lists, which are then traversed and output here.

  Note that this function currently only handles correctly the
  case where \a status is \c {Section::Obsolete}.
 */
QString HtmlGenerator::generateObsoleteQmlMembersFile(const Sections &sections, CodeMarker *marker)
{
    SectionPtrVector summary_spv;
    SectionPtrVector details_spv;
    if (!sections.hasObsoleteMembers(&summary_spv, &details_spv))
        return QString();

    Aggregate *aggregate = sections.aggregate();
    QString title = "Obsolete Members for " + aggregate->name();
    QString fileName = fileBase(aggregate) + "-obsolete." + fileExtension();
    QString link;
    if (useOutputSubdirs() && !Generator::outputSubdir().isEmpty())
        link = QString("../" + Generator::outputSubdir() + QLatin1Char('/'));
    link += fileName;
    aggregate->setObsoleteLink(link);

    beginSubPage(aggregate, fileName);
    generateHeader(title, aggregate, marker);
    generateSidebar();
    generateTitle(title, Text(), SmallSubTitle, aggregate, marker);

    out() << "<p><b>The following members of QML type "
          << "<a href=\"" << linkForNode(aggregate, 0) << "\">"
          << protectEnc(aggregate->name()) << "</a>"
          << " are obsolete.</b> "
          << "They are provided to keep old source code working. "
          << "We strongly advise against using them in new code.</p>\n";

    for (int i = 0; i < summary_spv.size(); ++i) {
        QString ref = registerRef(summary_spv.at(i)->title().toLower());
        out() << "<a name=\"" << ref
              << "\"></a>" << divNavTop << '\n';
        out() << "<h2 id=\"" << ref << "\">" << protectEnc(summary_spv.at(i)->title()) << "</h2>\n";
        generateQmlSummary(*(summary_spv.at(i)), aggregate, marker);
    }

    for (int i = 0; i < details_spv.size(); ++i) {
        out() << "<h2>" << protectEnc(details_spv.at(i)->title()) << "</h2>\n";
        NodeVector::ConstIterator m = details_spv.at(i)->members().constBegin();
        while (m != details_spv.at(i)->members().constEnd()) {
            generateDetailedQmlMember(*m, aggregate, marker);
            out() << "<br/>\n";
            ++m;
        }
    }

    generateFooter();
    endSubPage();
    return fileName;
}

void HtmlGenerator::generateClassHierarchy(const Node *relative, NodeMap& classMap)
{
    if (classMap.isEmpty())
        return;

    NodeMap topLevel;
    NodeMap::Iterator c = classMap.begin();
    while (c != classMap.end()) {
        ClassNode *classe = static_cast<ClassNode *>(*c);
        if (classe->baseClasses().isEmpty())
            topLevel.insert(classe->name(), classe);
        ++c;
    }

    QStack<NodeMap > stack;
    stack.push(topLevel);

    out() << "<ul>\n";
    while (!stack.isEmpty()) {
        if (stack.top().isEmpty()) {
            stack.pop();
            out() << "</ul>\n";
        }
        else {
            ClassNode* child = static_cast<ClassNode*>(*stack.top().begin());
            out() << "<li>";
            generateFullName(child, relative);
            out() << "</li>\n";
            stack.top().erase(stack.top().begin());

            NodeMap newTop;
            foreach (const RelatedClass &d, child->derivedClasses()) {
                if (d.node_ && !d.isPrivate() && !d.node_->isInternal() && d.node_->hasDoc())
                    newTop.insert(d.node_->name(), d.node_);
            }
            if (!newTop.isEmpty()) {
                stack.push(newTop);
                out() << "<ul>\n";
            }
        }
    }
}

/*!
  Output an annotated list of the nodes in \a nodeMap.
  A two-column table is output.
 */
void HtmlGenerator::generateAnnotatedList(const Node* relative,
                                          CodeMarker* marker,
                                          const NodeMultiMap& nmm)
{
    if (nmm.isEmpty())
        return;
    generateAnnotatedList(relative, marker, nmm.values());
}

/*!
 */
void HtmlGenerator::generateAnnotatedList(const Node *relative,
                                          CodeMarker *marker,
                                          const NodeList& unsortedNodes)
{
    NodeMultiMap nmm;
    bool allInternal = true;
    foreach (Node* node, unsortedNodes) {
        if (!node->isInternal() && !node->isObsolete()) {
            allInternal = false;
            nmm.insert(node->fullName(relative), node);
        }
    }
    if (allInternal)
        return;
    out() << "<div class=\"table\"><table class=\"annotated\">\n";
    int row = 0;
    NodeList nodes = nmm.values();
    std::sort(nodes.begin(), nodes.end(), Node::nodeNameLessThan);

    foreach (const Node* node, nodes) {
        if (++row % 2 == 1)
            out() << "<tr class=\"odd topAlign\">";
        else
            out() << "<tr class=\"even topAlign\">";
        out() << "<td class=\"tblName\"><p>";
        generateFullName(node, relative);
        out() << "</p></td>";

        if (!node->isDocumentNode()) {
            Text brief = node->doc().trimmedBriefText(node->name());
            if (!brief.isEmpty()) {
                out() << "<td class=\"tblDescr\"><p>";
                generateText(brief, node, marker);
                out() << "</p></td>";
            }
            else if (!node->reconstitutedBrief().isEmpty()) {
                out() << "<td class=\"tblDescr\"><p>";
                out() << node->reconstitutedBrief();
                out() << "</p></td>";
            }
        }
        else {
            out() << "<td class=\"tblDescr\"><p>";
            if (!node->reconstitutedBrief().isEmpty()) {
                out() << node->reconstitutedBrief();
            }
            else
                out() << protectEnc(node->doc().briefText().toString());
            out() << "</p></td>";
        }
        out() << "</tr>\n";
    }
    out() << "</table></div>\n";
}

/*!
  Outputs a series of annotated lists from the nodes in \a nmm,
  divided into sections based by the key names in the multimap.
 */
void HtmlGenerator::generateAnnotatedLists(const Node* relative,
                                          CodeMarker* marker,
                                          const NodeMultiMap& nmm)
{
    foreach (const QString &name, nmm.uniqueKeys()) {
        if (!name.isEmpty()) {
            out() << "<h2 id=\"" << registerRef(name.toLower())
                  << "\">" << protectEnc(name) << "</h2>\n";
        }
        generateAnnotatedList(relative, marker, nmm.values(name));
    }
}

/*!
  This function finds the common prefix of the names of all
  the classes in the class map \a nmm and then generates a
  compact list of the class names alphabetized on the part
  of the name not including the common prefix. You can tell
  the function to use \a comonPrefix as the common prefix,
  but normally you let it figure it out itself by looking at
  the name of the first and last classes in the class map
  \a nmm.
 */
void HtmlGenerator::generateCompactList(ListType listType,
                                        const Node *relative,
                                        const NodeMultiMap &nmm,
                                        bool includeAlphabet,
                                        QString commonPrefix)
{
    if (nmm.isEmpty())
        return;

    const int NumParagraphs = 37; // '0' to '9', 'A' to 'Z', '_'
    int commonPrefixLen = commonPrefix.length();

    /*
      Divide the data into 37 paragraphs: 0, ..., 9, A, ..., Z,
      underscore (_). QAccel will fall in paragraph 10 (A) and
      QXtWidget in paragraph 33 (X). This is the only place where we
      assume that NumParagraphs is 37. Each paragraph is a NodeMultiMap.
    */
    NodeMultiMap paragraph[NumParagraphs+1];
    QString paragraphName[NumParagraphs+1];
    QSet<char> usedParagraphNames;

    NodeMultiMap::ConstIterator c = nmm.constBegin();
    while (c != nmm.constEnd()) {
        QStringList pieces = c.key().split("::");
        QString key;
        int idx = commonPrefixLen;
        if (idx > 0 && !pieces.last().startsWith(commonPrefix, Qt::CaseInsensitive))
            idx = 0;
        key = pieces.last().mid(idx).toLower();

        int paragraphNr = NumParagraphs - 1;

        if (key[0].digitValue() != -1) {
            paragraphNr = key[0].digitValue();
        }
        else if (key[0] >= QLatin1Char('a') && key[0] <= QLatin1Char('z')) {
            paragraphNr = 10 + key[0].unicode() - 'a';
        }

        paragraphName[paragraphNr] = key[0].toUpper();
        usedParagraphNames.insert(key[0].toLower().cell());
        paragraph[paragraphNr].insert(c.key(), c.value());
        ++c;
    }

    /*
      Each paragraph j has a size: paragraph[j].count(). In the
      discussion, we will assume paragraphs 0 to 5 will have sizes
      3, 1, 4, 1, 5, 9.

      We now want to compute the paragraph offset. Paragraphs 0 to 6
      start at offsets 0, 3, 4, 8, 9, 14, 23.
    */
    int paragraphOffset[NumParagraphs + 1];     // 37 + 1
    paragraphOffset[0] = 0;
    for (int i=0; i<NumParagraphs; i++)         // i = 0..36
        paragraphOffset[i+1] = paragraphOffset[i] + paragraph[i].count();

    /*
      Output the alphabet as a row of links.
     */
    if (includeAlphabet) {
        out() << "<p  class=\"centerAlign functionIndex\"><b>";
        for (int i = 0; i < 26; i++) {
            QChar ch('a' + i);
            if (usedParagraphNames.contains(char('a' + i)))
                out() << QString("<a href=\"#%1\">%2</a>&nbsp;").arg(ch).arg(ch.toUpper());
        }
        out() << "</b></p>\n";
    }

    /*
      Output a <div> element to contain all the <dl> elements.
     */
    out() << "<div class=\"flowListDiv\">\n";
    numTableRows_ = 0;

    int curParNr = 0;
    int curParOffset = 0;
    QString previousName;
    bool multipleOccurrences = false;

    for (int i=0; i<nmm.count(); i++) {
        while ((curParNr < NumParagraphs) &&
               (curParOffset == paragraph[curParNr].count())) {
            ++curParNr;
            curParOffset = 0;
        }

        /*
          Starting a new paragraph means starting a new <dl>.
        */
        if (curParOffset == 0) {
            if (i > 0)
                out() << "</dl>\n";
            if (++numTableRows_ % 2 == 1)
                out() << "<dl class=\"flowList odd\">";
            else
                out() << "<dl class=\"flowList even\">";
            out() << "<dt class=\"alphaChar\">";
            if (includeAlphabet) {
                QChar c = paragraphName[curParNr][0].toLower();
                out() << QString("<a name=\"%1\"></a>").arg(c);
            }
            out() << "<b>"
                  << paragraphName[curParNr]
                  << "</b>";
            out() << "</dt>\n";
        }

        /*
          Output a <dd> for the current offset in the current paragraph.
         */
        out() << "<dd>";
        if ((curParNr < NumParagraphs) &&
                !paragraphName[curParNr].isEmpty()) {
            NodeMultiMap::Iterator it;
            NodeMultiMap::Iterator next;
            it = paragraph[curParNr].begin();
            for (int i=0; i<curParOffset; i++)
                ++it;

            if (listType == Generic) {
                /*
                  Previously, we used generateFullName() for this, but we
                  require some special formatting.
                */
                out() << "<a href=\"" << linkForNode(it.value(), relative) << "\">";
            }
            else if (listType == Obsolete) {
                QString fileName = fileBase(it.value()) + "-obsolete." + fileExtension();
                QString link;
                if (useOutputSubdirs()) {
                    link = QString("../" + it.value()->outputSubdirectory() + QLatin1Char('/'));
                }
                link += fileName;
                out() << "<a href=\"" << link << "\">";
            }

            QStringList pieces;
            if (it.value()->isQmlType() || it.value()->isJsType()) {
                QString name = it.value()->name();
                next = it;
                ++next;
                if (name != previousName)
                    multipleOccurrences = false;
                if ((next != paragraph[curParNr].end()) && (name == next.value()->name())) {
                    multipleOccurrences = true;
                    previousName = name;
                }
                if (multipleOccurrences)
                    name += ": " + it.value()->tree()->camelCaseModuleName();
                pieces << name;
            }
            else
                pieces = it.value()->fullName(relative).split("::");
            out() << protectEnc(pieces.last());
            out() << "</a>";
            if (pieces.size() > 1) {
                out() << " (";
                generateFullName(it.value()->parent(), relative);
                out() << ')';
            }
        }
        out() << "</dd>\n";
        curParOffset++;
    }
    if (nmm.count() > 0)
        out() << "</dl>\n";

    out() << "</div>\n";
}

void HtmlGenerator::generateFunctionIndex(const Node *relative)
{
    out() << "<p  class=\"centerAlign functionIndex\"><b>";
    for (int i = 0; i < 26; i++) {
        QChar ch('a' + i);
        out() << QString("<a href=\"#%1\">%2</a>&nbsp;").arg(ch).arg(ch.toUpper());
    }
    out() << "</b></p>\n";

    char nextLetter = 'a';
    char currentLetter;

    out() << "<ul>\n";
    NodeMapMap& funcIndex = qdb_->getFunctionIndex();
    QMap<QString, NodeMap >::ConstIterator f = funcIndex.constBegin();
    while (f != funcIndex.constEnd()) {
        out() << "<li>";
        out() << protectEnc(f.key()) << ':';

        currentLetter = f.key()[0].unicode();
        while (islower(currentLetter) && currentLetter >= nextLetter) {
            out() << QString("<a name=\"%1\"></a>").arg(nextLetter);
            nextLetter++;
        }

        NodeMap::ConstIterator s = (*f).constBegin();
        while (s != (*f).constEnd()) {
            out() << ' ';
            generateFullName((*s)->parent(), relative, *s);
            ++s;
        }
        out() << "</li>";
        out() << '\n';
        ++f;
    }
    out() << "</ul>\n";
}

void HtmlGenerator::generateLegaleseList(const Node *relative, CodeMarker *marker)
{
    TextToNodeMap& legaleseTexts = qdb_->getLegaleseTexts();
    QMap<Text, const Node *>::ConstIterator it = legaleseTexts.constBegin();
    while (it != legaleseTexts.constEnd()) {
        Text text = it.key();
        //out() << "<hr />\n";
        generateText(text, relative, marker);
        out() << "<ul>\n";
        do {
            out() << "<li>";
            generateFullName(it.value(), relative);
            out() << "</li>\n";
            ++it;
        } while (it != legaleseTexts.constEnd() && it.key() == text);
        out() << "</ul>\n";
    }
}

void HtmlGenerator::generateQmlItem(const Node *node,
                                    const Node *relative,
                                    CodeMarker *marker,
                                    bool summary)
{
    QString marked = marker->markedUpQmlItem(node,summary);
    QRegExp templateTag("(<[^@>]*>)");
    if (marked.indexOf(templateTag) != -1) {
        QString contents = protectEnc(marked.mid(templateTag.pos(1),
                                                 templateTag.cap(1).length()));
        marked.replace(templateTag.pos(1), templateTag.cap(1).length(),
                       contents);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i>\\1<sub>\\2</sub></i>");
    marked.replace("<@param>", "<i>");
    marked.replace("</@param>", "</i>");

    if (summary)
        marked.replace("@name>", "b>");

    marked.replace("<@extra>", "<code>");
    marked.replace("</@extra>", "</code>");

    if (summary) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }
    out() << highlightedCode(marked, relative, false, Node::QML);
}

void HtmlGenerator::generateList(const Node* relative, CodeMarker* marker, const QString& selector)
{
    CNMap cnm;
    Node::Genus genus = Node::DontCare;
    if (selector == QLatin1String("overviews"))
        genus = Node::DOC;
    else if (selector == QLatin1String("cpp-modules"))
        genus = Node::CPP;
    else if (selector == QLatin1String("qml-modules"))
        genus = Node::QML;
    else if (selector == QLatin1String("js-modules"))
        genus = Node::JS;
    if (genus != Node::DontCare) {
        NodeList nl;
        qdb_->mergeCollections(genus, cnm, relative);
        CollectionList cl = cnm.values();
        foreach (CollectionNode* cn, cl)
            nl.append(cn);
        generateAnnotatedList(relative, marker, nl);
    }
    else {
        /*
          \generatelist {selector} is only allowed in a
          comment where the topic is \group, \module,
          \qmlmodule, or \jsmodule
        */
        if (relative && !relative->isCollectionNode()) {
            relative->doc().location().warning(tr("\\generatelist {%1} is only allowed in \\group, "
                                                  "\\module, \\qmlmodule, and \\jsmodule comments.").arg(selector));
            return;
        }
        Node* n = const_cast<Node*>(relative);
        CollectionNode* cn = static_cast<CollectionNode*>(n);
        qdb_->mergeCollections(cn);
        generateAnnotatedList(cn, marker, cn->members());
    }
}

void HtmlGenerator::generateSection(const NodeVector& nv, const Node *relative, CodeMarker *marker)
{
    bool alignNames = true;
    if (!nv.isEmpty()) {
        bool twoColumn = false;
        if (nv.first()->type() == Node::Property) {
            twoColumn = (nv.count() >= 5);
            alignNames = false;
        }
        if (alignNames) {
            out() << "<div class=\"table\"><table class=\"alignedsummary\">\n";
        }
        else {
            if (twoColumn)
                out() << "<div class=\"table\"><table class=\"propsummary\">\n"
                      << "<tr><td class=\"topAlign\">";
            out() << "<ul>\n";
        }

        int i = 0;
        NodeVector::ConstIterator m = nv.constBegin();
        while (m != nv.constEnd()) {
            if ((*m)->access() == Node::Private) {
                ++m;
                continue;
            }

            if (alignNames) {
                out() << "<tr><td class=\"memItemLeft rightAlign topAlign\"> ";
            }
            else {
                if (twoColumn && i == (int) (nv.count() + 1) / 2)
                    out() << "</ul></td><td class=\"topAlign\"><ul>\n";
                out() << "<li class=\"fn\">";
            }

            generateSynopsis(*m, relative, marker, Section::Summary, alignNames);
            if (alignNames)
                out() << "</td></tr>\n";
            else
                out() << "</li>\n";
            i++;
            ++m;
        }
        if (alignNames)
            out() << "</table></div>\n";
        else {
            out() << "</ul>\n";
            if (twoColumn)
                out() << "</td></tr>\n</table></div>\n";
        }
    }
}

void HtmlGenerator::generateSectionList(const Section& section,
                                        const Node *relative,
                                        CodeMarker *marker,
                                        Section::Status status)
{
    bool alignNames = true;
    const NodeVector &members = (status == Section::Obsolete ? section.obsoleteMembers() : section.members());
    if (!members.isEmpty()) {
        bool hasPrivateSignals = false;
        bool isInvokable = false;
        bool twoColumn = false;
        if (section.style() == Section::AllMembers) {
            alignNames = false;
            twoColumn = (members.count() >= 16);
        }
        else if (members.first()->type() == Node::Property) {
            twoColumn = (members.count() >= 5);
            alignNames = false;
        }
        if (alignNames) {
            out() << "<div class=\"table\"><table class=\"alignedsummary\">\n";
        }
        else {
            if (twoColumn)
                out() << "<div class=\"table\"><table class=\"propsummary\">\n"
                      << "<tr><td class=\"topAlign\">";
            out() << "<ul>\n";
        }

        int i = 0;
        NodeVector::ConstIterator m = members.constBegin();
        while (m != members.constEnd()) {
            if ((*m)->access() == Node::Private) {
                ++m;
                continue;
            }

            if (alignNames) {
                out() << "<tr><td class=\"memItemLeft topAlign rightAlign\"> ";
            }
            else {
                if (twoColumn && i == (int) (members.count() + 1) / 2)
                    out() << "</ul></td><td class=\"topAlign\"><ul>\n";
                out() << "<li class=\"fn\">";
            }

            QString prefix;
            const QStringList& keys = section.keys(status);
            if (!keys.isEmpty()) {
                prefix = keys.at(i).mid(1);
                prefix = prefix.left(keys.at(i).indexOf("::") + 1);
            }
            generateSynopsis(*m, relative, marker, section.style(), alignNames, &prefix);
            if ((*m)->isFunction()) {
                const FunctionNode* fn = static_cast<const FunctionNode*>(*m);
                if (fn->isPrivateSignal()) {
                    hasPrivateSignals = true;
                    if (alignNames)
                        out() << "</td><td class=\"memItemRight bottomAlign\">[see note below]";
                }
                else if (fn->isInvokable()) {
                    isInvokable = true;
                    if (alignNames)
                        out() << "</td><td class=\"memItemRight bottomAlign\">[see note below]";
                }
            }
            if (alignNames)
                out() << "</td></tr>\n";
            else
                out() << "</li>\n";
            i++;
            ++m;
        }
        if (alignNames)
            out() << "</table></div>\n";
        else {
            out() << "</ul>\n";
            if (twoColumn)
                out() << "</td></tr>\n</table></div>\n";
        }
        if (hasPrivateSignals && alignNames)
            generatePrivateSignalNote(relative, marker);
        if (isInvokable && alignNames)
            generateInvokableNote(relative, marker);
    }

    if (status != Section::Obsolete && section.style() == Section::Summary && !section.inheritedMembers().isEmpty()) {
        out() << "<ul>\n";
        generateSectionInheritedList(section, relative);
        out() << "</ul>\n";
    }
}

void HtmlGenerator::generateSectionInheritedList(const Section& section, const Node *relative)
{
    QList<QPair<Aggregate *, int> >::ConstIterator p = section.inheritedMembers().constBegin();
    while (p != section.inheritedMembers().constEnd()) {
        out() << "<li class=\"fn\">";
        out() << (*p).second << ' ';
        if ((*p).second == 1) {
            out() << section.singular();
        }
        else {
            out() << section.plural();
        }
        out() << " inherited from <a href=\"" << fileName((*p).first)
              << '#' << Generator::cleanRef(section.title().toLower()) << "\">"
              << protectEnc((*p).first->plainFullName(relative))
              << "</a></li>\n";
        ++p;
    }
}

// generateSynopsis(qmn,relative,marker,Section::Details,false);
void HtmlGenerator::generateSynopsis(const Node *node,
                                     const Node *relative,
                                     CodeMarker *marker,
                                     Section::Style style,
                                     bool alignNames,
                                     const QString* prefix)
{
    QString marked = marker->markedUpSynopsis(node, relative, style);

    if (prefix)
        marked.prepend(*prefix);
    QRegExp templateTag("(<[^@>]*>)");
    if (marked.indexOf(templateTag) != -1) {
        QString contents = protectEnc(marked.mid(templateTag.pos(1),
                                                 templateTag.cap(1).length()));
        marked.replace(templateTag.pos(1), templateTag.cap(1).length(),
                       contents);
    }
    marked.replace(QRegExp("<@param>([a-z]+)_([1-9n])</@param>"),
                   "<i>\\1<sub>\\2</sub></i>");
    marked.replace("<@param>", "<i>");
    marked.replace("</@param>", "</i>");

    if (style == Section::Summary) {
        marked.remove("<@name>");   // was "<b>"
        marked.remove("</@name>");  // was "</b>"
    }

    if (style == Section::AllMembers) {
        QRegExp extraRegExp("<@extra>.*</@extra>");
        extraRegExp.setMinimal(true);
        marked.remove(extraRegExp);
    } else {
        marked.replace("<@extra>", "<code>");
        marked.replace("</@extra>", "</code>");
    }

    if (style != Section::Details) {
        marked.remove("<@type>");
        marked.remove("</@type>");
    }

    out() << highlightedCode(marked, relative, alignNames);
}

QString HtmlGenerator::highlightedCode(const QString& markedCode,
                                       const Node* relative,
                                       bool alignNames, Node::Genus genus)
{
    QString src = markedCode;
    QString html;
    html.reserve(src.size());
    QStringRef arg;
    QStringRef par1;

    const QChar charLangle = '<';
    const QChar charAt = '@';

    static const QString typeTag("type");
    static const QString headerTag("headerfile");
    static const QString funcTag("func");
    static const QString linkTag("link");

    // replace all <@link> tags: "(<@link node=\"([^\"]+)\">).*(</@link>)"
    // replace all <@func> tags: "(<@func target=\"([^\"]*)\">)(.*)(</@func>)"
    // replace all "(<@(type|headerfile)(?: +[^>]*)?>)(.*)(</@\\2>)" tags
    bool done = false;
    for (int i = 0, srcSize = src.size(); i < srcSize;) {
        if (src.at(i) == charLangle && src.at(i + 1) == charAt) {
            if (alignNames && !done) {
                html += QLatin1String("</td><td class=\"memItemRight bottomAlign\">");
                done = true;
            }
            i += 2;
            if (parseArg(src, linkTag, &i, srcSize, &arg, &par1)) {
                html += QLatin1String("<b>");
                const Node* n = CodeMarker::nodeForString(par1.toString());
                QString link = linkForNode(n, relative);
                addLink(link, arg, &html);
                html += QLatin1String("</b>");
            }
            else if (parseArg(src, funcTag, &i, srcSize, &arg, &par1)) {
                const Node* n = qdb_->findFunctionNode(par1.toString(), relative, genus);
                QString link = linkForNode(n, relative);
                addLink(link, arg, &html);
                par1 = QStringRef();
            }
            else if (parseArg(src, typeTag, &i, srcSize, &arg, &par1)) {
                par1 = QStringRef();
                const Node* n = qdb_->findTypeNode(arg.toString(), relative, genus);
                html += QLatin1String("<span class=\"type\">");
                if (n && (n->isQmlBasicType() || n->isJsBasicType())) {
                    if (relative && (relative->genus() == n->genus() || genus == n->genus()))
                        addLink(linkForNode(n,relative), arg, &html);
                    else
                        html += arg;
                }
                else
                    addLink(linkForNode(n,relative), arg, &html);
                html += QLatin1String("</span>");
            }
            else if (parseArg(src, headerTag, &i, srcSize, &arg, &par1)) {
                par1 = QStringRef();
                if (arg.startsWith(QLatin1Char('&')))
                    html += arg;
                else {
                    const Node* n = qdb_->findNodeForInclude(QStringList(arg.toString()));
                    if (n && n != relative)
                        addLink(linkForNode(n,relative), arg, &html);
                    else
                        html += arg;
                }
            }
            else {
                html += charLangle;
                html += charAt;
            }
        }
        else {
            html += src.at(i++);
        }
    }

    // replace all
    // "<@comment>" -> "<span class=\"comment\">";
    // "<@preprocessor>" -> "<span class=\"preprocessor\">";
    // "<@string>" -> "<span class=\"string\">";
    // "<@char>" -> "<span class=\"char\">";
    // "<@number>" -> "<span class=\"number\">";
    // "<@op>" -> "<span class=\"operator\">";
    // "<@type>" -> "<span class=\"type\">";
    // "<@name>" -> "<span class=\"name\">";
    // "<@keyword>" -> "<span class=\"keyword\">";
    // "</@(?:comment|preprocessor|string|char|number|op|type|name|keyword)>" -> "</span>"
    src = html;
    html = QString();
    html.reserve(src.size());
    static const QLatin1String spanTags[] = {
        QLatin1String("comment>"),       QLatin1String("<span class=\"comment\">"),
        QLatin1String("preprocessor>"),  QLatin1String("<span class=\"preprocessor\">"),
        QLatin1String("string>"),        QLatin1String("<span class=\"string\">"),
        QLatin1String("char>"),          QLatin1String("<span class=\"char\">"),
        QLatin1String("number>"),        QLatin1String("<span class=\"number\">"),
        QLatin1String("op>"),            QLatin1String("<span class=\"operator\">"),
        QLatin1String("type>"),          QLatin1String("<span class=\"type\">"),
        QLatin1String("name>"),          QLatin1String("<span class=\"name\">"),
        QLatin1String("keyword>"),       QLatin1String("<span class=\"keyword\">")
    };
    int nTags = 9;
    // Update the upper bound of k in the following code to match the length
    // of the above array.
    for (int i = 0, n = src.size(); i < n;) {
        if (src.at(i) == QLatin1Char('<')) {
            if (src.at(i + 1) == QLatin1Char('@')) {
                i += 2;
                bool handled = false;
                for (int k = 0; k != nTags; ++k) {
                    const QLatin1String& tag = spanTags[2 * k];
                    if (i + tag.size() <= src.length() &&
                        tag == QStringRef(&src, i, tag.size())) {
                        html += spanTags[2 * k + 1];
                        i += tag.size();
                        handled = true;
                        break;
                    }
                }
                if (!handled) {
                    // drop 'our' unknown tags (the ones still containing '@')
                    while (i < n && src.at(i) != QLatin1Char('>'))
                        ++i;
                    ++i;
                }
                continue;
            }
            else if (src.at(i + 1) == QLatin1Char('/') && src.at(i + 2) == QLatin1Char('@')) {
                i += 3;
                bool handled = false;
                for (int k = 0; k != nTags; ++k) {
                    const QLatin1String& tag = spanTags[2 * k];
                    if (i + tag.size() <= src.length() &&
                        tag == QStringRef(&src, i, tag.size())) {
                        html += QLatin1String("</span>");
                        i += tag.size();
                        handled = true;
                        break;
                    }
                }
                if (!handled) {
                    // drop 'our' unknown tags (the ones still containing '@')
                    while (i < n && src.at(i) != QLatin1Char('>'))
                        ++i;
                    ++i;
                }
                continue;
            }
        }
        html += src.at(i);
        ++i;
    }
    return html;
}

void HtmlGenerator::generateLink(const Atom* atom, CodeMarker* marker)
{
    static QRegExp camelCase("[A-Z][A-Z][a-z]|[a-z][A-Z0-9]|_");

    if (funcLeftParen.indexIn(atom->string()) != -1 && marker->recognizeLanguage("Cpp")) {
        // hack for C++: move () outside of link
        int k = funcLeftParen.pos(1);
        out() << protectEnc(atom->string().left(k));
        if (link_.isEmpty()) {
            if (showBrokenLinks)
                out() << "</i>";
        } else {
            out() << "</a>";
        }
        inLink_ = false;
        out() << protectEnc(atom->string().mid(k));
    } else {
        out() << protectEnc(atom->string());
    }
}

QString HtmlGenerator::registerRef(const QString& ref)
{
    QString clean = Generator::cleanRef(ref);

    for (;;) {
        QString& prevRef = refMap[clean.toLower()];
        if (prevRef.isEmpty()) {
            prevRef = ref;
            break;
        } else if (prevRef == ref) {
            break;
        }
        clean += QLatin1Char('x');
    }
    return clean;
}

QString HtmlGenerator::protectEnc(const QString &string)
{
#ifndef QT_NO_TEXTCODEC
    return protect(string, outputEncoding);
#else
    return protect(string);
#endif
}

QString HtmlGenerator::protect(const QString &string, const QString &outputEncoding)
{
#define APPEND(x) \
    if (html.isEmpty()) { \
    html = string; \
    html.truncate(i); \
} \
    html += (x);

    QString html;
    int n = string.length();

    for (int i = 0; i < n; ++i) {
        QChar ch = string.at(i);

        if (ch == QLatin1Char('&')) {
            APPEND("&amp;");
        } else if (ch == QLatin1Char('<')) {
            APPEND("&lt;");
        } else if (ch == QLatin1Char('>')) {
            APPEND("&gt;");
        } else if (ch == QLatin1Char('"')) {
            APPEND("&quot;");
        } else if ((outputEncoding == QLatin1String("ISO-8859-1") && ch.unicode() > 0x007F)
                   || (ch == QLatin1Char('*') && i + 1 < n && string.at(i) == QLatin1Char('/'))
                   || (ch == QLatin1Char('.') && i > 2 && string.at(i - 2) == QLatin1Char('.'))) {
            // we escape '*/' and the last dot in 'e.g.' and 'i.e.' for the Javadoc generator
            APPEND("&#x");
            html += QString::number(ch.unicode(), 16);
            html += QLatin1Char(';');
        } else {
            if (!html.isEmpty())
                html += ch;
        }
    }

    if (!html.isEmpty())
        return html;
    return string;

#undef APPEND
}

QString HtmlGenerator::fileBase(const Node *node) const
{
    QString result = Generator::fileBase(node);
    if (!node->isAggregate() && node->status() == Node::Obsolete)
        result += QLatin1String("-obsolete");
    return result;
}

QString HtmlGenerator::fileName(const Node *node)
{
    if (node->type() == Node::Document) {
        if (static_cast<const DocumentNode *>(node)->docSubtype() == Node::ExternalPage)
            return node->name();
        if (static_cast<const DocumentNode *>(node)->docSubtype() == Node::Image)
            return node->name();
    }
    return Generator::fileName(node);
}

QString HtmlGenerator::refForNode(const Node *node)
{
    const FunctionNode *func;
    const TypedefNode *typedeffe;
    QString ref;

    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
    default:
        break;
    case Node::Enum:
        ref = node->name() + "-enum";
        break;
    case Node::Typedef:
        typedeffe = static_cast<const TypedefNode *>(node);
        if (typedeffe->associatedEnum()) {
            return refForNode(typedeffe->associatedEnum());
        }
        else {
            ref = node->name() + "-typedef";
        }
        break;
    case Node::Function:
        func = static_cast<const FunctionNode *>(node);
        if (func->hasOneAssociatedProperty() && func->doc().isEmpty()) {
            return refForNode(func->firstAssociatedProperty());
        }
        else {
            ref = func->name();
            if (func->overloadNumber() != 0)
                ref += QLatin1Char('-') + QString::number(func->overloadNumber());
        }
        break;
    case Node::Document:
        break;
    case Node::QmlProperty:
        if (node->isAttached())
            ref = node->name() + "-attached-prop";
        else
            ref = node->name() + "-prop";
        break;
    case Node::QmlPropertyGroup:
    case Node::Property:
        ref = node->name() + "-prop";
        break;
    case Node::QmlSignal:
        ref = node->name() + "-signal";
        break;
    case Node::QmlSignalHandler:
        ref = node->name() + "-signal-handler";
        break;
    case Node::QmlMethod:
        func = static_cast<const FunctionNode *>(node);
        ref = func->name() + "-method";
        if (func->overloadNumber() != 0)
            ref += QLatin1Char('-') + QString::number(func->overloadNumber());
        break;
    case Node::Variable:
        ref = node->name() + "-var";
        break;
    }
    return registerRef(ref);
}

/*!
  This function is called for links, i.e. for words that
  are marked with the qdoc link command. For autolinks
  that are not marked with the qdoc link command, the
  getAutoLink() function is called

  It returns the string for a link found by using the data
  in the \a atom to search the database. It also sets \a node
  to point to the target node for that link. \a relative points
  to the node holding the qdoc comment where the link command
  was found.
 */
QString HtmlGenerator::getLink(const Atom *atom, const Node *relative, const Node** node)
{
    const QString& t = atom->string();
    if (t.at(0) == QChar('h')) {
        if (t.startsWith("http:") || t.startsWith("https:"))
            return t;
    }
    else if (t.at(0) == QChar('f')) {
        if (t.startsWith("file:") || t.startsWith("ftp:"))
            return t;
    }
    else if (t.at(0) == QChar('m')) {
        if (t.startsWith("mailto:"))
            return t;
    }
    return getAutoLink(atom, relative, node);
}

/*!
  This function is called for autolinks, i.e. for words that
  are not marked with the qdoc link command that qdoc has
  reason to believe should be links. For links marked with
  the qdoc link command, the getLink() function is called.

  It returns the string for a link found by using the data
  in the \a atom to search the database. It also sets \a node
  to point to the target node for that link. \a relative points
  to the node holding the qdoc comment where the link command
  was found.
 */
QString HtmlGenerator::getAutoLink(const Atom *atom, const Node *relative, const Node** node)
{
    QString ref;

    *node = qdb_->findNodeForAtom(atom, relative, ref);
    if (!(*node)) {
        return QString();
    }

    QString link = (*node)->url();
    if (link.isEmpty()) {
        link = linkForNode(*node, relative);
        if ((*node)->docSubtype() == Node::Image)
            link = "images/used-in-examples/" + link;
    }
    if (!ref.isEmpty()) {
        int hashtag = link.lastIndexOf(QChar('#'));
        if (hashtag != -1)
            link.truncate(hashtag);
        link += QLatin1Char('#') + ref;
    }
    return link;
}

/*!
  Construct the link string for the \a node and return it.
  The \a relative node is use to decide the link we are
  generating is in the same file as the target. Note the
  relative node can be 0, which pretty much guarantees
  that the link and the target aren't in the same file.
  */
QString HtmlGenerator::linkForNode(const Node *node, const Node *relative)
{
    if (node == 0)
        return QString();
    if (!node->url().isEmpty())
        return node->url();
    if (fileBase(node).isEmpty())
        return QString();
    if (node->access() == Node::Private)
        return QString();
    QString fn = fileName(node);
    if (node && node->parent() &&
        (node->parent()->isQmlType() || node->parent()->isJsType())
        && node->parent()->isAbstract()) {
        if (Generator::qmlTypeContext()) {
            if (Generator::qmlTypeContext()->inherits(node->parent())) {
                fn = fileName(Generator::qmlTypeContext());
            }
            else if (node->parent()->isInternal()) {
                node->doc().location().warning(tr("Cannot link to property in internal type '%1'").arg(node->parent()->name()));
                return QString();
            }
        }
    }
    QString link = fn;

    if ((!node->isAggregate() && !node->isCollectionNode())
        || node->isQmlPropertyGroup() || node->isJsPropertyGroup()) {
        QString ref = refForNode(node);
        if (relative && fn == fileName(relative) && ref == refForNode(relative))
            return QString();

        link += QLatin1Char('#');
        link += ref;
    }
    /*
      If the output is going to subdirectories, then if the
      two nodes will be output to different directories, then
      the link must go up to the parent directory and then
      back down into the other subdirectory.
     */
    if (node && relative && (node != relative)) {
        if (useOutputSubdirs() && !node->isExternalPage() &&
               node->outputSubdirectory() != relative->outputSubdirectory()) {
            if (link.startsWith(QString(node->outputSubdirectory() + QLatin1Char('/')))) {
                link.prepend(QString("../"));
            }
            else {
                link.prepend(QString("../" + node->outputSubdirectory() + QLatin1Char('/')));
            }
        }
    }
    return link;
}

void HtmlGenerator::generateFullName(const Node *apparentNode, const Node *relative, const Node *actualNode)
{
    if (actualNode == 0)
        actualNode = apparentNode;
    out() << "<a href=\"" << linkForNode(actualNode, relative);
    if (true || relative == 0 || relative->status() != actualNode->status()) {
        if (actualNode->status() == Node::Obsolete)
            out() << "\" class=\"obsolete";
    }
    out() << "\">";
    out() << protectEnc(apparentNode->fullName(relative));
    out() << "</a>";
}

void HtmlGenerator::generateDetailedMember(const Node *node,
                                           const Aggregate *relative,
                                           CodeMarker *marker)
{
    const EnumNode *etn;
#ifdef GENERATE_MAC_REFS
    generateMacRef(node, marker);
#endif
    generateExtractionMark(node, MemberMark);
    generateKeywordAnchors(node);
    QString nodeRef = 0;
    if (node->isSharedCommentNode()) {
        const SharedCommentNode *scn = reinterpret_cast<const SharedCommentNode*>(node);
        const QVector<Node*>& collective = scn->collective();
        if (collective.size() > 1)
            out() << "<div class=\"fngroup\">\n";
        foreach (const Node* n, collective) {
            if (n->isFunction()) {
                nodeRef = refForNode(n);
                out() << "<h3 class=\"fn fngroupitem\" id=\"" << nodeRef << "\">";
                out() << "<a name=\"" + nodeRef + "\"></a>";
                generateSynopsis(n, relative, marker, Section::Details);
                out() << "</h3>";
            }
        }
        if (collective.size() > 1)
            out() << "</div>";
        out() << divNavTop << '\n';
    }
    else {
        nodeRef = refForNode(node);
        if (node->isEnumType() && (etn = static_cast<const EnumNode *>(node))->flagsType()) {
#ifdef GENERATE_MAC_REFS
            generateMacRef(etn->flagsType(), marker);
#endif
            out() << "<h3 class=\"flags\" id=\"" << nodeRef << "\">";
            out() << "<a name=\"" + nodeRef + "\"></a>";
            generateSynopsis(etn, relative, marker, Section::Details);
            out() << "<br/>";
            generateSynopsis(etn->flagsType(), relative, marker, Section::Details);
            out() << "</h3>\n";
        }
        else {
            out() << "<h3 class=\"fn\" id=\"" << nodeRef << "\">";
            out() << "<a name=\"" + nodeRef + "\"></a>";
            generateSynopsis(node, relative, marker, Section::Details);
            out() << "</h3>" << divNavTop << '\n';
        }
    }

    generateStatus(node, marker);
    generateBody(node, marker);
    generateOverloadedSignal(node, marker);
    generateThreadSafeness(node, marker);
    generateSince(node, marker);

    if (node->isProperty()) {
        const PropertyNode *property = static_cast<const PropertyNode *>(node);
        Section section(Section::Accessors, Section::Active);

        section.appendMembers(property->getters().toVector());
        section.appendMembers(property->setters().toVector());
        section.appendMembers(property->resetters().toVector());

        if (!section.members().isEmpty()) {
            out() << "<p><b>Access functions:</b></p>\n";
            generateSectionList(section, node, marker);
        }

        Section notifiers(Section::Accessors, Section::Active);
        notifiers.appendMembers(property->notifiers().toVector());

        if (!notifiers.members().isEmpty()) {
            out() << "<p><b>Notifier signal:</b></p>\n";
            //out() << "<p>This signal is emitted when the property value is changed.</p>\n";
            generateSectionList(notifiers, node, marker);
        }
    }
    else if (node->isFunction()) {
        const FunctionNode* fn = static_cast<const FunctionNode*>(node);
        if (fn->isPrivateSignal())
            generatePrivateSignalNote(node, marker);
        if (fn->isInvokable())
            generateInvokableNote(node, marker);
        generateAssociatedPropertyNotes(fn);
    }
    else if (node->isEnumType()) {
        const EnumNode *etn = static_cast<const EnumNode *>(node);
        if (etn->flagsType()) {
            out() << "<p>The " << protectEnc(etn->flagsType()->name())
                  << " type is a typedef for "
                  << "<a href=\"" << qflagsHref_ << "\">QFlags</a>&lt;"
                  << protectEnc(etn->name())
                  << "&gt;. It stores an OR combination of "
                  << protectEnc(etn->name())
                  << " values.</p>\n";
        }
    }
    generateAlsoList(node, marker);
    generateExtractionMark(node, EndMark);
}

int HtmlGenerator::hOffset(const Node *node)
{
    switch (node->type()) {
    case Node::Namespace:
    case Node::Class:
    case Node::Module:
        return 2;
    case Node::QmlModule:
    case Node::QmlBasicType:
    case Node::QmlType:
    case Node::Document:
        return 1;
    case Node::Enum:
    case Node::Typedef:
    case Node::Function:
    case Node::Property:
    default:
        return 3;
    }
}

bool HtmlGenerator::isThreeColumnEnumValueTable(const Atom *atom)
{
    while (atom != 0 && !(atom->type() == Atom::ListRight && atom->string() == ATOM_LIST_VALUE)) {
        if (atom->type() == Atom::ListItemLeft && !matchAhead(atom, Atom::ListItemRight))
            return true;
        atom = atom->next();
    }
    return false;
}


const QPair<QString,QString> HtmlGenerator::anchorForNode(const Node *node)
{
    QPair<QString,QString> anchorPair;

    anchorPair.first = Generator::fileName(node);
    if (node->type() == Node::Document) {
        const DocumentNode *docNode = static_cast<const DocumentNode*>(node);
        anchorPair.second = docNode->title();
    }

    return anchorPair;
}

#ifdef GENERATE_MAC_REFS
/*
  No longer valid.
 */
void HtmlGenerator::generateMacRef(const Node *node, CodeMarker *marker)
{
    if (!pleaseGenerateMacRef || marker == 0)
        return;

    QStringList macRefs = marker->macRefsForNode(node);
    foreach (const QString &macRef, macRefs)
        out() << "<a name=\"" << "//apple_ref/" << macRef << "\"></a>\n";
}
#endif

void HtmlGenerator::beginLink(const QString &link, const Node *node, const Node *relative)
{
    link_ = link;
    if (link_.isEmpty()) {
        if (showBrokenLinks)
            out() << "<i>";
    }
    else if (node == 0 || (relative != 0 && node->status() == relative->status()))
        out() << "<a href=\"" << link_ << "\">";
    else if (node->status() == Node::Obsolete)
        out() << "<a href=\"" << link_ << "\" class=\"obsolete\">";
    else
        out() << "<a href=\"" << link_ << "\">";
    inLink_ = true;
}

void HtmlGenerator::endLink()
{
    if (inLink_) {
        if (link_.isEmpty()) {
            if (showBrokenLinks)
                out() << "</i>";
        }
        else {
            if (inObsoleteLink) {
                out() << "<sup>(obsolete)</sup>";
            }
            out() << "</a>";
        }
    }
    inLink_ = false;
    inObsoleteLink = false;
}

/*!
  Generates the summary for the \a section. Only used for
  sections of QML element documentation.
 */
void HtmlGenerator::generateQmlSummary(const Section& section,
                                       const Node *relative,
                                       CodeMarker *marker)
{
    if (!section.members().isEmpty()) {
        out() << "<ul>\n";
        NodeVector::const_iterator m = section.members().constBegin();
        while (m != section.members().constEnd()) {
            out() << "<li class=\"fn\">";
            generateQmlItem(*m,relative,marker,true);
            if ((*m)->type() == Node::QmlPropertyGroup) {
                const QmlPropertyGroupNode* qpgn = static_cast<const QmlPropertyGroupNode*>(*m);
                if (!qpgn->childNodes().isEmpty()) {
                    NodeList::ConstIterator p = qpgn->childNodes().constBegin();
                    out() << "<ul>\n";
                    while (p != qpgn->childNodes().constEnd()) {
                        if ((*p)->type() == Node::QmlProperty) {
                            out() << "<li class=\"fn\">";
                            generateQmlItem(*p, relative, marker, true);
                            out() << "</li>\n";
                        }
                        ++p;
                    }
                    out() << "</ul>\n";
                }
            }
            out() << "</li>\n";
            ++m;
        }
        out() << "</ul>\n";
    }
}

/*!
  Outputs the html detailed documentation for a section
  on a QML element reference page.
 */
void HtmlGenerator::generateDetailedQmlMember(Node *node,
                                              const Aggregate *relative,
                                              CodeMarker *marker)
{
    QmlPropertyNode* qpn = 0;
#ifdef GENERATE_MAC_REFS
    generateMacRef(node, marker);
#endif
    generateExtractionMark(node, MemberMark);
    generateKeywordAnchors(node);

    QString qmlItemHeader("<div class=\"qmlproto\">\n"
                          "<div class=\"table\"><table class=\"qmlname\">\n");

    QString qmlItemStart("<tr valign=\"top\" class=\"odd\" id=\"%1\">\n"
                    "<td class=\"%2\"><p>\n<a name=\"%3\"></a>");
    QString qmlItemEnd("</p></td></tr>\n");

    QString qmlItemFooter("</table></div>\n</div>");

    out() << "<div class=\"qmlitem\">";
    QString nodeRef = refForNode(node);
    if (node->type() == Node::QmlPropertyGroup) {
        const QmlPropertyGroupNode* qpgn = static_cast<const QmlPropertyGroupNode*>(node);
        NodeList::ConstIterator p = qpgn->childNodes().constBegin();
        out() << "<div class=\"qmlproto\">";
        out() << "<div class=\"table\"><table class=\"qmlname\">";

        QString heading = qpgn->name() + " group";
        out() << "<tr valign=\"top\" class=\"even\" id=\"" << nodeRef << "\">";
        out() << "<th class=\"centerAlign\"><p>";
        out() << "<a name=\"" + nodeRef + "\"></a>";
        out() << "<b>" << heading << "</b>";
        out() << "</p></th></tr>";
        while (p != qpgn->childNodes().constEnd()) {
            if ((*p)->type() == Node::QmlProperty) {
                qpn = static_cast<QmlPropertyNode*>(*p);
                nodeRef = refForNode(qpn);
                out() << "<tr valign=\"top\" class=\"odd\" id=\"" << nodeRef << "\">";
                out() << "<td class=\"tblQmlPropNode\"><p>";
                out() << "<a name=\"" + nodeRef + "\"></a>";

                if (!qpn->isWritable())
                    out() << "<span class=\"qmlreadonly\">[read-only] </span>";
                if (qpn->isDefault())
                    out() << "<span class=\"qmldefault\">[default] </span>";
                generateQmlItem(qpn, relative, marker, false);
                out() << "</p></td></tr>";
            }
            ++p;
        }
        out() << "</table></div>";
        out() << "</div>";
    }
    else if (node->type() == Node::QmlProperty) {
        qpn = static_cast<QmlPropertyNode*>(node);
        out() << qmlItemHeader;
        out() << qmlItemStart.arg(nodeRef, "tblQmlPropNode", refForNode(qpn));

        if (!qpn->isReadOnlySet()) {
            if (qpn->declarativeCppNode())
                qpn->setReadOnly(!qpn->isWritable());
        }
        if (qpn->isReadOnly())
            out() << "<span class=\"qmlreadonly\">[read-only] </span>";
        if (qpn->isDefault())
            out() << "<span class=\"qmldefault\">[default] </span>";

        generateQmlItem(qpn, relative, marker, false);
        out() << qmlItemEnd;
        out() << qmlItemFooter;
    } else if (node->isSharedCommentNode()) {
        const SharedCommentNode *scn = reinterpret_cast<const SharedCommentNode*>(node);
        const QVector<Node*>& collective = scn->collective();
        if (collective.size() > 1)
            out() << "<div class=\"fngroup\">\n";
        out() << qmlItemHeader;
        for (const auto m : collective) {
            if (m->type() == Node::QmlSignal ||
                     m->type() == Node::QmlSignalHandler ||
                     m->type() == Node::QmlMethod) {
                out() << qmlItemStart.arg(nodeRef, "tblQmlFuncNode", refForNode(m));
                generateSynopsis(m, relative, marker, Section::Details, false);
                out() << qmlItemEnd;
            }
        }
        out() << qmlItemFooter;
        if (collective.size() > 1)
            out() << "</div>";
    } else { // assume the node is a method/signal handler
        out() << qmlItemHeader;
        out() << qmlItemStart.arg(nodeRef, "tblQmlFuncNode", refForNode(node));
        generateSynopsis(node, relative, marker, Section::Details, false);
        out() << qmlItemEnd;
        out() << qmlItemFooter;
    }

    out() << "<div class=\"qmldoc\">";
    generateStatus(node, marker);
    generateBody(node, marker);
    generateThreadSafeness(node, marker);
    generateSince(node, marker);
    generateAlsoList(node, marker);
    out() << "</div></div>";
    generateExtractionMark(node, EndMark);
}

/*!
  Output the "Inherits" line for the QML element,
  if there should be one.
 */
void HtmlGenerator::generateQmlInherits(QmlTypeNode* qcn, CodeMarker* marker)
{
    if (!qcn)
        return;
    QmlTypeNode* base = qcn->qmlBaseNode();
    while (base && base->isInternal()) {
        base = base->qmlBaseNode();
    }
    if (base) {
        Text text;
        text << Atom::ParaLeft << "Inherits ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(base));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, base->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << Atom::ParaRight;
        generateText(text, qcn, marker);
    }
}

/*!
  Output the "[Xxx instantiates the C++ class QmlGraphicsXxx]"
  line for the QML element, if there should be one.

  If there is no class node, or if the class node status
  is set to Node::Internal, do nothing.
 */
void HtmlGenerator::generateQmlInstantiates(QmlTypeNode* qcn, CodeMarker* marker)
{
    ClassNode* cn = qcn->classNode();
    if (cn && (cn->status() != Node::Internal)) {
        Text text;
        text << Atom::ParaLeft;
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        QString name = qcn->name();
        /*
          Remove the "QML:" prefix, if present.
          It shouldn't be present anymore.
        */
        if (name.startsWith(QLatin1String("QML:")))
            name = name.mid(4); // remove the "QML:" prefix
        text << Atom(Atom::String, name);
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << " instantiates the C++ class ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << Atom::ParaRight;
        generateText(text, qcn, marker);
    }
}

/*!
  Output the "[QmlGraphicsXxx is instantiated by QML Type Xxx]"
  line for the class, if there should be one.

  If there is no QML element, or if the class node status
  is set to Node::Internal, do nothing.
 */
void HtmlGenerator::generateInstantiatedBy(ClassNode* cn, CodeMarker* marker)
{
    if (cn &&  cn->status() != Node::Internal && cn->qmlElement() != 0) {
        const QmlTypeNode* qcn = cn->qmlElement();
        Text text;
        text << Atom::ParaLeft;
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(cn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, cn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        if (qcn->isQmlType())
            text << " is instantiated by QML Type ";
        else
            text << " is instantiated by Javascript Type ";
        text << Atom(Atom::LinkNode,CodeMarker::stringForNode(qcn));
        text << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
        text << Atom(Atom::String, qcn->name());
        text << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
        text << Atom::ParaRight;
        generateText(text, cn, marker);
    }
}

void HtmlGenerator::generateExtractionMark(const Node *node, ExtractionMarkType markType)
{
    if (markType != EndMark) {
        out() << "<!-- $$$" + node->name();
        if (markType == MemberMark) {
            if (node->type() == Node::Function) {
                const FunctionNode *func = static_cast<const FunctionNode *>(node);
                if (!func->hasAssociatedProperties()) {
                    if (func->overloadNumber() == 0)
                        out() << "[overload1]";
                    out() << "$$$" + func->name() + func->rawParameters().remove(' ');
                }
            } else if (node->type() == Node::Property) {
                out() << "-prop";
                const PropertyNode *prop = static_cast<const PropertyNode *>(node);
                const NodeList &list = prop->functions();
                foreach (const Node *propFuncNode, list) {
                    if (propFuncNode->type() == Node::Function) {
                        const FunctionNode *func = static_cast<const FunctionNode *>(propFuncNode);
                        out() << "$$$" + func->name() + func->rawParameters().remove(' ');
                    }
                }
            } else if (node->type() == Node::Enum) {
                const EnumNode *enumNode = static_cast<const EnumNode *>(node);
                foreach (const EnumItem &item, enumNode->items())
                    out() << "$$$" + item.name();
            }
        } else if (markType == BriefMark) {
            out() << "-brief";
        } else if (markType == DetailedDescriptionMark) {
            out() << "-description";
        }
        out() << " -->\n";
    } else {
        out() << "<!-- @@@" + node->name() + " -->\n";
    }
}


/*!
  This function outputs one or more manifest files in XML.
  They are used by Creator.
 */
void HtmlGenerator::generateManifestFiles()
{
    generateManifestFile("examples", "example");
    generateManifestFile("demos", "demo");
    qdb_->exampleNodeMap().clear();
    manifestMetaContent.clear();
}

/*!
  This function is called by generateManifestFiles(), once
  for each manifest file to be generated. \a manifest is the
  type of manifest file.
 */
void HtmlGenerator::generateManifestFile(const QString &manifest, const QString &element)
{
    ExampleNodeMap& exampleNodeMap = qdb_->exampleNodeMap();
    if (exampleNodeMap.isEmpty())
        return;
    QString fileName = manifest +"-manifest.xml";
    QFile file(outputDir() + QLatin1Char('/') + fileName);
    bool demos = false;
    if (manifest == QLatin1String("demos"))
        demos = true;

    bool proceed = false;
    ExampleNodeMap::Iterator i = exampleNodeMap.begin();
    while (i != exampleNodeMap.end()) {
        const ExampleNode* en = i.value();
        if (demos) {
            if (en->name().startsWith("demos")) {
                proceed = true;
                break;
            }
        }
        else if (!en->name().startsWith("demos")) {
            proceed = true;
            break;
        }
        ++i;
    }
    if (!proceed || !file.open(QFile::WriteOnly | QFile::Text))
        return;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("instructionals");
    writer.writeAttribute("module", project);
    writer.writeStartElement(manifest);

    QStringList usedAttributes;
    i = exampleNodeMap.begin();
    while (i != exampleNodeMap.end()) {
        const ExampleNode* en = i.value();
        if (demos) {
            if (!en->name().startsWith("demos")) {
                ++i;
                continue;
            }
        }
        else if (en->name().startsWith("demos")) {
            ++i;
            continue;
        }
        // attributes that are always written for the element
        usedAttributes.clear();
        usedAttributes << "name" << "docUrl" << "projectPath";

        writer.writeStartElement(element);
        writer.writeAttribute("name", en->title());
        QString docUrl = manifestDir + fileBase(en) + ".html";
        writer.writeAttribute("docUrl", docUrl);
        QStringList proFiles;
        foreach (const Node* child, en->childNodes()) {
            if (child->docSubtype() == Node::File) {
                QString file = child->name();
                if (file.endsWith(".pro") || file.endsWith(".qmlproject")) {
                    proFiles << file;
                }
            }
        }
        if (!proFiles.isEmpty()) {
            if (proFiles.size() == 1) {
                writer.writeAttribute("projectPath", examplesPath + proFiles[0]);
            }
            else {
                QString exampleName = en->name().split('/').last();
                bool proWithExampleNameFound = false;
                for (int j = 0; j < proFiles.size(); j++)
                {
                    if (proFiles[j].endsWith(QStringLiteral("%1/%1.pro").arg(exampleName))
                            || proFiles[j].endsWith(QStringLiteral("%1/%1.qmlproject").arg(exampleName))) {
                        writer.writeAttribute("projectPath", examplesPath + proFiles[j]);
                        proWithExampleNameFound = true;
                        break;
                    }
                }
                if (!proWithExampleNameFound)
                    writer.writeAttribute("projectPath", examplesPath + proFiles[0]);
            }
        }
        if (!en->imageFileName().isEmpty()) {
            writer.writeAttribute("imageUrl", manifestDir + en->imageFileName());
            usedAttributes << "imageUrl";
        }

        QString fullName = project + QLatin1Char('/') + en->title();
        QSet<QString> tags;
        for (int idx=0; idx < manifestMetaContent.size(); ++idx) {
            foreach (const QString &name, manifestMetaContent[idx].names) {
                bool match = false;
                int wildcard = name.indexOf(QChar('*'));
                switch (wildcard) {
                case -1: // no wildcard, exact match
                    match = (fullName == name);
                    break;
                case 0: // '*' matches all
                    match = true;
                    break;
                default: // match with wildcard at the end
                    match = fullName.startsWith(name.left(wildcard));
                }
                if (match) {
                    tags += manifestMetaContent[idx].tags;
                    foreach (const QString &attr, manifestMetaContent[idx].attributes) {
                        QLatin1Char div(':');
                        QStringList attrList = attr.split(div);
                        if (attrList.count() == 1)
                            attrList.append(QStringLiteral("true"));
                        QString attrName = attrList.takeFirst();
                        if (!usedAttributes.contains(attrName)) {
                            writer.writeAttribute(attrName, attrList.join(div));
                            usedAttributes << attrName;
                        }
                    }
                }
            }
        }

        writer.writeStartElement("description");
        Text brief = en->doc().briefText();
        if (!brief.isEmpty())
            writer.writeCDATA(brief.toString());
        else
            writer.writeCDATA(QString("No description available"));
        writer.writeEndElement(); // description

        // Add words from module name as tags
        // QtQuickControls -> qt,quick,controls
        // QtOpenGL -> qt,opengl
        QRegExp re("([A-Z]+[a-z0-9]*(3D|GL)?)");
        int pos = 0;
        while ((pos = re.indexIn(project, pos)) != -1) {
            tags << re.cap(1).toLower();
            pos += re.matchedLength();
        }

        // Include tags added via \meta {tag} {tag1[,tag2,...]}
        // within \example topic
        for (const auto &tag : en->doc().metaTagMap().values("tag"))
            tags += QSet<QString>::fromList(tag.toLower().split(QLatin1Char(',')));

        tags += QSet<QString>::fromList(en->title().toLower().split(QLatin1Char(' ')));

        // Clean up tags, exclude invalid and common words
        QSet<QString>::iterator tag_it = tags.begin();
        QSet<QString> modified;
        while (tag_it != tags.end()) {
            QString s = *tag_it;
            if (s.at(0) == '(')
                s.remove(0, 1).chop(1);
            if (s.endsWith(QLatin1Char(':')))
                s.chop(1);

            if (s.length() < 2
                || s.at(0).isDigit()
                || s.at(0) == '-'
                || s == QLatin1String("qt")
                || s == QLatin1String("the")
                || s == QLatin1String("and")
                || s.startsWith(QLatin1String("example"))
                || s.startsWith(QLatin1String("chapter")))
                tag_it = tags.erase(tag_it);
            else if (s != *tag_it) {
                modified << s;
                tag_it = tags.erase(tag_it);
            }
            else
                ++tag_it;
        }
        tags += modified;

        if (!tags.isEmpty()) {
            writer.writeStartElement("tags");
            bool wrote_one = false;
            QStringList sortedTags = tags.toList();
            sortedTags.sort();
            foreach (const QString &tag, sortedTags) {
                if (wrote_one)
                    writer.writeCharacters(",");
                writer.writeCharacters(tag);
                wrote_one = true;
            }
            writer.writeEndElement(); // tags
        }

        QString ename = en->name().mid(en->name().lastIndexOf('/')+1);
        QMap<int, const Node*> filesToOpen;
        foreach (const Node* child, en->childNodes()) {
            if (child->docSubtype() == Node::File) {
                QFileInfo fileInfo(child->name());
                QString fileName = fileInfo.fileName().toLower();
                // open .qml, .cpp and .h files with a
                // basename matching the example (project) name
                // QMap key indicates the priority -
                // the lowest value will be the top-most file
                if ((fileInfo.baseName().compare(ename, Qt::CaseInsensitive) == 0)) {
                    if (fileName.endsWith(".qml"))
                        filesToOpen.insert(0, child);
                    else if (fileName.endsWith(".cpp"))
                        filesToOpen.insert(1, child);
                    else if (fileName.endsWith(".h"))
                        filesToOpen.insert(2, child);
                }
                // main.qml takes precedence over main.cpp
                else if (fileName.endsWith("main.qml")) {
                    filesToOpen.insert(3, child);
                }
                else if (fileName.endsWith("main.cpp")) {
                    filesToOpen.insert(4, child);
               }
            }
        }

        QMap<int, const Node*>::const_iterator it = filesToOpen.constEnd();
        while (it != filesToOpen.constBegin()) {
            writer.writeStartElement("fileToOpen");
            if (--it == filesToOpen.constBegin()) {
                writer.writeAttribute(QStringLiteral("mainFile"), QStringLiteral("true"));
            }
            writer.writeCharacters(examplesPath + it.value()->name());
            writer.writeEndElement();
        }

        writer.writeEndElement(); // example
        ++i;
    }

    writer.writeEndElement(); // examples
    writer.writeEndElement(); // instructionals
    writer.writeEndDocument();
    file.close();
}

/*!
  Reads metacontent - additional attributes and tags to apply
  when generating manifest files, read from config. Takes the
  configuration class \a config as a parameter.

  The manifest metacontent map is cleared immediately after
  the manifest files have been generated.
 */
void HtmlGenerator::readManifestMetaContent(const Config &config)
{
    QStringList names = config.getStringList(CONFIG_MANIFESTMETA + Config::dot + QStringLiteral("filters"));

    foreach (const QString &manifest, names) {
        ManifestMetaFilter filter;
        QString prefix = CONFIG_MANIFESTMETA + Config::dot + manifest + Config::dot;
        filter.names = config.getStringSet(prefix + QStringLiteral("names"));
        filter.attributes = config.getStringSet(prefix + QStringLiteral("attributes"));
        filter.tags = config.getStringSet(prefix + QStringLiteral("tags"));
        manifestMetaContent.append(filter);
    }
}

/*!
  Find global entities that have documentation but no
  \e{relates} comand. Report these as errors if they
  are not also marked \e {internal}.

  type: Class
  type: Namespace

  subtype: Example
  subtype: External page
  subtype: Group
  subtype: Header file
  subtype: Module
  subtype: Page
  subtype: QML basic type
  subtype: QML class
  subtype: QML module
 */
void HtmlGenerator::reportOrphans(const Aggregate* parent)
{
    const NodeList& children = parent->childNodes();
    if (children.size() == 0)
        return;

    bool related;
    QString message;
    for (int i=0; i<children.size(); ++i) {
        Node* child = children[i];
        if (!child || child->isInternal() || child->doc().isEmpty())
            continue;
        if (child->relates()) {
            related = true;
            message = child->relates()->name();
        }
        else {
            related = false;
            message = "has documentation but no \\relates command";
        }
        switch (child->type()) {
        case Node::Namespace:
            break;
        case Node::Class:
            break;
        case Node::QmlType:
            break;
        case Node::QmlBasicType:
            break;
        case Node::Group:
            break;
        case Node::Module:
            break;
        case Node::QmlModule:
            break;
        case Node::Document:
            switch (child->docSubtype()) {
            case Node::Example:
                break;
            case Node::HeaderFile:
                break;
            case Node::File:
                break;
            case Node::Image:
                break;
            case Node::Page:
                break;
            case Node::ExternalPage:
                break;
            default:
                break;
            }
            break;
        case Node::Enum:
            if (!related)
                child->location().warning(tr("Global enum, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::Typedef:
            if (!related)
                child->location().warning(tr("Global typedef, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::Function:
            if (!related) {
                const FunctionNode* fn = static_cast<const FunctionNode*>(child);
                if (fn->isMacro())
                    child->location().warning(tr("Global macro, %1, %2").arg(child->name()).arg(message));
                else
                    child->location().warning(tr("Global function, %1(), %2").arg(child->name()).arg(message));
            }
            break;
        case Node::Property:
            break;
        case Node::Variable:
            if (!related)
                child->location().warning(tr("Global variable, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::QmlPropertyGroup:
            break;
        case Node::QmlProperty:
            if (!related)
                child->location().warning(tr("Global QML property, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::QmlSignal:
            if (!related)
                child->location().warning(tr("Global QML, signal, %1 %2").arg(child->name()).arg(message));
            break;
        case Node::QmlSignalHandler:
            if (!related)
                child->location().warning(tr("Global QML signal handler, %1, %2").arg(child->name()).arg(message));
            break;
        case Node::QmlMethod:
            if (!related)
                child->location().warning(tr("Global QML method, %1, %2").arg(child->name()).arg(message));
            break;
        default:
            break;
        }
    }
}

/*!
  Returns a reference to the XML stream writer currently in use.
  There is one XML stream writer open for each XML file being
  written, and they are kept on a stack. The one on top of the
  stack is the one being written to at the moment. In the HTML
  output generator, it is perhaps impossible for there to ever
  be more than one writer open.
 */
QXmlStreamWriter& HtmlGenerator::xmlWriter()
{
    return *xmlWriterStack.top();
}

/*!
  This function is only called for writing ditamaps.

  Calls beginSubPage() in the base class to open the file.
  Then creates a new XML stream writer using the IO device
  from opened file and pushes the XML writer onto a stackj.
  Creates the file named \a fileName in the output directory.
  Attaches a QTextStream to the created file, which is written
  to all over the place using out(). Finally, it sets some
  parameters in the XML writer and calls writeStartDocument().

  It also ensures that a GUID map is created for the output file.
 */
void HtmlGenerator::beginDitamapPage(const Aggregate* node, const QString& fileName)
{
    Generator::beginSubPage(node,fileName);
    QXmlStreamWriter* writer = new QXmlStreamWriter(out().device());
    xmlWriterStack.push(writer);
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(4);
    writer->writeStartDocument();
}

/*!
  This function is only called for writing ditamaps.

  Calls writeEndDocument() and then pops the XML stream writer
  off the stack and deletes it. Then it calls endSubPage() in
  the base class to close the device.
 */
void HtmlGenerator::endDitamapPage()
{
    xmlWriter().writeEndDocument();
    delete xmlWriterStack.pop();
    Generator::endSubPage();
}

/*!
  This function is only called for writing ditamaps.

  Creates the DITA map from the topicrefs in \a node,
  which is a DitaMapNode.
 */
void HtmlGenerator::writeDitaMap(const DitaMapNode* node)
{
    beginDitamapPage(node,node->name());

    QString doctype = "<!DOCTYPE map PUBLIC \"-//OASIS//DTD DITA Map//EN\" \"map.dtd\">";

    xmlWriter().writeDTD(doctype);
    xmlWriter().writeStartElement("map");
    xmlWriter().writeStartElement("topicmeta");
    xmlWriter().writeStartElement("shortdesc");
    xmlWriter().writeCharacters(node->title());
    xmlWriter().writeEndElement(); // </shortdesc>
    xmlWriter().writeEndElement(); // </topicmeta>
    DitaRefList map = node->map();
    writeDitaRefs(map);
    endDitamapPage();
}

/*!
  Write the \a ditarefs to the current output file.
 */
void HtmlGenerator::writeDitaRefs(const DitaRefList& ditarefs)
{
    foreach (DitaRef* t, ditarefs) {
        if (t->isMapRef())
            xmlWriter().writeStartElement("mapref");
        else
            xmlWriter().writeStartElement("topicref");
        xmlWriter().writeAttribute("navtitle",t->navtitle());
        if (t->href().isEmpty()) {
            const DocumentNode* dn = qdb_->findDocumentNodeByTitle(t->navtitle());
            if (dn)
                xmlWriter().writeAttribute("href",fileName(dn));
        }
        else
            xmlWriter().writeAttribute("href",t->href());
        if (t->subrefs() && !t->subrefs()->isEmpty())
            writeDitaRefs(*(t->subrefs()));
        xmlWriter().writeEndElement(); // </topicref> or </mapref>
    }
}

/*!
  Generates bold Note lines that explain how function \a fn
  is associated with each of its associated properties.
 */
void HtmlGenerator::generateAssociatedPropertyNotes(const FunctionNode* fn)
{
    if (fn->hasAssociatedProperties()) {
        out() << "<p><b>Note:</b> ";
        PropNodeList propertyNodes = fn->associatedProperties();
        std::sort(propertyNodes.begin(), propertyNodes.end(), Node::nodeNameLessThan);
        foreach (const PropertyNode* pn, propertyNodes) {
            QString msg;
            switch (pn->role(fn)) {
            case PropertyNode::Getter:
                msg = QStringLiteral("Getter function ");
                break;
            case PropertyNode::Setter:
                msg = QStringLiteral("Setter function ");
                break;
            case PropertyNode::Resetter:
                msg = QStringLiteral("Resetter function ");
                break;
            case PropertyNode::Notifier:
                msg = QStringLiteral("Notifier signal ");
                break;
            default:
                break;
            }
            QString link = linkForNode(pn, 0);
            out() << msg << "for property <a href=\"" << link << "\">" << pn->name() << "</a>. ";
        }
        out() << "</p>";
    }
}

QT_END_NAMESPACE
