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

#include <qglobal.h>
#include <qhashfunctions.h>
#include <stdlib.h>
#include "codemarker.h"
#include "codeparser.h"
#include "config.h"
#include "cppcodemarker.h"
#include "cppcodeparser.h"
#include "doc.h"
#include "htmlgenerator.h"
#include "loggingcategory.h"
#include "webxmlgenerator.h"
#include "location.h"
#include "puredocparser.h"
#include "tokenizer.h"
#include "tree.h"
#include "qdocdatabase.h"
#include "jscodemarker.h"
#include "qmlcodemarker.h"
#include "qmlcodeparser.h"
#include "clangcodeparser.h"
#include <qdatetime.h>
#include <qdebug.h>
#include "qtranslator.h"
#ifndef QT_BOOTSTRAPPED
#  include "qcoreapplication.h"
#endif
#include "qcommandlineoption.h"
#include "qcommandlineparser.h"
#include <qhashfunctions.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQdoc, "qt.qdoc")

bool creationTimeBefore(const QFileInfo &fi1, const QFileInfo &fi2)
{
    return fi1.lastModified() < fi2.lastModified();
}

static bool highlighting = false;
static bool showInternal = false;
static bool singleExec = false;
static bool writeQaPages = false;
static bool redirectDocumentationToDevNull = false;
static bool noLinkErrors = false;
static bool autolinkErrors = false;
static bool obsoleteLinks = false;
static QStringList defines;
static QStringList includesPaths;
static QStringList dependModules;
static QStringList indexDirs;
static QString currentDir;
static QString prevCurrentDir;
static QHash<QString,QString> defaults;
#ifndef QT_NO_TRANSLATION
typedef QPair<QString, QTranslator*> Translator;
static QList<Translator> translators;
#endif
static ClangCodeParser* clangParser_ = 0;

