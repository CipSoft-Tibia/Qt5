// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DEFAULT_CONTAINER_H
#define DEFAULT_CONTAINER_H

#include <QtDesigner/container.h>
#include <QtDesigner/extension.h>
#include <extensionfactory_p.h>

#include <QtWidgets/qstackedwidget.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qtoolbox.h>
#include <QtWidgets/qscrollarea.h>
#include <QtWidgets/qdockwidget.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

// ------------ QStackedWidgetContainer
class QStackedWidgetContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QStackedWidgetContainer(QStackedWidget *widget, QObject *parent = nullptr);

    int count() const override { return m_widget->count(); }
    QWidget *widget(int index) const override { return m_widget->widget(index); }

    int currentIndex() const override { return m_widget->currentIndex(); }
    void setCurrentIndex(int index) override;

    bool canAddWidget() const override { return true; }
    void addWidget(QWidget *widget) override;
    void insertWidget(int index, QWidget *widget) override;
    bool canRemove(int) const override { return true; }
    void remove(int index) override;

private:
    QStackedWidget *m_widget;
};

// ------------ QTabWidgetContainer
class QTabWidgetContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QTabWidgetContainer(QTabWidget *widget, QObject *parent = nullptr);

    int count() const override { return m_widget->count(); }
    QWidget *widget(int index) const override { return m_widget->widget(index); }

    int currentIndex() const override { return m_widget->currentIndex(); }
    void setCurrentIndex(int index) override;

    bool canAddWidget() const override { return true; }
    void addWidget(QWidget *widget) override;
    void insertWidget(int index, QWidget *widget) override;
    bool canRemove(int) const override { return true; }
    void remove(int index) override;

private:
    QTabWidget *m_widget;
};

// ------------  QToolBoxContainer
class QToolBoxContainer: public QObject, public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QToolBoxContainer(QToolBox *widget, QObject *parent = nullptr);

    int count() const override { return m_widget->count(); }
    QWidget *widget(int index) const override { return m_widget->widget(index); }

    int currentIndex() const override { return m_widget->currentIndex(); }
    void setCurrentIndex(int index) override;

    bool canAddWidget() const override { return true; }
    void addWidget(QWidget *widget) override;
    void insertWidget(int index, QWidget *widget) override;
    bool canRemove(int) const override { return true; }
    void remove(int index) override;

private:
    QToolBox *m_widget;
};

// ------------ SingleChildContainer:
//  Template for containers that have a single child widget using widget()/setWidget().

template <class Container>
class SingleChildContainer: public QDesignerContainerExtension
{
protected:
    explicit SingleChildContainer(Container *widget, bool active = true);
public:
    int count() const override;
    QWidget *widget(int index) const override;
    int currentIndex() const override;
    void setCurrentIndex(int /*index*/) override {}
    void addWidget(QWidget *widget) override;
    void insertWidget(int index, QWidget *widget) override;
    void remove(int /*index*/) override {}

    bool canAddWidget() const override { return false; }
    bool canRemove(int) const override { return false; }

private:
    const bool m_active;
    Container *m_container;
};

template <class Container>
SingleChildContainer<Container>::SingleChildContainer(Container *widget, bool active) :
    m_active(active),
    m_container(widget)
{
}

template <class Container>
int SingleChildContainer<Container>::count() const
{
    return  m_active && m_container->widget() ? 1 : 0;
}

template <class Container>
QWidget *SingleChildContainer<Container>::widget(int /* index */) const
{
    return m_container->widget();
}

template <class Container>
int SingleChildContainer<Container>::currentIndex() const
{
    return m_active && m_container->widget() ? 0 : -1;
}

template <class Container>
void SingleChildContainer<Container>::addWidget(QWidget *widget)
{
    Q_ASSERT(m_container->widget() == nullptr);
    widget->setParent(m_container);
    m_container->setWidget(widget);
}

template <class Container>
void SingleChildContainer<Container>::insertWidget(int /* index */, QWidget *widget)
{
    addWidget(widget);
}

// ------------  QScrollAreaContainer
class QScrollAreaContainer: public QObject, public SingleChildContainer<QScrollArea>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QScrollAreaContainer(QScrollArea *widget, QObject *parent = nullptr);
};

// --------------- QDockWidgetContainer
class QDockWidgetContainer: public QObject, public SingleChildContainer<QDockWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit QDockWidgetContainer(QDockWidget *widget, QObject *parent = nullptr);
};

using QDesignerStackedWidgetContainerFactory = ExtensionFactory<QDesignerContainerExtension, QStackedWidget, QStackedWidgetContainer>;
using QDesignerTabWidgetContainerFactory = ExtensionFactory<QDesignerContainerExtension, QTabWidget, QTabWidgetContainer>;
using QDesignerToolBoxContainerFactory = ExtensionFactory<QDesignerContainerExtension, QToolBox, QToolBoxContainer>;
using QScrollAreaContainerFactory = ExtensionFactory<QDesignerContainerExtension, QScrollArea, QScrollAreaContainer>;
using QDockWidgetContainerFactory = ExtensionFactory<QDesignerContainerExtension,  QDockWidget, QDockWidgetContainer>;
}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // DEFAULT_CONTAINER_H
