/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "abstractdialoggui_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QDesignerDialogGuiInterface
    \since 4.4
    \internal

    \brief The QDesignerDialogGuiInterface allows integrations of \QD to replace the
           message boxes displayed by \QD by custom dialogs.

    \inmodule QtDesigner

    QDesignerDialogGuiInterface provides virtual functions that can be overwritten
    to display message boxes and file dialogs.
    \sa QMessageBox, QFileDialog
*/

/*!
    \enum QDesignerDialogGuiInterface::Message

    This enum specifies the context from within the message box is called.

   \value FormLoadFailureMessage      Loading of a form failed
   \value UiVersionMismatchMessage    Attempt to load a file created with an old version of Designer
   \value ResourceLoadFailureMessage  Resources specified in a file could not be found
   \value TopLevelSpacerMessage       Spacer items detected on a container without layout
   \value PropertyEditorMessage       Messages of the propert yeditor
   \value SignalSlotEditorMessage     Messages of the signal / slot editor
   \value FormEditorMessage           Messages of the form editor
   \value PreviewFailureMessage       A preview could not be created
   \value PromotionErrorMessage       Messages related to promotion of a widget
   \value ResourceEditorMessage       Messages of the resource editor
   \value ScriptDialogMessage         Messages of the script dialog
   \value SignalSlotDialogMessage     Messages of the signal slot dialog
   \value OtherMessage                Unspecified context
*/

/*!
    Constructs a QDesignerDialogGuiInterface object.
*/

QDesignerDialogGuiInterface::QDesignerDialogGuiInterface()
{
}

/*!
    Destroys the QDesignerDialogGuiInterface object.
*/
QDesignerDialogGuiInterface::~QDesignerDialogGuiInterface()
{
}

/*!
    \fn QMessageBox::StandardButton QDesignerDialogGuiInterface::message(QWidget *parent, Message context, QMessageBox::Icon icon, const QString &title, const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)

     Opens a message box as child of \a parent within the context \a context, using \a icon, \a title, \a text, \a buttons and \a defaultButton
     and returns the button chosen by the user.
*/

/*!
    \fn QString QDesignerDialogGuiInterface::getExistingDirectory(QWidget *parent, const QString &caption, const QString &dir, QFileDialog::Options options)

     Opens a file dialog as child of \a parent using the parameters \a caption, \a dir and \a options that prompts the
     user for an existing directory. Returns a directory selected by the user.
*/

/*!
    \fn QString QDesignerDialogGuiInterface::getOpenFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options)

    Opens a file dialog as child of \a parent using the parameters \a caption, \a dir, \a filter, \a selectedFilter and \a options
    that prompts the user for an existing file. Returns a file selected by the user.
*/

/*!
    \fn QStringList QDesignerDialogGuiInterface::getOpenFileNames(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options)

    Opens a file dialog as child of \a parent using the parameters \a caption, \a dir, \a filter, \a selectedFilter and \a options
    that prompts the user for a set of existing files. Returns one or more existing files selected by the user.
*/

/*!
    Opens a file dialog with image browsing capabilities as child of \a parent using the parameters \a caption, \a dir, \a filter, \a selectedFilter and \a options
    that prompts the user for an existing file. Returns a file selected by the user.

    The default implementation simply calls getOpenFileName(). On platforms that do not support an image preview in the QFileDialog,
    the function can be reimplemented to provide an image browser.

    \since 4.5
*/

QString QDesignerDialogGuiInterface::getOpenImageFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
    return getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
}

/*!
    Opens a file dialog with image browsing capabilities as child of \a parent using the parameters \a caption, \a dir, \a filter, \a selectedFilter and \a options
    that prompts the user for a set of existing files. Returns one or more existing files selected by the user.

    The default implementation simply calls getOpenFileNames(). On platforms that do not support an image preview in the QFileDialog,
    the function can be reimplemented to provide an image browser.

    \since 4.5
*/

QStringList QDesignerDialogGuiInterface::getOpenImageFileNames(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options options)
{
    return getOpenFileNames(parent, caption, dir, filter, selectedFilter, options);
}

/*!
    \fn QString QDesignerDialogGuiInterface::getSaveFileName(QWidget *parent, const QString &caption, const QString &dir, const QString &filter, QString *selectedFilter, QFileDialog::Options)

    Opens a file dialog as child of \a parent using the parameters \a caption, \a dir, \a filter, \a selectedFilter and \a options
    that prompts the user for a file. Returns a file selected by the user. The file does not have to exist.
*/

QT_END_NAMESPACE