/*!
  Read some XML indexes containing definitions from other
  documentation sets. \a config contains a variable that
  lists directories where index files can be found. It also
  contains the \c depends variable, which lists the modules
  that the current module depends on. \a formats contains
  a list of output formats; each format may have a different
  output subdirectory where index files are located.
*/
static void loadIndexFiles(Config& config, const QSet<QString> &formats)
{
    QDocDatabase* qdb = QDocDatabase::qdocDB();
    QStringList indexFiles;
    QStringList configIndexes = config.getStringList(CONFIG_INDEXES);
    foreach (const QString &index, configIndexes) {
        QFileInfo fi(index);
        if (fi.exists() && fi.isFile())
            indexFiles << index;
        else
            Location::null.warning(QString("Index file not found: %1").arg(index));
    }

    dependModules += config.getStringList(CONFIG_DEPENDS);
    dependModules.removeDuplicates();
    QSet<QString> subDirs;

    for (const auto &format : formats) {
        if (config.getBool(format + Config::dot + "nosubdirs")) {
            QString singleOutputSubdir = config.getString(format
                                                          + Config::dot
                                                          + "outputsubdir");
            if (singleOutputSubdir.isEmpty())
                singleOutputSubdir = "html";
            subDirs << singleOutputSubdir;
        }
    }

    if (dependModules.size() > 0) {
        if (indexDirs.size() > 0) {
            for (int i = 0; i < indexDirs.size(); i++) {
                if (indexDirs[i].startsWith("..")) {
                    const QString prefix(QDir(currentDir).relativeFilePath(prevCurrentDir));
                    if (!prefix.isEmpty())
                        indexDirs[i].prepend(prefix + QLatin1Char('/'));
                }
            }
            /*
              Add all subdirectories of the indexdirs as dependModules,
              when an asterisk is used in the 'depends' list.
            */
            if (dependModules.contains("*")) {
                dependModules.removeOne("*");
                for (int i = 0; i < indexDirs.size(); i++) {
                    QDir scanDir = QDir(indexDirs[i]);
                    scanDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
                    QFileInfoList dirList = scanDir.entryInfoList();
                    for (int j = 0; j < dirList.size(); j++) {
                        if (dirList[j].fileName().toLower() != config.getString(CONFIG_PROJECT).toLower())
                            dependModules.append(dirList[j].fileName());
                    }
                }
            }
            for (int i = 0; i < dependModules.size(); i++) {
                QString indexToAdd;
                QList<QFileInfo> foundIndices;
                // Always look in module-specific subdir, even with *.nosubdirs config
                subDirs << dependModules[i];
                for (int j = 0; j < indexDirs.size(); j++) {
                    for (const auto &subDir : subDirs) {
                        QString fileToLookFor = indexDirs[j]
                                + QLatin1Char('/') + subDir
                                + QLatin1Char('/') + dependModules[i] + ".index";
                        if (QFile::exists(fileToLookFor)) {
                            QFileInfo tempFileInfo(fileToLookFor);
                            if (!foundIndices.contains(tempFileInfo))
                                foundIndices.append(tempFileInfo);
                        }
                    }
                }
                subDirs.remove(dependModules[i]);
                std::sort(foundIndices.begin(), foundIndices.end(), creationTimeBefore);
                if (foundIndices.size() > 1) {
                    /*
                        QDoc should always use the last entry in the multimap when there are
                        multiple index files for a module, since the last modified file has the
                        highest UNIX timestamp.
                    */
                    QStringList indexPaths;
                    for (int k = 0; k < foundIndices.size(); k++)
                        indexPaths << foundIndices[k].absoluteFilePath();
                    Location::null.warning(QString("Multiple index files found for dependency \"%1\":\n%2").arg(
                                               dependModules[i], indexPaths.join('\n')));
                    Location::null.warning(QString("Using %1 as index file for dependency \"%2\"").arg(
                                               foundIndices[foundIndices.size() - 1].absoluteFilePath(),
                                               dependModules[i]));
                    indexToAdd = foundIndices[foundIndices.size() - 1].absoluteFilePath();
                }
                else if (foundIndices.size() == 1) {
                    indexToAdd = foundIndices[0].absoluteFilePath();
                }
                if (!indexToAdd.isEmpty()) {
                    if (!indexFiles.contains(indexToAdd))
                        indexFiles << indexToAdd;
                }
                else {
                    Location::null.warning(QString("\"%1\" Cannot locate index file for dependency \"%2\"").arg(
                                               config.getString(CONFIG_PROJECT), dependModules[i]));
                }
            }
        }
        else {
            Location::null.warning(QLatin1String("Dependent modules specified, but no index directories were set. There will probably be errors for missing links."));
        }
    }
    qdb->readIndexes(indexFiles);
}

/*!
  Processes the qdoc config file \a fileName. This is the
  controller for all of qdoc.
 */
