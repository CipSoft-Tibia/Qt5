// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTESTUTILS_H
#define QQMLTESTUTILS_H

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtTest/QTest>

/* Base class for tests with data that are located in a "data" subfolder. */

class QQmlDataTest : public QObject
{
    Q_OBJECT
public:
    QQmlDataTest();
    virtual ~QQmlDataTest();

    QString testFile(const QString &fileName) const;
    inline QString testFile(const char *fileName) const
        { return testFile(QLatin1String(fileName)); }
    inline QUrl testFileUrl(const QString &fileName) const
        {
            const QString fn = testFile(fileName);
            return fn.startsWith(QLatin1Char(':'))
                ? QUrl(QLatin1String("qrc") + fn)
                : QUrl::fromLocalFile(fn);
        }
    inline QUrl testFileUrl(const char *fileName) const
        { return testFileUrl(QLatin1String(fileName)); }

    inline QString dataDirectory() const { return m_dataDirectory; }
    inline QUrl dataDirectoryUrl() const { return m_dataDirectoryUrl; }
    inline QString directory() const  { return m_directory; }

    static inline QQmlDataTest *instance() { return m_instance; }

public slots:
    virtual void initTestCase();

private:
    static QQmlDataTest *m_instance;

    const QString m_dataDirectory;
    const QUrl m_dataDirectoryUrl;
    QString m_directory;
};

class QQmlTestMessageHandler
{
    Q_DISABLE_COPY(QQmlTestMessageHandler)
public:
    QQmlTestMessageHandler();
    ~QQmlTestMessageHandler();

    const QStringList &messages() const { return m_messages; }
    const QString messageString() const { return m_messages.join(QLatin1Char('\n')); }

    void clear() { m_messages.clear(); }

    void setIncludeCategoriesEnabled(bool enabled) { m_includeCategories = enabled; }

private:
    static void messageHandler(QtMsgType, const QMessageLogContext &context, const QString &message);

    static QQmlTestMessageHandler *m_instance;
    QStringList m_messages;
    QtMessageHandler m_oldHandler;
    bool m_includeCategories;
};

#endif // QQMLTESTUTILS_H
