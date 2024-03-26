// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "util.h"

#include <QtQml/QQmlComponent>
#include <QtQml/QQmlError>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

QQmlDataTest *QQmlDataTest::m_instance = 0;

QQmlDataTest::QQmlDataTest() :
#ifdef QT_TESTCASE_BUILDDIR
    m_dataDirectory(QTest::qFindTestData("data", QT_QMLTEST_DATADIR, 0, QT_TESTCASE_BUILDDIR)),
#else
    m_dataDirectory(QTest::qFindTestData("data", QT_QMLTEST_DATADIR, 0)),
#endif

    m_dataDirectoryUrl(QUrl::fromLocalFile(m_dataDirectory + QLatin1Char('/')))
{
    m_instance = this;
}

QQmlDataTest::~QQmlDataTest()
{
    m_instance = 0;
}

void QQmlDataTest::initTestCase()
{
    QVERIFY2(!m_dataDirectory.isEmpty(), "'data' directory not found");
    m_directory = QFileInfo(m_dataDirectory).absolutePath();
    QVERIFY2(QDir::setCurrent(m_directory), qPrintable(QLatin1String("Could not chdir to ") + m_directory));
}

QString QQmlDataTest::testFile(const QString &fileName) const
{
    if (m_directory.isEmpty())
        qFatal("QQmlDataTest::initTestCase() not called.");
    QString result = m_dataDirectory;
    result += QLatin1Char('/');
    result += fileName;
    return result;
}

QByteArray QQmlDataTest::msgComponentError(const QQmlComponent &c,
                                                   const QQmlEngine *engine /* = 0 */)
{
    QString result;
    const QList<QQmlError> errors = c.errors();
    QTextStream str(&result);
    str << "Component '" << c.url().toString() << "' has " << errors.size()
        << " errors: '";
    for (int i = 0; i < errors.size(); ++i) {
        if (i)
            str << ", '";
        str << errors.at(i).toString() << '\'';

    }
    if (!engine)
        if (QQmlContext *context = c.creationContext())
            engine = context->engine();
    if (engine) {
        str << " Import paths: (" << engine->importPathList().join(QStringLiteral(", "))
            << ") Plugin paths: (" << engine->pluginPathList().join(QStringLiteral(", "))
            << ')';
    }
    return result.toLocal8Bit();
}

Q_GLOBAL_STATIC(QMutex, qQmlTestMessageHandlerMutex)

QQmlTestMessageHandler *QQmlTestMessageHandler::m_instance = 0;

void QQmlTestMessageHandler::messageHandler(QtMsgType, const QMessageLogContext &, const QString &message)
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    if (QQmlTestMessageHandler::m_instance)
        QQmlTestMessageHandler::m_instance->m_messages.push_back(message);
}

QQmlTestMessageHandler::QQmlTestMessageHandler()
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    Q_ASSERT(!QQmlTestMessageHandler::m_instance);
    QQmlTestMessageHandler::m_instance = this;
    m_oldHandler = qInstallMessageHandler(messageHandler);
}

QQmlTestMessageHandler::~QQmlTestMessageHandler()
{
    QMutexLocker locker(qQmlTestMessageHandlerMutex());
    Q_ASSERT(QQmlTestMessageHandler::m_instance);
    qInstallMessageHandler(m_oldHandler);
    QQmlTestMessageHandler::m_instance = 0;
}

#include "moc_util.cpp"