static void processQdocconfFile(const QString &fileName)
{
    /*
      The Config instance represents the configuration data for qdoc.
      All the other classes are initialized with the config. Below, we
      initialize the configuration with some default values.

      I don't think the call to translate() does anything here. For one
      thing, the translators haven't been installed at this point. And
      I doubt any translator would translate QDoc anyway. But I left it
      here because it does no harm.
     */
    Config config(QCoreApplication::translate("QDoc", "qdoc"));

    QHash<QString,QString>::iterator iter;
    for (iter = defaults.begin(); iter != defaults.end(); ++iter)
        config.setStringList(iter.key(), QStringList() << iter.value());

    config.setStringList(CONFIG_SYNTAXHIGHLIGHTING, QStringList(highlighting ? "true" : "false"));
    config.setStringList(CONFIG_SHOWINTERNAL, QStringList(showInternal ? "true" : "false"));
    config.setStringList(CONFIG_SINGLEEXEC, QStringList(singleExec ? "true" : "false"));
    config.setStringList(CONFIG_WRITEQAPAGES, QStringList(writeQaPages ? "true" : "false"));
    config.setStringList(CONFIG_REDIRECTDOCUMENTATIONTODEVNULL, QStringList(redirectDocumentationToDevNull ? "true" : "false"));
    config.setStringList(CONFIG_NOLINKERRORS, QStringList(noLinkErrors ? "true" : "false"));
    config.setStringList(CONFIG_AUTOLINKERRORS, QStringList(autolinkErrors ? "true" : "false"));
    config.setStringList(CONFIG_OBSOLETELINKS, QStringList(obsoleteLinks ? "true" : "false"));

    prevCurrentDir = QDir::currentPath();

    /*
      With the default configuration values in place, load
      the qdoc configuration file. Note that the configuration
      file may include other configuration files.

      The Location class keeps track of the current location
      in the file being processed, mainly for error reporting
      purposes.
     */
    Location::initialize(config);
    config.load(fileName);
    QString project = config.getString(CONFIG_PROJECT);
    QString moduleHeader = config.getString(CONFIG_MODULEHEADER);
    /*
      Add the defines to the configuration variables.
     */
    QStringList defs = defines + config.getStringList(CONFIG_DEFINES);
    config.setStringList(CONFIG_DEFINES,defs);
    QStringList incs = includesPaths + config.getStringList(CONFIG_INCLUDEPATHS);
    config.setStringList(CONFIG_INCLUDEPATHS, incs);
    Location::terminate();

    currentDir = QFileInfo(fileName).path();
    if (!currentDir.isEmpty())
        QDir::setCurrent(currentDir);

    QString phase = " in -";
    if (Generator::singleExec())
        phase += "single exec mode, ";
    else
        phase += "separate exec mode, ";
    if (Generator::preparing())
        phase += "prepare phase ";
    else if (Generator::generating())
        phase += "generate phase ";

    QString msg = "Running qdoc for " + config.getString(CONFIG_PROJECT) + phase;
    Location::logToStdErr(msg);

    /*
      Initialize all the classes and data structures with the
      qdoc configuration. This is safe to do for each qdocconf
      file processed, because all the data structures created
      are either cleared after they have been used, or they
      are cleared in the terminate() functions below.
     */
    Location::initialize(config);
    Tokenizer::initialize(config);
    CodeMarker::initialize(config);
    CodeParser::initialize(config);
    Generator::initialize(config);
    Doc::initialize(config);

#ifndef QT_NO_TRANSLATION
    /*
      Load the language translators, if the configuration specifies any,
      but only if they haven't already been loaded. This works in both
      -prepare/-generate mode and -singleexec mode.
     */
    QStringList fileNames = config.getStringList(CONFIG_TRANSLATORS);
    QStringList::ConstIterator fn = fileNames.constBegin();
    while (fn != fileNames.constEnd()) {
        bool found = false;
        if (!translators.isEmpty()) {
            for (int i=0; i<translators.size(); ++i) {
                if (translators.at(i).first == *fn) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            QTranslator *translator = new QTranslator(0);
            if (!translator->load(*fn)) {
                config.lastLocation().error(QCoreApplication::translate("QDoc", "Cannot load translator '%1'").arg(*fn));
            }
            else {
                QCoreApplication::instance()->installTranslator(translator);
                translators.append(Translator(*fn, translator));
            }
        }
        ++fn;
    }
#endif

    //QSet<QString> outputLanguages = config.getStringSet(CONFIG_OUTPUTLANGUAGES);

    /*
      Get the source language (Cpp) from the configuration
      and the location in the configuration file where the
      source language was set.
     */
    QString lang = config.getString(CONFIG_LANGUAGE);
    Location langLocation = config.lastLocation();

    /*
      Initialize the qdoc database, where all the parsed source files
      will be stored. The database includes a tree of nodes, which gets
      built as the source files are parsed. The documentation output is
      generated by traversing that tree.

      Note: qdocDB() allocates a new instance only if no instance exists.
      So it is safe to call qdocDB() any time.
     */
    QDocDatabase* qdb = QDocDatabase::qdocDB();
    qdb->setVersion(config.getString(CONFIG_VERSION));
    qdb->setShowInternal(config.getBool(CONFIG_SHOWINTERNAL));
    qdb->setSingleExec(config.getBool(CONFIG_SINGLEEXEC));
    /*
      By default, the only output format is HTML.
     */
    QSet<QString> outputFormats = config.getOutputFormats();
    Location outputFormatsLocation = config.lastLocation();

    qdb->clearSearchOrder();
    if (!Generator::singleExec()) {
        if (!Generator::preparing()) {
            qCDebug(lcQdoc, "  loading index files");
            loadIndexFiles(config, outputFormats);
            qCDebug(lcQdoc, "  done loading index files");
        }
        qdb->newPrimaryTree(project);
    }
    else if (Generator::preparing())
        qdb->newPrimaryTree(project);
    else
        qdb->setPrimaryTree(project);
    if (!moduleHeader.isNull())
        clangParser_->setModuleHeader(moduleHeader);
    else
        clangParser_->setModuleHeader(project);

    dependModules = config.getStringList(CONFIG_DEPENDS);
    dependModules.removeDuplicates();
    qdb->setSearchOrder(dependModules);

    // Store the title of the index (landing) page
    NamespaceNode* root = qdb->primaryTreeRoot();
    if (root) {
        QString title = config.getString(CONFIG_NAVIGATION
                                        + Config::dot
                                        + CONFIG_LANDINGPAGE);
        root->tree()->setIndexTitle(config.getString(CONFIG_NAVIGATION
                                        + Config::dot
                                        + CONFIG_LANDINGTITLE, title));
    }

    QSet<QString> excludedDirs = QSet<QString>::fromList(config.getCanonicalPathList(CONFIG_EXCLUDEDIRS));
    QSet<QString> excludedFiles = QSet<QString>::fromList(config.getCanonicalPathList(CONFIG_EXCLUDEFILES));

    qCDebug(lcQdoc, "Adding doc/image dirs found in exampledirs to imagedirs");
    QSet<QString> exampleImageDirs;
    QStringList exampleImageList = config.getExampleImageFiles(excludedDirs, excludedFiles);
    for (int i = 0; i < exampleImageList.size(); ++i) {
        if (exampleImageList[i].contains("doc/images")) {
            QString t = exampleImageList[i].left(exampleImageList[i].lastIndexOf("doc/images") + 10);
            if (!exampleImageDirs.contains(t)) {
                exampleImageDirs.insert(t);
            }
        }
    }
    Generator::augmentImageDirs(exampleImageDirs);

    if (!Generator::singleExec() || !Generator::generating()) {
        QStringList headerList;
        QStringList sourceList;

        qCDebug(lcQdoc, "Reading headerdirs");
        headerList = config.getAllFiles(CONFIG_HEADERS,CONFIG_HEADERDIRS,excludedDirs,excludedFiles);
        QMap<QString,QString> headers;
        QMultiMap<QString,QString> headerFileNames;
        for (int i=0; i<headerList.size(); ++i) {
            if (headerList[i].contains(QString("doc/snippets")))
                continue;
            if (headers.contains(headerList[i]))
                continue;
            headers.insert(headerList[i],headerList[i]);
            QString t = headerList[i].mid(headerList[i].lastIndexOf('/')+1);
            headerFileNames.insert(t,t);
        }

        qCDebug(lcQdoc, "Reading sourcedirs");
        sourceList = config.getAllFiles(CONFIG_SOURCES,CONFIG_SOURCEDIRS,excludedDirs,excludedFiles);
        QMap<QString,QString> sources;
        QMultiMap<QString,QString> sourceFileNames;
        for (int i=0; i<sourceList.size(); ++i) {
            if (sourceList[i].contains(QString("doc/snippets")))
                continue;
            if (sources.contains(sourceList[i]))
                continue;
            sources.insert(sourceList[i],sourceList[i]);
            QString t = sourceList[i].mid(sourceList[i].lastIndexOf('/')+1);
            sourceFileNames.insert(t,t);
        }
        /*
          Find all the qdoc files in the example dirs, and add
          them to the source files to be parsed.
        */
        qCDebug(lcQdoc, "Reading exampledirs");
        QStringList exampleQdocList = config.getExampleQdocFiles(excludedDirs, excludedFiles);
        for (int i=0; i<exampleQdocList.size(); ++i) {
            if (!sources.contains(exampleQdocList[i])) {
                sources.insert(exampleQdocList[i],exampleQdocList[i]);
                QString t = exampleQdocList[i].mid(exampleQdocList[i].lastIndexOf('/')+1);
                sourceFileNames.insert(t,t);
            }
        }
        /*
          Parse each header file in the set using the appropriate parser and add it
          to the big tree.
        */

        qCDebug(lcQdoc, "Parsing header files");
        int parsed = 0;
        QMap<QString,QString>::ConstIterator h = headers.constBegin();
        while (h != headers.constEnd()) {
            CodeParser *codeParser = CodeParser::parserForHeaderFile(h.key());
            if (codeParser) {
                ++parsed;
                qCDebug(lcQdoc, "Parsing %s", qPrintable(h.key()));
                codeParser->parseHeaderFile(config.location(), h.key());
            }
            ++h;
        }

        clangParser_->precompileHeaders();

        /*
          Parse each source text file in the set using the appropriate parser and
          add it to the big tree.
        */
        parsed = 0;
        qCDebug(lcQdoc, "Parsing source files");
        QMap<QString,QString>::ConstIterator s = sources.constBegin();
        while (s != sources.constEnd()) {
            CodeParser *codeParser = CodeParser::parserForSourceFile(s.key());
            if (codeParser) {
                ++parsed;
                qCDebug(lcQdoc, "Parsing %s", qPrintable(s.key()));
                codeParser->parseSourceFile(config.location(), s.key());
            }
            ++s;
        }
        qCDebug(lcQdoc, "Parsing done.");

        /*
          Now the primary tree has been built from all the header and
          source files. Resolve all the class names, function names,
          targets, URLs, links, and other stuff that needs resolving.
        */
        qCDebug(lcQdoc, "Resolving stuff prior to generating docs");
        qdb->resolveInheritance();
        qdb->resolveIssues();
    }
    else {
        qdb->resolveStuff();
    }

    /*
      The primary tree is built and all the stuff that needed
      resolving has been resolved. Now traverse the tree and
      generate the documentation output. More than one output
      format can be requested. The tree is traversed for each
      one.
     */
    qCDebug(lcQdoc, "Generating docs");
    QSet<QString>::ConstIterator of = outputFormats.constBegin();
    while (of != outputFormats.constEnd()) {
        Generator* generator = Generator::generatorForFormat(*of);
        if (generator == 0)
            outputFormatsLocation.fatal(QCoreApplication::translate("QDoc",
                                               "Unknown output format '%1'").arg(*of));
        generator->initializeFormat(config);
        generator->generateDocs();
        ++of;
    }
#if 0
    if (Generator::generating() && Generator::writeQaPages())
        qdb->printLinkCounts(project);
#endif
    qdb->clearLinkCounts();

    qCDebug(lcQdoc, "Terminating qdoc classes");
    if (Generator::debugging())
        Generator::stopDebugging(project);

    QDocDatabase::qdocDB()->setVersion(QString());
    Generator::terminate();
    CodeParser::terminate();
    CodeMarker::terminate();
    Doc::terminate();
    Tokenizer::terminate();
    Location::terminate();
    QDir::setCurrent(prevCurrentDir);

    qCDebug(lcQdoc, "qdoc classes terminated");
}

class QDocCommandLineParser : public QCommandLineParser
{
public:
    QDocCommandLineParser();
    void process(const QCoreApplication &app);

private:
    QCommandLineOption defineOption, dependsOption, highlightingOption;
    QCommandLineOption showInternalOption, redirectDocumentationToDevNullOption;
    QCommandLineOption noExamplesOption, indexDirOption, installDirOption;
    QCommandLineOption obsoleteLinksOption, outputDirOption, outputFormatOption;
    QCommandLineOption noLinkErrorsOption, autoLinkErrorsOption, debugOption;
    QCommandLineOption prepareOption, generateOption, logProgressOption;
    QCommandLineOption singleExecOption, writeQaPagesOption;
    QCommandLineOption includePathOption, includePathSystemOption, frameworkOption;
};

QDocCommandLineParser::QDocCommandLineParser()
    : QCommandLineParser(),
      defineOption(QStringList() << QStringLiteral("D")),
      dependsOption(QStringList() << QStringLiteral("depends")),
      highlightingOption(QStringList() << QStringLiteral("highlighting")),
      showInternalOption(QStringList() << QStringLiteral("showinternal")),
      redirectDocumentationToDevNullOption(QStringList() << QStringLiteral("redirect-documentation-to-dev-null")),
      noExamplesOption(QStringList() << QStringLiteral("no-examples")),
      indexDirOption(QStringList() << QStringLiteral("indexdir")),
      installDirOption(QStringList() << QStringLiteral("installdir")),
      obsoleteLinksOption(QStringList() << QStringLiteral("obsoletelinks")),
      outputDirOption(QStringList() << QStringLiteral("outputdir")),
      outputFormatOption(QStringList() << QStringLiteral("outputformat")),
      noLinkErrorsOption(QStringList() << QStringLiteral("no-link-errors")),
      autoLinkErrorsOption(QStringList() << QStringLiteral("autolink-errors")),
      debugOption(QStringList() << QStringLiteral("debug")),
      prepareOption(QStringList() << QStringLiteral("prepare")),
      generateOption(QStringList() << QStringLiteral("generate")),
      logProgressOption(QStringList() << QStringLiteral("log-progress")),
      singleExecOption(QStringList() << QStringLiteral("single-exec")),
      writeQaPagesOption(QStringList() << QStringLiteral("write-qa-pages")),
      includePathOption("I", "Add dir to the include path for header files.", "path"),
      includePathSystemOption("isystem", "Add dir to the system include path for header files.", "path"),
      frameworkOption("F", "Add macOS framework to the include path for header files.", "framework")
{
    setApplicationDescription(QCoreApplication::translate("qdoc", "Qt documentation generator"));
    addHelpOption();
    addVersionOption();

    setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    addPositionalArgument("file1.qdocconf ...", QCoreApplication::translate("qdoc", "Input files"));

    defineOption.setDescription(QCoreApplication::translate("qdoc", "Define the argument as a macro while parsing sources"));
    defineOption.setValueName(QStringLiteral("macro[=def]"));
    addOption(defineOption);

    dependsOption.setDescription(QCoreApplication::translate("qdoc", "Specify dependent modules"));
    dependsOption.setValueName(QStringLiteral("module"));
    addOption(dependsOption);

    highlightingOption.setDescription(QCoreApplication::translate("qdoc", "Turn on syntax highlighting (makes qdoc run slower)"));
    addOption(highlightingOption);

    showInternalOption.setDescription(QCoreApplication::translate("qdoc", "Include content marked internal"));
    addOption(showInternalOption);

    redirectDocumentationToDevNullOption.setDescription(QCoreApplication::translate("qdoc", "Save all documentation content to /dev/null. Useful if someone is interested in qdoc errors only."));
    addOption(redirectDocumentationToDevNullOption);

    noExamplesOption.setDescription(QCoreApplication::translate("qdoc", "Do not generate documentation for examples"));
    addOption(noExamplesOption);

    indexDirOption.setDescription(QCoreApplication::translate("qdoc", "Specify a directory where QDoc should search for index files to load"));
    indexDirOption.setValueName(QStringLiteral("dir"));
    addOption(indexDirOption);

    installDirOption.setDescription(QCoreApplication::translate("qdoc", "Specify the directory where the output will be after running \"make install\""));
    installDirOption.setValueName(QStringLiteral("dir"));
    addOption(installDirOption);

    obsoleteLinksOption.setDescription(QCoreApplication::translate("qdoc", "Report links from obsolete items to non-obsolete items"));
    addOption(obsoleteLinksOption);

    outputDirOption.setDescription(QCoreApplication::translate("qdoc", "Specify output directory, overrides setting in qdocconf file"));
    outputDirOption.setValueName(QStringLiteral("dir"));
    addOption(outputDirOption);

    outputFormatOption.setDescription(QCoreApplication::translate("qdoc", "Specify output format, overrides setting in qdocconf file"));
    outputFormatOption.setValueName(QStringLiteral("format"));
    addOption(outputFormatOption);

    noLinkErrorsOption.setDescription(QCoreApplication::translate("qdoc", "Do not print link errors (i.e. missing targets)"));
    addOption(noLinkErrorsOption);

    autoLinkErrorsOption.setDescription(QCoreApplication::translate("qdoc", "Show errors when automatic linking fails"));
    addOption(autoLinkErrorsOption);

    debugOption.setDescription(QCoreApplication::translate("qdoc", "Enable debug output"));
    addOption(debugOption);

    prepareOption.setDescription(QCoreApplication::translate("qdoc", "Run qdoc only to generate an index file, not the docs"));
    addOption(prepareOption);

    generateOption.setDescription(QCoreApplication::translate("qdoc", "Run qdoc to read the index files and generate the docs"));
    addOption(generateOption);

    logProgressOption.setDescription(QCoreApplication::translate("qdoc", "Log progress on stderr."));
    addOption(logProgressOption);

    singleExecOption.setDescription(QCoreApplication::translate("qdoc", "Run qdoc once over all the qdoc conf files."));
    addOption(singleExecOption);

    writeQaPagesOption.setDescription(QCoreApplication::translate("qdoc", "Write QA pages."));
    addOption(writeQaPagesOption);

    includePathOption.setFlags(QCommandLineOption::ShortOptionStyle);
    addOption(includePathOption);

    addOption(includePathSystemOption);

    frameworkOption.setFlags(QCommandLineOption::ShortOptionStyle);
    addOption(frameworkOption);
}

void QDocCommandLineParser::process(const QCoreApplication &app)
{
    QCommandLineParser::process(app);

    defines += values(defineOption);
    dependModules += values(dependsOption);
    highlighting = isSet(highlightingOption);
    showInternal = isSet(showInternalOption);
    singleExec = isSet(singleExecOption);
    writeQaPages = isSet(writeQaPagesOption);
    redirectDocumentationToDevNull = isSet(redirectDocumentationToDevNullOption);
    Config::generateExamples = !isSet(noExamplesOption);
    foreach (const QString &indexDir, values(indexDirOption)) {
        if (QFile::exists(indexDir))
            indexDirs += indexDir;
        else
            qDebug() << "Cannot find index directory" << indexDir;
    }
    if (isSet(installDirOption))
        Config::installDir = value(installDirOption);
    obsoleteLinks = isSet(obsoleteLinksOption);
    if (isSet(outputDirOption))
        Config::overrideOutputDir = value(outputDirOption);
    foreach (const QString &format, values(outputFormatOption))
        Config::overrideOutputFormats.insert(format);
    noLinkErrors = isSet(noLinkErrorsOption) || qEnvironmentVariableIsSet("QDOC_NOLINKERRORS");
    autolinkErrors = isSet(autoLinkErrorsOption);
    if (isSet(debugOption))
        Generator::startDebugging(QString("command line"));
    qCDebug(lcQdoc).noquote() << "Arguments :" << QCoreApplication::arguments();

    if (isSet(prepareOption))
        Generator::setQDocPass(Generator::Prepare);
    if (isSet(generateOption))
        Generator::setQDocPass(Generator::Generate);
    if (isSet(singleExecOption)) {
        Generator::setSingleExec();
        if (isSet(indexDirOption))
            qDebug() << "WARNING: -indexdir option ignored: Index files are not used in -single-exec mode.";
    }
    if (isSet(writeQaPagesOption))
        Generator::setWriteQaPages();
    if (isSet(logProgressOption))
        Location::startLoggingProgress();

    QDir currentDir = QDir::current();
    const auto paths = values(includePathOption);
    for (const auto &i : paths)
        includesPaths << "-I" << currentDir.absoluteFilePath(i);
#ifdef QDOC_PASS_ISYSTEM
    const auto paths2 = values(includePathSystemOption);
    for (const auto &i : paths2)
        includesPaths << "-isystem" << currentDir.absoluteFilePath(i);
#endif
    const auto paths3 = values(frameworkOption);
    for (const auto &i : paths3)
        includesPaths << "-F" << currentDir.absoluteFilePath(i);

    /*
      The default indent for code is 0.
      The default value for false is 0.
      The default supported file extensions are cpp, h, qdoc and qml.
      The default language is c++.
      The default output format is html.
      The default tab size is 8.
      And those are all the default values for configuration variables.
     */
    if (defaults.isEmpty()) {
        defaults.insert(CONFIG_CODEINDENT, QLatin1String("0"));
        defaults.insert(CONFIG_FALSEHOODS, QLatin1String("0"));
        defaults.insert(CONFIG_FILEEXTENSIONS, QLatin1String("*.cpp *.h *.qdoc *.qml"));
        defaults.insert(CONFIG_LANGUAGE, QLatin1String("Cpp"));
        defaults.insert(CONFIG_OUTPUTFORMATS, QLatin1String("HTML"));
        defaults.insert(CONFIG_TABSIZE, QLatin1String("8"));
    }
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    QT_USE_NAMESPACE

    // Initialize Qt:
#ifndef QT_BOOTSTRAPPED
    qSetGlobalQHashSeed(0); // set the hash seed to 0 if it wasn't set yet
#endif
    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QLatin1String(QT_VERSION_STR));

    // Instantiate various singletons (used via static methods):
    /*
      Create code parsers for the languages to be parsed,
      and create a tree for C++.
     */
    ClangCodeParser clangParser;
    clangParser_ = &clangParser;
    QmlCodeParser qmlParser;
    PureDocParser docParser;

    /*
      Create code markers for plain text, C++,
      javascript, and QML.
     */
    PlainCodeMarker plainMarker;
    CppCodeMarker cppMarker;
    JsCodeMarker jsMarker;
    QmlCodeMarker qmlMarker;

    HtmlGenerator htmlGenerator;
    WebXMLGenerator webXMLGenerator;

    // Set the globals declared at the top of this file:
    QDocCommandLineParser parser;
    parser.process(app);

    // Get the list of files to act on:
    QStringList qdocFiles = parser.positionalArguments();
    if (qdocFiles.isEmpty())
        parser.showHelp();

    if (singleExec)
        qdocFiles = Config::loadMaster(qdocFiles.at(0));

    // Main loop (adapted, when needed, to handle single exec mode):
    if (Generator::singleExec())
        Generator::setQDocPass(Generator::Prepare);
    foreach (const QString &qf, qdocFiles) {
        dependModules.clear();
        processQdocconfFile(qf);
    }
    if (Generator::singleExec()) {
        Generator::setQDocPass(Generator::Generate);
        QDocDatabase* qdb = QDocDatabase::qdocDB();
        qdb->processForest();
        foreach (const QString &qf, qdocFiles) {
            dependModules.clear();
            processQdocconfFile(qf);
        }
    }

    // Tidy everything away:
#ifndef QT_NO_TRANSLATION
    if (!translators.isEmpty()) {
        for (int i=0; i<translators.size(); ++i) {
            delete translators.at(i).second;
        }
    }
    translators.clear();
#endif
    QmlTypeNode::terminate();

#ifdef DEBUG_SHUTDOWN_CRASH
    qDebug() << "main(): Delete qdoc database";
#endif
    QDocDatabase::destroyQdocDB();
#ifdef DEBUG_SHUTDOWN_CRASH
    qDebug() << "main(): qdoc database deleted";
#endif

    return Location::exitCode();
}
