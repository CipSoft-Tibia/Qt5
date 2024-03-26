// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpagesetupdialog.h"
#include <private/qpagesetupdialog_p.h>

#include <QtPrintSupport/qprinter.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPageSetupDialog

    \brief The QPageSetupDialog class provides a configuration dialog
    for the page-related options on a printer.

    \ingroup standard-dialogs
    \ingroup printing
    \inmodule QtPrintSupport

    On Windows and \macos the page setup dialog is implemented using
    the native page setup dialogs.

    Note that on Windows and \macos custom paper sizes won't be
    reflected in the native page setup dialogs. Additionally, custom
    page margins set on a QPrinter won't show in the native \macos
    page setup dialog.

    \sa QPrinter, QPrintDialog
*/


/*!
    \fn QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)

    Constructs a page setup dialog that configures \a printer with \a
    parent as the parent widget.
*/

/*!
    \fn QPageSetupDialog::~QPageSetupDialog()

    Destroys the page setup dialog.
*/

/*!
    \since 4.5

    \fn QPageSetupDialog::QPageSetupDialog(QWidget *parent)

    Constructs a page setup dialog that configures a default-constructed
    QPrinter with \a parent as the parent widget.

    \sa printer()
*/

/*!
    \fn QPrinter *QPageSetupDialog::printer()

    Returns the printer that was passed to the QPageSetupDialog
    constructor.
*/

QPageSetupDialogPrivate::QPageSetupDialogPrivate(QPrinter *prntr) : printer(nullptr), ownsPrinter(false)
{
    setPrinter(prntr);
}

void QPageSetupDialogPrivate::setPrinter(QPrinter *newPrinter)
{
    if (printer && ownsPrinter)
        delete printer;

    if (newPrinter) {
        printer = newPrinter;
        ownsPrinter = false;
    } else {
        printer = new QPrinter;
        ownsPrinter = true;
    }
    if (printer->outputFormat() != QPrinter::NativeFormat)
        qWarning("QPageSetupDialog: Cannot be used on non-native printers");
}

/*!
    \overload
    \since 4.5

    Opens the dialog and connects its accepted() signal to the slot specified
    by \a receiver and \a member.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QPageSetupDialog::open(QObject *receiver, const char *member)
{
    Q_D(QPageSetupDialog);
    connect(this, SIGNAL(accepted()), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
/*! \fn void QPageSetupDialog::setVisible(bool visible)
    \reimp
*/
#endif

QPageSetupDialog::~QPageSetupDialog()
{
    Q_D(QPageSetupDialog);
    if (d->ownsPrinter)
        delete d->printer;
}

QPrinter *QPageSetupDialog::printer()
{
    Q_D(QPageSetupDialog);
    return d->printer;
}

/*!
    \fn int QPageSetupDialog::exec()

    This virtual function is called to pop up the dialog. It must be
    reimplemented in subclasses.
*/

/*!
    \reimp
*/
void QPageSetupDialog::done(int result)
{
    Q_D(QPageSetupDialog);
    QDialog::done(result);
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, SIGNAL(accepted()),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = nullptr;
    }
    d->memberToDisconnectOnClose.clear();

}

QT_END_NAMESPACE
