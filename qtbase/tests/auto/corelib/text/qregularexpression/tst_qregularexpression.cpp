// Copyright (C) 2015 Giuseppe D'Angelo <dangelog@gmail.com>.
// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <qstring.h>
#include <qlist.h>
#include <qstringlist.h>
#include <qhash.h>

#include <qobject.h>
#include <qregularexpression.h>
#include <qthread.h>

#include <iostream>
#include <optional>

Q_DECLARE_METATYPE(QRegularExpression::PatternOptions)
Q_DECLARE_METATYPE(QRegularExpression::MatchType)
Q_DECLARE_METATYPE(QRegularExpression::MatchOptions)

class tst_QRegularExpression : public QObject
{
    Q_OBJECT

public:
    static void initMain();

private slots:
    void defaultConstructors();
    void moveSemantics();
    void moveSemanticsMatch();
    void moveSemanticsMatchIterator();
    void gettersSetters_data();
    void gettersSetters();
    void escape_data();
    void escape();
    void validity_data();
    void validity();
    void patternOptions_data();
    void patternOptions();
    void normalMatch_data();
    void normalMatch();
    void partialMatch_data();
    void partialMatch();
    void globalMatch_data();
    void globalMatch();
    void serialize_data();
    void serialize();
    void operatoreq_data();
    void operatoreq();
    void captureCount_data();
    void captureCount();
    void captureNames_data();
    void captureNames();
    void captureNamesNul();
    void pcreJitStackUsage_data();
    void pcreJitStackUsage();
    void regularExpressionMatch_data();
    void regularExpressionMatch();
    void JOptionUsage_data();
    void JOptionUsage();
    void QStringAndQStringViewEquivalence();
    void threadSafety_data();
    void threadSafety();

    void returnsViewsIntoOriginalString();
    void wildcard_data();
    void wildcard();
    void testInvalidWildcard_data();
    void testInvalidWildcard();

private:
    void provideRegularExpressions();
};

using CapturedList = QVector<std::optional<QString>>;

struct Match
{
    Match()
    {
        clear();
    }

    void clear()
    {
        isValid = false;
        hasMatch = false;
        hasPartialMatch = false;
        captured.clear();
        namedCaptured.clear();
    }

    bool isValid;
    bool hasMatch;
    bool hasPartialMatch;
    CapturedList captured;
    QHash<QString, std::optional<QString>> namedCaptured;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Match, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Match)

bool operator==(const QRegularExpressionMatch &rem, const Match &m)
{
    if (rem.isValid() != m.isValid)
        return false;
    if (!rem.isValid())
        return true;
    if ((rem.hasMatch() != m.hasMatch) || (rem.hasPartialMatch() != m.hasPartialMatch))
        return false;
    if (rem.hasMatch() || rem.hasPartialMatch()) {
        if (!rem.hasCaptured(0))
            return false;
        if (rem.lastCapturedIndex() != (m.captured.size() - 1))
            return false;
        for (int i = 0; i <= rem.lastCapturedIndex(); ++i) {
            auto mMaybeCaptured = m.captured.at(i);
            QString remCaptured = rem.captured(i);
            if (!mMaybeCaptured) {
                if (rem.hasCaptured(i))
                    return false;
                if (!remCaptured.isNull())
                    return false;
            } else {
                if (!rem.hasCaptured(i))
                    return false;
                QString mCaptured = *mMaybeCaptured;
                if (remCaptured != mCaptured
                    || remCaptured.isNull() != mCaptured.isNull()
                    || remCaptured.isEmpty() != mCaptured.isEmpty()) {
                    return false;
                }
            }
        }

        for (auto it = m.namedCaptured.begin(), end = m.namedCaptured.end(); it != end; ++it) {
            const QString capturedGroupName = it.key();
            const QString remCaptured = rem.captured(capturedGroupName);
            const auto mMaybeCaptured = it.value();
            if (!mMaybeCaptured) {
                if (rem.hasCaptured(capturedGroupName))
                    return false;
                if (!remCaptured.isNull())
                    return false;
            } else {
                if (!rem.hasCaptured(capturedGroupName))
                    return false;
                const auto mCaptured = *mMaybeCaptured;
                if (remCaptured != mCaptured
                    || remCaptured.isNull() != mCaptured.isNull()
                    || remCaptured.isEmpty() != mCaptured.isEmpty()) {
                    return false;
                }
            }
        }
    } else {
        if (rem.hasCaptured(0))
            return false;
    }

    return true;
}

bool operator==(const Match &m, const QRegularExpressionMatch &rem)
{
    return operator==(rem, m);
}

bool operator!=(const QRegularExpressionMatch &rem, const Match &m)
{
    return !operator==(rem, m);
}

bool operator!=(const Match &m, const QRegularExpressionMatch &rem)
{
    return !operator==(m, rem);
}


bool operator==(const QRegularExpressionMatchIterator &iterator, const QList<Match> &expectedMatchList)
{
    QRegularExpressionMatchIterator i = iterator;

    for (const Match &expectedMatch : expectedMatchList) {
        if (!i.hasNext())
            return false;

        QRegularExpressionMatch match = i.next();
        if (match != expectedMatch)
            return false;
    }

    if (i.hasNext())
        return false;

    i = iterator;

    int index = 0;
    for (const QRegularExpressionMatch &match : i) {
        if (match != expectedMatchList[index++])
            return false;
    }

    if (index != expectedMatchList.size())
        return false;

    // do it again
    index = 0;
    for (const QRegularExpressionMatch &match : i) {
        if (match != expectedMatchList[index++])
            return false;
    }

    if (index != expectedMatchList.size())
        return false;

    return true;
}

bool operator==(const QList<Match> &expectedMatchList, const QRegularExpressionMatchIterator &iterator)
{
    return operator==(iterator, expectedMatchList);
}

bool operator!=(const QRegularExpressionMatchIterator &iterator, const QList<Match> &expectedMatchList)
{
    return !operator==(iterator, expectedMatchList);
}

bool operator!=(const QList<Match> &expectedMatchList, const QRegularExpressionMatchIterator &iterator)
{
    return !operator==(expectedMatchList, iterator);
}

void consistencyCheck(const QRegularExpressionMatch &match)
{
    if (match.isValid()) {
        QVERIFY(match.regularExpression().isValid());
        QVERIFY(!(match.hasMatch() && match.hasPartialMatch()));

        if (match.hasMatch() || match.hasPartialMatch()) {
            QVERIFY(match.lastCapturedIndex() >= 0);
            if (match.hasPartialMatch())
                QVERIFY(match.lastCapturedIndex() == 0);

            for (int i = 0; i <= match.lastCapturedIndex(); ++i) {
                qsizetype startPos = match.capturedStart(i);
                qsizetype endPos = match.capturedEnd(i);
                qsizetype length = match.capturedLength(i);
                QString captured = match.captured(i);
                QStringView capturedView = match.capturedView(i);

                if (!captured.isNull()) {
                    QVERIFY(startPos >= 0);
                    QVERIFY(endPos >= 0);
                    QVERIFY(length >= 0);
                    QVERIFY(endPos >= startPos);
                    QVERIFY((endPos - startPos) == length);
                    QVERIFY(captured == capturedView);
                } else {
                    // A null capture can either mean no capture at all,
                    // or capture of length 0 over a null subject.
                    QVERIFY(startPos == endPos);
                    QVERIFY(((startPos == -1) && (endPos == -1)) // no capture
                            || ((startPos == 0) && (endPos == 0))); // null subject
                    QVERIFY((endPos - startPos) == length);
                    QVERIFY(capturedView.isNull());
                }
            }
        }
    } else {
        QVERIFY(!match.hasMatch());
        QVERIFY(!match.hasPartialMatch());
        QVERIFY(match.captured(0).isNull());
        QVERIFY(match.capturedStart(0) == -1);
        QVERIFY(match.capturedEnd(0) == -1);
        QVERIFY(match.capturedLength(0) == 0);
    }
}

void consistencyCheck(const QRegularExpressionMatchIterator &iterator)
{
    QRegularExpressionMatchIterator i(iterator); // make a copy, we modify it
    if (i.isValid()) {
        while (i.hasNext()) {
            QRegularExpressionMatch peeked = i.peekNext();
            QRegularExpressionMatch match = i.next();
            consistencyCheck(peeked);
            if (QTest::currentTestFailed())
                return;
            consistencyCheck(match);
            if (QTest::currentTestFailed())
                return;
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch() || match.hasPartialMatch());
            QCOMPARE(i.regularExpression(), match.regularExpression());
            QCOMPARE(i.matchOptions(), match.matchOptions());
            QCOMPARE(i.matchType(), match.matchType());

            QVERIFY(peeked.isValid() == match.isValid());
            QVERIFY(peeked.hasMatch() == match.hasMatch());
            QVERIFY(peeked.hasPartialMatch() == match.hasPartialMatch());
            QVERIFY(peeked.lastCapturedIndex() == match.lastCapturedIndex());
            for (int i = 0; i <= peeked.lastCapturedIndex(); ++i) {
                QVERIFY(peeked.captured(i) == match.captured(i));
                QVERIFY(peeked.capturedStart(i) == match.capturedStart(i));
                QVERIFY(peeked.capturedEnd(i) == match.capturedEnd(i));
            }
        }
    } else {
        QVERIFY(!i.hasNext());
        QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatchIterator::peekNext() called on an iterator already at end");
        QRegularExpressionMatch peeked = i.peekNext();
        QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatchIterator::next() called on an iterator already at end");
        QRegularExpressionMatch match = i.next();
        consistencyCheck(peeked);
        consistencyCheck(match);
        QVERIFY(!match.isValid());
        QVERIFY(!peeked.isValid());
    }

}

