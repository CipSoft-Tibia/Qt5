// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ABSTRACTVIEWER_H
#define ABSTRACTVIEWER_H

#include <QObject>
#include <QtCompilerDetection>

#if defined(QT_SHARED) || !defined(QT_STATIC)
#  if defined(BUILD_ABSTRACTVIEWER_LIB)
#    define ABSTRACTVIEWER_EXPORT Q_DECL_EXPORT
#  else
#    define ABSTRACTVIEWER_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define ABSTRACTVIEWER_EXPORT
#endif

QT_BEGIN_NAMESPACE
class QAction;
class QFile;
class QMainWindow;
class QMenu;
class QMenuBar;
class QPrinter;
class QToolBar;
class QTabWidget;
class QScrollArea;
class QStatusBar;
QT_END_NAMESPACE

class ABSTRACTVIEWER_EXPORT AbstractViewer : public QObject
{
    Q_OBJECT

protected:
    AbstractViewer();

public:
    virtual ~AbstractViewer();
    virtual void init(QFile *file, QWidget *widget, QMainWindow *mainWindow);
    void initViewer(QAction *back, QAction *forward, QAction *help, QTabWidget *tabs);
    virtual bool isModified() const;
    virtual bool saveDocument();
    virtual bool saveDocumentAs();
    virtual QString viewerName() const = 0;
    virtual bool supportsOverview() const;
    virtual QByteArray saveState() const = 0;
    virtual bool restoreState(QByteArray &) = 0;
    virtual bool hasContent() const;
    virtual QStringList supportedMimeTypes() const = 0;
    virtual QStringList supportedExtensions() const { return {}; }
    virtual bool isDefaultViewer() const;
    virtual void cleanup();
    bool isEmpty() const;
    bool isPrintingEnabled() const;
    AbstractViewer *viewer();
    const AbstractViewer *viewer() const;

    QList<QAction *> actions() const;
    QWidget *widget() const;
    QList<QMenu *> menus() const;

#ifdef QT_DOCUMENTVIEWER_PRINTSUPPORT
protected:
    virtual void printDocument(QPrinter *) const {};
#endif

signals:
    void uiInitialized();
    void printingEnabledChanged(bool enabled);
    void showMessage(const QString &message, int timeout = 8000);
    void documentLoaded(const QString &fileName);

public slots:
    void print();

protected:

    struct UiAssets {
        QMainWindow *mainWindow = nullptr;
        QAction *back = nullptr;
        QAction *forward = nullptr;
        QAction *help = nullptr;
        QTabWidget *tabs = nullptr;
    } m_uiAssets;

    void statusMessage(const QString &message, const QString &type = QString(), int timeout = 8000);
    QToolBar *addToolBar(const QString &);
    QMenu *addMenu(const QString &);
    QMenu *fileMenu();
    QMainWindow *mainWindow() const;
    QStatusBar *statusBar() const;
    QMenuBar *menuBar() const;

    std::unique_ptr<QFile> m_file;
    QList<QAction *> m_actions;
    QWidget *m_widget = nullptr;
    QList<QMetaObject::Connection> m_connections;

protected slots:
    void maybeSetPrintingEnabled(bool enabled);
    void maybeEnablePrinting();
    void disablePrinting();

private:
    QList<QMenu *> m_menus;
    QList<QToolBar *> m_toolBars;
    bool m_printingEnabled = false;
};

#endif // ABSTRACTVIEWER_H
