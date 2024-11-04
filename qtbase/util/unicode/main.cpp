// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qbytearray.h>
#include <qchar.h>
#include <qdebug.h>
#include <qfile.h>
#include <qhash.h>
#include <qlist.h>
#include <qstring.h>
#include <qbitarray.h>
#include <private/qstringiterator_p.h>
#if 0
#include <private/qunicodetables_p.h>
#endif

#define DATA_VERSION_S "15.0"
#define DATA_VERSION_STR "QChar::Unicode_15_0"


static QHash<QByteArray, QChar::UnicodeVersion> age_map;

static void initAgeMap()
{
    struct AgeMap {
        const QChar::UnicodeVersion version;
        const char *age;
    } ageMap[] = {
        { QChar::Unicode_1_1,   "1.1" },
        { QChar::Unicode_2_0,   "2.0" },
        { QChar::Unicode_2_1_2, "2.1" },
        { QChar::Unicode_3_0,   "3.0" },
        { QChar::Unicode_3_1,   "3.1" },
        { QChar::Unicode_3_2,   "3.2" },
        { QChar::Unicode_4_0,   "4.0" },
        { QChar::Unicode_4_1,   "4.1" },
        { QChar::Unicode_5_0,   "5.0" },
        { QChar::Unicode_5_1,   "5.1" },
        { QChar::Unicode_5_2,   "5.2" },
        { QChar::Unicode_6_0,   "6.0" },
        { QChar::Unicode_6_1,   "6.1" },
        { QChar::Unicode_6_2,   "6.2" },
        { QChar::Unicode_6_3,   "6.3" },
        { QChar::Unicode_7_0,   "7.0" },
        { QChar::Unicode_8_0,   "8.0" },
        { QChar::Unicode_9_0,   "9.0" },
        { QChar::Unicode_10_0,   "10.0" },
        { QChar::Unicode_11_0,   "11.0" },
        { QChar::Unicode_12_0,   "12.0" },
        { QChar::Unicode_12_1,   "12.1" }, // UCD Revision 24
        { QChar::Unicode_13_0,   "13.0" }, // UCD Revision 26
        { QChar::Unicode_14_0,   "14.0" }, // UCD Revision 28
        { QChar::Unicode_15_0,   "15.0" }, // UCD Revision 30
        { QChar::Unicode_Unassigned, 0 }
    };
    AgeMap *d = ageMap;
    while (d->age) {
        age_map.insert(d->age, d->version);
        ++d;
    }
}

static const char *east_asian_width_string =
R"(enum class EastAsianWidth : unsigned int {
    A,
    F,
    H,
    N,
    Na,
    W,
};

)";

enum class EastAsianWidth : unsigned int {
    A,
    F,
    H,
    N,
    Na,
    W,
};

static QHash<QByteArray, EastAsianWidth> eastAsianWidthMap;

static void initEastAsianWidthMap()
{
    constexpr struct W {
        EastAsianWidth width;
        const char *name;
    } widths[] = {
        { EastAsianWidth::A,  "A"  },
        { EastAsianWidth::F,  "F"  },
        { EastAsianWidth::H,  "H"  },
        { EastAsianWidth::N,  "N"  },
        { EastAsianWidth::Na, "Na" },
        { EastAsianWidth::W,  "W"  },
    };

    for (auto &w : widths)
        eastAsianWidthMap.insert(w.name, w.width);
}

static QHash<QByteArray, QChar::Category> categoryMap;

static void initCategoryMap()
{
    struct Cat {
        QChar::Category cat;
        const char *name;
    } categories[] = {
        { QChar::Mark_NonSpacing,          "Mn" },
        { QChar::Mark_SpacingCombining,    "Mc" },
        { QChar::Mark_Enclosing,           "Me" },

        { QChar::Number_DecimalDigit,      "Nd" },
        { QChar::Number_Letter,            "Nl" },
        { QChar::Number_Other,             "No" },

        { QChar::Separator_Space,          "Zs" },
        { QChar::Separator_Line,           "Zl" },
        { QChar::Separator_Paragraph,      "Zp" },

        { QChar::Other_Control,            "Cc" },
        { QChar::Other_Format,             "Cf" },
        { QChar::Other_Surrogate,          "Cs" },
        { QChar::Other_PrivateUse,         "Co" },
        { QChar::Other_NotAssigned,        "Cn" },

        { QChar::Letter_Uppercase,         "Lu" },
        { QChar::Letter_Lowercase,         "Ll" },
        { QChar::Letter_Titlecase,         "Lt" },
        { QChar::Letter_Modifier,          "Lm" },
        { QChar::Letter_Other,             "Lo" },

        { QChar::Punctuation_Connector,    "Pc" },
        { QChar::Punctuation_Dash,         "Pd" },
        { QChar::Punctuation_Open,         "Ps" },
        { QChar::Punctuation_Close,        "Pe" },
        { QChar::Punctuation_InitialQuote, "Pi" },
        { QChar::Punctuation_FinalQuote,   "Pf" },
        { QChar::Punctuation_Other,        "Po" },

        { QChar::Symbol_Math,              "Sm" },
        { QChar::Symbol_Currency,          "Sc" },
        { QChar::Symbol_Modifier,          "Sk" },
        { QChar::Symbol_Other,             "So" },
        { QChar::Other_NotAssigned, 0 }
    };
    Cat *c = categories;
    while (c->name) {
        categoryMap.insert(c->name, c->cat);
        ++c;
    }
}


static QHash<QByteArray, QChar::Decomposition> decompositionMap;

static void initDecompositionMap()
{
    struct Dec {
        QChar::Decomposition dec;
        const char *name;
    } decompositions[] = {
        { QChar::Canonical, "<canonical>" },
        { QChar::Font, "<font>" },
        { QChar::NoBreak, "<noBreak>" },
        { QChar::Initial, "<initial>" },
        { QChar::Medial, "<medial>" },
        { QChar::Final, "<final>" },
        { QChar::Isolated, "<isolated>" },
        { QChar::Circle, "<circle>" },
        { QChar::Super, "<super>" },
        { QChar::Sub, "<sub>" },
        { QChar::Vertical, "<vertical>" },
        { QChar::Wide, "<wide>" },
        { QChar::Narrow, "<narrow>" },
        { QChar::Small, "<small>" },
        { QChar::Square, "<square>" },
        { QChar::Compat, "<compat>" },
        { QChar::Fraction, "<fraction>" },
        { QChar::NoDecomposition, 0 }
    };
    Dec *d = decompositions;
    while (d->name) {
        decompositionMap.insert(d->name, d->dec);
        ++d;
    }
}


enum Direction {
    DirL = QChar::DirL,
    DirR = QChar::DirR,
    DirEN = QChar::DirEN,
    DirES = QChar::DirES,
    DirET = QChar::DirET,
    DirAN = QChar::DirAN,
    DirCS = QChar::DirCS,
    DirB = QChar::DirB,
    DirS = QChar::DirS,
    DirWS = QChar::DirWS,
    DirON = QChar::DirON,
    DirLRE = QChar::DirLRE,
    DirLRO = QChar::DirLRO,
    DirAL = QChar::DirAL,
    DirRLE = QChar::DirRLE,
    DirRLO = QChar::DirRLO,
    DirPDF = QChar::DirPDF,
    DirNSM = QChar::DirNSM,
    DirBN = QChar::DirBN,
    DirLRI = QChar::DirLRI,
    DirRLI = QChar::DirRLI,
    DirFSI = QChar::DirFSI,
    DirPDI = QChar::DirPDI,

    Dir_Unassigned
};

static QHash<QByteArray, Direction> directionMap;

static void initDirectionMap()
{
    struct Dir {
        Direction dir;
        const char *name;
    } directions[] = {
        { DirL, "L" },
        { DirR, "R" },
        { DirEN, "EN" },
        { DirES, "ES" },
        { DirET, "ET" },
        { DirAN, "AN" },
        { DirCS, "CS" },
        { DirB, "B" },
        { DirS, "S" },
        { DirWS, "WS" },
        { DirON, "ON" },
        { DirLRE, "LRE" },
        { DirLRO, "LRO" },
        { DirAL, "AL" },
        { DirRLE, "RLE" },
        { DirRLO, "RLO" },
        { DirPDF, "PDF" },
        { DirNSM, "NSM" },
        { DirBN, "BN" },
        { DirLRI, "LRI" },
        { DirRLI, "RLI" },
        { DirFSI, "FSI" },
        { DirPDI, "PDI" },
        { Dir_Unassigned, 0 }
    };
    Dir *d = directions;
    while (d->name) {
        directionMap.insert(d->name, d->dir);
        ++d;
    }
}


enum JoiningType {
    Joining_None,
    Joining_Causing,
    Joining_Dual,
    Joining_Right,
    Joining_Left,
    Joining_Transparent,

    Joining_Unassigned
};

static QHash<QByteArray, JoiningType> joining_map;

static void initJoiningMap()
{
    struct JoiningList {
        JoiningType joining;
        const char *name;
    } joinings[] = {
        { Joining_None,        "U" },
        { Joining_Causing,     "C" },
        { Joining_Dual,        "D" },
        { Joining_Right,       "R" },
        { Joining_Left,        "L" },
        { Joining_Transparent, "T" },
        { Joining_Unassigned, 0 }
    };
    JoiningList *d = joinings;
    while (d->name) {
        joining_map.insert(d->name, d->joining);
        ++d;
    }
}


static const char *grapheme_break_class_string =
    "enum GraphemeBreakClass {\n"
    "    GraphemeBreak_Any,\n"
    "    GraphemeBreak_CR,\n"
    "    GraphemeBreak_LF,\n"
    "    GraphemeBreak_Control,\n"
    "    GraphemeBreak_Extend,\n"
    "    GraphemeBreak_ZWJ,\n"
    "    GraphemeBreak_RegionalIndicator,\n"
    "    GraphemeBreak_Prepend,\n"
    "    GraphemeBreak_SpacingMark,\n"
    "    GraphemeBreak_L,\n"
    "    GraphemeBreak_V,\n"
    "    GraphemeBreak_T,\n"
    "    GraphemeBreak_LV,\n"
    "    GraphemeBreak_LVT,\n"
    "    GraphemeBreak_Extended_Pictographic,\n"
    "\n"
    "    NumGraphemeBreakClasses\n"
    "};\n\n";

enum GraphemeBreakClass {
    GraphemeBreak_Any,
    GraphemeBreak_CR,
    GraphemeBreak_LF,
    GraphemeBreak_Control,
    GraphemeBreak_Extend,
    GraphemeBreak_ZWJ,
    GraphemeBreak_RegionalIndicator,
    GraphemeBreak_Prepend,
    GraphemeBreak_SpacingMark,
    GraphemeBreak_L,
    GraphemeBreak_V,
    GraphemeBreak_T,
    GraphemeBreak_LV,
    GraphemeBreak_LVT,
    GraphemeBreak_Extended_Pictographic,

    GraphemeBreak_Unassigned
};

static QHash<QByteArray, GraphemeBreakClass> grapheme_break_map;