template<typename Result>
static void prepareResultForNoMatchType(Result *r, const Result &orig)
{
    Q_UNUSED(r);
    Q_UNUSED(orig);
}

static void prepareResultForNoMatchType(Match *m, const Match &orig)
{
    m->isValid = orig.isValid;
}

template<typename QREMatch, typename QREMatchFunc, typename Subject, typename Result>
static void testMatchImpl(const QRegularExpression &regexp,
                          QREMatchFunc matchingMethod,
                          const Subject &subject,
                          qsizetype offset,
                          QRegularExpression::MatchType matchType,
                          QRegularExpression::MatchOptions matchOptions,
                          const Result &result)
{
    {
        const QREMatch m = (regexp.*matchingMethod)(subject, offset, matchType, matchOptions);
        consistencyCheck(m);
        QVERIFY(m == result);
        QCOMPARE(m.regularExpression(), regexp);
        QCOMPARE(m.matchType(), matchType);
        QCOMPARE(m.matchOptions(), matchOptions);
    }
    {
        // ignore the expected results provided by the match object --
        // we'll never get any result when testing the NoMatch type.
        // Just check the validity of the match here.
        Result realMatch;
        prepareResultForNoMatchType(&realMatch, result);

        const QREMatch m = (regexp.*matchingMethod)(subject, offset, QRegularExpression::NoMatch, matchOptions);
        consistencyCheck(m);
        QVERIFY(m == realMatch);
        QCOMPARE(m.regularExpression(), regexp);
        QCOMPARE(m.matchType(), QRegularExpression::NoMatch);
        QCOMPARE(m.matchOptions(), matchOptions);
    }
}

template<typename QREMatch, typename QREMatchFuncForString, typename QREMatchFuncForStringRef, typename Result>
static void testMatch(const QRegularExpression &regexp,
                      QREMatchFuncForString matchingMethodForString,
                      QREMatchFuncForStringRef matchingMethodForStringRef,
                      const QString &subject,
                      qsizetype offset,
                      QRegularExpression::MatchType matchType,
                      QRegularExpression::MatchOptions matchOptions,
                      const Result &result)
{
    // test with QString as subject type
    testMatchImpl<QREMatch>(regexp, matchingMethodForString, subject, offset, matchType, matchOptions, result);

    // test with QStringView as subject type
    testMatchImpl<QREMatch>(regexp,
                            matchingMethodForStringRef,
                            QStringView(subject),
                            offset,
                            matchType,
                            matchOptions,
                            result);
}

// ### Qt 7: there should no longer be the need for these
typedef QRegularExpressionMatch (QRegularExpression::*QREMatchStringPMF)(const QString &, qsizetype, QRegularExpression::MatchType, QRegularExpression::MatchOptions) const;
typedef QRegularExpressionMatch (QRegularExpression::*QREMatchStringViewPMF)(QStringView, qsizetype, QRegularExpression::MatchType, QRegularExpression::MatchOptions) const;
typedef QRegularExpressionMatchIterator (QRegularExpression::*QREGlobalMatchStringPMF)(const QString &, qsizetype, QRegularExpression::MatchType, QRegularExpression::MatchOptions) const;
typedef QRegularExpressionMatchIterator (QRegularExpression::*QREGlobalMatchStringViewPMF)(QStringView, qsizetype, QRegularExpression::MatchType, QRegularExpression::MatchOptions) const;

void tst_QRegularExpression::provideRegularExpressions()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QRegularExpression::PatternOptions>("patternOptions");

    QTest::newRow("emptynull01") << QString()
                                 << QRegularExpression::PatternOptions{};
    QTest::newRow("emptynull02") << QString()
                                 << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                       | QRegularExpression::DotMatchesEverythingOption
                                                                       | QRegularExpression::MultilineOption);
    QTest::newRow("emptynull03") << ""
                                 << QRegularExpression::PatternOptions{};
    QTest::newRow("emptynull04") << ""
                                 << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                       | QRegularExpression::DotMatchesEverythingOption
                                                                       | QRegularExpression::MultilineOption);

    QTest::newRow("regexp01") << "a pattern"
                              << QRegularExpression::PatternOptions{};
    QTest::newRow("regexp02") << "^a (.*) more complicated(?<P>pattern)$"
                              << QRegularExpression::PatternOptions{};
    QTest::newRow("regexp03") << "(?:a) pAttErN"
                              << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("regexp04") << "a\nmultiline\npattern"
                              << QRegularExpression::PatternOptions(QRegularExpression::MultilineOption);
    QTest::newRow("regexp05") << "an extended # IGNOREME\npattern"
                              << QRegularExpression::PatternOptions(QRegularExpression::ExtendedPatternSyntaxOption);
    QTest::newRow("regexp06") << "a [sS]ingleline .* match"
                              << QRegularExpression::PatternOptions(QRegularExpression::DotMatchesEverythingOption);
    QTest::newRow("regexp07") << "multiple.*options"
                              << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                    | QRegularExpression::DotMatchesEverythingOption
                                                                    | QRegularExpression::MultilineOption
                                                                    | QRegularExpression::DontCaptureOption
                                                                    | QRegularExpression::InvertedGreedinessOption);

    QTest::newRow("unicode01") << QString::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$")
                               << QRegularExpression::PatternOptions{};
    QTest::newRow("unicode02") << QString::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$")
                               << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                     | QRegularExpression::DotMatchesEverythingOption
                                                                     | QRegularExpression::InvertedGreedinessOption);
    QTest::newRow("unicode03") << QString::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f")
                               << QRegularExpression::PatternOptions{};
    QTest::newRow("unicode04") << QString::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f")
                               << QRegularExpression::PatternOptions(QRegularExpression::CaseInsensitiveOption
                                                                     | QRegularExpression::DotMatchesEverythingOption
                                                                     | QRegularExpression::InvertedGreedinessOption);
}

static const char enableJitEnvironmentVariable[] = "QT_ENABLE_REGEXP_JIT";

void tst_QRegularExpression::initMain()
{
    if (!qEnvironmentVariableIsSet(enableJitEnvironmentVariable)) {
        std::cerr << "Enabling QRegularExpression JIT for testing; set QT_ENABLE_REGEXP_JIT to 0 to disable it.\n";
        qputenv(enableJitEnvironmentVariable, "1");
    }
}

void tst_QRegularExpression::defaultConstructors()
{
    QRegularExpression re;
    QCOMPARE(re.pattern(), QString());
    QCOMPARE(re.patternOptions(), QRegularExpression::NoPatternOption);
    QCOMPARE(re.isValid(), true);
    QCOMPARE(re.patternErrorOffset(), -1);
    QCOMPARE(re.captureCount(), 0);
    QCOMPARE(re.namedCaptureGroups(), QStringList { QString() });

    QRegularExpressionMatch match;
    QCOMPARE(match.regularExpression(), QRegularExpression());
    QCOMPARE(match.regularExpression(), re);
    QCOMPARE(match.matchType(), QRegularExpression::NoMatch);
    QCOMPARE(match.matchOptions(), QRegularExpression::NoMatchOption);
    QCOMPARE(match.hasMatch(), false);
    QCOMPARE(match.hasPartialMatch(), false);
    QCOMPARE(match.isValid(), true);
    QCOMPARE(match.lastCapturedIndex(), -1);
    QCOMPARE(match.captured(), QString());
    QCOMPARE(match.captured("test"), QString());
    QCOMPARE(match.capturedTexts(), QStringList());
    QCOMPARE(match.capturedStart(), -1);
    QCOMPARE(match.capturedEnd(), -1);
    QCOMPARE(match.capturedLength(), 0);
    QCOMPARE(match.capturedStart("test"), -1);
    QCOMPARE(match.capturedEnd("test"), -1);
    QCOMPARE(match.capturedLength("test"), 0);

    QRegularExpressionMatchIterator iterator;
    QCOMPARE(iterator.regularExpression(), QRegularExpression());
    QCOMPARE(iterator.regularExpression(), re);
    QCOMPARE(iterator.matchType(), QRegularExpression::NoMatch);
    QCOMPARE(iterator.matchOptions(), QRegularExpression::NoMatchOption);
    QCOMPARE(iterator.isValid(), true);
    QCOMPARE(iterator.hasNext(), false);
}

