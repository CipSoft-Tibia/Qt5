// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtAxServer/QAxFactory>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QToolBar>

#include <QtGui/QClipboard>
#include <QtGui/QWindow>

#include <QtCore/QDebug>
#include <QtCore/QLibraryInfo>

/* A sample control for testing embedding Active X controls with
 * functionality to dump window parameters. See testcontrol.html
 * on how to use it with Internet Explorer. */

QT_USE_NAMESPACE

class QTestControl : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("ClassID", "{DF16845C-92CD-4AAB-A982-EB9840E7466A}")
    Q_CLASSINFO("InterfaceID", "{616F620B-91C5-4410-A74E-6B81C76FFFE1}")
    Q_CLASSINFO("EventsID", "{E1816BBA-BF5D-4A31-9855-D6BA43205510}")

public:
    explicit QTestControl(QWidget *parent = nullptr);

public slots:
    void appendText(const QString &t) { m_logWindow->appendPlainText(t); }
    void testMapToGlobal();
    void dumpWindowInfo();
    void copyAll();

private:
    QPlainTextEdit *m_logWindow;
};

QTestControl::QTestControl(QWidget *parent)
    : QMainWindow(parent)
    , m_logWindow(new QPlainTextEdit)
{
    QMenuBar *menubar = menuBar();
    QToolBar *toolbar = new QToolBar;
    addToolBar(Qt::TopToolBarArea, toolbar);
    QMenu *testMenu = menubar->addMenu(QLatin1String("&Test"));
    QAction *a = testMenu->addAction(QLatin1String("&Dump Window"), this, &QTestControl::dumpWindowInfo);
    toolbar->addAction(a);
    a = testMenu->addAction(QLatin1String("&Map to Global"), this, &QTestControl::testMapToGlobal);
    toolbar->addAction(a);

    QMenu *editMenu = menubar->addMenu(QLatin1String("&Edit"));
    a = editMenu->addAction(QLatin1String("Copy"), this, &QTestControl::dumpWindowInfo);
    a->setShortcut(QKeySequence::Copy);
    toolbar->addAction(a);

    m_logWindow->setReadOnly(true);
    setCentralWidget(m_logWindow);
    appendText(QLatin1String(QLibraryInfo::build()));
}

void QTestControl::testMapToGlobal()
{
    QString text;
    QPoint global = mapToGlobal(QPoint(0, 0));
    QDebug(&text) << "\nmapToGlobal:" << global << ", back:" << mapFromGlobal(global) << '\n';
    appendText(text);
}

void QTestControl::dumpWindowInfo()
{
    QString text;
    QDebug debug(&text);
    debug.setVerbosity(3);
    debug << "\nQWidget: " << this <<"\n\nQWindow: " << windowHandle() << '\n';
    appendText(text);
}

void QTestControl::copyAll()
{
    QGuiApplication::clipboard()->setText(m_logWindow->toPlainText());
}

#include "main.moc"

QAXFACTORY_BEGIN("{EC08F8FC-2754-47AB-8EFE-56A54057F34F}", "{A095BA0C-224F-4933-A458-2DD7F6B85D90}")
    QAXCLASS(QTestControl)
QAXFACTORY_END()