static void initGraphemeBreak()
{
    struct GraphemeBreakList {
        GraphemeBreakClass brk;
        const char *name;
    } breaks[] = {
        { GraphemeBreak_Any, "Any" },
        { GraphemeBreak_CR, "CR" },
        { GraphemeBreak_LF, "LF" },
        { GraphemeBreak_Control, "Control" },
        { GraphemeBreak_Extend, "Extend" },
        { GraphemeBreak_ZWJ, "ZWJ" },
        { GraphemeBreak_RegionalIndicator, "Regional_Indicator" },
        { GraphemeBreak_Prepend, "Prepend" },
        { GraphemeBreak_SpacingMark, "SpacingMark" },
        { GraphemeBreak_L, "L" },
        { GraphemeBreak_V, "V" },
        { GraphemeBreak_T, "T" },
        { GraphemeBreak_LV, "LV" },
        { GraphemeBreak_LVT, "LVT" },
        { GraphemeBreak_Extended_Pictographic, "Extended_Pictographic" },
        { GraphemeBreak_Unassigned, nullptr }
    };
    GraphemeBreakList *d = breaks;
    while (d->name) {
        grapheme_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *word_break_class_string =
    "enum WordBreakClass {\n"
    "    WordBreak_Any,\n"
    "    WordBreak_CR,\n"
    "    WordBreak_LF,\n"
    "    WordBreak_Newline,\n"
    "    WordBreak_Extend,\n"
    "    WordBreak_ZWJ,\n"
    "    WordBreak_Format,\n"
    "    WordBreak_RegionalIndicator,\n"
    "    WordBreak_Katakana,\n"
    "    WordBreak_HebrewLetter,\n"
    "    WordBreak_ALetter,\n"
    "    WordBreak_SingleQuote,\n"
    "    WordBreak_DoubleQuote,\n"
    "    WordBreak_MidNumLet,\n"
    "    WordBreak_MidLetter,\n"
    "    WordBreak_MidNum,\n"
    "    WordBreak_Numeric,\n"
    "    WordBreak_ExtendNumLet,\n"
    "    WordBreak_WSegSpace,\n"
    "\n"
    "    NumWordBreakClasses\n"
    "};\n\n";

enum WordBreakClass {
    WordBreak_Any,
    WordBreak_CR,
    WordBreak_LF,
    WordBreak_Newline,
    WordBreak_Extend,
    WordBreak_ZWJ,
    WordBreak_Format,
    WordBreak_RegionalIndicator,
    WordBreak_Katakana,
    WordBreak_HebrewLetter,
    WordBreak_ALetter,
    WordBreak_SingleQuote,
    WordBreak_DoubleQuote,
    WordBreak_MidNumLet,
    WordBreak_MidLetter,
    WordBreak_MidNum,
    WordBreak_Numeric,
    WordBreak_ExtendNumLet,
    WordBreak_WSegSpace,

    WordBreak_Unassigned
};

static QHash<QByteArray, WordBreakClass> word_break_map;

static void initWordBreak()
{
    struct WordBreakList {
        WordBreakClass brk;
        const char *name;
    } breaks[] = {
        { WordBreak_Any, "Any" },
        { WordBreak_CR, "CR" },
        { WordBreak_LF, "LF" },
        { WordBreak_Newline, "Newline" },
        { WordBreak_Extend, "Extend" },
        { WordBreak_ZWJ, "ZWJ" },
        { WordBreak_Format, "Format" },
        { WordBreak_RegionalIndicator, "Regional_Indicator" },
        { WordBreak_Katakana, "Katakana" },
        { WordBreak_HebrewLetter, "Hebrew_Letter" },
        { WordBreak_ALetter, "ALetter" },
        { WordBreak_SingleQuote, "Single_Quote" },
        { WordBreak_DoubleQuote, "Double_Quote" },
        { WordBreak_MidNumLet, "MidNumLet" },
        { WordBreak_MidLetter, "MidLetter" },
        { WordBreak_MidNum, "MidNum" },
        { WordBreak_Numeric, "Numeric" },
        { WordBreak_ExtendNumLet, "ExtendNumLet" },
        { WordBreak_WSegSpace, "WSegSpace" },
        { WordBreak_Unassigned, 0 }
    };
    WordBreakList *d = breaks;
    while (d->name) {
        word_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *sentence_break_class_string =
    "enum SentenceBreakClass {\n"
    "    SentenceBreak_Any,\n"
    "    SentenceBreak_CR,\n"
    "    SentenceBreak_LF,\n"
    "    SentenceBreak_Sep,\n"
    "    SentenceBreak_Extend,\n"
    "    SentenceBreak_Sp,\n"
    "    SentenceBreak_Lower,\n"
    "    SentenceBreak_Upper,\n"
    "    SentenceBreak_OLetter,\n"
    "    SentenceBreak_Numeric,\n"
    "    SentenceBreak_ATerm,\n"
    "    SentenceBreak_SContinue,\n"
    "    SentenceBreak_STerm,\n"
    "    SentenceBreak_Close,\n"
    "\n"
    "    NumSentenceBreakClasses\n"
    "};\n\n";

enum SentenceBreakClass {
    SentenceBreak_Any,
    SentenceBreak_CR,
    SentenceBreak_LF,
    SentenceBreak_Sep,
    SentenceBreak_Extend,
    SentenceBreak_Sp,
    SentenceBreak_Lower,
    SentenceBreak_Upper,
    SentenceBreak_OLetter,
    SentenceBreak_Numeric,
    SentenceBreak_ATerm,
    SentenceBreak_SContinue,
    SentenceBreak_STerm,
    SentenceBreak_Close,

    SentenceBreak_Unassigned
};

static QHash<QByteArray, SentenceBreakClass> sentence_break_map;

static void initSentenceBreak()
{
    struct SentenceBreakList {
        SentenceBreakClass brk;
        const char *name;
    } breaks[] = {
        { SentenceBreak_Any, "Any" },
        { SentenceBreak_CR, "CR" },
        { SentenceBreak_LF, "LF" },
        { SentenceBreak_Sep, "Sep" },
        { SentenceBreak_Extend, "Extend" },
        { SentenceBreak_Extend, "Format" },
        { SentenceBreak_Sp, "Sp" },
        { SentenceBreak_Lower, "Lower" },
        { SentenceBreak_Upper, "Upper" },
        { SentenceBreak_OLetter, "OLetter" },
        { SentenceBreak_Numeric, "Numeric" },
        { SentenceBreak_ATerm, "ATerm" },
        { SentenceBreak_SContinue, "SContinue" },
        { SentenceBreak_STerm, "STerm" },
        { SentenceBreak_Close, "Close" },
        { SentenceBreak_Unassigned, 0 }
    };
    SentenceBreakList *d = breaks;
    while (d->name) {
        sentence_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *line_break_class_string =
    "// see http://www.unicode.org/reports/tr14/tr14-30.html\n"
    "// we don't use the XX and AI classes and map them to AL instead.\n"
    "enum LineBreakClass {\n"
    "    LineBreak_OP, LineBreak_CL, LineBreak_CP, LineBreak_QU, LineBreak_GL,\n"
    "    LineBreak_NS, LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR,\n"
    "    LineBreak_PO, LineBreak_NU, LineBreak_AL, LineBreak_HL, LineBreak_ID,\n"
    "    LineBreak_IN, LineBreak_HY, LineBreak_BA, LineBreak_BB, LineBreak_B2,\n"
    "    LineBreak_ZW, LineBreak_CM, LineBreak_WJ, LineBreak_H2, LineBreak_H3,\n"
    "    LineBreak_JL, LineBreak_JV, LineBreak_JT, LineBreak_RI, LineBreak_CB,\n"
    "    LineBreak_EB, LineBreak_EM, LineBreak_ZWJ,\n"
    "    LineBreak_SA, LineBreak_SG, LineBreak_SP,\n"
    "    LineBreak_CR, LineBreak_LF, LineBreak_BK,\n"
    "\n"
    "    NumLineBreakClasses\n"
    "};\n\n";

enum LineBreakClass {
    LineBreak_OP, LineBreak_CL, LineBreak_CP, LineBreak_QU, LineBreak_GL,
    LineBreak_NS, LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR,
    LineBreak_PO, LineBreak_NU, LineBreak_AL, LineBreak_HL, LineBreak_ID,
    LineBreak_IN, LineBreak_HY, LineBreak_BA, LineBreak_BB, LineBreak_B2,
    LineBreak_ZW, LineBreak_CM, LineBreak_WJ, LineBreak_H2, LineBreak_H3,
    LineBreak_JL, LineBreak_JV, LineBreak_JT, LineBreak_RI, LineBreak_CB,
    LineBreak_EB, LineBreak_EM, LineBreak_ZWJ,
    LineBreak_SA, LineBreak_SG, LineBreak_SP,
    LineBreak_CR, LineBreak_LF, LineBreak_BK,

    LineBreak_Unassigned
};

static QHash<QByteArray, LineBreakClass> line_break_map;

static void initLineBreak()
{
    // ### Classes XX and AI are left out and mapped to AL for now.
    // ### Class NL is mapped to BK.
    // ### Treating characters of class CJ as class NS will give CSS strict line breaking;
    //     treating them as class ID will give CSS normal breaking.
    struct LineBreakList {
        LineBreakClass brk;
        const char *name;
    } breaks[] = {
        { LineBreak_BK, "BK" },
        { LineBreak_CR, "CR" },
        { LineBreak_LF, "LF" },
        { LineBreak_CM, "CM" },
        { LineBreak_BK, "NL" },
        { LineBreak_SG, "SG" },
        { LineBreak_WJ, "WJ" },
        { LineBreak_ZW, "ZW" },
        { LineBreak_GL, "GL" },
        { LineBreak_SP, "SP" },
        { LineBreak_B2, "B2" },
        { LineBreak_BA, "BA" },
        { LineBreak_BB, "BB" },
        { LineBreak_HY, "HY" },
        { LineBreak_CB, "CB" },
        { LineBreak_NS, "CJ" },
        { LineBreak_CL, "CL" },
        { LineBreak_CP, "CP" },
        { LineBreak_EX, "EX" },
        { LineBreak_IN, "IN" },
        { LineBreak_NS, "NS" },
        { LineBreak_OP, "OP" },
        { LineBreak_QU, "QU" },
        { LineBreak_IS, "IS" },
        { LineBreak_NU, "NU" },
        { LineBreak_PO, "PO" },
        { LineBreak_PR, "PR" },
        { LineBreak_SY, "SY" },
        { LineBreak_AL, "AI" },
        { LineBreak_AL, "AL" },
        { LineBreak_HL, "HL" },
        { LineBreak_H2, "H2" },
        { LineBreak_H3, "H3" },
        { LineBreak_ID, "ID" },
        { LineBreak_JL, "JL" },
        { LineBreak_JV, "JV" },
        { LineBreak_JT, "JT" },
        { LineBreak_RI, "RI" },
        { LineBreak_SA, "SA" },
        { LineBreak_AL, "XX" },
        { LineBreak_EB, "EB" },
        { LineBreak_EM, "EM" },
        { LineBreak_ZWJ, "ZWJ" },
        { LineBreak_Unassigned, 0 }
    };
    LineBreakList *d = breaks;
    while (d->name) {
        line_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static QHash<QByteArray, QChar::Script> scriptMap;

static void initScriptMap()
{
    struct Scrpt {
        QChar::Script script;
        const char *name;
    } scripts[] = {
        // general
        { QChar::Script_Unknown,                "Unknown" },
        { QChar::Script_Inherited,              "Inherited" },
        { QChar::Script_Common,                 "Common" },
        // pre-4.0
        { QChar::Script_Latin,                  "Latin" },
        { QChar::Script_Greek,                  "Greek" },
        { QChar::Script_Cyrillic,               "Cyrillic" },
        { QChar::Script_Armenian,               "Armenian" },
        { QChar::Script_Hebrew,                 "Hebrew" },
        { QChar::Script_Arabic,                 "Arabic" },
        { QChar::Script_Syriac,                 "Syriac" },
        { QChar::Script_Thaana,                 "Thaana" },
        { QChar::Script_Devanagari,             "Devanagari" },
        { QChar::Script_Bengali,                "Bengali" },
        { QChar::Script_Gurmukhi,               "Gurmukhi" },
        { QChar::Script_Gujarati,               "Gujarati" },
        { QChar::Script_Oriya,                  "Oriya" },
        { QChar::Script_Tamil,                  "Tamil" },
        { QChar::Script_Telugu,                 "Telugu" },
        { QChar::Script_Kannada,                "Kannada" },
        { QChar::Script_Malayalam,              "Malayalam" },
        { QChar::Script_Sinhala,                "Sinhala" },
        { QChar::Script_Thai,                   "Thai" },
        { QChar::Script_Lao,                    "Lao" },
        { QChar::Script_Tibetan,                "Tibetan" },
        { QChar::Script_Myanmar,                "Myanmar" },
        { QChar::Script_Georgian,               "Georgian" },
        { QChar::Script_Hangul,                 "Hangul" },
        { QChar::Script_Ethiopic,               "Ethiopic" },
        { QChar::Script_Cherokee,               "Cherokee" },
        { QChar::Script_CanadianAboriginal,     "CanadianAboriginal" },
        { QChar::Script_Ogham,                  "Ogham" },
        { QChar::Script_Runic,                  "Runic" },
        { QChar::Script_Khmer,                  "Khmer" },
        { QChar::Script_Mongolian,              "Mongolian" },
        { QChar::Script_Hiragana,               "Hiragana" },
        { QChar::Script_Katakana,               "Katakana" },
        { QChar::Script_Bopomofo,               "Bopomofo" },
        { QChar::Script_Han,                    "Han" },
        { QChar::Script_Yi,                     "Yi" },
        { QChar::Script_OldItalic,              "OldItalic" },
        { QChar::Script_Gothic,                 "Gothic" },
        { QChar::Script_Deseret,                "Deseret" },
        { QChar::Script_Tagalog,                "Tagalog" },
        { QChar::Script_Hanunoo,                "Hanunoo" },
        { QChar::Script_Buhid,                  "Buhid" },
        { QChar::Script_Tagbanwa,               "Tagbanwa" },
        { QChar::Script_Coptic,                 "Coptic" },
        // 4.0
        { QChar::Script_Limbu,                  "Limbu" },
        { QChar::Script_TaiLe,                  "TaiLe" },
        { QChar::Script_LinearB,                "LinearB" },
        { QChar::Script_Ugaritic,               "Ugaritic" },
        { QChar::Script_Shavian,                "Shavian" },
        { QChar::Script_Osmanya,                "Osmanya" },
        { QChar::Script_Cypriot,                "Cypriot" },
        { QChar::Script_Braille,                "Braille" },
        // 4.1
        { QChar::Script_Buginese,               "Buginese" },
        { QChar::Script_NewTaiLue,              "NewTaiLue" },
        { QChar::Script_Glagolitic,             "Glagolitic" },
        { QChar::Script_Tifinagh,               "Tifinagh" },
        { QChar::Script_SylotiNagri,            "SylotiNagri" },
        { QChar::Script_OldPersian,             "OldPersian" },
        { QChar::Script_Kharoshthi,             "Kharoshthi" },
        // 5.0
        { QChar::Script_Balinese,               "Balinese" },
        { QChar::Script_Cuneiform,              "Cuneiform" },
        { QChar::Script_Phoenician,             "Phoenician" },
        { QChar::Script_PhagsPa,                "PhagsPa" },
        { QChar::Script_Nko,                    "Nko" },
        // 5.1
        { QChar::Script_Sundanese,              "Sundanese" },
        { QChar::Script_Lepcha,                 "Lepcha" },
        { QChar::Script_OlChiki,                "OlChiki" },
        { QChar::Script_Vai,                    "Vai" },
        { QChar::Script_Saurashtra,             "Saurashtra" },
        { QChar::Script_KayahLi,                "KayahLi" },
        { QChar::Script_Rejang,                 "Rejang" },
        { QChar::Script_Lycian,                 "Lycian" },
        { QChar::Script_Carian,                 "Carian" },
        { QChar::Script_Lydian,                 "Lydian" },
        { QChar::Script_Cham,                   "Cham" },
        // 5.2
        { QChar::Script_TaiTham,                "TaiTham" },
        { QChar::Script_TaiViet,                "TaiViet" },
        { QChar::Script_Avestan,                "Avestan" },
        { QChar::Script_EgyptianHieroglyphs,    "EgyptianHieroglyphs" },
        { QChar::Script_Samaritan,              "Samaritan" },
        { QChar::Script_Lisu,                   "Lisu" },
        { QChar::Script_Bamum,                  "Bamum" },
        { QChar::Script_Javanese,               "Javanese" },
        { QChar::Script_MeeteiMayek,            "MeeteiMayek" },
        { QChar::Script_ImperialAramaic,        "ImperialAramaic" },
        { QChar::Script_OldSouthArabian,        "OldSouthArabian" },
        { QChar::Script_InscriptionalParthian,  "InscriptionalParthian" },
        { QChar::Script_InscriptionalPahlavi,   "InscriptionalPahlavi" },
        { QChar::Script_OldTurkic,              "OldTurkic" },
        { QChar::Script_Kaithi,                 "Kaithi" },
        // 6.0
        { QChar::Script_Batak,                  "Batak" },
        { QChar::Script_Brahmi,                 "Brahmi" },
        { QChar::Script_Mandaic,                "Mandaic" },
        // 6.1
        { QChar::Script_Chakma,                 "Chakma" },
        { QChar::Script_MeroiticCursive,        "MeroiticCursive" },
        { QChar::Script_MeroiticHieroglyphs,    "MeroiticHieroglyphs" },
        { QChar::Script_Miao,                   "Miao" },
        { QChar::Script_Sharada,                "Sharada" },
        { QChar::Script_SoraSompeng,            "SoraSompeng" },
        { QChar::Script_Takri,                  "Takri" },
        // 7.0
        { QChar::Script_CaucasianAlbanian,      "CaucasianAlbanian" },
        { QChar::Script_BassaVah,               "BassaVah" },
        { QChar::Script_Duployan,               "Duployan" },
        { QChar::Script_Elbasan,                "Elbasan" },
        { QChar::Script_Grantha,                "Grantha" },
        { QChar::Script_PahawhHmong,            "PahawhHmong" },
        { QChar::Script_Khojki,                 "Khojki" },
        { QChar::Script_LinearA,                "LinearA" },
        { QChar::Script_Mahajani,               "Mahajani" },
        { QChar::Script_Manichaean,             "Manichaean" },
        { QChar::Script_MendeKikakui,           "MendeKikakui" },
        { QChar::Script_Modi,                   "Modi" },
        { QChar::Script_Mro,                    "Mro" },
        { QChar::Script_OldNorthArabian,        "OldNorthArabian" },
        { QChar::Script_Nabataean,              "Nabataean" },
        { QChar::Script_Palmyrene,              "Palmyrene" },
        { QChar::Script_PauCinHau,              "PauCinHau" },
        { QChar::Script_OldPermic,              "OldPermic" },
        { QChar::Script_PsalterPahlavi,         "PsalterPahlavi" },
        { QChar::Script_Siddham,                "Siddham" },
        { QChar::Script_Khudawadi,              "Khudawadi" },
        { QChar::Script_Tirhuta,                "Tirhuta" },
        { QChar::Script_WarangCiti,             "WarangCiti" },
        // 8.0
        { QChar::Script_Ahom,                   "Ahom" },
        { QChar::Script_AnatolianHieroglyphs,   "AnatolianHieroglyphs" },
        { QChar::Script_Hatran,                 "Hatran" },
        { QChar::Script_Multani,                "Multani" },
        { QChar::Script_OldHungarian,           "OldHungarian" },
        { QChar::Script_SignWriting,            "SignWriting" },
        // 9.0
        { QChar::Script_Adlam,                  "Adlam" },
        { QChar::Script_Bhaiksuki,              "Bhaiksuki" },
        { QChar::Script_Marchen,                "Marchen" },
        { QChar::Script_Newa,                   "Newa" },
        { QChar::Script_Osage,                  "Osage" },
        { QChar::Script_Tangut,                 "Tangut" },
        // 10.0
        { QChar::Script_MasaramGondi,           "MasaramGondi" },
        { QChar::Script_Nushu,                  "Nushu" },
        { QChar::Script_Soyombo,                "Soyombo" },
        { QChar::Script_ZanabazarSquare,        "ZanabazarSquare" },
        // 12.1
        { QChar::Script_Dogra,                  "Dogra" },
        { QChar::Script_GunjalaGondi,           "GunjalaGondi" },
        { QChar::Script_HanifiRohingya,         "HanifiRohingya" },
        { QChar::Script_Makasar,                "Makasar" },
        { QChar::Script_Medefaidrin,            "Medefaidrin" },
        { QChar::Script_OldSogdian,             "OldSogdian" },
        { QChar::Script_Sogdian,                "Sogdian" },
        { QChar::Script_Elymaic,                "Elymaic" },
        { QChar::Script_Nandinagari,            "Nandinagari" },
        { QChar::Script_NyiakengPuachueHmong,   "NyiakengPuachueHmong" },
        { QChar::Script_Wancho,                 "Wancho" },
        // 13.0
        { QChar::Script_Chorasmian,             "Chorasmian" },
        { QChar::Script_DivesAkuru,             "DivesAkuru" },
        { QChar::Script_KhitanSmallScript,      "KhitanSmallScript" },
        { QChar::Script_Yezidi,                 "Yezidi" },

        // 14.0
        { QChar::Script_CyproMinoan,            "CyproMinoan"},
        { QChar::Script_OldUyghur,              "OldUyghur"},
        { QChar::Script_Tangsa,                 "Tangsa"},
        { QChar::Script_Toto,                   "Toto"},
        { QChar::Script_Vithkuqi,               "Vithkuqi"},

        // 15.0
        { QChar::Script_Kawi,                   "Kawi"},
        { QChar::Script_NagMundari,             "NagMundari"},

        // unhandled
        { QChar::Script_Unknown,                0 }
    };
    Scrpt *p = scripts;
    while (p->name) {
        scriptMap.insert(p->name, p->script);
        ++p;
    }
}

// IDNA status as present int the data file
enum class IdnaRawStatus : unsigned int {
    Disallowed,
    Valid,
    Ignored,
    Mapped,
    Deviation,
    DisallowedStd3Valid,
    DisallowedStd3Mapped,
};

static QHash<QByteArray, IdnaRawStatus> idnaStatusMap;

static void initIdnaStatusMap()
{
    struct {
        IdnaRawStatus status;
        const char *name;
    } data[] = {
        {IdnaRawStatus::Disallowed,           "disallowed"},
        {IdnaRawStatus::Valid,                "valid"},
        {IdnaRawStatus::Ignored,              "ignored"},
        {IdnaRawStatus::Mapped,               "mapped"},
        {IdnaRawStatus::Deviation,            "deviation"},
        {IdnaRawStatus::DisallowedStd3Valid,  "disallowed_STD3_valid"},
        {IdnaRawStatus::DisallowedStd3Mapped, "disallowed_STD3_mapped"},
    };

    for (const auto &entry : data)
        idnaStatusMap[entry.name] = entry.status;
}

static const char *idna_status_string =
    "enum class IdnaStatus : unsigned int {\n"
    "    Disallowed,\n"
    "    Valid,\n"
    "    Ignored,\n"
    "    Mapped,\n"
    "    Deviation\n"
    "};\n\n";

// Resolved IDNA status as it goes into the database.
// Qt extends host name validity rules to allow underscores
// NOTE: The members here should come in the same order and have the same values
// as in IdnaRawStatus
enum class IdnaStatus : unsigned int {
    Disallowed,
    Valid,
    Ignored,
    Mapped,
    Deviation,
};

// Keep this one in sync with the code in createPropertyInfo
static const char *property_string =
    "enum Case {\n"
    "    LowerCase,\n"
    "    UpperCase,\n"
    "    TitleCase,\n"
    "    CaseFold,\n"
    "\n"
    "    NumCases\n"
    "};\n"
    "\n"
    "struct Properties {\n"
    "    ushort category            : 8; /* 5 used */\n"
    "    ushort direction           : 8; /* 5 used */\n"
    "    ushort combiningClass      : 8;\n"
    "    ushort joining             : 3;\n"
    "    signed short digitValue    : 5;\n"
    "    signed short mirrorDiff    : 16;\n"
    "    ushort unicodeVersion      : 5; /* 5 used */\n"
    "    ushort eastAsianWidth      : 3; /* 3 used */\n"
    "    ushort nfQuickCheck        : 8;\n" // could be narrowed
    "#ifdef Q_OS_WASM\n"
    "    unsigned char              : 0; //wasm 64 packing trick\n"
    "#endif\n"
    "    struct {\n"
    "        ushort special    : 1;\n"
    "        signed short diff : 15;\n"
    "    } cases[NumCases];\n"
    "#ifdef Q_OS_WASM\n"
    "    unsigned char              : 0; //wasm 64 packing trick\n"
    "#endif\n"
    "    ushort graphemeBreakClass  : 5; /* 5 used */\n"
    "    ushort wordBreakClass      : 5; /* 5 used */\n"
    "    ushort lineBreakClass      : 6; /* 6 used */\n"
    "    ushort sentenceBreakClass  : 4; /* 4 used */\n"
    "    ushort idnaStatus          : 4; /* 3 used */\n"
    "    ushort script              : 8;\n"
    "};\n\n"
    "Q_CORE_EXPORT const Properties * QT_FASTCALL properties(char32_t ucs4) noexcept;\n"
    "Q_CORE_EXPORT const Properties * QT_FASTCALL properties(char16_t ucs2) noexcept;\n"
    "\n";

static const char *methods =
    "Q_CORE_EXPORT GraphemeBreakClass QT_FASTCALL graphemeBreakClass(char32_t ucs4) noexcept;\n"
    "inline GraphemeBreakClass graphemeBreakClass(QChar ch) noexcept\n"
    "{ return graphemeBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT WordBreakClass QT_FASTCALL wordBreakClass(char32_t ucs4) noexcept;\n"
    "inline WordBreakClass wordBreakClass(QChar ch) noexcept\n"
    "{ return wordBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT SentenceBreakClass QT_FASTCALL sentenceBreakClass(char32_t ucs4) noexcept;\n"
    "inline SentenceBreakClass sentenceBreakClass(QChar ch) noexcept\n"
    "{ return sentenceBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT LineBreakClass QT_FASTCALL lineBreakClass(char32_t ucs4) noexcept;\n"
    "inline LineBreakClass lineBreakClass(QChar ch) noexcept\n"
    "{ return lineBreakClass(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT IdnaStatus QT_FASTCALL idnaStatus(char32_t ucs4) noexcept;\n"
    "inline IdnaStatus idnaStatus(QChar ch) noexcept\n"
    "{ return idnaStatus(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT QStringView QT_FASTCALL idnaMapping(char32_t usc4) noexcept;\n"
    "inline QStringView idnaMapping(QChar ch) noexcept\n"
    "{ return idnaMapping(ch.unicode()); }\n"
    "\n"
    "Q_CORE_EXPORT EastAsianWidth QT_FASTCALL eastAsianWidth(char32_t ucs4) noexcept;\n"
    "inline EastAsianWidth eastAsianWidth(QChar ch) noexcept\n"
    "{ return eastAsianWidth(ch.unicode()); }\n"
    "\n";

static const int SizeOfPropertiesStruct = 20;

static const QByteArray sizeOfPropertiesStructCheck =
        "static_assert(sizeof(Properties) == " + QByteArray::number(SizeOfPropertiesStruct) + ");\n\n";

struct PropertyFlags {
    PropertyFlags()
        : combiningClass(0)
        , category(QChar::Other_NotAssigned) // Cn
        , direction(QChar::DirL)
        , joining(QChar::Joining_None)
        , age(QChar::Unicode_Unassigned)
        , mirrorDiff(0) {}

    bool operator==(const PropertyFlags &o) const {
        return (combiningClass == o.combiningClass
                && category == o.category
                && direction == o.direction
                && joining == o.joining
                && age == o.age
                && eastAsianWidth == o.eastAsianWidth
                && digitValue == o.digitValue
                && mirrorDiff == o.mirrorDiff
                && lowerCaseDiff == o.lowerCaseDiff
                && upperCaseDiff == o.upperCaseDiff
                && titleCaseDiff == o.titleCaseDiff
                && caseFoldDiff == o.caseFoldDiff
                && lowerCaseSpecial == o.lowerCaseSpecial
                && upperCaseSpecial == o.upperCaseSpecial
                && titleCaseSpecial == o.titleCaseSpecial
                && caseFoldSpecial == o.caseFoldSpecial
                && graphemeBreakClass == o.graphemeBreakClass
                && wordBreakClass == o.wordBreakClass
                && sentenceBreakClass == o.sentenceBreakClass
                && lineBreakClass == o.lineBreakClass
                && script == o.script
                && nfQuickCheck == o.nfQuickCheck
                && idnaStatus == o.idnaStatus
            );
    }
    // from UnicodeData.txt
    uchar combiningClass : 8;
    QChar::Category category : 5;
    QChar::Direction direction : 5;
    // from ArabicShaping.txt
    QChar::JoiningType joining : 3;
    // from DerivedAge.txt
    QChar::UnicodeVersion age : 5;
    // From EastAsianWidth.txt
    EastAsianWidth eastAsianWidth = EastAsianWidth::N;
    int digitValue = -1;

    int mirrorDiff : 16;

    int lowerCaseDiff = 0;
    int upperCaseDiff = 0;
    int titleCaseDiff = 0;
    int caseFoldDiff = 0;
    bool lowerCaseSpecial = 0;
    bool upperCaseSpecial = 0;
    bool titleCaseSpecial = 0;
    bool caseFoldSpecial = 0;
    GraphemeBreakClass graphemeBreakClass = GraphemeBreak_Any;
    WordBreakClass wordBreakClass = WordBreak_Any;
    SentenceBreakClass sentenceBreakClass = SentenceBreak_Any;
    LineBreakClass lineBreakClass = LineBreak_AL;
    int script = QChar::Script_Unknown;
    // from DerivedNormalizationProps.txt
    uchar nfQuickCheck = 0;
    IdnaStatus idnaStatus = IdnaStatus::Disallowed;
};


static QList<int> specialCaseMap;

static int appendToSpecialCaseMap(const QList<int> &map)
{
    QList<int> utf16map;
    for (int i = 0; i < map.size(); ++i) {
        uint codepoint = map.at(i);
        // if the condition below doesn't hold anymore we need to modify our special case mapping code
        Q_ASSERT(!QChar::requiresSurrogates(codepoint));
        if (QChar::requiresSurrogates(codepoint)) {
            utf16map << QChar::highSurrogate(codepoint);
            utf16map << QChar::lowSurrogate(codepoint);
        } else {
            utf16map << codepoint;
        }
    }
    int length = utf16map.size();
    utf16map.prepend(length);

    if (specialCaseMap.isEmpty())
        specialCaseMap << 0; // placeholder

    int i = 1;
    while (i < specialCaseMap.size()) {
        int n = specialCaseMap.at(i);
        if (n == length) {
            int j;
            for (j = 1; j <= n; ++j) {
                if (specialCaseMap.at(i+j) != utf16map.at(j))
                    break;
            }
            if (j > n)
                return i;
        }
        i += n + 1;
    }

    int pos = specialCaseMap.size();
    specialCaseMap << utf16map;
    return pos;
}

// DerivedCoreProperties.txt
static inline bool isDefaultIgnorable(uint ucs4)
{
    // Default_Ignorable_Code_Point:
    //  Generated from
    //    Other_Default_Ignorable_Code_Point + Cf + Variation_Selector
    //    - White_Space - FFF9..FFFB (Annotation Characters)
    //    - 0600..0604, 06DD, 070F, 110BD (exceptional Cf characters that should be visible)
    if (ucs4 <= 0xff)
        return ucs4 == 0xad;

    return ucs4 == 0x034f
            || ucs4 == 0x061c
            || (ucs4 >= 0x115f && ucs4 <= 0x1160)
            || (ucs4 >= 0x17b4 && ucs4 <= 0x17b5)
            || (ucs4 >= 0x180b && ucs4 <= 0x180d)
            || ucs4 == 0x180e
            || (ucs4 >= 0x200b && ucs4 <= 0x200f)
            || (ucs4 >= 0x202a && ucs4 <= 0x202e)
            || (ucs4 >= 0x2060 && ucs4 <= 0x206f)
            || ucs4 == 0x3164
            || (ucs4 >= 0xfe00 && ucs4 <= 0xfe0f)
            || ucs4 == 0xfeff
            || ucs4 == 0xffa0
            || (ucs4 >= 0xfff0 && ucs4 <= 0xfff8)
            || (ucs4 >= 0x1bca0 && ucs4 <= 0x1bca3)
            || (ucs4 >= 0x1d173 && ucs4 <= 0x1d17a)
            || (ucs4 >= 0xe0000 && ucs4 <= 0xe0fff);
}

struct UnicodeData {
    UnicodeData(int codepoint = 0) {
        p.direction = QChar::DirL;
        // DerivedBidiClass.txt
        // The unassigned code points that default to AL are in the ranges:
        //     [U+0600..U+07BF, U+08A0..U+08FF, U+FB50..U+FDCF, U+FDF0..U+FDFF, U+FE70..U+FEFF, U+1EE00..U+1EEFF]
        if ((codepoint >= 0x0600 && codepoint <= 0x07BF)
            || (codepoint >= 0x08A0 && codepoint <= 0x08FF)
            || (codepoint >= 0xFB50 && codepoint <= 0xFDCF)
            || (codepoint >= 0xFDF0 && codepoint <= 0xFDFF)
            || (codepoint >= 0xFE70 && codepoint <= 0xFEFF)
            || (codepoint >= 0x1EE00 && codepoint <= 0x1EEFF)) {
            p.direction = QChar::DirAL;
        }
        // The unassigned code points that default to R are in the ranges:
        //     [U+0590..U+05FF, U+07C0..U+089F, U+FB1D..U+FB4F, U+10800..U+10FFF, U+1E800..U+1EDFF, U+1EF00..U+1EFFF]
        else if ((codepoint >= 0x0590 && codepoint <= 0x05FF)
            || (codepoint >= 0x07C0 && codepoint <= 0x089F)
            || (codepoint >= 0xFB1D && codepoint <= 0xFB4F)
            || (codepoint >= 0x10800 && codepoint <= 0x10FFF)
            || (codepoint >= 0x1E800 && codepoint <= 0x1EDFF)
            || (codepoint >= 0x1EF00 && codepoint <= 0x1EFFF)) {
            p.direction = QChar::DirR;
        }
        // The unassigned code points that default to ET are in the range:
        //     [U+20A0..U+20CF]
        else if (codepoint >= 0x20A0 && codepoint <= 0x20CF) {
            p.direction = QChar::DirET;
        }
        // The unassigned code points that default to BN have one of the following properties:
        //     Default_Ignorable_Code_Point
        //     Noncharacter_Code_Point
        else if (QChar::isNonCharacter(codepoint) || isDefaultIgnorable(codepoint)) {
            p.direction = QChar::DirBN;
        }

        p.lineBreakClass = LineBreak_AL; // XX -> AL
        // LineBreak.txt
        // The unassigned code points that default to "ID" include ranges in the following blocks:
        //     [U+3400..U+4DBF, U+4E00..U+9FFF, U+F900..U+FAFF, U+20000..U+2A6DF, U+2A700..U+2B73F, U+2B740..U+2B81F, U+2B820..U+2CEAF, U+2F800..U+2FA1F]
        // and any other reserved code points on
        //     [U+20000..U+2FFFD, U+30000..U+3FFFD]
        if ((codepoint >= 0x3400 && codepoint <= 0x4DBF)
            || (codepoint >= 0x4E00 && codepoint <= 0x9FFF)
            || (codepoint >= 0xF900 && codepoint <= 0xFAFF)
            || (codepoint >= 0x20000 && codepoint <= 0x2A6DF)
            || (codepoint >= 0x2A700 && codepoint <= 0x2B73F)
            || (codepoint >= 0x2B740 && codepoint <= 0x2B81F)
            || (codepoint >= 0x2B820 && codepoint <= 0x2CEAF)
            || (codepoint >= 0x2F800 && codepoint <= 0x2FA1F)
            || (codepoint >= 0x20000 && codepoint <= 0x2FFFD)
            || (codepoint >= 0x30000 && codepoint <= 0x3FFFD)) {
            p.lineBreakClass = LineBreak_ID;
        }
        // The unassigned code points that default to "PR" comprise a range in the following block:
        //     [U+20A0..U+20CF]
        else if (codepoint >= 0x20A0 && codepoint <= 0x20CF) {
            p.lineBreakClass = LineBreak_PR;
        }
    }

    static UnicodeData &valueRef(int codepoint);

    PropertyFlags p;

    // from UnicodeData.txt
    QChar::Decomposition decompositionType = QChar::NoDecomposition;
    QList<int> decomposition;

    QList<int> specialFolding;

    // from BidiMirroring.txt
    int mirroredChar = 0;

    // DerivedNormalizationProps.txt
    bool excludedComposition = false;

    // computed position of unicode property set
    int propertyIndex = -1;

    IdnaRawStatus idnaRawStatus = IdnaRawStatus::Disallowed;
};

static QList<UnicodeData> unicodeData;

UnicodeData &UnicodeData::valueRef(int codepoint)
{
    static bool initialized = false;
    if (!initialized) {
        unicodeData.reserve(QChar::LastValidCodePoint + 1);
        for (int uc = 0; uc <= QChar::LastValidCodePoint; ++uc)
            unicodeData.append(UnicodeData(uc));
        initialized = true;
    }

    Q_ASSERT(codepoint <= 0x10ffff);
    return unicodeData[codepoint];
}


static QHash<int, int> decompositionLength;
static int highestComposedCharacter = 0;
static int numLigatures = 0;
static int highestLigature = 0;

struct Ligature {
    int u1;
    int u2;
    int ligature;
};
// we need them sorted after the first component for fast lookup
bool operator < (const Ligature &l1, const Ligature &l2)
{ return l1.u1 < l2.u1; }

static QHash<int, QList<Ligature> > ligatureHashes;

static QHash<int, int> combiningClassUsage;

static int maxLowerCaseDiff = 0;
static int maxUpperCaseDiff = 0;
static int maxTitleCaseDiff = 0;

static void readUnicodeData()
{
    qDebug("Reading UnicodeData.txt");

    enum UniDataFields {
        UD_Value,
        UD_Name,
        UD_Category,
        UD_CombiningClass,
        UD_BidiCategory,
        UD_Decomposition,
        UD_DecimalDigitValue,
        UD_DigitValue,
        UD_NumericValue,
        UD_Mirrored,
        UD_OldName,
        UD_Comment,
        UD_UpperCase,
        UD_LowerCase,
        UD_TitleCase
    };

    QFile f("data/UnicodeData.txt");
    if (!f.exists())
        qFatal("Couldn't find UnicodeData.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.truncate(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        if (line.isEmpty())
            continue;

        QList<QByteArray> properties = line.split(';');
        bool ok;
        int codepoint = properties[UD_Value].toInt(&ok, 16);
        Q_ASSERT(ok);
        Q_ASSERT(codepoint <= QChar::LastValidCodePoint);
        int lastCodepoint = codepoint;

        QByteArray name = properties[UD_Name];
        if (name.startsWith('<') && name.contains("First")) {
            QByteArray nextLine;
            nextLine.resize(1024);
            f.readLine(nextLine.data(), 1024);
            QList<QByteArray> properties = nextLine.split(';');
            Q_ASSERT(properties[UD_Name].startsWith('<') && properties[UD_Name].contains("Last"));
            lastCodepoint = properties[UD_Value].toInt(&ok, 16);
            Q_ASSERT(ok);
            Q_ASSERT(lastCodepoint <= QChar::LastValidCodePoint);
        }

        UnicodeData &data = UnicodeData::valueRef(codepoint);
        data.p.category = categoryMap.value(properties[UD_Category], QChar::Other_NotAssigned);
        data.p.combiningClass = properties[UD_CombiningClass].toInt();
        if (!combiningClassUsage.contains(data.p.combiningClass))
            combiningClassUsage[data.p.combiningClass] = 1;
        else
            ++combiningClassUsage[data.p.combiningClass];

        Direction dir = directionMap.value(properties[UD_BidiCategory], Dir_Unassigned);
        if (dir == Dir_Unassigned)
            qFatal("unhandled direction value: %s", properties[UD_BidiCategory].constData());
        data.p.direction = QChar::Direction(dir);

        if (!properties[UD_UpperCase].isEmpty()) {
            int upperCase = properties[UD_UpperCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = upperCase - codepoint;
            // if the conditions below doesn't hold anymore we need to modify our upper casing code
            Q_ASSERT(QChar::requiresSurrogates(codepoint) == QChar::requiresSurrogates(upperCase));
            if (QChar::requiresSurrogates(codepoint)) {
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(upperCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(upperCase));
            }
            if (qAbs(diff) >= (1<<13)) {
                data.p.upperCaseSpecial = true;
                data.p.upperCaseDiff = appendToSpecialCaseMap(QList<int>() << upperCase);
            } else {
                data.p.upperCaseDiff = diff;
                maxUpperCaseDiff = qMax(maxUpperCaseDiff, qAbs(diff));
            }
        }
        if (!properties[UD_LowerCase].isEmpty()) {
            int lowerCase = properties[UD_LowerCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = lowerCase - codepoint;
            // if the conditions below doesn't hold anymore we need to modify our lower casing code
            Q_ASSERT(QChar::requiresSurrogates(codepoint) == QChar::requiresSurrogates(lowerCase));
            if (QChar::requiresSurrogates(codepoint)) {
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(lowerCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(lowerCase));
            }
            if (qAbs(diff) >= (1<<13)) {
                data.p.lowerCaseSpecial = true;
                data.p.lowerCaseDiff = appendToSpecialCaseMap(QList<int>() << lowerCase);
            } else {
                data.p.lowerCaseDiff = diff;
                maxLowerCaseDiff = qMax(maxLowerCaseDiff, qAbs(diff));
            }
        }
        // we want toTitleCase to map to ToUpper in case we don't have any titlecase.
        if (properties[UD_TitleCase].isEmpty())
            properties[UD_TitleCase] = properties[UD_UpperCase];
        if (!properties[UD_TitleCase].isEmpty()) {
            int titleCase = properties[UD_TitleCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = titleCase - codepoint;
            // if the conditions below doesn't hold anymore we need to modify our title casing code
            Q_ASSERT(QChar::requiresSurrogates(codepoint) == QChar::requiresSurrogates(titleCase));
            if (QChar::requiresSurrogates(codepoint)) {
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(titleCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(titleCase));
            }
            if (qAbs(diff) >= (1<<13)) {
                data.p.titleCaseSpecial = true;
                data.p.titleCaseDiff = appendToSpecialCaseMap(QList<int>() << titleCase);
            } else {
                data.p.titleCaseDiff = diff;
                maxTitleCaseDiff = qMax(maxTitleCaseDiff, qAbs(diff));
            }
        }

        if (!properties[UD_DigitValue].isEmpty())
            data.p.digitValue = properties[UD_DigitValue].toInt();

        // decompositition
        QByteArray decomposition = properties[UD_Decomposition];
        if (!decomposition.isEmpty()) {
            highestComposedCharacter = qMax(highestComposedCharacter, codepoint);
            QList<QByteArray> d = decomposition.split(' ');
            if (d[0].contains('<')) {
                data.decompositionType = decompositionMap.value(d[0], QChar::NoDecomposition);
                if (data.decompositionType == QChar::NoDecomposition)
                    qFatal("unhandled decomposition type: %s", d[0].constData());
                d.takeFirst();
            } else {
                data.decompositionType = QChar::Canonical;
            }
            for (int i = 0; i < d.size(); ++i) {
                data.decomposition.append(d[i].toInt(&ok, 16));
                Q_ASSERT(ok);
            }
            ++decompositionLength[data.decomposition.size()];
        }

        for (int i = codepoint; i <= lastCodepoint; ++i)
            unicodeData[i] = data;
    }
}

static int maxMirroredDiff = 0;

static void readBidiMirroring()
{
    qDebug("Reading BidiMirroring.txt");

    QFile f("data/BidiMirroring.txt");
    if (!f.exists())
        qFatal("Couldn't find BidiMirroring.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;
        line = line.replace(" ", "");

        QList<QByteArray> pair = line.split(';');
        Q_ASSERT(pair.size() == 2);

        bool ok;
        int codepoint = pair[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int mirror = pair[1].toInt(&ok, 16);
        Q_ASSERT(ok);

        UnicodeData &d = UnicodeData::valueRef(codepoint);
        d.mirroredChar = mirror;
        d.p.mirrorDiff = d.mirroredChar - codepoint;
        maxMirroredDiff = qMax(maxMirroredDiff, qAbs(d.p.mirrorDiff));
    }
}

static void readArabicShaping()
{
    qDebug("Reading ArabicShaping.txt");

    // Initialize defaults:
    // Code points that are not explicitly listed in ArabicShaping.txt are either of joining type T or U:
    // - Those that not explicitly listed that are of General Category Mn, Me, or Cf have joining type T.
    // - All others not explicitly listed have joining type U.
    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &d = UnicodeData::valueRef(codepoint);
        if (d.p.joining == QChar::Joining_None) {
            if (d.p.category == QChar::Mark_NonSpacing || d.p.category == QChar::Mark_Enclosing || d.p.category == QChar::Other_Format)
                d.p.joining = QChar::Joining_Transparent;
        }
    }

    QFile f("data/ArabicShaping.txt");
    if (!f.exists())
        qFatal("Couldn't find ArabicShaping.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line = line.trimmed();

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 4);

        bool ok;
        int codepoint = l[0].toInt(&ok, 16);
        Q_ASSERT(ok);

        UnicodeData &d = UnicodeData::valueRef(codepoint);
        JoiningType joining = joining_map.value(l[2].trimmed(), Joining_Unassigned);
        switch (joining) {
        case Joining_Unassigned:
            qFatal("%x: unassigned or unhandled joining type: %s", codepoint, l[2].constData());
            break;
        case Joining_Transparent:
            switch (d.p.category) {
            case QChar::Mark_Enclosing:
            case QChar::Mark_NonSpacing:
            case QChar::Letter_Modifier:
            case QChar::Other_Format:
                break;
            default:
                qFatal("%x: joining type '%s' was met (category: %d); "
                       "the current implementation needs to be revised!",
                       codepoint, l[2].constData(), d.p.category);
            }
            Q_FALLTHROUGH();
        default:
            d.p.joining = QChar::JoiningType(joining);
            break;
        }
    }
}

static void readDerivedAge()
{
    qDebug("Reading DerivedAge.txt");

    QFile f("data/DerivedAge.txt");
    if (!f.exists())
        qFatal("Couldn't find DerivedAge.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        QChar::UnicodeVersion age = age_map.value(l[1].trimmed(), QChar::Unicode_Unassigned);
        //qDebug() << Qt::hex << from << ".." << to << ba << age;
        if (age == QChar::Unicode_Unassigned)
            qFatal("unassigned or unhandled age value: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &d = UnicodeData::valueRef(codepoint);
            d.p.age = age;
        }
    }
}

static void readEastAsianWidth()
{
    qDebug("Reading EastAsianWidth.txt");

    QFile f("data/EastAsianWidth.txt");
    if (!f.exists() || !f.open(QFile::ReadOnly))
        qFatal("Couldn't find or read EastAsianWidth.txt");

    while (!f.atEnd()) {
        QByteArray line = f.readLine().trimmed();

        int comment = line.indexOf('#');
        line = (comment < 0 ? line : line.left(comment)).simplified();

        if (line.isEmpty())
            continue;

        QList<QByteArray> fields = line.split(';');
        Q_ASSERT(fields.size() == 2);

        // That would be split(".."), but that API does not exist.
        const QByteArray codePoints = fields[0].trimmed().replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');
        Q_ASSERT(cl.size() >= 1 && cl.size() <= 2);

        const QByteArray widthString = fields[1].trimmed();
        if (!eastAsianWidthMap.contains(widthString)) {
            qFatal("Unhandled EastAsianWidth property value for %s: %s",
                   qPrintable(codePoints), qPrintable(widthString));
        }
        auto width = eastAsianWidthMap.value(widthString);

        bool ok;
        const int first = cl[0].toInt(&ok, 16);
        const int last = ok && cl.size() == 2 ? cl[1].toInt(&ok, 16) : first;
        Q_ASSERT(ok);

        for (int codepoint = first; codepoint <= last; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            // Ensure that ranges don't overlap.
            Q_ASSERT(ud.p.eastAsianWidth == EastAsianWidth::N);
            ud.p.eastAsianWidth = width;
        }
    }
}

static void readDerivedNormalizationProps()
{
    qDebug("Reading DerivedNormalizationProps.txt");

    QFile f("data/DerivedNormalizationProps.txt");
    if (!f.exists())
        qFatal("Couldn't find DerivedNormalizationProps.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.trimmed().isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() >= 2);

        QByteArray propName = l[1].trimmed();
        if (propName != "Full_Composition_Exclusion" &&
            propName != "NFD_QC" && propName != "NFC_QC" &&
            propName != "NFKD_QC" && propName != "NFKC_QC") {
            // ###
            continue;
        }

        QByteArray codes = l[0].trimmed();
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &d = UnicodeData::valueRef(codepoint);
            if (propName == "Full_Composition_Exclusion") {
                d.excludedComposition = true;
            } else {
                static_assert(QString::NormalizationForm_D == 0);
                static_assert(QString::NormalizationForm_C == 1);
                static_assert(QString::NormalizationForm_KD == 2);
                static_assert(QString::NormalizationForm_KC == 3);

                QString::NormalizationForm form;
                if (propName == "NFD_QC")
                    form = QString::NormalizationForm_D;
                else if (propName == "NFC_QC")
                    form = QString::NormalizationForm_C;
                else if (propName == "NFKD_QC")
                    form = QString::NormalizationForm_KD;
                else// if (propName == "NFKC_QC")
                    form = QString::NormalizationForm_KC;

                Q_ASSERT(l.size() == 3);
                l[2] = l[2].trimmed();

                enum { NFQC_YES = 0, NFQC_NO = 1, NFQC_MAYBE = 3 };
                uchar ynm = (l[2] == "N" ? NFQC_NO : l[2] == "M" ? NFQC_MAYBE : NFQC_YES);
                if (ynm == NFQC_MAYBE) {
                    // if this changes, we need to revise the normalizationQuickCheckHelper() implementation
                    Q_ASSERT(form == QString::NormalizationForm_C || form == QString::NormalizationForm_KC);
                }
                d.p.nfQuickCheck |= (ynm << (form << 1)); // 2 bits per NF
            }
        }
    }

    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &d = UnicodeData::valueRef(codepoint);
        if (!d.excludedComposition
            && d.decompositionType == QChar::Canonical
            && d.decomposition.size() > 1) {
            Q_ASSERT(d.decomposition.size() == 2);

            int part1 = d.decomposition.at(0);
            int part2 = d.decomposition.at(1);

            // all non-starters are listed in DerivedNormalizationProps.txt
            // and already excluded from composition
            Q_ASSERT(UnicodeData::valueRef(part1).p.combiningClass == 0);

            ++numLigatures;
            highestLigature = qMax(highestLigature, part1);
            Ligature l = { part1, part2, codepoint };
            ligatureHashes[part2].append(l);
        }
    }
}


struct NormalizationCorrection {
    uint codepoint;
    uint mapped;
    int version;
};

static QByteArray createNormalizationCorrections()
{
    qDebug("Reading NormalizationCorrections.txt");

    QFile f("data/NormalizationCorrections.txt");
    if (!f.exists())
        qFatal("Couldn't find NormalizationCorrections.txt");

    f.open(QFile::ReadOnly);

    QByteArray out
         = "struct NormalizationCorrection {\n"
           "    uint ucs4;\n"
           "    uint old_mapping;\n"
           "    int version;\n"
           "};\n\n"

           "static constexpr NormalizationCorrection uc_normalization_corrections[] = {\n";

    int maxVersion = 0;
    int numCorrections = 0;
    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        Q_ASSERT(!line.contains(".."));

        QList<QByteArray> fields = line.split(';');
        Q_ASSERT(fields.size() == 4);

        NormalizationCorrection c = { 0, 0, 0 };
        bool ok;
        c.codepoint = fields.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        c.mapped = fields.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (fields.at(3) == "3.2.0")
            c.version = QChar::Unicode_3_2;
        else if (fields.at(3) == "4.0.0")
            c.version = QChar::Unicode_4_0;
        else
            qFatal("unknown unicode version in NormalizationCorrection.txt");

        out += "    { 0x" + QByteArray::number(c.codepoint, 16) + ", 0x"
               + QByteArray::number(c.mapped, 16) + ", "
               + QByteArray::number(c.version) + " },\n";
        ++numCorrections;
        maxVersion = qMax(c.version, maxVersion);
    }
    if (out.endsWith(",\n"))
        out.chop(2);

    out += "\n};\n\n"

           "enum { NumNormalizationCorrections = " + QByteArray::number(numCorrections) + " };\n"
           "enum { NormalizationCorrectionsVersionMax = " + QByteArray::number(maxVersion) + " };\n\n";

    return out;
}

static void readLineBreak()
{
    qDebug("Reading LineBreak.txt");

    QFile f("data/LineBreak.txt");
    if (!f.exists())
        qFatal("Couldn't find LineBreak.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        LineBreakClass lb = line_break_map.value(l[1], LineBreak_Unassigned);
        if (lb == LineBreak_Unassigned)
            qFatal("unassigned line break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &d = UnicodeData::valueRef(codepoint);
            d.p.lineBreakClass = lb;
        }
    }
}

static void readSpecialCasing()
{
    qDebug("Reading SpecialCasing.txt");

    QFile f("data/SpecialCasing.txt");
    if (!f.exists())
        qFatal("Couldn't find SpecialCasing.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');

        QByteArray condition = l.size() < 5 ? QByteArray() : l[4].trimmed();
        if (!condition.isEmpty())
            // #####
            continue;

        bool ok;
        int codepoint = l[0].trimmed().toInt(&ok, 16);
        Q_ASSERT(ok);

        // if the condition below doesn't hold anymore we need to modify our
        // lower/upper/title casing code and case folding code
        Q_ASSERT(!QChar::requiresSurrogates(codepoint));

//         qDebug() << "codepoint" << Qt::hex << codepoint;
//         qDebug() << line;

        QList<QByteArray> lower = l[1].trimmed().split(' ');
        QList<int> lowerMap;
        for (int i = 0; i < lower.size(); ++i) {
            bool ok;
            lowerMap.append(lower.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        QList<QByteArray> title = l[2].trimmed().split(' ');
        QList<int> titleMap;
        for (int i = 0; i < title.size(); ++i) {
            bool ok;
            titleMap.append(title.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        QList<QByteArray> upper = l[3].trimmed().split(' ');
        QList<int> upperMap;
        for (int i = 0; i < upper.size(); ++i) {
            bool ok;
            upperMap.append(upper.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }


        UnicodeData &ud = UnicodeData::valueRef(codepoint);
        Q_ASSERT(lowerMap.size() > 1 || lowerMap.at(0) == codepoint + ud.p.lowerCaseDiff);
        Q_ASSERT(titleMap.size() > 1 || titleMap.at(0) == codepoint + ud.p.titleCaseDiff);
        Q_ASSERT(upperMap.size() > 1 || upperMap.at(0) == codepoint + ud.p.upperCaseDiff);

        if (lowerMap.size() > 1) {
            ud.p.lowerCaseSpecial = true;
            ud.p.lowerCaseDiff = appendToSpecialCaseMap(lowerMap);
        }
        if (titleMap.size() > 1) {
            ud.p.titleCaseSpecial = true;
            ud.p.titleCaseDiff = appendToSpecialCaseMap(titleMap);
        }
        if (upperMap.size() > 1) {
            ud.p.upperCaseSpecial = true;
            ud.p.upperCaseDiff = appendToSpecialCaseMap(upperMap);
        }
    }
}

static int maxCaseFoldDiff = 0;

static void readCaseFolding()
{
    qDebug("Reading CaseFolding.txt");

    QFile f("data/CaseFolding.txt");
    if (!f.exists())
        qFatal("Couldn't find CaseFolding.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');

        bool ok;
        int codepoint = l[0].trimmed().toInt(&ok, 16);
        Q_ASSERT(ok);


        l[1] = l[1].trimmed();
        if (l[1] == "F" || l[1] == "T")
            continue;

//         qDebug() << "codepoint" << Qt::hex << codepoint;
//         qDebug() << line;
        QList<QByteArray> fold = l[2].trimmed().split(' ');
        QList<int> foldMap;
        for (int i = 0; i < fold.size(); ++i) {
            bool ok;
            foldMap.append(fold.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        UnicodeData &ud = UnicodeData::valueRef(codepoint);
        if (foldMap.size() == 1) {
            int caseFolded = foldMap.at(0);
            int diff = caseFolded - codepoint;
            // if the conditions below doesn't hold anymore we need to modify our case folding code
            Q_ASSERT(QChar::requiresSurrogates(codepoint) == QChar::requiresSurrogates(caseFolded));
            if (QChar::requiresSurrogates(codepoint)) {
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(caseFolded));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(caseFolded));
            }
            if (qAbs(diff) >= (1<<13)) {
                ud.p.caseFoldSpecial = true;
                ud.p.caseFoldDiff = appendToSpecialCaseMap(foldMap);
            } else {
                ud.p.caseFoldDiff = diff;
                maxCaseFoldDiff = qMax(maxCaseFoldDiff, qAbs(diff));
            }
        } else {
            qFatal("we currently don't support full case foldings");
//             qDebug() << "special" << Qt::hex << foldMap;
            ud.p.caseFoldSpecial = true;
            ud.p.caseFoldDiff = appendToSpecialCaseMap(foldMap);
        }
    }
}

static void readGraphemeBreak()
{
    qDebug("Reading GraphemeBreakProperty.txt");

    QFile f("data/GraphemeBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find GraphemeBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        GraphemeBreakClass brk = grapheme_break_map.value(l[1], GraphemeBreak_Unassigned);
        if (brk == GraphemeBreak_Unassigned)
            qFatal("unassigned grapheme break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.graphemeBreakClass = brk;
        }
    }
}

static void readEmojiData()
{
    qDebug("Reading emoji-data.txt");

    QFile f("data/emoji-data.txt");
    if (!f.open(QFile::ReadOnly))
        qFatal("Couldn't find emoji-data.txt");

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        // NOTE: for the moment we process emoji_data only to extract
        // the code points with Extended_Pictographic. This is needed by
        // extended grapheme clustering (cf. the GB11 rule in UAX #29).
        if (l[1] != "Extended_Pictographic")
            continue;

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            // Check we're not overwriting the data from GraphemeBreakProperty.txt...
            Q_ASSERT(ud.p.graphemeBreakClass == GraphemeBreak_Any);
            ud.p.graphemeBreakClass = GraphemeBreak_Extended_Pictographic;
        }
    }
}

static void readWordBreak()
{
    qDebug("Reading WordBreakProperty.txt");

    QFile f("data/WordBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find WordBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        WordBreakClass brk = word_break_map.value(l[1], WordBreak_Unassigned);
        if (brk == WordBreak_Unassigned)
            qFatal("unassigned word break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            // ### [
            // as of Unicode 5.1, some punctuation marks were mapped to MidLetter and MidNumLet
            // which caused "hi.there" to be treated like if it were just a single word;
            // until we have a tailoring mechanism, retain the old behavior by remapping those characters here.
            if (codepoint == 0x002E) // FULL STOP
                brk = WordBreak_MidNum;
            else if (codepoint == 0x003A) // COLON
                brk = WordBreak_Any;
            // ] ###
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.wordBreakClass = brk;
        }
    }
}

static void readSentenceBreak()
{
    qDebug("Reading SentenceBreakProperty.txt");

    QFile f("data/SentenceBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find SentenceBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        SentenceBreakClass brk = sentence_break_map.value(l[1], SentenceBreak_Unassigned);
        if (brk == SentenceBreak_Unassigned)
            qFatal("unassigned sentence break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.sentenceBreakClass = brk;
        }
    }
}

#if 0
// this piece of code does full case folding and comparison. We currently
// don't use it, since this gives lots of issues with things as case insensitive
// search and replace.
static inline void foldCase(uint ch, ushort *out)
{
    const QUnicodeTables::Properties *p = qGetProp(ch);
    if (!p->caseFoldSpecial) {
        *(out++) = ch + p->caseFoldDiff;
    } else {
        const ushort *folded = specialCaseMap + p->caseFoldDiff;
        ushort length = *folded++;
        while (length--)
            *out++ = *folded++;
    }
    *out = 0;
}

static int ucstricmp(const ushort *a, const ushort *ae, const ushort *b, const ushort *be)
{
    if (a == b)
        return 0;
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    while (a != ae && b != be) {
        const QUnicodeTables::Properties *pa = qGetProp(*a);
        const QUnicodeTables::Properties *pb = qGetProp(*b);
        if (pa->caseFoldSpecial | pb->caseFoldSpecial)
            goto special;
            int diff = (int)(*a + pa->caseFoldDiff) - (int)(*b + pb->caseFoldDiff);
        if ((diff))
            return diff;
        ++a;
        ++b;
        }
    }
    if (a == ae) {
        if (b == be)
            return 0;
        return -1;
    }
    return 1;
special:
    ushort abuf[SPECIAL_CASE_MAX_LEN + 1];
    ushort bbuf[SPECIAL_CASE_MAX_LEN + 1];
    abuf[0] = bbuf[0] = 0;
    ushort *ap = abuf;
    ushort *bp = bbuf;
    while (1) {
        if (!*ap) {
            if (a == ae) {
                if (!*bp && b == be)
                    return 0;
                return -1;
            }
            foldCase(*(a++), abuf);
            ap = abuf;
        }
        if (!*bp) {
            if (b == be)
                return 1;
            foldCase(*(b++), bbuf);
            bp = bbuf;
        }
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ++ap;
        ++bp;
    }
}


static int ucstricmp(const ushort *a, const ushort *ae, const uchar *b)
{
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    while (a != ae && *b) {
        const QUnicodeTables::Properties *pa = qGetProp(*a);
        const QUnicodeTables::Properties *pb = qGetProp((ushort)*b);
        if (pa->caseFoldSpecial | pb->caseFoldSpecial)
            goto special;
        int diff = (int)(*a + pa->caseFoldDiff) - (int)(*b + pb->caseFoldDiff);
        if ((diff))
            return diff;
        ++a;
        ++b;
    }
    if (a == ae) {
        if (!*b)
            return 0;
        return -1;
    }
    return 1;

special:
    ushort abuf[SPECIAL_CASE_MAX_LEN + 1];
    ushort bbuf[SPECIAL_CASE_MAX_LEN + 1];
    abuf[0] = bbuf[0] = 0;
    ushort *ap = abuf;
    ushort *bp = bbuf;
    while (1) {
        if (!*ap) {
            if (a == ae) {
                if (!*bp && !*b)
                    return 0;
                return -1;
            }
            foldCase(*(a++), abuf);
            ap = abuf;
        }
        if (!*bp) {
            if (!*b)
                return 1;
            foldCase(*(b++), bbuf);
            bp = bbuf;
        }
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ++ap;
        ++bp;
    }
}
#endif

#if 0
static QList<QByteArray> blockNames;
struct BlockInfo
{
    int blockIndex;
    int firstCodePoint;
    int lastCodePoint;
};
static QList<BlockInfo> blockInfoList;

static void readBlocks()
{
    qDebug("Reading Blocks.txt");

    QFile f("data/Blocks.txt");
    if (!f.exists())
        qFatal("Couldn't find Blocks.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        line.resize(line.size() - 1);

        int comment = line.indexOf("#");
        if (comment >= 0)
            line = line.left(comment);

        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        int semicolon = line.indexOf(';');
        Q_ASSERT(semicolon >= 0);
        QByteArray codePoints = line.left(semicolon);
        QByteArray blockName = line.mid(semicolon + 1);

        int blockIndex = blockNames.indexOf(blockName);
        if (blockIndex == -1) {
            blockIndex = blockNames.size();
            blockNames.append(blockName);
        }

        codePoints.replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');

        bool ok;
        int first = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int last = first;
        if (cl.size() == 2) {
            last = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        BlockInfo blockInfo = { blockIndex, first, last };
        blockInfoList.append(blockInfo);
    }
}
#endif

static void readScripts()
{
    qDebug("Reading Scripts.txt");

    QFile f("data/Scripts.txt");
    if (!f.exists())
        qFatal("Couldn't find Scripts.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        line.resize(line.size() - 1);

        int comment = line.indexOf("#");
        if (comment >= 0)
            line = line.left(comment);

        line.replace(" ", "");
        line.replace("_", "");

        if (line.isEmpty())
            continue;

        int semicolon = line.indexOf(';');
        Q_ASSERT(semicolon >= 0);
        QByteArray codePoints = line.left(semicolon);
        QByteArray scriptName = line.mid(semicolon + 1);

        codePoints.replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');

        bool ok;
        int first = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int last = first;
        if (cl.size() == 2) {
            last = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        if (!scriptMap.contains(scriptName))
            qFatal("Unhandled script property value: %s", scriptName.constData());
        QChar::Script script = scriptMap.value(scriptName, QChar::Script_Unknown);

        for (int codepoint = first; codepoint <= last; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            ud.p.script = script;
        }
    }
}

static QMap<char32_t, QString> idnaMappingTable;

static void readIdnaMappingTable()
{
    qDebug("Reading IdnaMappingTable.txt");

    QFile f("data/IdnaMappingTable.txt");
    if (!f.exists() || !f.open(QFile::ReadOnly))
        qFatal("Couldn't find or read IdnaMappingTable.txt");

    while (!f.atEnd()) {
        QByteArray line = f.readLine().trimmed();

        int comment = line.indexOf('#');
        line = (comment < 0 ? line : line.left(comment)).simplified();

        if (line.isEmpty())
            continue;

        QList<QByteArray> fields = line.split(';');
        Q_ASSERT(fields.size() >= 2);

        // That would be split(".."), but that API does not exist.
        const QByteArray codePoints = fields[0].trimmed().replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');
        Q_ASSERT(cl.size() >= 1 && cl.size() <= 2);

        const QByteArray statusString = fields[1].trimmed();
        if (!idnaStatusMap.contains(statusString))
            qFatal("Unhandled IDNA status property value for %s: %s",
                   qPrintable(codePoints), qPrintable(statusString));
        IdnaRawStatus rawStatus = idnaStatusMap.value(statusString);

        bool ok;
        const int first = cl[0].toInt(&ok, 16);
        const int last = ok && cl.size() == 2 ? cl[1].toInt(&ok, 16) : first;
        Q_ASSERT(ok);

        QString mapping;

        switch (rawStatus) {
        case IdnaRawStatus::Disallowed:
        case IdnaRawStatus::Valid:
        case IdnaRawStatus::Ignored:
        case IdnaRawStatus::DisallowedStd3Valid:
            break;

        case IdnaRawStatus::Mapped:
        case IdnaRawStatus::Deviation:
        case IdnaRawStatus::DisallowedStd3Mapped:
            Q_ASSERT(fields.size() >= 3);

            for (const auto &s : fields[2].trimmed().split(' ')) {
                if (!s.isEmpty()) {
                    bool ok;
                    int val = s.toInt(&ok, 16);
                    Q_ASSERT_X(ok, "readIdnaMappingTable", qPrintable(line));
                    for (auto c : QChar::fromUcs4(val))
                        mapping.append(c);
                }
            }

            // Some deviations have empty mappings, others should not...
            if (mapping.isEmpty()) {
                Q_ASSERT(rawStatus == IdnaRawStatus::Deviation);
                qDebug() << "    Empty IDNA mapping for" << codePoints;
            }

            break;
        }

        for (int codepoint = first; codepoint <= last; ++codepoint) {
            UnicodeData &ud = UnicodeData::valueRef(codepoint);
            // Ensure that ranges don't overlap.
            Q_ASSERT(ud.idnaRawStatus == IdnaRawStatus::Disallowed);
            ud.idnaRawStatus = rawStatus;

            // ASCII codepoints are skipped here because they are processed in separate
            // optimized code paths that do not use this mapping table.
            if (codepoint >= 0x80 && !mapping.isEmpty())
                idnaMappingTable[codepoint] = mapping;
        }
    }
}

/*
    Resolve IDNA status by deciding whether to allow STD3 violations

    Underscores are normally prohibited by STD3 rules but Qt allows underscores
    to be used inside URLs (see QTBUG-7434 for example). This code changes the
    underscore status to Valid. The same is done to mapped codepoints that
    map to underscores combined with other Valid codepoints.

    Underscores in domain names are required when using DNS-SD protocol and they
    are also allowed by the SMB protocol.
*/
static void resolveIdnaStatus()
{
    qDebug("resolveIdnaStatus:");

    UnicodeData::valueRef(u'_').idnaRawStatus = IdnaRawStatus::Valid;

    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &ud = UnicodeData::valueRef(codepoint);
        switch (ud.idnaRawStatus) {
        case IdnaRawStatus::Disallowed:
        case IdnaRawStatus::Valid:
        case IdnaRawStatus::Ignored:
        case IdnaRawStatus::Deviation:
        case IdnaRawStatus::Mapped:
            ud.p.idnaStatus = static_cast<IdnaStatus>(ud.idnaRawStatus);
            break;
        case IdnaRawStatus::DisallowedStd3Valid:
            ud.p.idnaStatus = IdnaStatus::Disallowed;
            break;
        case IdnaRawStatus::DisallowedStd3Mapped: {
            Q_ASSERT(idnaMappingTable.contains(codepoint));
            const auto &mapping = idnaMappingTable[codepoint];

            bool allow = true;
            for (QStringIterator iter(mapping); iter.hasNext();) {
                if (UnicodeData::valueRef(iter.next()).idnaRawStatus != IdnaRawStatus::Valid) {
                    allow = false;
                    break;
                }
            }

            if (allow) {
                qDebug() << "    Allowing" << Qt::hex << codepoint;
                ud.p.idnaStatus = IdnaStatus::Mapped;
            } else {
                ud.p.idnaStatus = IdnaStatus::Disallowed;
                idnaMappingTable.remove(codepoint);
            }
            break;
        }
        }
    }
}

/*
    Return maximum overlap for strings left and right in this order.

    The input strings should not be substrings of each other.
*/
static qsizetype overlap(const QString &left, const QString &right)
{
    for (qsizetype n = std::min(left.size(), right.size()) - 1; n > 0; n--) {
        if (left.last(n) == right.first(n))
            return n;
    }
    return 0;
}

using GraphNode = unsigned int;

struct OverlapGraphEdge
{
    GraphNode start;
    GraphNode end;
    qsizetype overlap;
};

/*
    Returns a common superstring of all inputs.

    Ideally this function would return the superstring of the smallest
    possible size, but the shortest common superstring problem is know to be
    NP-hard so an approximation must be used here.

    This function implements the greedy algorithm for building the superstring.

    As an optimization this function is allowed to destroy its inputs.
*/
static QString buildSuperstring(QList<QString> &inputs)
{
    // Ensure that the inputs don't contain substrings.
    // First, sort the array by length to make substring removal easier.
    std::sort(inputs.begin(), inputs.end(), [](const QString &a, const QString &b) {
        return a.size() == b.size() ? a > b : a.size() > b.size();
    });

    // Remove duplicates and other substrings
    for (auto i = inputs.begin() + 1; i != inputs.end();) {
        bool isSubstring = std::any_of(inputs.begin(), i, [i](const QString &s) {
            return s.contains(*i);
        });
        i = isSubstring ? inputs.erase(i) : i + 1;
    }

    // Build overlap graph for the remaining inputs. It is fully-connected.
    QList<OverlapGraphEdge> graphEdges;
    graphEdges.reserve(inputs.size() * (inputs.size() - 1));

    for (GraphNode i = 0; i < inputs.size(); i++) {
        for (GraphNode j = 0; j < inputs.size(); j++) {
            if (i != j)
                graphEdges.append(OverlapGraphEdge {i, j, overlap(inputs[i], inputs[j])});
        }
    }

    // Build a Hamiltonian path through the overlap graph, taking nodes with highest overlap
    // first.
    std::sort(graphEdges.begin(), graphEdges.end(), [](const auto &a, const auto &b) {
        return a.overlap == b.overlap
                ? a.start == b.start ? a.end < b.end : a.start < b.start
                : a.overlap > b.overlap;
    });

    QBitArray starts(inputs.size());
    QBitArray ends(inputs.size());
    QMap<GraphNode, OverlapGraphEdge> pathEdges;

    auto createsCycle = [&](const OverlapGraphEdge &edge) {
        if (!starts[edge.end] || !ends[edge.start])
            return false;
        Q_ASSERT(!pathEdges.contains(edge.start)); // Caller checks it's not yet a start.

        GraphNode node = edge.end;
        while (pathEdges.contains(node))
            node = pathEdges[node].end;

        return node == edge.start;
    };

    for (const auto &edge : graphEdges) {
        if (!starts[edge.start] && !ends[edge.end] && !createsCycle(edge)) {
            starts.setBit(edge.start);
            ends.setBit(edge.end);
            pathEdges[edge.start] = edge;
            if (pathEdges.size() == inputs.size() - 1)
                break;
        }
    }

    Q_ASSERT(ends.count(false) == 1);
    Q_ASSERT(starts.count(false) == 1);

    // Find the start node of the path.
    GraphNode node = 0;
    while (node < ends.size() && ends[node])
        node++;
    Q_ASSERT(node < ends.size());

    QString superstring = inputs[node];
    qsizetype pathNodes = 1; // Count path nodes for sanity check

    while (pathEdges.contains(node)) {
        const auto &edge = pathEdges[node];
        Q_ASSERT(edge.start == node);

        superstring.append(QStringView { inputs[edge.end] }.sliced(edge.overlap));

        node = edge.end;
        pathNodes++;
    }
    Q_ASSERT(pathNodes == inputs.size());

    return superstring;
}

/*
    Stores IDNA mapping information.

    The mapping table is an array of IdnaMapEntry instances sorted
    by codePoint. For mapping resulting in a single QChar, that character
    is stored inside the entry in charOrOffset. Otherwise the entry contains
    offset inside idnaMappingData array.

    It should be possible to find all mapped strings with size > 1 inside
    idnaMappingData, otherwise the construction of this array should be optimized
    to take advantage of common substrings and minimize the data size.
*/
static QByteArray createIdnaMapping()
{
    qDebug("createIdnaMapping:");

    QList<QString> values;
    values.reserve(idnaMappingTable.size());
    qsizetype uncompressedSize = 0;

    for (const auto &v : idnaMappingTable.values()) {
        if (v.size() > 2) {
            values.append(v);
            uncompressedSize += v.size();
        }
    }

    QString idnaMappingData = buildSuperstring(values);
    qDebug() << "    uncompressed size:" << uncompressedSize << "characters";
    qDebug() << "    consolidated size:" << idnaMappingData.size() << "characters";

    qsizetype memoryUsage = 0;

    QByteArray out =
        "static constexpr char16_t idnaMappingData[] = {";

    int col = 0;
    for (auto c : idnaMappingData) {
        if (col == 0)
            out += "\n   ";
        out += " 0x" + QByteArray::number(c.unicode(), 16) + ",";
        col = (col + 1) % 12;
        memoryUsage += 2;
    }
    out += "\n};\n\n";

    // Check if the values fit into IdnaMapEntry below.
    Q_ASSERT(idnaMappingData.size() < (1 << 16));

    // This could be written more elegantly with a union and designated initializers,
    // but designated initizers is a C++20 feature
    out +=
        "struct IdnaMapEntry {\n"
        "    // 21 bits suffice for any valid code-point (LastValidCodePoint = 0x10ffff)\n"
        "    unsigned codePoint : 24;\n"
        "    unsigned size : 8;\n"
        "    char16_t ucs[2]; // ucs[0] is offset if size > 2\n"
        "};\n"
        "static_assert(sizeof(IdnaMapEntry) == 8);\n\n"
        "static constexpr IdnaMapEntry idnaMap[] = {\n";

    for (auto i = idnaMappingTable.keyValueBegin(); i != idnaMappingTable.keyValueEnd(); i++) {
        const QString &mapping = i->second;
        Q_ASSERT(!mapping.isEmpty());

        qsizetype mappingIndex = idnaMappingData.indexOf(mapping);
        Q_ASSERT(mappingIndex >= 0 || mapping.size() <= 2);

        out += "    { 0x" + QByteArray::number(i->first, 16) +
               ", " + QByteArray::number(mapping.size());
        if (mapping.size() <= 2) {
            out += ", { 0x" + QByteArray::number(mapping[0].unicode(), 16);
            if (mapping.size() == 2)
                out += ", 0x" + QByteArray::number(mapping[1].unicode(), 16);
            else
                out += ", 0";
        } else {
            out += ", { " + QByteArray::number(mappingIndex);
            out += ", 0";
        }
        out += " } },\n";
        memoryUsage += 8;
    }

    qDebug() << "    memory usage:" << memoryUsage << "bytes";

    out +=
        "};\n\n"
        "Q_CORE_EXPORT QStringView QT_FASTCALL idnaMapping(char32_t ucs4) noexcept\n"
        "{\n"
        "    auto i = std::lower_bound(std::begin(idnaMap), std::end(idnaMap), ucs4,\n"
        "                              [](const auto &p, char32_t c) { return p.codePoint < c; });\n"
        "    if (i == std::end(idnaMap) || i->codePoint != ucs4)\n"
        "        return {};\n\n"
        "    return QStringView(i->size > 2 ? idnaMappingData + i->ucs[0] : i->ucs, i->size);\n"
        "}\n\n";

    return out;
}

#if 0
static void dump(int from, int to)
{
    for (int i = from; i <= to; ++i) {
        UnicodeData &d = UnicodeData::valueRef(i);
        qDebug("0x%04x: cat=%d combining=%d dir=%d case=%x mirror=%x joining=%d age=%d",
               i, d.p.category, d.p.combiningClass, d.p.direction, d.otherCase, d.mirroredChar, d.p.joining, d.p.age);
        if (d.decompositionType != QChar::NoDecomposition) {
            qDebug("    decomposition: type=%d, length=%d, first=%x", d.decompositionType, d.decomposition.size(),
                   d.decomposition[0]);
        }
    }
    qDebug(" ");
}
#endif

static QList<PropertyFlags> uniqueProperties;

static void computeUniqueProperties()
{
    qDebug("computeUniqueProperties:");
    for (int codepoint = 0; codepoint <= QChar::LastValidCodePoint; ++codepoint) {
        UnicodeData &d = UnicodeData::valueRef(codepoint);
        int index = uniqueProperties.indexOf(d.p);
        if (index == -1) {
            index = uniqueProperties.size();
            uniqueProperties.append(d.p);
        }
        d.propertyIndex = index;
    }
    qDebug("    %zd unique unicode properties found", ssize_t(uniqueProperties.size()));
}

struct UniqueBlock {
    inline UniqueBlock() : index(-1) {}

    inline bool operator==(const UniqueBlock &other) const
    { return values == other.values; }

    int index;
    QList<int> values;
};

static QByteArray createPropertyInfo()
{
    qDebug("createPropertyInfo:");

    // we reserve one bit more than in the assert below for the sign
    Q_ASSERT(maxMirroredDiff < (1<<12));
    Q_ASSERT(maxLowerCaseDiff < (1<<13));
    Q_ASSERT(maxUpperCaseDiff < (1<<13));
    Q_ASSERT(maxTitleCaseDiff < (1<<13));
    Q_ASSERT(maxCaseFoldDiff < (1<<13));

    const int BMP_BLOCKSIZE = 32;
    const int BMP_SHIFT = 5;
    const int BMP_END = 0x11000;
    const int SMP_END = 0x110000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    QList<UniqueBlock> uniqueBlocks;
    QList<int> blockMap;
    int used = 0;

    // Group BMP data into blocks indexed by their 12 most significant bits
    // (blockId = ucs >> 5):
    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(BMP_BLOCKSIZE);
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            b.values.append(d.propertyIndex);
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int bmp_blocks = uniqueBlocks.size();

    // Group SMP data into blocks indexed by their 9 most significant bits, plus
    // an offset to put them after the BMP blocks (blockId = (ucs >> 8) + 0x880):
    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(SMP_BLOCKSIZE);
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            b.values.append(d.propertyIndex);
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int smp_blocks = uniqueBlocks.size() - bmp_blocks;

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", bmp_blocks);
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = smp_blocks*SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", smp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    int prop_data = uniqueProperties.size() * SizeOfPropertiesStruct;
    qDebug("\n        properties data uses : %d bytes", prop_data);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + prop_data);

    Q_ASSERT(blockMap.size() == BMP_END/BMP_BLOCKSIZE +(SMP_END-BMP_END)/SMP_BLOCKSIZE); // 0x1870
    Q_ASSERT(blockMap.last() + blockMap.size() < (1<<(sizeof(unsigned short)*8)));

    QByteArray out = "static constexpr unsigned short uc_property_trie[] = {\n";
    // First write the map from blockId to indices of unique blocks:
    out += "    // [0x0..0x" + QByteArray::number(BMP_END, 16) + ")";
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // [0x" + QByteArray::number(BMP_END, 16) + "..0x" + QByteArray::number(SMP_END, 16) + ")\n";
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // Then write the contents of the unique blocks, at the anticipated indices.
    // Each unique block is a list of UnicodeData::propertyIndex values, whch
    // are indices into the uc_properties table.
    for (int i = 0; i < uniqueBlocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const UniqueBlock &b = uniqueBlocks.at(i);
        for (int j = 0; j < b.values.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += QByteArray::number(b.values.at(j));
            out += ", ";
        }
    }
    if (out.endsWith(", "))
        out.chop(2);
    out += "\n};\n\n";

    out += "static constexpr Properties uc_properties[] = {";
    // keep in sync with the property declaration
    for (int i = 0; i < uniqueProperties.size(); ++i) {
        const PropertyFlags &p = uniqueProperties.at(i);
        out += "\n    { ";
//     "        ushort category            : 8; /* 5 used */\n"
        out += QByteArray::number( p.category );
        out += ", ";
//     "        ushort direction           : 8; /* 5 used */\n"
        out += QByteArray::number( p.direction );
        out += ", ";
//     "        ushort combiningClass      : 8;\n"
        out += QByteArray::number( p.combiningClass );
        out += ", ";
//     "        ushort joining             : 3;\n"
        out += QByteArray::number( p.joining );
        out += ", ";
//     "        signed short digitValue    : 5;\n"
        out += QByteArray::number( p.digitValue );
        out += ", ";
//     "        signed short mirrorDiff    : 16;\n"
        out += QByteArray::number( p.mirrorDiff );
        out += ", ";
//     "        ushort unicodeVersion      : 5; /* 5 used */\n"
        out += QByteArray::number( p.age );
        out += ", ";
//     "        ushort eastAsianWidth      : 3;" /* 3 used */\n"
        out += QByteArray::number( static_cast<unsigned int>(p.eastAsianWidth) );
        out += ", ";
//     "        ushort nfQuickCheck        : 8;\n"
        out += QByteArray::number( p.nfQuickCheck );
        out += ", ";
//     "        struct {\n"
//     "            ushort special    : 1;\n"
//     "            signed short diff : 15;\n"
//     "        } cases[NumCases];\n"
        out += " { {";
        out += QByteArray::number( p.lowerCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.lowerCaseDiff );
        out += "}, {";
        out += QByteArray::number( p.upperCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.upperCaseDiff );
        out += "}, {";
        out += QByteArray::number( p.titleCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.titleCaseDiff );
        out += "}, {";
        out += QByteArray::number( p.caseFoldSpecial );
        out += ", ";
        out += QByteArray::number( p.caseFoldDiff );
        out += "} }, ";
//     "        ushort graphemeBreakClass  : 5; /* 5 used */\n"
//     "        ushort wordBreakClass      : 5; /* 5 used */\n"
//     "        ushort lineBreakClass      : 6; /* 6 used */\n"
        out += QByteArray::number( p.graphemeBreakClass );
        out += ", ";
        out += QByteArray::number( p.wordBreakClass );
        out += ", ";
        out += QByteArray::number( p.lineBreakClass );
        out += ", ";
//     "        ushort sentenceBreakClass  : 4; /* 4 used */\n"
        out += QByteArray::number( p.sentenceBreakClass );
        out += ", ";
//     "        ushort idnaStatus          : 4; /* 3 used */\n"
        out += QByteArray::number( static_cast<unsigned int>(p.idnaStatus) );
        out += ", ";
//     "        ushort script              : 8;\n"
        out += QByteArray::number( p.script );
        out += " },";
    }
    if (out.endsWith(','))
        out.chop(1);
    out += "\n};\n\n";

    out += "Q_DECL_CONST_FUNCTION static inline const Properties *qGetProp(char32_t ucs4) noexcept\n"
           "{\n"
           "    Q_ASSERT(ucs4 <= QChar::LastValidCodePoint);\n"
           "    if (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + ")\n"
           "        return uc_properties + uc_property_trie[uc_property_trie[ucs4 >> "
           + QByteArray::number(BMP_SHIFT) + "] + (ucs4 & 0x"
           + QByteArray::number(BMP_BLOCKSIZE - 1, 16)+ ")];\n"
           "\n"
           "    return uc_properties\n"
           "        + uc_property_trie[uc_property_trie[((ucs4 - 0x"
           + QByteArray::number(BMP_END, 16) + ") >> "
           + QByteArray::number(SMP_SHIFT) + ") + 0x"
           + QByteArray::number(BMP_END / BMP_BLOCKSIZE, 16) + "] + (ucs4 & 0x"
           + QByteArray::number(SMP_BLOCKSIZE - 1, 16) + ")];\n"
           "}\n"
           "\n"
           "Q_DECL_CONST_FUNCTION static inline const Properties *qGetProp(char16_t ucs2) noexcept\n"
           "{\n"
           "    return uc_properties + uc_property_trie[uc_property_trie[ucs2 >> "
           + QByteArray::number(BMP_SHIFT) + "] + (ucs2 & 0x"
           + QByteArray::number(BMP_BLOCKSIZE - 1, 16) + ")];\n"
           "}\n"
           "\n"
           "Q_DECL_CONST_FUNCTION Q_CORE_EXPORT const Properties * QT_FASTCALL properties(char32_t ucs4) noexcept\n"
           "{\n"
           "    return qGetProp(ucs4);\n"
           "}\n"
           "\n"
           "Q_DECL_CONST_FUNCTION Q_CORE_EXPORT const Properties * QT_FASTCALL properties(char16_t ucs2) noexcept\n"
           "{\n"
           "    return qGetProp(ucs2);\n"
           "}\n\n";

    out += "Q_CORE_EXPORT GraphemeBreakClass QT_FASTCALL graphemeBreakClass(char32_t ucs4) noexcept\n"
           "{\n"
           "    return static_cast<GraphemeBreakClass>(qGetProp(ucs4)->graphemeBreakClass);\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT WordBreakClass QT_FASTCALL wordBreakClass(char32_t ucs4) noexcept\n"
           "{\n"
           "    return static_cast<WordBreakClass>(qGetProp(ucs4)->wordBreakClass);\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT SentenceBreakClass QT_FASTCALL sentenceBreakClass(char32_t ucs4) noexcept\n"
           "{\n"
           "    return static_cast<SentenceBreakClass>(qGetProp(ucs4)->sentenceBreakClass);\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT LineBreakClass QT_FASTCALL lineBreakClass(char32_t ucs4) noexcept\n"
           "{\n"
           "    return static_cast<LineBreakClass>(qGetProp(ucs4)->lineBreakClass);\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT IdnaStatus QT_FASTCALL idnaStatus(char32_t ucs4) noexcept\n"
           "{\n"
           "    return static_cast<IdnaStatus>(qGetProp(ucs4)->idnaStatus);\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT EastAsianWidth QT_FASTCALL eastAsianWidth(char32_t ucs4) noexcept\n"
           "{\n"
           "    return static_cast<EastAsianWidth>(qGetProp(ucs4)->eastAsianWidth);\n"
           "}\n"
           "\n";

    return out;
}

static QByteArray createSpecialCaseMap()
{
    qDebug("createSpecialCaseMap:");

    QByteArray out
         = "static constexpr unsigned short specialCaseMap[] = {\n"
           "    0x0, // placeholder";

    int i = 1;
    int maxN = 0;
    while (i < specialCaseMap.size()) {
        out += "\n   ";
        int n = specialCaseMap.at(i);
        for (int j = 0; j <= n; ++j) {
            out += QByteArray(" 0x") + QByteArray::number(specialCaseMap.at(i+j), 16);
            out += ",";
        }
        i += n + 1;
        maxN = std::max(maxN, n);
    }
    out.chop(1);
    out += "\n};\n\nconstexpr unsigned int MaxSpecialCaseLength = ";
    out += QByteArray::number(maxN);
    out += ";\n\n";

    qDebug("    memory usage: %zd bytes", ssize_t(specialCaseMap.size() * sizeof(unsigned short)));

    return out;
}


static QByteArray createCompositionInfo()
{
    qDebug("createCompositionInfo: highestComposedCharacter=0x%x", highestComposedCharacter);

    const int BMP_BLOCKSIZE = 16;
    const int BMP_SHIFT = 4;
    const int BMP_END = 0x3400; // start of Han
    const int SMP_END = 0x30000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    if (SMP_END <= highestComposedCharacter)
        qFatal("end of table smaller than highest composed character 0x%x", highestComposedCharacter);

    QList<unsigned short> decompositions;
    int tableIndex = 0;

    QList<UniqueBlock> uniqueBlocks;
    QList<int> blockMap;
    int used = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(BMP_BLOCKSIZE);
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            if (!d.decomposition.isEmpty()) {
                int utf16Length = 0;
                decompositions.append(0);
                for (int j = 0; j < d.decomposition.size(); ++j) {
                    int code = d.decomposition.at(j);
                    if (QChar::requiresSurrogates(code)) {
                        // save as surrogate pair
                        decompositions.append(QChar::highSurrogate(code));
                        decompositions.append(QChar::lowSurrogate(code));
                        utf16Length += 2;
                    } else {
                        decompositions.append(code);
                        utf16Length++;
                    }
                }
                decompositions[tableIndex] = d.decompositionType + (utf16Length<<8);
                b.values.append(tableIndex);
                tableIndex += utf16Length + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int bmp_blocks = uniqueBlocks.size();

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(SMP_BLOCKSIZE);
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            UnicodeData &d = UnicodeData::valueRef(uc);
            if (!d.decomposition.isEmpty()) {
                int utf16Length = 0;
                decompositions.append(0);
                for (int j = 0; j < d.decomposition.size(); ++j) {
                    int code = d.decomposition.at(j);
                    if (QChar::requiresSurrogates(code)) {
                        // save as surrogate pair
                        decompositions.append(QChar::highSurrogate(code));
                        decompositions.append(QChar::lowSurrogate(code));
                        utf16Length += 2;
                    } else {
                        decompositions.append(code);
                        utf16Length++;
                    }
                }
                decompositions[tableIndex] = d.decompositionType + (utf16Length<<8);
                b.values.append(tableIndex);
                tableIndex += utf16Length + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int smp_blocks = uniqueBlocks.size() - bmp_blocks;

    // if the condition below doesn't hold anymore we need to modify our decomposition code
    Q_ASSERT(tableIndex < 0xffff);

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", bmp_blocks);
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = smp_blocks*SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", smp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    int decomposition_data = decompositions.size() * 2;
    qDebug("\n        decomposition data uses : %d bytes", decomposition_data);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + decomposition_data);

    Q_ASSERT(blockMap.last() + blockMap.size() < (1<<(sizeof(unsigned short)*8)));

    QByteArray out = "static constexpr unsigned short uc_decomposition_trie[] = {\n";
    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < uniqueBlocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const UniqueBlock &b = uniqueBlocks.at(i);
        for (int j = 0; j < b.values.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += "0x" + QByteArray::number(b.values.at(j), 16);
            out += ", ";
        }
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    out += "#define GET_DECOMPOSITION_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_decomposition_trie[uc_decomposition_trie[ucs4 >> "
           + QByteArray::number(BMP_SHIFT) + "] + (ucs4 & 0x"
           + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : ucs4 < 0x" + QByteArray::number(SMP_END, 16) + " \\\n"
           "        ? uc_decomposition_trie[uc_decomposition_trie[((ucs4 - 0x"
           + QByteArray::number(BMP_END, 16) + ") >> "
           + QByteArray::number(SMP_SHIFT) + ") + 0x"
           + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "] + (ucs4 & 0x"
           + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")] \\\n"
           "        : 0xffff)\n\n";

    out += "static constexpr unsigned short uc_decomposition_map[] = {";
    for (int i = 0; i < decompositions.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            out += "\n    ";
        }
        out += "0x" + QByteArray::number(decompositions.at(i), 16);
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    return out;
}

static QByteArray createLigatureInfo()
{
    qDebug("createLigatureInfo: numLigatures=%d, highestLigature=0x%x", numLigatures, highestLigature);

    for (int i = 0; i < ligatureHashes.size(); ++i) {
        const QList<Ligature> &l = ligatureHashes.value(i);
        for (int j = 0; j < l.size(); ++j) {
            // if the condition below doesn't hold anymore we need to modify our ligatureHelper code
            Q_ASSERT(QChar::requiresSurrogates(l.at(j).u2) == QChar::requiresSurrogates(l.at(j).ligature) &&
                     QChar::requiresSurrogates(l.at(j).u2) == QChar::requiresSurrogates(l.at(j).u1));
        }
    }

    const int BMP_BLOCKSIZE = 32;
    const int BMP_SHIFT = 5;
    const int BMP_END = 0x3100;
    const int SMP_END = 0x12000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    if (SMP_END <= highestLigature)
        qFatal("end of table smaller than highest ligature character 0x%x", highestLigature);

    QList<unsigned short> ligatures;
    int tableIndex = 0;

    QList<UniqueBlock> uniqueBlocks;
    QList<int> blockMap;
    int used = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(BMP_BLOCKSIZE);
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            QList<Ligature> l = ligatureHashes.value(uc);
            if (!l.isEmpty()) {
                Q_ASSERT(!QChar::requiresSurrogates(uc));
                std::sort(l.begin(), l.end()); // needed for bsearch in ligatureHelper code

                ligatures.append(l.size());
                for (int j = 0; j < l.size(); ++j) {
                    ligatures.append(l.at(j).u1);
                    ligatures.append(l.at(j).ligature);
                }
                b.values.append(tableIndex);
                tableIndex += 2*l.size() + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int bmp_blocks = uniqueBlocks.size();

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        UniqueBlock b;
        b.values.reserve(SMP_BLOCKSIZE);
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            QList<Ligature> l = ligatureHashes.value(uc);
            if (!l.isEmpty()) {
                Q_ASSERT(QChar::requiresSurrogates(uc));
                std::sort(l.begin(), l.end()); // needed for bsearch in ligatureHelper code

                ligatures.append(l.size());
                for (int j = 0; j < l.size(); ++j) {
                    ligatures.append(QChar::highSurrogate(l.at(j).u1));
                    ligatures.append(QChar::lowSurrogate(l.at(j).u1));
                    ligatures.append(QChar::highSurrogate(l.at(j).ligature));
                    ligatures.append(QChar::lowSurrogate(l.at(j).ligature));
                }
                b.values.append(tableIndex);
                tableIndex += 4*l.size() + 1;
            } else {
                b.values.append(0xffff);
            }
        }
        int index = uniqueBlocks.indexOf(b);
        if (index == -1) {
            index = uniqueBlocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            uniqueBlocks.append(b);
        }
        blockMap.append(uniqueBlocks.at(index).index);
    }
    int smp_blocks = uniqueBlocks.size() - bmp_blocks;

    // if the condition below doesn't hold anymore we need to modify our composition code
    Q_ASSERT(tableIndex < 0xffff);

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*sizeof(unsigned short);
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", bmp_blocks);
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = smp_blocks*SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*sizeof(unsigned short);
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", smp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    int ligature_data = ligatures.size() * 2;
    qDebug("\n        ligature data uses : %d bytes", ligature_data);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + ligature_data);

    Q_ASSERT(blockMap.last() + blockMap.size() < (1<<(sizeof(unsigned short)*8)));

    QByteArray out = "static constexpr unsigned short uc_ligature_trie[] = {\n";
    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < uniqueBlocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const UniqueBlock &b = uniqueBlocks.at(i);
        for (int j = 0; j < b.values.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += "0x" + QByteArray::number(b.values.at(j), 16);
            out += ", ";
        }
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n\n";

    out += "#define GET_LIGATURE_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_ligature_trie[uc_ligature_trie[ucs4 >> "
           + QByteArray::number(BMP_SHIFT) + "] + (ucs4 & 0x"
           + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : ucs4 < 0x" + QByteArray::number(SMP_END, 16) + " \\\n"
           "        ? uc_ligature_trie[uc_ligature_trie[((ucs4 - 0x"
           + QByteArray::number(BMP_END, 16) + ") >> "
           + QByteArray::number(SMP_SHIFT) + ") + 0x"
           + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "]" " + (ucs4 & 0x"
           + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")] \\\n"
           "        : 0xffff)\n\n";

    out += "static constexpr unsigned short uc_ligature_map[] = {";
    for (int i = 0; i < ligatures.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            out += "\n    ";
        }
        out += "0x" + QByteArray::number(ligatures.at(i), 16);
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(2);
    out += "\n};\n";

    return out;
}

QByteArray createCasingInfo()
{
    QByteArray out
         = "struct CasingInfo {\n"
           "    uint codePoint : 16;\n"
           "    uint flags : 8;\n"
           "    uint offset : 8;\n"
           "};\n\n";

    return out;
}


int main(int, char **)
{
    initAgeMap();
    initEastAsianWidthMap();
    initCategoryMap();
    initDecompositionMap();
    initDirectionMap();
    initJoiningMap();
    initGraphemeBreak();
    initWordBreak();
    initSentenceBreak();
    initLineBreak();
    initScriptMap();
    initIdnaStatusMap();

    readUnicodeData();
    readBidiMirroring();
    readArabicShaping();
    readDerivedAge();
    readEastAsianWidth();
    readDerivedNormalizationProps();
    readSpecialCasing();
    readCaseFolding();
    // readBlocks();
    readScripts();
    readGraphemeBreak();
    readEmojiData();
    readWordBreak();
    readSentenceBreak();
    readLineBreak();
    readIdnaMappingTable();

    resolveIdnaStatus();

    computeUniqueProperties();
    QByteArray properties = createPropertyInfo();
    QByteArray specialCases = createSpecialCaseMap();
    QByteArray compositions = createCompositionInfo();
    QByteArray ligatures = createLigatureInfo();
    QByteArray normalizationCorrections = createNormalizationCorrections();
    QByteArray idnaMapping = createIdnaMapping();

    QByteArray header =
        "// Copyright (C) 2020 The Qt Company Ltd.\n"
        "// SPDX-License-Identifier: Unicode-3.0\n"
        "\n";

    QByteArray note =
        "/* This file is autogenerated from the Unicode " DATA_VERSION_S " database. Do not edit */\n\n";

    QByteArray warning =
        "//\n"
        "//  W A R N I N G\n"
        "//  -------------\n"
        "//\n"
        "// This file is not part of the Qt API.  It exists for the convenience\n"
        "// of internal files.  This header file may change from version to version\n"
        "// without notice, or even be removed.\n"
        "//\n"
        "// We mean it.\n"
        "//\n\n";

    QFile f("../../src/corelib/text/qunicodetables.cpp");
    f.open(QFile::WriteOnly|QFile::Truncate);
    f.write(header);
    f.write(note);
    f.write("#include \"qunicodetables_p.h\"\n\n");
    f.write("QT_BEGIN_NAMESPACE\n\n");
    f.write("namespace QUnicodeTables {\n\n");
    f.write(properties);
    f.write(specialCases);
    f.write(compositions);
    f.write(ligatures);
    f.write("\n");
    f.write(normalizationCorrections);
    f.write(idnaMapping);
    f.write("} // namespace QUnicodeTables\n\n");
    f.write("using namespace QUnicodeTables;\n\n");
    f.write("QT_END_NAMESPACE\n");
    f.close();

    f.setFileName("../../src/corelib/text/qunicodetables_p.h");
    f.open(QFile::WriteOnly | QFile::Truncate);
    f.write(header);
    f.write(note);
    f.write(warning);
    f.write("#ifndef QUNICODETABLES_P_H\n"
            "#define QUNICODETABLES_P_H\n\n"
            "#include <QtCore/private/qglobal_p.h>\n\n"
            "#include <QtCore/qchar.h>\n\n"
            "QT_BEGIN_NAMESPACE\n\n");
    f.write("#define UNICODE_DATA_VERSION " DATA_VERSION_STR "\n\n");
    f.write("namespace QUnicodeTables {\n\n");
    f.write(property_string);
    f.write(sizeOfPropertiesStructCheck);
    f.write(east_asian_width_string);
    f.write(grapheme_break_class_string);
    f.write(word_break_class_string);
    f.write(sentence_break_class_string);
    f.write(line_break_class_string);
    f.write(idna_status_string);
    f.write(methods);
    f.write("} // namespace QUnicodeTables\n\n"
            "QT_END_NAMESPACE\n\n"
            "#endif // QUNICODETABLES_P_H\n");
    f.close();

    qDebug() << "maxMirroredDiff  = " << Qt::hex << maxMirroredDiff;
    qDebug() << "maxLowerCaseDiff = " << Qt::hex << maxLowerCaseDiff;
    qDebug() << "maxUpperCaseDiff = " << Qt::hex << maxUpperCaseDiff;
    qDebug() << "maxTitleCaseDiff = " << Qt::hex << maxTitleCaseDiff;
    qDebug() << "maxCaseFoldDiff  = " << Qt::hex << maxCaseFoldDiff;
#if 0
//     dump(0, 0x7f);
//     dump(0x620, 0x640);
//     dump(0x10000, 0x10020);
//     dump(0x10800, 0x10820);

    qDebug("decompositionLength used:");
    int totalcompositions = 0;
    int sum = 0;
    for (int i = 1; i < 20; ++i) {
        qDebug("    length %d used %d times", i, decompositionLength.value(i, 0));
        totalcompositions += i*decompositionLength.value(i, 0);
        sum += decompositionLength.value(i, 0);
    }
    qDebug("    len decomposition map %d, average length %f, num composed chars %d",
           totalcompositions, (float)totalcompositions/(float)sum, sum);
    qDebug("highest composed character %x", highestComposedCharacter);
    qDebug("num ligatures = %d highest=%x, maxLength=%d", numLigatures, highestLigature, longestLigature);

    qBubbleSort(ligatures);
    for (int i = 0; i < ligatures.size(); ++i)
        qDebug("%s", ligatures.at(i).data());

//     qDebug("combiningClass usage:");
//     int numClasses = 0;
//     for (int i = 0; i < 255; ++i) {
//         int num = combiningClassUsage.value(i, 0);
//         if (num) {
//             ++numClasses;
//             qDebug("    combiningClass %d used %d times", i, num);
//         }
//     }
//     qDebug("total of %d combining classes used", numClasses);

#endif
}