void tst_QRegularExpression::moveSemantics()
{
    const QString pattern = "pattern";
    const QRegularExpression::PatternOptions options = QRegularExpression::CaseInsensitiveOption;
    QRegularExpression expr1(pattern, options);
    QCOMPARE(expr1.pattern(), pattern);
    QCOMPARE(expr1.patternOptions(), options);

    QRegularExpression expr2(std::move(expr1));
    QCOMPARE(expr2.pattern(), pattern);
    QCOMPARE(expr2.patternOptions(), options);

    const QString pattern2 = "pattern2";
    QRegularExpression expr3(pattern2);
    QCOMPARE(expr3.pattern(), pattern2);
    QCOMPARE(expr3.patternOptions(), QRegularExpression::NoPatternOption);

    // check that (move)assigning to the moved-from object is ok
    expr1 = std::move(expr3);
    QCOMPARE(expr1.pattern(), pattern2);
    QCOMPARE(expr1.patternOptions(), QRegularExpression::NoPatternOption);

    // here expr3 is in the moved-from state, so destructor call for moved-from
    // object is also checked
}

void tst_QRegularExpression::moveSemanticsMatch()
{
    QRegularExpression re("test");
    QRegularExpressionMatch match1 = re.match("abctestdef");
    QCOMPARE(match1.hasMatch(), true);
    QCOMPARE(match1.capturedStart(), 3);
    QCOMPARE(match1.capturedEnd(), 7);

    QRegularExpressionMatch match2(std::move(match1));
    QCOMPARE(match2.hasMatch(), true);
    QCOMPARE(match2.capturedStart(), 3);
    QCOMPARE(match2.capturedEnd(), 7);
    consistencyCheck(match2);
    if (QTest::currentTestFailed())
        return;

    QRegularExpressionMatch match3 = re.match("test1");
    QCOMPARE(match3.hasMatch(), true);
    QCOMPARE(match3.capturedStart(), 0);
    QCOMPARE(match3.capturedEnd(), 4);

    // check that (move)assigning to the moved-from object is ok
    match1 = std::move(match3);
    QCOMPARE(match1.hasMatch(), true);
    QCOMPARE(match1.capturedStart(), 0);
    QCOMPARE(match1.capturedEnd(), 4);
    consistencyCheck(match1);
    if (QTest::currentTestFailed())
        return;

    // here match3 is in the moved-from state, so destructor call for moved-from
    // object is also checked
}

void tst_QRegularExpression::moveSemanticsMatchIterator()
{
    QRegularExpression re("(\\w+)");
    QRegularExpressionMatchIterator it1 = re.globalMatch("some test");
    QVERIFY(it1.isValid());
    QVERIFY(it1.hasNext());
    QCOMPARE(it1.regularExpression(), re);

    QRegularExpressionMatchIterator it2(std::move(it1));
    QVERIFY(it2.isValid());
    QVERIFY(it2.hasNext());
    QCOMPARE(it2.regularExpression(), re);
    consistencyCheck(it2);
    if (QTest::currentTestFailed())
        return;

    QRegularExpression re2("test");
    QRegularExpressionMatchIterator it3 = re2.globalMatch("123test456");
    QVERIFY(it3.isValid());
    QVERIFY(it3.hasNext());
    QCOMPARE(it3.regularExpression(), re2);

    // check that (move)assigning to the moved-from object is ok
    it1 = std::move(it3);
    QVERIFY(it1.isValid());
    QVERIFY(it1.hasNext());
    QCOMPARE(it1.regularExpression(), re2);
    consistencyCheck(it1);
    if (QTest::currentTestFailed())
        return;

    // here it3 is in the moved-from state, so destructor call for moved-from
    // object is also checked
}

void tst_QRegularExpression::gettersSetters_data()
{
    provideRegularExpressions();
}

void tst_QRegularExpression::gettersSetters()
{
    QFETCH(QString, pattern);
    QFETCH(QRegularExpression::PatternOptions, patternOptions);
    {
        QRegularExpression re;
        re.setPattern(pattern);
        QCOMPARE(re.pattern(), pattern);
        QCOMPARE(re.patternOptions(), QRegularExpression::NoPatternOption);
    }
    {
        QRegularExpression re;
        re.setPatternOptions(patternOptions);
        QCOMPARE(re.pattern(), QString());
        QCOMPARE(re.patternOptions(), patternOptions);
    }
    {
        QRegularExpression re(pattern);
        QCOMPARE(re.pattern(), pattern);
        QCOMPARE(re.patternOptions(), QRegularExpression::NoPatternOption);
    }
    {
        QRegularExpression re(pattern, patternOptions);
        QCOMPARE(re.pattern(), pattern);
        QCOMPARE(re.patternOptions(), patternOptions);
    }
}

void tst_QRegularExpression::escape_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("escaped");
    QTest::newRow("escape01") << "a normal pattern"
                              << "a\\ normal\\ pattern";

    QTest::newRow("escape02") << "abcdefghijklmnopqrstuvzABCDEFGHIJKLMNOPQRSTUVZ1234567890_"
                              << "abcdefghijklmnopqrstuvzABCDEFGHIJKLMNOPQRSTUVZ1234567890_";

    QTest::newRow("escape03") << "^\\ba\\b.*(?<NAME>reg|exp)$"
                              << "\\^\\\\ba\\\\b\\.\\*\\(\\?\\<NAME\\>reg\\|exp\\)\\$";

    QString nulString("abcXabcXXabc");
    nulString[3] = nulString[7] = nulString[8] = QChar(0, 0);
    QTest::newRow("NUL") << nulString
                         << "abc\\0abc\\0\\0abc";

    QTest::newRow("unicode01") << QString::fromUtf8("^s[ome] latin-1 \xc3\x80\xc3\x88\xc3\x8c\xc3\x92\xc3\x99 chars$")
                               << QString::fromUtf8("\\^s\\[ome\\]\\ latin\\-1\\ \\\xc3\x80\\\xc3\x88\\\xc3\x8c\\\xc3\x92\\\xc3\x99\\ chars\\$");
    QTest::newRow("unicode02") << QString::fromUtf8("Unicode \xf0\x9d\x85\x9d \xf0\x9d\x85\x9e\xf0\x9d\x85\x9f")
                               << QString::fromUtf8("Unicode\\ \\\xf0\x9d\x85\x9d\\ \\\xf0\x9d\x85\x9e\\\xf0\x9d\x85\x9f");

    QString unicodeAndNulString = QString::fromUtf8("^\xc3\x80\xc3\x88\xc3\x8cN\xc3\x92NN\xc3\x99 chars$");
    unicodeAndNulString[4] = unicodeAndNulString[6] = unicodeAndNulString[7] = QChar(0, 0);
    QTest::newRow("unicode03") << unicodeAndNulString
                               << QString::fromUtf8("\\^\\\xc3\x80\\\xc3\x88\\\xc3\x8c\\0\\\xc3\x92\\0\\0\\\xc3\x99\\ chars\\$");
}

void tst_QRegularExpression::escape()
{
    QFETCH(QString, string);
    QFETCH(QString, escaped);
    QCOMPARE(QRegularExpression::escape(string), escaped);
    QRegularExpression re(escaped);
    QCOMPARE(re.isValid(), true);
}

void tst_QRegularExpression::validity_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("validity");

    QTest::newRow("valid01") << "a pattern" << true;
    QTest::newRow("valid02") << "(a|pattern)" << true;
    QTest::newRow("valid03") << "a [pP]attern" << true;
    QTest::newRow("valid04") << "^(?<article>a).*(?<noun>pattern)$" << true;
    QTest::newRow("valid05") << "a \\P{Ll}attern" << true;

    QTest::newRow("invalid01") << "a pattern\\" << false;
    QTest::newRow("invalid02") << "(a|pattern" << false;
    QTest::newRow("invalid03") << "a \\P{BLAH}attern" << false;

    QString pattern;
    // 0xD800 (high surrogate) not followed by a low surrogate
    pattern = "abcdef";
    pattern[3] = QChar(0x00, 0xD8);
    QTest::newRow("invalidUnicode01") << pattern << false;
}

void tst_QRegularExpression::validity()
{
    static const QRegularExpression ignoreMessagePattern(
        "^" + QRegularExpression::escape("QRegularExpressionPrivate::doMatch(): "
                                         "called on an invalid QRegularExpression object")
    );

    QFETCH(QString, pattern);
    QFETCH(bool, validity);
    QRegularExpression re(pattern);
    QCOMPARE(re.isValid(), validity);
    if (!validity)
        QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QRegularExpressionMatch match = re.match("a pattern");
    QCOMPARE(match.isValid(), validity);
    consistencyCheck(match);

    if (!validity)
        QTest::ignoreMessage(QtWarningMsg, ignoreMessagePattern);
    QRegularExpressionMatchIterator iterator = re.globalMatch("a pattern");
    QCOMPARE(iterator.isValid(), validity);
}

