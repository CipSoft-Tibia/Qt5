// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QAxFactory>
#include <QTabWidget>
#include <QScopedPointer>
#include <QTimer>
#include <QList>

class Application;
class DocumentList;

//! [0]
class Document : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("ClassID", "{2b5775cd-72c2-43da-bc3b-b0e8d1e1c4f7}")
    Q_CLASSINFO("InterfaceID", "{2ce1761e-07a3-415c-bd11-0eab2c7283de}")

    Q_PROPERTY(Application *application READ application)
    Q_PROPERTY(QString title READ title WRITE setTitle)

public:
    explicit Document(DocumentList *list);
    virtual ~Document();

    Application *application() const;

    QString title() const;
    void setTitle(const QString &title);

private:
    QScopedPointer <QWidget> m_page;
};
//! [0]

//! [1]
class DocumentList : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("ClassID", "{496b761d-924b-4554-a18a-8f3704d2a9a6}")
    Q_CLASSINFO("InterfaceID", "{6c9e30e8-3ff6-4e6a-9edc-d219d074a148}")

    Q_PROPERTY(Application* application READ application)
    Q_PROPERTY(int count READ count)

public:
    explicit DocumentList(Application *application);

    int count() const;
    Application *application() const;

public slots:
    Document *addDocument();
    Document *item(int index) const;

private:
    QList<Document *> m_list;
};
//! [1]

//! [2]
class Application : public QObject
{
    Q_OBJECT

    Q_CLASSINFO("ClassID", "{b50a71db-c4a7-4551-8d14-49983566afee}")
    Q_CLASSINFO("InterfaceID", "{4a427759-16ef-4ed8-be79-59ffe5789042}")
    Q_CLASSINFO("RegisterObject", "yes")

    Q_PROPERTY(DocumentList* documents READ documents)
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible)

public:
    explicit Application(QObject *parent = nullptr);
    DocumentList *documents() const;

    QString id() const { return objectName(); }

    void setVisible(bool on);
    bool isVisible() const;

    QTabWidget *window() const { return m_ui.data(); }

public slots:
    void quit();

private:
    QScopedPointer <DocumentList> m_docs;
    QScopedPointer <QTabWidget> m_ui;
};
//! [2]

//! [3]
Document::Document(DocumentList *list)
: QObject(list)
{
    QTabWidget *tabs = list->application()->window();
    m_page.reset(new QWidget(tabs));
    m_page->setWindowTitle(tr("Unnamed"));
    tabs->addTab(m_page.data(), m_page->windowTitle());

    m_page->show();
}

Document::~Document() = default;

Application *Document::application() const
{
    return qobject_cast<DocumentList *>(parent())->application();
}

QString Document::title() const
{
    return m_page->windowTitle();
}

void Document::setTitle(const QString &t)
{
    m_page->setWindowTitle(t);

    QTabWidget *tabs = application()->window();
    int index = tabs->indexOf(m_page.data());
    tabs->setTabText(index, m_page->windowTitle());
}

//! [3] //! [4]
DocumentList::DocumentList(Application *application)
: QObject(application)
{
}

Application *DocumentList::application() const
{
    return qobject_cast<Application *>(parent());
}

int DocumentList::count() const
{
    return m_list.size();
}

Document *DocumentList::item(int index) const
{
    return m_list.value(index, nullptr);
}

Document *DocumentList::addDocument()
{
    Document *document = new Document(this);
    m_list.append(document);

    return document;
}

//! [4] //! [5]
Application::Application(QObject *parent)
: QObject(parent),
  m_ui(new QTabWidget),
  m_docs(new DocumentList(this))
{
    setObjectName(QStringLiteral("From QAxFactory"));
}

DocumentList *Application::documents() const
{
    return m_docs.data();
}

void Application::setVisible(bool on)
{
    m_ui->setVisible(on);
}

bool Application::isVisible() const
{
    return m_ui->isVisible();
}

void Application::quit()
{
    m_docs.reset();
    m_ui.reset();
    QTimer::singleShot(0 /*ms*/, qApp, &QCoreApplication::quit);
}

#include "main.moc"
//! [5] //! [6]


QAXFACTORY_BEGIN("{edd3e836-f537-4c6f-be7d-6014c155cc7a}", "{b7da3de8-83bb-4bbe-9ab7-99a05819e201}")
   QAXCLASS(Application)
   QAXTYPE(Document)
   QAXTYPE(DocumentList)
QAXFACTORY_END()

//! [6] //! [7]
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // started by COM - don't do anything
    if (QAxFactory::isServer())
        return app.exec();

    // started by user
    Application appobject;
    appobject.setObjectName(QStringLiteral("From Application"));

    QAxFactory::startServer();
    QAxFactory::registerActiveObject(&appobject);

    appobject.window()->setMinimumSize(300, 100);
    appobject.setVisible(true);

    QObject::connect(&app, &QGuiApplication::lastWindowClosed, &appobject, &Application::quit);

    return app.exec();
}
//! [7]
