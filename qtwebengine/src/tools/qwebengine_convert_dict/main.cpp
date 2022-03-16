/******************************************************************************
** This is just slightly modified version of convert_dict.cc
** chromium/chrome/tools/convert_dict/convert_dict.cc
**
** Original work:
** Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
** Modified work:
** Copyright (C) 2016 The Qt Company Ltd.
**
** Use of this source code is governed by a BSD-style license that can be
** found in the LICENSE.Chromium file.
**
** This tool converts Hunspell .aff/.dic pairs to a combined binary dictionary
** format (.bdic). This format is more compact, and can be more efficiently
** read by the client application.
**
******************************************************************************/

#include <base/at_exit.h>
#include <base/containers/span.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/i18n/icu_util.h>
#include <build/build_config.h>
#include <chrome/tools/convert_dict/aff_reader.h>
#include <chrome/tools/convert_dict/dic_reader.h>
#include <third_party/hunspell/google/bdict_reader.h>
#include <third_party/hunspell/google/bdict_writer.h>
#include <base/path_service.h>

#include <QTextStream>
#include <QLibraryInfo>
#include <QDir>
#include <QCoreApplication>

// see also src/core/type_conversion.h
inline base::FilePath::StringType toFilePathString(const QString &str)
{
#if defined(Q_OS_WIN)
    return QDir::toNativeSeparators(str).toStdWString();
#else
    return str.toStdString();
#endif
}

inline base::FilePath toFilePath(const QString &str)
{
    return base::FilePath(toFilePathString(str));
}

inline QString toQt(const base::string16 &string)
{
#if defined(OS_WIN)
    return QString::fromStdWString(string.data());
#else
    return QString::fromUtf16(string.data());
#endif
}

inline QString toQt(const std::string &string)
{
    return QString::fromStdString(string);
}

template<class T>
QTextStream &operator<<(QTextStream &out, base::span<T> span)
{
    out << '[';
    QString prefix;
    for (const auto &element : span) {
        out << prefix;
        out << element;
        prefix = QStringLiteral(",");
    }
    out << ']';
    return out;
}

// Compares the given word list with the serialized trie to make sure they
// are the same.
inline bool VerifyWords(const convert_dict::DicReader::WordList& org_words,
                        const std::string& serialized, QTextStream& out)
{
    hunspell::BDictReader reader;
    if (!reader.Init(reinterpret_cast<const unsigned char*>(serialized.data()),
                     serialized.size())) {
        out << "BDict is invalid" << endl;
        return false;
    }
    hunspell::WordIterator iter = reader.GetAllWordIterator();

    int affix_ids[hunspell::BDict::MAX_AFFIXES_PER_WORD];

    static const int buf_size = 128;
    char buf[buf_size];
    for (size_t i = 0; i < org_words.size(); i++) {
        int affix_matches = iter.Advance(buf, buf_size, affix_ids);
        if (affix_matches == 0) {
            out << "Found the end before we expected" << endl;
            return false;
        }

        if (org_words[i].first != buf) {
            out << "Word does not match!\n"
                << "  Index:    " << i << "\n"
                << "  Expected: " << QString::fromStdString(org_words[i].first) << "\n"
                << "  Actual:   " << QString::fromUtf8(buf) << endl;
            return false;
        }

        base::span<const int> expectedAffixes(org_words[i].second);
        base::span<const int> actualAffixes(affix_ids, affix_matches);

        if (expectedAffixes != actualAffixes) {
            out << "Affixes do not match!\n"
                << "  Index:    " << i << "\n"
                << "  Word:     " << QString::fromUtf8(buf) << "\n"
                << "  Expected: " << expectedAffixes << "\n"
                << "  Actual:   " << actualAffixes << endl;
            return false;
        }
    }

    return true;
}

#if defined(OS_MACOSX) && defined(QT_MAC_FRAMEWORK_BUILD)
QString frameworkIcuDataPath()
{
    return QLibraryInfo::location(QLibraryInfo::LibrariesPath) +
            QStringLiteral("/QtWebEngineCore.framework/Resources/");
}
#endif