void tst_QRegularExpression::patternOptions_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<Match>("match");

    // none of these would successfully match if the respective
    // pattern option is not set

    Match m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString::fromUtf8("AbC\xc3\xa0");
    QTest::newRow("/i") << QRegularExpression(QString::fromUtf8("abc\xc3\x80"), QRegularExpression::CaseInsensitiveOption)
                        << QString::fromUtf8("AbC\xc3\xa0")
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "abc123\n678def";
    QTest::newRow("/s") << QRegularExpression("\\Aabc.*def\\z", QRegularExpression::DotMatchesEverythingOption)
                        << "abc123\n678def"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "jumped over";
    QTest::newRow("/m") << QRegularExpression("^\\w+ \\w+$", QRegularExpression::MultilineOption)
                        << "the quick fox\njumped over\nthe lazy\ndog"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "abc 123456";
    QTest::newRow("/x") << QRegularExpression("\\w+  # a word\n"
                                              "\\ # a space\n"
                                              "\\w+ # another word",
                                              QRegularExpression::ExtendedPatternSyntaxOption)
                        << "abc 123456 def"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "the quick fox" << "the" << "quick fox";
    QTest::newRow("/U") << QRegularExpression("(.+) (.+?)", QRegularExpression::InvertedGreedinessOption)
                        << "the quick fox"
                        << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "the quick fox" << "quick";
    m.namedCaptured["named"] = "quick";
    QTest::newRow("no cap") << QRegularExpression("(\\w+) (?<named>\\w+) (\\w+)", QRegularExpression::DontCaptureOption)
                            << "the quick fox"
                            << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString::fromUtf8("abc\xc3\x80\xc3\xa0 12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98")
               << QString::fromUtf8("abc\xc3\x80\xc3\xa0")
               << QString::fromUtf8("12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98");
    QTest::newRow("unicode properties") << QRegularExpression("(\\w+) (\\d+)", QRegularExpression::UseUnicodePropertiesOption)
                            << QString::fromUtf8("abc\xc3\x80\xc3\xa0 12\xdb\xb1\xdb\xb2\xf0\x9d\x9f\x98")
                            << m;
}

void tst_QRegularExpression::patternOptions()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(Match, match);

    QRegularExpressionMatch m = regexp.match(subject);
    consistencyCheck(m);
    QVERIFY(m == match);
}

void tst_QRegularExpression::normalMatch_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<qsizetype>("offset");
    QTest::addColumn<QRegularExpression::MatchOptions>("matchOptions");
    QTest::addColumn<Match>("match");

    Match m;
    qsizetype offset = 0;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "string" << "string";
    QTest::newRow("match01") << QRegularExpression("(\\bstring\\b)")
                             << "a string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "a string" << "a" << "string";
    QTest::newRow("match02") << QRegularExpression("(\\w+) (\\w+)")
                             << "a string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "a string" << "a" << "string";
    m.namedCaptured["article"] = "a";
    m.namedCaptured["noun"] = "string";
    QTest::newRow("match03") << QRegularExpression("(?<article>\\w+) (?<noun>\\w+)")
                             << "a string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << " string" << std::nullopt << "string";
    QTest::newRow("match04") << QRegularExpression("(\\w+)? (\\w+)")
                             << " string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << " string" << QString("") << "string";
    QTest::newRow("match05") << QRegularExpression("(\\w*) (\\w+)")
                             << " string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "c123def" << "c12" << "3" << "def";
    offset = 2;
    for (qsizetype i = 0; i <= offset; ++i) {
        QTest::newRow(("match06-offset" + QByteArray::number(i)).constData())
                << QRegularExpression("(\\w*)(\\d+)(\\w*)")
                << QStringLiteral("abc123def").mid(offset - i)
                << i
                << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                << m;
    }

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("");
    offset = 9;
    for (qsizetype i = 0; i <= offset; ++i) {
        QTest::newRow(("match07-offset" + QByteArray::number(i)).constData())
                << QRegularExpression("\\w*")
                << QStringLiteral("abc123def").mid(offset - i)
                << i
                << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                << m;
    }

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("a string") << QString("a string") << QString("");
    QTest::newRow("match08") << QRegularExpression("(.*)(.*)")
                             << "a string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("a string") << QString("") << QString("a string");
    QTest::newRow("match09") << QRegularExpression("(.*?)(.*)")
                             << "a string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString();
    QTest::newRow("empty-in-null-string") << QRegularExpression("")
                             << QString()
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString("");
    QTest::newRow("empty-in-empty-string") << QRegularExpression("")
                             << QString("")
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    // non existing names for capturing groups
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "a string" << "a" << "string";
    m.namedCaptured["article"] = "a";
    m.namedCaptured["noun"] = "string";
    m.namedCaptured["nonexisting1"] = std::nullopt;
    m.namedCaptured["nonexisting2"] = std::nullopt;
    m.namedCaptured["nonexisting3"] = std::nullopt;
    QTest::newRow("match10") << QRegularExpression("(?<article>\\w+) (?<noun>\\w+)")
                             << "a string"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "" << "";
    m.namedCaptured["digits"] = ""; // empty VS null
    m.namedCaptured["nonexisting"] = std::nullopt;
    QTest::newRow("match11") << QRegularExpression("(?<digits>\\d*)")
                             << "abcde"
                             << qsizetype(0)
                             << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                             << m;

    // ***

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "bcd";
    QTest::newRow("match12")
            << QRegularExpression("\\Bbcd\\B")
            << "abcde"
            << qsizetype(1)
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;

    // ***

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString() << QString();
    QTest::newRow("capture-in-null-string")
            << QRegularExpression("(a*)")
            << QString()
            << qsizetype(0)
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString() << QString() << QString();
    QTest::newRow("capture-in-null-string-2")
            << QRegularExpression("(a*)(b*)")
            << QString()
            << qsizetype(0)
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << QString();
    QTest::newRow("no-capture-in-null-string")
            << QRegularExpression("(a+)?")
            << QString()
            << qsizetype(0)
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "bb" << QString("") << "bb";
    QTest::newRow("empty-capture-in-non-null-string")
            << QRegularExpression("(a*)(b*)")
            << QString("bbc")
            << qsizetype(0)
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "bb" << std::nullopt << "bb";
    QTest::newRow("no-capture-in-non-null-string")
            << QRegularExpression("(a+)?(b+)?")
            << QString("bbc")
            << qsizetype(0)
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch01") << QRegularExpression("\\d+")
                               << "a string"
                               << qsizetype(0)
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    offset = 1;
    for (qsizetype i = 0; i <= offset; ++i) {
        QTest::newRow(("nomatch02-offset" + QByteArray::number(i)).constData())
            << QRegularExpression("(\\w+) (\\w+)")
            << QStringLiteral("a string").mid(offset - i)
            << i
            << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
            << m;
    }

    m.clear();
    m.isValid = true;
    offset = 9;
    for (qsizetype i = 0; i <= offset; ++i) {
        QTest::newRow(("nomatch03-offset" + QByteArray::number(i)).constData())
                << QRegularExpression("\\w+")
                << QStringLiteral("abc123def").mid(offset - i)
                << i
                << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                << m;
    }

    // ***

    m.clear();
    m.isValid = true;
    QTest::newRow("anchoredmatch01") << QRegularExpression("\\d+")
                                     << "abc123def"
                                     << qsizetype(0)
                                     << QRegularExpression::MatchOptions(QRegularExpression::AnchorAtOffsetMatchOption)
                                     << m;

    // ***

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "678";
    QTest::newRow("negativeoffset01") << QRegularExpression("\\d+")
                                      << "abc123def678ghi"
                                      << qsizetype(-6)
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "678";
    QTest::newRow("negativeoffset02") << QRegularExpression("\\d+")
                                      << "abc123def678ghi"
                                      << qsizetype(-8)
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "678ghi" << "678" << "ghi";
    QTest::newRow("negativeoffset03") << QRegularExpression("(\\d+)(\\w+)")
                                      << "abc123def678ghi"
                                      << qsizetype(-8)
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("negativeoffset04") << QRegularExpression("\\d+")
                                      << "abc123def678ghi"
                                      << qsizetype(-3)
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;

    m.clear();
    m.isValid = true; m.hasMatch =  true;
    m.captured << "678";
    QTest::newRow("negativeoffset05") << QRegularExpression("^\\d+", QRegularExpression::MultilineOption)
                                      << "a\nbc123\ndef\n678gh\ni"
                                      << qsizetype(-10)
                                      << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                      << m;
}


void tst_QRegularExpression::normalMatch()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(qsizetype, offset);
    QFETCH(QRegularExpression::MatchOptions, matchOptions);
    QFETCH(Match, match);

    testMatch<QRegularExpressionMatch>(regexp,
                                       static_cast<QREMatchStringPMF>(&QRegularExpression::match),
                                       static_cast<QREMatchStringViewPMF>(&QRegularExpression::matchView),
                                       subject,
                                       offset,
                                       QRegularExpression::NormalMatch,
                                       matchOptions,
                                       match);
}

