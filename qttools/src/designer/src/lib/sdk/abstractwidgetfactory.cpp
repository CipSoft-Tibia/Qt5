// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtDesigner/abstractwidgetfactory.h>
#include "abstractformeditor.h"
#include "abstractwidgetdatabase.h"

QT_BEGIN_NAMESPACE

/*!
    \class QDesignerWidgetFactoryInterface
    \brief The QDesignerWidgetFactoryInterface class provides an interface that is used to control
    the widget factory used by Qt Designer.
    \inmodule QtDesigner
    \internal
*/

/*!
    \fn QDesignerWidgetFactoryInterface::QDesignerWidgetFactoryInterface(QObject *parent)

    Constructs an interface to a widget factory with the given \a parent.
*/
QDesignerWidgetFactoryInterface::QDesignerWidgetFactoryInterface(QObject *parent)
    : QObject(parent)
{
}

/*!
    \fn virtual QDesignerWidgetFactoryInterface::~QDesignerWidgetFactoryInterface()
*/
QDesignerWidgetFactoryInterface::~QDesignerWidgetFactoryInterface() = default;

/*!
    \fn virtual QDesignerFormEditorInterface *QDesignerWidgetFactoryInterface::core() const = 0

    Returns the core form editor interface associated with this interface.
*/

/*!
    \fn virtual QWidget* QDesignerWidgetFactoryInterface::containerOfWidget(QWidget *child) const = 0

    Returns the widget that contains the specified \a child widget.
*/

/*!
    \fn virtual QWidget* QDesignerWidgetFactoryInterface::widgetOfContainer(QWidget *container) const = 0


*/

/*!
    \fn virtual QWidget *QDesignerWidgetFactoryInterface::createWidget(const QString &name, QWidget *parent) const = 0

    Returns a new widget with the given \a name and \a parent widget. If no parent is specified,
    the widget created will be a top-level widget.
*/

/*!
    \fn virtual QLayout *QDesignerWidgetFactoryInterface::createLayout(QWidget *widget, QLayout *layout, int type) const = 0

    Returns a new layout of the specified \a type for the given \a widget or \a layout.
*/

/*!
    \fn virtual bool QDesignerWidgetFactoryInterface::isPassiveInteractor(QWidget *widget) = 0
*/

/*!
    \fn virtual void QDesignerWidgetFactoryInterface::initialize(QObject *object) const = 0
*/

QT_END_NAMESPACE