int main(int argc, char *argv[])
{
    // Required only for making QLibraryInfo::location() return a valid path, when the application
    // picks up a qt.conf file (which is the case for official Qt packages).
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    QTextStream out(stdout);

    if (argc != 3) {
        QTextStream out(stdout);
        out << "Usage: qwebengine_convert_dict <dic file> <bdic file>\n\nExample:\n"
               "qwebengine_convert_dict ./en-US.dic ./en-US.bdic\nwill read en-US.dic, "
               "en-US.dic_delta, and en-US.aff from the current directory and generate "
               "en-US.bdic\n" << endl;
        return 1;
    }

    bool icuDataDirFound = false;
    QString icuDataDir = QLibraryInfo::location(QLibraryInfo::DataPath)
            % QLatin1String("/resources");

    // Try to look up the path to the ICU data directory via an environment variable
    // (e.g. for the case when the tool is ran during build phase, and regular installed
    // ICU data file is not available).
    QString icuPossibleEnvDataDir = QString::fromLatin1(qgetenv("QT_WEBENGINE_ICU_DATA_DIR"));
    if (!icuPossibleEnvDataDir.isEmpty() && QFileInfo::exists(icuPossibleEnvDataDir)) {
        icuDataDir = icuPossibleEnvDataDir;
        icuDataDirFound = true;
    }
#if defined(OS_MACOSX) && defined(QT_MAC_FRAMEWORK_BUILD)
    // In a macOS Qt framework build, the resources are inside the QtWebEngineCore framework
    // Resources directory, rather than in the Qt install location.
    else if (QFileInfo::exists(frameworkIcuDataPath())) {
        icuDataDir = frameworkIcuDataPath();
        icuDataDirFound = true;
    }
#endif
    // Try to find the ICU data directory in the installed Qt location.
    else if (QFileInfo::exists(icuDataDir)) {
        icuDataDirFound = true;
    }

    if (icuDataDirFound) {
        base::PathService::Override(base::DIR_QT_LIBRARY_DATA, toFilePath(icuDataDir));
    } else {
        QTextStream out(stdout);
        out << "Couldn't find ICU data directory. Please check that the following path exists: "
            << icuDataDir
            << "\nAlternatively provide the directory path via the QT_WEBENGINE_ICU_DAT_DIR "
               "environment variable.\n" << endl;
        return 1;
    }


    base::AtExitManager exit_manager;
    base::i18n::InitializeICU();

    base::FilePath file_in_path = toFilePath(argv[1]);
    base::FilePath file_out_path = toFilePath(argv[2]);
    base::FilePath aff_path = file_in_path.ReplaceExtension(FILE_PATH_LITERAL(".aff"));

    out << "Reading " << toQt(aff_path.value()) << endl;
    convert_dict::AffReader aff_reader(aff_path);

    if (!aff_reader.Read()) {
        out << "Unable to read the aff file." << endl;
        return 1;
    }

    base::FilePath dic_path = file_in_path.ReplaceExtension(FILE_PATH_LITERAL(".dic"));
    out << "Reading " << toQt(dic_path.value()) << endl;

    // DicReader will also read the .dic_delta file.
    convert_dict::DicReader dic_reader(dic_path);
    if (!dic_reader.Read(&aff_reader)) {
        out << "Unable to read the dic file." << endl;
        return 1;
    }

    hunspell::BDictWriter writer;
    writer.SetComment(aff_reader.comments());
    writer.SetAffixRules(aff_reader.affix_rules());
    writer.SetAffixGroups(aff_reader.GetAffixGroups());
    writer.SetReplacements(aff_reader.replacements());
    writer.SetOtherCommands(aff_reader.other_commands());
    writer.SetWords(dic_reader.words());

    out << "Serializing..." << endl;

    std::string serialized = writer.GetBDict();

    out << "Verifying..." << endl;

    if (!VerifyWords(dic_reader.words(), serialized, out)) {
        out << "ERROR converting, the dictionary does not check out OK." << endl;
        return 1;
    }

    out << "Writing " << toQt(file_out_path.value()) << endl;
    FILE *out_file = base::OpenFile(file_out_path, "wb");
    if (!out_file) {
        out << "ERROR writing file" << endl;
        return 1;
    }
    size_t written = fwrite(&serialized[0], 1, serialized.size(), out_file);
    Q_ASSERT(written == serialized.size());
    base::CloseFile(out_file);
    out << "Success. Dictionary converted." << endl;
    return 0;
}