void tst_QRegularExpression::partialMatch_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<qsizetype>("offset");
    QTest::addColumn<QRegularExpression::MatchType>("matchType");
    QTest::addColumn<QRegularExpression::MatchOptions>("matchOptions");
    QTest::addColumn<Match>("match");

    Match m;
    qsizetype offset = 0;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "str";
    QTest::newRow("softmatch01") << QRegularExpression("string")
                                    << "a str"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("softmatch02") << QRegularExpression("\\bstring\\b")
                                    << "a str"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("softmatch03") << QRegularExpression("(\\bstring\\b)")
                                    << "a str"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "8 Dec 19";
    QTest::newRow("softmatch04") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 19"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "8 Dec 1985" << "8" << "Dec" << "1985";
    QTest::newRow("softmatch05") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 1985"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured << "def";
    QTest::newRow("softmatch06") << QRegularExpression("abc\\w+X|def")
                                    << "abcdef"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abcdef";
    QTest::newRow("softmatch07") << QRegularExpression("abc\\w+X|defY")
                                    << "abcdef"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferCompleteMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "def";
    offset = 1;
    for (qsizetype i = 0; i <= offset; ++i) {
        QTest::newRow(("softmatch08-offset" + QByteArray::number(i)).constData())
                << QRegularExpression("abc\\w+X|defY")
                << QStringLiteral("abcdef").mid(offset - i)
                << i
                << QRegularExpression::PartialPreferCompleteMatch
                << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                << m;
    }

    // ***

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "str";
    QTest::newRow("hardmatch01") << QRegularExpression("string")
                                    << "a str"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("hardmatch02") << QRegularExpression("\\bstring\\b")
                                    << "a str"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << " str";
    QTest::newRow("hardmatch03") << QRegularExpression("(\\bstring\\b)")
                                    << "a str"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "8 Dec 19";
    QTest::newRow("hardmatch04") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 19"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "8 Dec 1985";
    QTest::newRow("hardmatch05") << QRegularExpression("^(\\d{1,2}) (\\w{3}) (\\d{4})$")
                                    << "8 Dec 1985"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abcdef";
    QTest::newRow("hardmatch06") << QRegularExpression("abc\\w+X|def")
                                    << "abcdef"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abcdef";
    QTest::newRow("hardmatch07") << QRegularExpression("abc\\w+X|defY")
                                    << "abcdef"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "def";
    offset = 1;
    for (qsizetype i = 0; i <= offset; ++i) {
        QTest::newRow(("hardmatch08-offset" + QByteArray::number(i)).constData())
                << QRegularExpression("abc\\w+X|defY")
                << QStringLiteral("abcdef").mid(offset - i)
                << i
                << QRegularExpression::PartialPreferFirstMatch
                << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                << m;
    }

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "ab";
    QTest::newRow("hardmatch09") << QRegularExpression("abc|ab")
                                    << "ab"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abc";
    QTest::newRow("hardmatch10") << QRegularExpression("abc(def)?")
                                    << "abc"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;

    m.clear();
    m.isValid = true; m.hasPartialMatch = true;
    m.captured << "abc";
    QTest::newRow("hardmatch11") << QRegularExpression("(abc)*")
                                    << "abc"
                                    << qsizetype(0)
                                    << QRegularExpression::PartialPreferFirstMatch
                                    << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                    << m;


    // ***

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch01") << QRegularExpression("abc\\w+X|defY")
                               << "123456"
                               << qsizetype(0)
                               << QRegularExpression::PartialPreferCompleteMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch02") << QRegularExpression("abc\\w+X|defY")
                               << "123456"
                               << qsizetype(0)
                               << QRegularExpression::PartialPreferFirstMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch03") << QRegularExpression("abc\\w+X|defY")
                               << "ab123"
                               << qsizetype(0)
                               << QRegularExpression::PartialPreferCompleteMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

    m.clear();
    m.isValid = true;
    QTest::newRow("nomatch04") << QRegularExpression("abc\\w+X|defY")
                               << "ab123"
                               << qsizetype(0)
                               << QRegularExpression::PartialPreferFirstMatch
                               << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                               << m;

}

void tst_QRegularExpression::partialMatch()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(qsizetype, offset);
    QFETCH(QRegularExpression::MatchType, matchType);
    QFETCH(QRegularExpression::MatchOptions, matchOptions);
    QFETCH(Match, match);

    testMatch<QRegularExpressionMatch>(regexp,
                                       static_cast<QREMatchStringPMF>(&QRegularExpression::match),
                                       static_cast<QREMatchStringViewPMF>(&QRegularExpression::matchView),
                                       subject,
                                       offset,
                                       matchType,
                                       matchOptions,
                                       match);
}

void tst_QRegularExpression::globalMatch_data()
{
    QTest::addColumn<QRegularExpression>("regexp");
    QTest::addColumn<QString>("subject");
    QTest::addColumn<qsizetype>("offset");
    QTest::addColumn<QRegularExpression::MatchType>("matchType");
    QTest::addColumn<QRegularExpression::MatchOptions>("matchOptions");
    QTest::addColumn<QList<Match> >("matchList");

    QList<Match> matchList;
    Match m;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the";
    matchList << m;
    m.captured = CapturedList() << "quick";
    matchList << m;
    m.captured = CapturedList() << "fox";
    matchList << m;
    QTest::newRow("globalmatch01") << QRegularExpression("\\w+")
                                   << "the quick fox"
                                   << qsizetype(0)
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                   << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the" << "t" << "he";
    matchList << m;
    m.captured = CapturedList() << "quick" << "q" << "uick";
    matchList << m;
    m.captured = CapturedList() << "fox" << "f" << "ox";
    matchList << m;
    QTest::newRow("globalmatch02") << QRegularExpression("(\\w+?)(\\w+)")
                                   << "the quick fox"
                                   << qsizetype(0)
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                   << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "ACA""GTG""CGA""AAA";
    matchList << m;
    m.captured = CapturedList() << "AAA";
    matchList << m;
    m.captured = CapturedList() << "AAG""GAA""AAG""AAA";
    matchList << m;
    m.captured = CapturedList() << "AAA";
    matchList << m;
    QTest::newRow("globalmatch03") << QRegularExpression("\\G(?:\\w\\w\\w)*?AAA")
                                   << "ACA""GTG""CGA""AAA""AAA""AAG""GAA""AAG""AAA""AAA"
                                   << qsizetype(0)
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                   << matchList;

    QTest::newRow("globalmatch04") << QRegularExpression("(?:\\w\\w\\w)*?AAA")
                                   << "ACA""GTG""CGA""AAA""AAA""AAG""GAA""AAG""AAA""AAA"
                                   << qsizetype(0)
                                   << QRegularExpression::NormalMatch
                                   << QRegularExpression::MatchOptions(QRegularExpression::AnchorAtOffsetMatchOption)
                                   << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "c";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "c";
    matchList << m;
    m.captured = CapturedList() << "aabb";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures01") << QRegularExpression("a*b*|c")
                                                 << "ccaabbd"
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "quick";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "fox";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures02") << QRegularExpression(".*")
                                                 << "the\nquick\nfox"
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "quick";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "fox";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures03") << QRegularExpression(".*")
                                                 << "the\nquick\nfox\n"
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "quick";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "fox";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures04") << QRegularExpression("(*CRLF).*")
                                                 << "the\r\nquick\r\nfox"
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "quick";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "fox";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures05") << QRegularExpression("(*CRLF).*")
                                                 << "the\r\nquick\r\nfox\r\n"
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "the";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "quick";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "fox";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "jumped";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;

    QTest::newRow("globalmatch_emptycaptures06") << QRegularExpression("(*ANYCRLF).*")
                                                 << "the\r\nquick\nfox\rjumped"
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << "ABC";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "DEF";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << "GHI";
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    QTest::newRow("globalmatch_emptycaptures07") << QRegularExpression("[\\x{0000}-\\x{FFFF}]*")
                                                 << QString::fromUtf8("ABC""\xf0\x9d\x85\x9d""DEF""\xf0\x9d\x85\x9e""GHI")
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;

    matchList.clear();
    m.clear();
    m.isValid = true; m.hasMatch = true;
    m.captured = CapturedList() << QString::fromUtf8("ABC""\xc3\x80");
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    m.captured = CapturedList() << QString::fromUtf8("\xc3\x80""DEF""\xc3\x80");
    matchList << m;
    m.captured = CapturedList() << "";
    matchList << m;
    QTest::newRow("globalmatch_emptycaptures08") << QRegularExpression("[\\x{0000}-\\x{FFFF}]*")
                                                 << QString::fromUtf8("ABC""\xc3\x80""\xf0\x9d\x85\x9d""\xc3\x80""DEF""\xc3\x80")
                                                 << qsizetype(0)
                                                 << QRegularExpression::NormalMatch
                                                 << QRegularExpression::MatchOptions(QRegularExpression::NoMatchOption)
                                                 << matchList;
}

void tst_QRegularExpression::globalMatch()
{
    QFETCH(QRegularExpression, regexp);
    QFETCH(QString, subject);
    QFETCH(qsizetype, offset);
    QFETCH(QRegularExpression::MatchType, matchType);
    QFETCH(QRegularExpression::MatchOptions, matchOptions);
    QFETCH(QList<Match>, matchList);

    testMatch<QRegularExpressionMatchIterator>(regexp,
                                               static_cast<QREGlobalMatchStringPMF>(&QRegularExpression::globalMatch),
                                               static_cast<QREGlobalMatchStringViewPMF>(&QRegularExpression::globalMatchView),
                                               subject,
                                               offset,
                                               matchType,
                                               matchOptions,
                                               matchList);
}

void tst_QRegularExpression::serialize_data()
{
    provideRegularExpressions();
}

