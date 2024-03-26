// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOGGINGREGISTRY_P_H
#define QLOGGINGREGISTRY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>

class tst_QLoggingRegistry;

QT_BEGIN_NAMESPACE

#define Q_LOGGING_CATEGORY_WITH_ENV_OVERRIDE(name, env, categoryName) \
    const QLoggingCategory &name() \
    { \
        static constexpr char cname[] = categoryName;                               \
        static_assert(cname[0] == 'q' && cname[1] == 't' && cname[2] == '.'         \
                      && cname[4] != '\0', "Category name must start with 'qt.'");  \
        static const QLoggingCategoryWithEnvironmentOverride category(cname, env);  \
        return category;                                                            \
    }

class Q_AUTOTEST_EXPORT QLoggingRule
{
public:
    QLoggingRule();
    QLoggingRule(QStringView pattern, bool enabled);
    int pass(QLatin1StringView categoryName, QtMsgType type) const;

    enum PatternFlag {
        FullText = 0x1,
        LeftFilter = 0x2,
        RightFilter = 0x4,
        MidFilter = LeftFilter | RightFilter
    };
    Q_DECLARE_FLAGS(PatternFlags, PatternFlag)

    QString category;
    int messageType;
    PatternFlags flags;
    bool enabled;

private:
    void parse(QStringView pattern);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLoggingRule::PatternFlags)
Q_DECLARE_TYPEINFO(QLoggingRule, Q_RELOCATABLE_TYPE);

class Q_AUTOTEST_EXPORT QLoggingSettingsParser
{
public:
    void setImplicitRulesSection(bool inRulesSection) { m_inRulesSection = inRulesSection; }

    void setContent(QStringView content);
    void setContent(QTextStream &stream);

    QList<QLoggingRule> rules() const { return _rules; }

private:
    void parseNextLine(QStringView line);

private:
    bool m_inRulesSection = false;
    QList<QLoggingRule> _rules;
};

class Q_AUTOTEST_EXPORT QLoggingRegistry
{
    Q_DISABLE_COPY_MOVE(QLoggingRegistry)
public:
    QLoggingRegistry();

    void initializeRules();

    void registerCategory(QLoggingCategory *category, QtMsgType enableForLevel);
    void unregisterCategory(QLoggingCategory *category);

#ifndef QT_BUILD_INTERNAL
    Q_CORE_EXPORT   // always export from QtCore
#endif
    void registerEnvironmentOverrideForCategory(const char *categoryName, const char *environment);

    void setApiRules(const QString &content);

    QLoggingCategory::CategoryFilter
    installFilter(QLoggingCategory::CategoryFilter filter);

    static QLoggingRegistry *instance();

private:
    void updateRules();

    static void defaultCategoryFilter(QLoggingCategory *category);

    enum RuleSet {
        // sorted by order in which defaultCategoryFilter considers them:
        QtConfigRules,
        ConfigRules,
        ApiRules,
        EnvironmentRules,

        NumRuleSets
    };

    QMutex registryMutex;

    // protected by mutex:
    QList<QLoggingRule> ruleSets[NumRuleSets];
    QHash<QLoggingCategory *, QtMsgType> categories;
    QLoggingCategory::CategoryFilter categoryFilter;
    QMap<QByteArrayView, const char *> qtCategoryEnvironmentOverrides;

    friend class ::tst_QLoggingRegistry;
};

class QLoggingCategoryWithEnvironmentOverride : public QLoggingCategory
{
public:
    QLoggingCategoryWithEnvironmentOverride(const char *category, const char *env)
        : QLoggingCategory(registerOverride(category, env), QtInfoMsg)
    {}

private:
    static const char *registerOverride(const char *categoryName, const char *environment)
    {
        QLoggingRegistry *c = QLoggingRegistry::instance();
        if (c)
            c->registerEnvironmentOverrideForCategory(categoryName, environment);
        return categoryName;
    }
};

QT_END_NAMESPACE

#endif // QLOGGINGREGISTRY_P_H
