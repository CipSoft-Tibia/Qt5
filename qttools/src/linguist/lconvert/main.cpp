/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

#include "translator.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTranslator>
#include <QtCore/QLibraryInfo>

#include <iostream>

QT_USE_NAMESPACE

class LC {
    Q_DECLARE_TR_FUNCTIONS(LConvert)
};

static int usage(const QStringList &args)
{
    Q_UNUSED(args);

    QString loaders;
    QString line(QLatin1String("    %1 - %2\n"));
    foreach (Translator::FileFormat format, Translator::registeredFileFormats())
        loaders += line.arg(format.extension, -5).arg(format.description());

    std::cout << qPrintable(LC::tr("\nUsage:\n"
        "    lconvert [options] <infile> [<infile>...]\n\n"
        "lconvert is part of Qt's Linguist tool chain. It can be used as a\n"
        "stand-alone tool to convert and filter translation data files.\n"
        "The following file formats are supported:\n\n%1\n"
        "If multiple input files are specified, they are merged with\n"
        "translations from later files taking precedence.\n\n"
        "Options:\n"
        "    -h\n"
        "    -help  Display this information and exit.\n\n"
        "    -i <infile>\n"
        "    -input-file <infile>\n"
        "           Specify input file. Use if <infile> might start with a dash.\n"
        "           This option can be used several times to merge inputs.\n"
        "           May be '-' (standard input) for use in a pipe.\n\n"
        "    -o <outfile>\n"
        "    -output-file <outfile>\n"
        "           Specify output file. Default is '-' (standard output).\n\n"
        "    -if <informat>\n"
        "    -input-format <format>\n"
        "           Specify input format for subsequent <infile>s.\n"
        "           The format is auto-detected from the file name and defaults to 'ts'.\n\n"
        "    -of <outformat>\n"
        "    -output-format <outformat>\n"
        "           Specify output format. See -if.\n\n"
        "    -drop-tags <regexp>\n"
        "           Drop named extra tags when writing TS or XLIFF files.\n"
        "           May be specified repeatedly.\n\n"
        "    -drop-translations\n"
        "           Drop existing translations and reset the status to 'unfinished'.\n"
        "           Note: this implies --no-obsolete.\n\n"
        "    -source-language <language>[_<region>]\n"
        "           Specify/override the language of the source strings. Defaults to\n"
        "           POSIX if not specified and the file does not name it yet.\n\n"
        "    -target-language <language>[_<region>]\n"
        "           Specify/override the language of the translation.\n"
        "           The target language is guessed from the file name if this option\n"
        "           is not specified and the file contents name no language yet.\n\n"
        "    -no-obsolete\n"
        "           Drop obsolete messages.\n\n"
        "    -no-finished\n"
        "           Drop finished messages.\n\n"
        "    -no-untranslated\n"
        "           Drop untranslated messages.\n\n"
        "    -sort-contexts\n"
        "           Sort contexts in output TS file alphabetically.\n\n"
        "    -locations {absolute|relative|none}\n"
        "           Override how source code references are saved in TS files.\n"
        "           Default is absolute.\n\n"
        "    -no-ui-lines\n"
        "           Drop line numbers from references to UI files.\n\n"
        "    -verbose\n"
        "           be a bit more verbose\n\n"
        "Long options can be specified with only one leading dash, too.\n\n"
        "Return value:\n"
        "    0 on success\n"
        "    1 on command line parse failures\n"
        "    2 on read failures\n"
        "    3 on write failures\n").arg(loaders));
    return 1;
}

struct File
{
    QString name;
    QString format;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
#ifndef QT_BOOTSTRAPPED
#ifndef Q_OS_WIN32
    QTranslator translator;
    QTranslator qtTranslator;
    QString sysLocale = QLocale::system().name();
    QString resourceDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    if (translator.load(QLatin1String("linguist_") + sysLocale, resourceDir)
        && qtTranslator.load(QLatin1String("qt_") + sysLocale, resourceDir)) {
        app.installTranslator(&translator);
        app.installTranslator(&qtTranslator);
    }
#endif // Q_OS_WIN32
#endif

    QStringList args = app.arguments();
    QList<File> inFiles;
    QString inFormat(QLatin1String("auto"));
    QString outFileName;
    QString outFormat(QLatin1String("auto"));
    QString targetLanguage;
    QString sourceLanguage;
    bool dropTranslations = false;
    bool noObsolete = false;
    bool noFinished = false;
    bool noUntranslated = false;
    bool verbose = false;
    bool noUiLines = false;
    Translator::LocationsType locations = Translator::DefaultLocations;

    ConversionData cd;
    Translator tr;