void tst_QRegularExpression::serialize()
{
    QFETCH(QString, pattern);
    QFETCH(QRegularExpression::PatternOptions, patternOptions);
    QRegularExpression outRe(pattern, patternOptions);

    QByteArray buffer;
    {
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << outRe;
    }
    QRegularExpression inRe;
    {
        QDataStream in(&buffer, QIODevice::ReadOnly);
        in >> inRe;
    }
    QCOMPARE(inRe, outRe);
}

static void verifyEquality(const QRegularExpression &re1, const QRegularExpression &re2)
{
    QVERIFY(re1 == re2);
    QVERIFY(re2 == re1);
    QCOMPARE(qHash(re1), qHash(re2));
    QVERIFY(!(re1 != re2));
    QVERIFY(!(re2 != re1));

    QRegularExpression re3(re1);

    QVERIFY(re1 == re3);
    QVERIFY(re3 == re1);
    QCOMPARE(qHash(re1), qHash(re3));
    QVERIFY(!(re1 != re3));
    QVERIFY(!(re3 != re1));

    QVERIFY(re2 == re3);
    QVERIFY(re3 == re2);
    QCOMPARE(qHash(re2), qHash(re3));
    QVERIFY(!(re2 != re3));
    QVERIFY(!(re3 != re2));

    re3 = re2;
    QVERIFY(re1 == re3);
    QVERIFY(re3 == re1);
    QCOMPARE(qHash(re1), qHash(re3));
    QVERIFY(!(re1 != re3));
    QVERIFY(!(re3 != re1));

    QVERIFY(re2 == re3);
    QVERIFY(re3 == re2);
    QCOMPARE(qHash(re2), qHash(re3));
    QVERIFY(!(re2 != re3));
    QVERIFY(!(re3 != re2));
}

void tst_QRegularExpression::operatoreq_data()
{
    provideRegularExpressions();
}

void tst_QRegularExpression::operatoreq()
{
    QFETCH(QString, pattern);
    QFETCH(QRegularExpression::PatternOptions, patternOptions);
    {
        QRegularExpression re1(pattern);
        QRegularExpression re2(pattern);

        verifyEquality(re1, re2);
    }
    {
        QRegularExpression re1(QString(), patternOptions);
        QRegularExpression re2(QString(), patternOptions);

        verifyEquality(re1, re2);
    }
    {
        QRegularExpression re1(pattern, patternOptions);
        QRegularExpression re2(pattern, patternOptions);

        verifyEquality(re1, re2);
    }
}

void tst_QRegularExpression::captureCount_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("captureCount");
    QTest::newRow("captureCount01") << "a pattern" << 0;
    QTest::newRow("captureCount02") << "a.*pattern" << 0;
    QTest::newRow("captureCount03") << "(a) pattern" << 1;
    QTest::newRow("captureCount04") << "(a).*(pattern)" << 2;
    QTest::newRow("captureCount05") << "^(?<article>\\w+) (?<noun>\\w+)$" << 2;
    QTest::newRow("captureCount06") << "^(\\w+) (?<word>\\w+) (.)$" << 3;
    QTest::newRow("captureCount07") << "(?:non capturing) (capturing) (?<n>named) (?:non (capturing))" << 3;
    QTest::newRow("captureCount08") << "(?|(a)(b)|(c)(d))" << 2;
    QTest::newRow("captureCount09") << "(?|(a)(b)|(c)(d)(?:e))" << 2;
    QTest::newRow("captureCount10") << "(?|(a)(b)|(c)(d)(e)) (f)(g)" << 5;
    QTest::newRow("captureCount11") << "(?|(a)(b)|(c)(d)(e)) (f)(?:g)" << 4;
    QTest::newRow("captureCount_invalid01") << "(.*" << -1;
    QTest::newRow("captureCount_invalid02") << "\\" << -1;
    QTest::newRow("captureCount_invalid03") << "(?<noun)" << -1;
}

void tst_QRegularExpression::captureCount()
{
    QFETCH(QString, pattern);
    QRegularExpression re(pattern);

    QTEST(re.captureCount(), "captureCount");
    if (!re.isValid())
        QCOMPARE(re.captureCount(), -1);
}

// the comma in the template breaks QFETCH...
typedef QMultiHash<QString, int> StringToIntMap;
Q_DECLARE_METATYPE(StringToIntMap)

void tst_QRegularExpression::captureNames_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<StringToIntMap>("namedCapturesIndexMap");
    StringToIntMap map;

    QTest::newRow("captureNames01") << "a pattern" << map;
    QTest::newRow("captureNames02") << "a.*pattern" << map;
    QTest::newRow("captureNames03") << "(a) pattern" << map;
    QTest::newRow("captureNames04") << "(a).*(pattern)" << map;

    map.clear();
    map.replace("named", 1);
    QTest::newRow("captureNames05") << "a.*(?<named>pattern)" << map;

    map.clear();
    map.replace("named", 2);
    QTest::newRow("captureNames06") << "(a).*(?<named>pattern)" << map;

    map.clear();
    map.replace("name1", 1);
    map.replace("name2", 2);
    QTest::newRow("captureNames07") << "(?<name1>a).*(?<name2>pattern)" << map;

    map.clear();
    map.replace("name1", 2);
    map.replace("name2", 1);
    QTest::newRow("captureNames08") << "(?<name2>a).*(?<name1>pattern)" << map;

    map.clear();
    map.replace("date", 1);
    map.replace("month", 2);
    map.replace("year", 3);
    QTest::newRow("captureNames09") << "^(?<date>\\d\\d)/(?<month>\\d\\d)/(?<year>\\d\\d\\d\\d)$" << map;

    map.clear();
    map.replace("date", 2);
    map.replace("month", 1);
    map.replace("year", 3);
    QTest::newRow("captureNames10") << "^(?<month>\\d\\d)/(?<date>\\d\\d)/(?<year>\\d\\d\\d\\d)$" << map;

    map.clear();
    map.replace("noun", 2);
    QTest::newRow("captureNames11") << "(a)(?|(?<noun>b)|(?<noun>c))(d)" << map;

    map.clear();
    QTest::newRow("captureNames_invalid01") << "(.*" << map;
    QTest::newRow("captureNames_invalid02") << "\\" << map;
    QTest::newRow("captureNames_invalid03") << "(?<noun)" << map;
    QTest::newRow("captureNames_invalid04") << "(?|(?<noun1>a)|(?<noun2>b))" << map;
}

void tst_QRegularExpression::captureNames()
{
    QFETCH(QString, pattern);
    QFETCH(StringToIntMap, namedCapturesIndexMap);

    QRegularExpression re(pattern);

    QStringList namedCaptureGroups = re.namedCaptureGroups();
    int namedCaptureGroupsCount = namedCaptureGroups.size();

    QCOMPARE(namedCaptureGroupsCount, re.captureCount() + 1);

    for (int i = 0; i < namedCaptureGroupsCount; ++i) {
        const QString &name = namedCaptureGroups.at(i);

        if (name.isEmpty()) {
            QVERIFY(!namedCapturesIndexMap.contains(name));
        } else {
            QVERIFY(namedCapturesIndexMap.contains(name));
            QCOMPARE(i, namedCapturesIndexMap.value(name));
        }
    }

}

void tst_QRegularExpression::captureNamesNul()
{
    QRegularExpression re("a(\\d+)b(?<name>\\d+)c(?<anotherName>\\d+)d(\\d+)e$");
    QVERIFY(re.isValid());

    QCOMPARE(re.captureCount(), 4);

    QStringList namedCaptureGroups = re.namedCaptureGroups();
    QCOMPARE(namedCaptureGroups[0], QString());
    QCOMPARE(namedCaptureGroups[1], QString());
    QCOMPARE(namedCaptureGroups[2], "name");
    QCOMPARE(namedCaptureGroups[3], "anotherName");
    QCOMPARE(namedCaptureGroups[4], QString());

    QRegularExpressionMatch m = re.match("a12b456c789d0e");
    QVERIFY(m.hasMatch());

    QString captureName("name");
    QCOMPARE(m.captured(captureName), "456");
    QCOMPARE(m.captured(QStringView(captureName)), "456");
    QCOMPARE(m.captured(qToStringViewIgnoringNull(captureName)), "456");
    QCOMPARE(m.captured(u"name"), "456");

    captureName = "anotherName";
    QCOMPARE(m.captured(captureName), "789");
    QCOMPARE(m.captured(QStringView(captureName)), "789");
    QCOMPARE(m.captured(qToStringViewIgnoringNull(captureName)), "789");
    QCOMPARE(m.captured(u"anotherName"), "789");
}

void tst_QRegularExpression::pcreJitStackUsage_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("subject");
    // these patterns cause enough backtrack (or even infinite recursion)
    // in the regexp engine, so that JIT requests more memory.
    QTest::newRow("jitstack01") << "(?(R)a*(?1)|((?R))b)" << "aaaabcde";
    QTest::newRow("jitstack02") << "(?(R)a*(?1)|((?R))b)" << "aaaaaaabcde";
}

void tst_QRegularExpression::pcreJitStackUsage()
{
    QFETCH(QString, pattern);
    QFETCH(QString, subject);

    QRegularExpression re(pattern);

    QVERIFY(re.isValid());
    QRegularExpressionMatch match = re.match(subject);
    consistencyCheck(match);
    QRegularExpressionMatchIterator iterator = re.globalMatch(subject);
    consistencyCheck(iterator);
    while (iterator.hasNext()) {
        match = iterator.next();
        consistencyCheck(match);
    }
}

void tst_QRegularExpression::regularExpressionMatch_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("subject");

    QTest::newRow("validity01") << "(?<digits>\\d+)" << "1234 abcd";
    QTest::newRow("validity02") << "(?<digits>\\d+) (?<alpha>\\w+)" << "1234 abcd";
}

void tst_QRegularExpression::regularExpressionMatch()
{
    QFETCH(QString, pattern);
    QFETCH(QString, subject);

    QRegularExpression re(pattern);

    QVERIFY(re.isValid());
    QRegularExpressionMatch match = re.match(subject);
    consistencyCheck(match);
    QCOMPARE(match.captured("non-existing").isNull(), true);
    QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatch::captured: empty capturing group name passed");
    QCOMPARE(match.captured("").isNull(), true);
    QTest::ignoreMessage(QtWarningMsg, "QRegularExpressionMatch::captured: empty capturing group name passed");
    QCOMPARE(match.captured(QString()).isNull(), true);
}

void tst_QRegularExpression::JOptionUsage_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("JOptionUsed");

    QTest::newRow("joption-notused-01") << "a.*b" << true << false;
    QTest::newRow("joption-notused-02") << "^a(b)(c)$" << true << false;
    QTest::newRow("joption-notused-03") << "a(b)(?<c>d)|e" << true << false;
    QTest::newRow("joption-notused-04") << "(?<a>.)(?<a>.)" << false << false;

    QTest::newRow("joption-used-01") << "(?J)a.*b" << true << true;
    QTest::newRow("joption-used-02") << "(?-J)a.*b" << true << true;
    QTest::newRow("joption-used-03") << "(?J)(?<a>.)(?<a>.)" << true << true;
    QTest::newRow("joption-used-04") << "(?-J)(?<a>.)(?<a>.)" << false << true;

}

void tst_QRegularExpression::JOptionUsage()
{
    QFETCH(QString, pattern);
    QFETCH(bool, isValid);
    QFETCH(bool, JOptionUsed);

    const QString warningMessage = QStringLiteral("QRegularExpressionPrivate::getPatternInfo(): the pattern '%1'\n    is using the (?J) option; duplicate capturing group names are not supported by Qt");

    QRegularExpression re(pattern);
    if (isValid && JOptionUsed)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warningMessage.arg(pattern)));
    QCOMPARE(re.isValid(), isValid);
}

void tst_QRegularExpression::QStringAndQStringViewEquivalence()
{
    const QString subject = QStringLiteral("Mississippi");
    {
        const QRegularExpression re("\\Biss\\B");
        QVERIFY(re.isValid());
        {
            const QRegularExpressionMatch match = re.match(subject);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 1);
            QCOMPARE(match.capturedEnd(), 4);
        }
        {
            const QRegularExpressionMatch match = re.matchView(QStringView(subject));
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 1);
            QCOMPARE(match.capturedEnd(), 4);
        }
        {
            const QRegularExpressionMatch match = re.match(subject, 1);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 1);
            QCOMPARE(match.capturedEnd(), 4);
        }
        {
            const QRegularExpressionMatch match = re.matchView(QStringView(subject), 1);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 1);
            QCOMPARE(match.capturedEnd(), 4);
        }
        {
            const QRegularExpressionMatch match = re.match(subject.mid(1));
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);
        }
        {
            const QRegularExpressionMatch match = re.matchView(QStringView(subject).mid(1));
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);
        }
        {
            const QRegularExpressionMatch match = re.match(subject.mid(1), 1);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);
        }
        {
            const QRegularExpressionMatch match = re.matchView(QStringView(subject).mid(1), 1);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);
        }
        {
            const QRegularExpressionMatch match = re.match(subject, 4);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 4);
            QCOMPARE(match.capturedEnd(), 7);
        }
        {
            const QRegularExpressionMatch match = re.matchView(QStringView(subject), 4);
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 4);
            QCOMPARE(match.capturedEnd(), 7);
        }
        {
            const QRegularExpressionMatch match = re.match(subject.mid(4));
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(!match.hasMatch());
        }
        {
            const QRegularExpressionMatch match = re.matchView(QStringView(subject).mid(4));
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(!match.hasMatch());
        }

        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match1 = i.next();
            consistencyCheck(match1);
            QVERIFY(match1.isValid());
            QVERIFY(match1.hasMatch());
            QCOMPARE(match1.captured(), QStringLiteral("iss"));
            QCOMPARE(match1.capturedStart(), 1);
            QCOMPARE(match1.capturedEnd(), 4);

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match2 = i.next();
            consistencyCheck(match2);
            QVERIFY(match2.isValid());
            QVERIFY(match2.hasMatch());
            QCOMPARE(match2.captured(), QStringLiteral("iss"));
            QCOMPARE(match2.capturedStart(), 4);
            QCOMPARE(match2.capturedEnd(), 7);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject));
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match1 = i.next();
            consistencyCheck(match1);
            QVERIFY(match1.isValid());
            QVERIFY(match1.hasMatch());
            QCOMPARE(match1.captured(), QStringLiteral("iss"));
            QCOMPARE(match1.capturedStart(), 1);
            QCOMPARE(match1.capturedEnd(), 4);

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match2 = i.next();
            consistencyCheck(match2);
            QVERIFY(match2.isValid());
            QVERIFY(match2.hasMatch());
            QCOMPARE(match2.captured(), QStringLiteral("iss"));
            QCOMPARE(match2.capturedStart(), 4);
            QCOMPARE(match2.capturedEnd(), 7);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject, 1);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match1 = i.next();
            consistencyCheck(match1);
            QVERIFY(match1.isValid());
            QVERIFY(match1.hasMatch());
            QCOMPARE(match1.captured(), QStringLiteral("iss"));
            QCOMPARE(match1.capturedStart(), 1);
            QCOMPARE(match1.capturedEnd(), 4);

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match2 = i.next();
            consistencyCheck(match2);
            QVERIFY(match2.isValid());
            QVERIFY(match2.hasMatch());
            QCOMPARE(match2.captured(), QStringLiteral("iss"));
            QCOMPARE(match2.capturedStart(), 4);
            QCOMPARE(match2.capturedEnd(), 7);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject), 1);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match1 = i.next();
            consistencyCheck(match1);
            QVERIFY(match1.isValid());
            QVERIFY(match1.hasMatch());
            QCOMPARE(match1.captured(), QStringLiteral("iss"));
            QCOMPARE(match1.capturedStart(), 1);
            QCOMPARE(match1.capturedEnd(), 4);

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match2 = i.next();
            consistencyCheck(match2);
            QVERIFY(match2.isValid());
            QVERIFY(match2.hasMatch());
            QCOMPARE(match2.captured(), QStringLiteral("iss"));
            QCOMPARE(match2.capturedStart(), 4);
            QCOMPARE(match2.capturedEnd(), 7);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject.mid(1));
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject).mid(1));
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject.mid(1), 1);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject).mid(1), 1);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject.mid(1), 1);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject).mid(1), 1);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 3);
            QCOMPARE(match.capturedEnd(), 6);

            QVERIFY(!i.hasNext());
        }

        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject, 4);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 4);
            QCOMPARE(match.capturedEnd(), 7);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject), 4);
            QVERIFY(i.isValid());

            consistencyCheck(i);
            QVERIFY(i.hasNext());
            const QRegularExpressionMatch match = i.next();
            consistencyCheck(match);
            QVERIFY(match.isValid());
            QVERIFY(match.hasMatch());
            QCOMPARE(match.captured(), QStringLiteral("iss"));
            QCOMPARE(match.capturedStart(), 4);
            QCOMPARE(match.capturedEnd(), 7);

            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatch(subject.mid(4));
            consistencyCheck(i);
            QVERIFY(i.isValid());
            QVERIFY(!i.hasNext());
        }
        {
            QRegularExpressionMatchIterator i = re.globalMatchView(QStringView(subject).mid(4));
            consistencyCheck(i);
            QVERIFY(i.isValid());
            QVERIFY(!i.hasNext());
        }
    }
}

class MatcherThread : public QThread
{
public:
    explicit MatcherThread(const QRegularExpression &re, const QString &subject, QObject *parent = nullptr)
        : QThread(parent),
          m_re(re),
          m_subject(subject)
    {
    }

private:
    static const int MATCH_ITERATIONS = 50;

    void run() override
    {
        yieldCurrentThread();
        for (int i = 0; i < MATCH_ITERATIONS; ++i)
            (void)m_re.match(m_subject);
    }

    const QRegularExpression &m_re;
    const QString &m_subject;
};