    for (int i = 1; i < args.size(); ++i) {
        if (args[i].startsWith(QLatin1String("--")))
            args[i].remove(0, 1);
        if (args[i] == QLatin1String("-o")
         || args[i] == QLatin1String("-output-file")) {
            if (++i >= args.size())
                return usage(args);
            outFileName = args[i];
        } else if (args[i] == QLatin1String("-of")
                || args[i] == QLatin1String("-output-format")) {
            if (++i >= args.size())
                return usage(args);
            outFormat = args[i];
        } else if (args[i] == QLatin1String("-i")
                || args[i] == QLatin1String("-input-file")) {
            if (++i >= args.size())
                return usage(args);
            File file;
            file.name = args[i];
            file.format = inFormat;
            inFiles.append(file);
        } else if (args[i] == QLatin1String("-if")
                || args[i] == QLatin1String("-input-format")) {
            if (++i >= args.size())
                return usage(args);
            inFormat = args[i];
        } else if (args[i] == QLatin1String("-drop-tag") || args[i] == QLatin1String("-drop-tags")) {
            if (++i >= args.size())
                return usage(args);
            cd.m_dropTags.append(args[i]);
        } else if (args[i] == QLatin1String("-drop-translations")) {
            dropTranslations = true;
        } else if (args[i] == QLatin1String("-target-language")) {
            if (++i >= args.size())
                return usage(args);
            targetLanguage = args[i];
        } else if (args[i] == QLatin1String("-source-language")) {
            if (++i >= args.size())
                return usage(args);
            sourceLanguage = args[i];
        } else if (args[i].startsWith(QLatin1String("-h"))) {
            usage(args);
            return 0;
        } else if (args[i] == QLatin1String("-no-obsolete")) {
            noObsolete = true;
        } else if (args[i] == QLatin1String("-no-finished")) {
            noFinished = true;
        } else if (args[i] == QLatin1String("-no-untranslated")) {
            noUntranslated = true;
        } else if (args[i] == QLatin1String("-sort-contexts")) {
            cd.m_sortContexts = true;
        } else if (args[i] == QLatin1String("-locations")) {
            if (++i >= args.size())
                return usage(args);
            if (args[i] == QLatin1String("none"))
                locations = Translator::NoLocations;
            else if (args[i] == QLatin1String("relative"))
                locations = Translator::RelativeLocations;
            else if (args[i] == QLatin1String("absolute"))
                locations = Translator::AbsoluteLocations;
            else
                return usage(args);
        } else if (args[i] == QLatin1String("-no-ui-lines")) {
            noUiLines = true;
        } else if (args[i] == QLatin1String("-verbose")) {
            verbose = true;
        } else if (args[i].startsWith(QLatin1Char('-'))) {
            return usage(args);
        } else {
            File file;
            file.name = args[i];
            file.format = inFormat;
            inFiles.append(file);
        }
    }

    if (inFiles.isEmpty())
        return usage(args);

    tr.setLanguageCode(Translator::guessLanguageCodeFromFileName(inFiles[0].name));

    if (!tr.load(inFiles[0].name, cd, inFiles[0].format)) {
        std::cerr << qPrintable(cd.error());
        return 2;
    }
    tr.reportDuplicates(tr.resolveDuplicates(), inFiles[0].name, verbose);

    for (int i = 1; i < inFiles.size(); ++i) {
        Translator tr2;
        if (!tr2.load(inFiles[i].name, cd, inFiles[i].format)) {
            std::cerr << qPrintable(cd.error());
            return 2;
        }
        tr2.reportDuplicates(tr2.resolveDuplicates(), inFiles[i].name, verbose);
        for (int j = 0; j < tr2.messageCount(); ++j)
            tr.replaceSorted(tr2.message(j));
    }

    if (!targetLanguage.isEmpty())
        tr.setLanguageCode(targetLanguage);
    if (!sourceLanguage.isEmpty())
        tr.setSourceLanguageCode(sourceLanguage);
    if (noObsolete)
        tr.stripObsoleteMessages();
    if (noFinished)
        tr.stripFinishedMessages();
    if (noUntranslated)
        tr.stripUntranslatedMessages();
    if (dropTranslations)
        tr.dropTranslations();
    if (noUiLines)
        tr.dropUiLines();
    if (locations != Translator::DefaultLocations)
        tr.setLocationsType(locations);

    tr.normalizeTranslations(cd);
    if (!cd.errors().isEmpty()) {
        std::cerr << qPrintable(cd.error());
        cd.clearErrors();
    }
    if (!tr.save(outFileName, cd, outFormat)) {
        std::cerr << qPrintable(cd.error());
        return 3;
    }
    return 0;
}