void tst_QRegularExpression::threadSafety_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("subject");

    int i = 0;
    QTest::addRow("pattern%d", ++i) << "ab.*cd" << "abcd";
    QTest::addRow("pattern%d", ++i) << "ab.*cd" << "abd";
    QTest::addRow("pattern%d", ++i) << "ab.*cd" << "abbbbcccd";
    QTest::addRow("pattern%d", ++i) << "ab.*cd" << "abababcd";
    QTest::addRow("pattern%d", ++i) << "ab.*cd" << "abcabcd";
    QTest::addRow("pattern%d", ++i) << "ab.*cd" << "abccccccababd";

    {
        QString subject(512*1024, QLatin1Char('x'));
        QTest::addRow("pattern%d", ++i) << "ab.*cd" << subject;
    }

    {
        QString subject = "ab";
        subject.append(QString(512*1024, QLatin1Char('x')));
        subject.append("c");
        QTest::addRow("pattern%d", ++i) << "ab.*cd" << subject;
    }

    {
        QString subject = "ab";
        subject.append(QString(512*1024, QLatin1Char('x')));
        subject.append("cd");
        QTest::addRow("pattern%d", ++i) << "ab.*cd" << subject;
    }

    QTest::addRow("pattern%d", ++i) << "(?(R)a*(?1)|((?R))b)" << "aaaabcde";
    QTest::addRow("pattern%d", ++i) << "(?(R)a*(?1)|((?R))b)" << "aaaaaaabcde";
}

void tst_QRegularExpression::threadSafety()
{
    QFETCH(QString, pattern);
    QFETCH(QString, subject);

    QElapsedTimer time;
    time.start();
    static const int THREAD_SAFETY_ITERATIONS = 50;
    const int threadCount = qMax(QThread::idealThreadCount(), 4);

    for (int threadSafetyIteration = 0; threadSafetyIteration < THREAD_SAFETY_ITERATIONS && time.elapsed() < 2000; ++threadSafetyIteration) {
        QRegularExpression re(pattern);

        QList<MatcherThread *> threads;
        for (int i = 0; i < threadCount; ++i) {
            MatcherThread *thread = new MatcherThread(re, subject);
            thread->start();
            threads.push_back(thread);
        }

        for (int i = 0; i < threadCount; ++i)
            threads[i]->wait();

        qDeleteAll(threads);
    }
}

void tst_QRegularExpression::returnsViewsIntoOriginalString()
{
    // https://bugreports.qt.io/browse/QTBUG-98653

    auto to_void = [](const auto *p) -> const void* { return p; };

    // GIVEN
    //  a QString with dynamically-allocated data:
    const QString string = QLatin1String("A\nA\nB\nB\n\nC\nC"); // NOT QStringLiteral!
    const auto stringDataAddress = to_void(string.data());

    //  and a view over said QString:
    QStringView view(string);
    const auto viewDataAddress = to_void(view.data());
    QCOMPARE(stringDataAddress, viewDataAddress);

    // WHEN
    //  we call view.split() with a temporary QRegularExpression object
    const auto split = view.split(QRegularExpression( "(\r\n|\n|\r)" ), Qt::KeepEmptyParts);

    // THEN
    //  the returned views should point into the underlying string:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QEXPECT_FAIL("", "QTBUG-98653", Continue);
#endif
    QCOMPARE(to_void(split.front().data()), stringDataAddress);
}

void tst_QRegularExpression::wildcard_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("string");
    QTest::addColumn<bool>("matchesPathGlob");
    QTest::addColumn<bool>("matchesNonPathGlob");
    QTest::addColumn<bool>("anchored");

    auto addRow = [](const char *pattern, const char *string, bool matchesPathGlob, bool matchesNonPathGlob, bool anchored = true) {
        QTest::addRow("%s@%s", pattern, string) << pattern << string << matchesPathGlob << matchesNonPathGlob << anchored;
    };

    addRow("*.html", "test.html", true, true);
    addRow("*.html", "test.htm", false, false);
    addRow("*bar*", "foobarbaz", true, true);
    addRow("*", "Qt Rocks!", true, true);
    addRow("*.h", "test.cpp", false, false);
    addRow("*.???l", "test.html", true, true);
    addRow("*?", "test.html", true, true);
    addRow("*?ml", "test.html", true, true);
    addRow("*[*]", "test.html", false, false);
    addRow("*[?]","test.html", false, false);
    addRow("*[?]ml","test.h?ml", true, true);
    addRow("*[[]ml","test.h[ml", true, true);
    addRow("*[]]ml","test.h]ml", true, true);
    addRow("*.h[a-z]ml", "test.html", true, true);
    addRow("*.h[A-Z]ml", "test.html", false, false);
    addRow("*.h[A-Z]ml", "test.hTml", true, true);
    addRow("*.h[!A-Z]ml", "test.hTml", false, false);
    addRow("*.h[!A-Z]ml", "test.html", true, true);
    addRow("*.h[!T]ml", "test.hTml", false, false);
    addRow("*.h[!T]ml", "test.html", true, true);
    addRow("*.h[!T]m[!L]", "test.htmL", false, false);
    addRow("*.h[!T]m[!L]", "test.html", true, true);
    addRow("*.h[][!]ml", "test.h]ml", true, true);
    addRow("*.h[][!]ml", "test.h[ml", true, true);
    addRow("*.h[][!]ml", "test.h!ml", true, true);

    addRow("foo/*/bar", "foo/baz/bar", true, true);
    addRow("foo/*/bar", "foo/fie/baz/bar", false, true);
    addRow("foo?bar", "foo/bar", false, true);
    addRow("foo/(*)/bar", "foo/baz/bar", false, false);
    addRow("foo/(*)/bar", "foo/(baz)/bar", true, true);
    addRow("foo/?/bar", "foo/Q/bar", true, true);
    addRow("foo/?/bar", "foo/Qt/bar", false, false);
    addRow("foo/(?)/bar", "foo/Q/bar", false, false);
    addRow("foo/(?)/bar", "foo/(Q)/bar", true, true);

    addRow("foo*bar", "foo/fie/baz/bar", false, true);
    addRow("foo*bar", "foo bar", true, true);
    addRow("foo*bar", "foo\tbar", true, true);
    addRow("foo*bar", "foo\nbar", true, true);
    addRow("foo*bar", "foo\r\nbar", true, true);

    // different anchor modes
    addRow("foo", "afoob", false, false, true);
    addRow("foo", "afoob", true, true, false);

    addRow("fie*bar", "foo/fie/baz/bar", false, false, true);
    addRow("fie*bar", "foo/fie/baz/bar", false, true, false);

#ifdef Q_OS_WIN
    addRow("foo\\*\\bar", "foo\\baz\\bar", true, true);
    addRow("foo\\*\\bar", "foo/baz/bar", true, false);
    addRow("foo\\*\\bar", "foo/baz\\bar", true, false);
    addRow("foo\\*\\bar", "foo\\fie\\baz\\bar", false, true);
    addRow("foo\\*\\bar", "foo/fie/baz/bar", false, false);
    addRow("foo/*/bar", "foo\\baz\\bar", true, false);
    addRow("foo/*/bar", "foo/baz/bar", true, true);
    addRow("foo/*/bar", "foo\\fie\\baz\\bar", false, false);
    addRow("foo/*/bar", "foo/fie/baz/bar", false, true);
    addRow("foo\\(*)\\bar", "foo\\baz\\bar", false, false);
    addRow("foo\\(*)\\bar", "foo\\(baz)\\bar", true, true);
    addRow("foo\\?\\bar", "foo\\Q\\bar", true, true);
    addRow("foo\\?\\bar", "foo\\Qt\\bar", false, false);
    addRow("foo\\(?)\\bar", "foo\\Q\\bar", false, false);
    addRow("foo\\(?)\\bar", "foo\\(Q)\\bar", true, true);
#endif
}

void tst_QRegularExpression::wildcard()
{
    QFETCH(QString, pattern);
    QFETCH(QString, string);
    QFETCH(bool, matchesPathGlob);
    QFETCH(bool, matchesNonPathGlob);
    QFETCH(bool, anchored);

    QRegularExpression::WildcardConversionOptions options = {};
    if (!anchored)
        options |= QRegularExpression::UnanchoredWildcardConversion;

    {
        QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pattern, options));
        QCOMPARE(string.contains(re), matchesPathGlob);
    }
    {
        QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pattern, options | QRegularExpression::NonPathWildcardConversion));
        QCOMPARE(string.contains(re), matchesNonPathGlob);
    }
}

void tst_QRegularExpression::testInvalidWildcard_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("valid []") << "[abc]" << true;
    QTest::newRow("valid ending ]") << "abc]" << true;
    QTest::newRow("invalid [") << "[abc" << false;
    QTest::newRow("ending [") << "abc[" << false;
    QTest::newRow("ending [^") << "abc[^" << false;
    QTest::newRow("ending [\\") << "abc[\\" << false;
    QTest::newRow("ending []") << "abc[]" << false;
    QTest::newRow("ending [[") << "abc[[" << false;
}

void tst_QRegularExpression::testInvalidWildcard()
{
    QFETCH(QString, pattern);
    QFETCH(bool, isValid);

    QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pattern));
    QCOMPARE(re.isValid(), isValid);
}

QTEST_APPLESS_MAIN(tst_QRegularExpression)

#include "tst_qregularexpression.moc"
