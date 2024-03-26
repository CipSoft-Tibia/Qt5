// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "certificateinfo.h"
#include "sslclient.h"

#include "ui_sslclient.h"
#include "ui_sslerrors.h"

SslClient::SslClient(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupSecureSocket();
}

SslClient::~SslClient()
{
    delete socket;
    delete form;
}

void SslClient::updateEnabledState()
{
    const bool unconnected = socket->state() == QAbstractSocket::UnconnectedState;
    form->hostNameEdit->setReadOnly(!unconnected);
    form->hostNameEdit->setFocusPolicy(unconnected ? Qt::StrongFocus : Qt::NoFocus);
    form->hostNameLabel->setEnabled(unconnected);
    form->portBox->setEnabled(unconnected);
    form->portLabel->setEnabled(unconnected);
    form->connectButton->setEnabled(unconnected && !form->hostNameEdit->text().isEmpty());

    const bool connected = socket->state() == QAbstractSocket::ConnectedState;
    form->sessionOutput->setEnabled(connected);
    form->sessionInput->setEnabled(connected);
    form->sessionInputLabel->setEnabled(connected);
    form->sendButton->setEnabled(connected);
}

void SslClient::secureConnect()
{
    socket->connectToHostEncrypted(form->hostNameEdit->text(), form->portBox->value());
    updateEnabledState();
}

void SslClient::socketStateChanged(QAbstractSocket::SocketState state)
{
    if (executingDialog)
        return;

    updateEnabledState();

    if (state == QAbstractSocket::UnconnectedState) {
        form->sessionInput->clear();
        form->hostNameEdit->setPalette(QPalette());
        form->hostNameEdit->setFocus();
        form->cipherLabel->setText(tr("<none>"));
        padLock->hide();
    }
}

void SslClient::socketEncrypted()
{
    form->sessionOutput->clear();
    form->sessionInput->setFocus();

    QPalette palette;
    palette.setColor(QPalette::Base, QColor(255, 255, 192));
    form->hostNameEdit->setPalette(palette);

    const QSslCipher cipher = socket->sessionCipher();
    const QString cipherInfo = QString("%1, %2 (%3/%4)").arg(cipher.authenticationMethod())
                                       .arg(cipher.name()).arg(cipher.usedBits())
                                       .arg(cipher.supportedBits());;
    form->cipherLabel->setText(cipherInfo);
    padLock->show();
}

void SslClient::socketReadyRead()
{
    appendString(QString::fromUtf8(socket->readAll()));
}

void SslClient::sendData()
{
    const QString input = form->sessionInput->text();
    appendString(input + '\n');
    socket->write(input.toUtf8() + "\r\n");
    form->sessionInput->clear();
}

void SslClient::socketError(QAbstractSocket::SocketError)
{
    if (handlingSocketError)
        return;

    handlingSocketError = true;
    QMessageBox::critical(this, tr("Connection error"), socket->errorString());
    handlingSocketError = false;
}

void SslClient::sslErrors(const QList<QSslError> &errors)
{
    QDialog errorDialog(this);
    Ui_SslErrors ui;
    ui.setupUi(&errorDialog);
    connect(ui.certificateChainButton, &QPushButton::clicked,
            this, &SslClient::displayCertificateInfo);

    for (const auto &error : errors)
        ui.sslErrorList->addItem(error.errorString());

    executingDialog = true;
    if (errorDialog.exec() == QDialog::Accepted)
        socket->ignoreSslErrors();
    executingDialog = false;

    // did the socket state change?
    if (socket->state() != QAbstractSocket::ConnectedState)
        socketStateChanged(socket->state());
}

void SslClient::displayCertificateInfo()
{
    CertificateInfo info;
    info.setCertificateChain(socket->peerCertificateChain());
    info.exec();
}

void SslClient::setupUi()
{
    if (form)
        return;

    form = new Ui_Form;
    form->setupUi(this);
    form->hostNameEdit->setSelection(0, form->hostNameEdit->text().size());
    form->sessionOutput->setHtml(tr("&lt;not connected&gt;"));

    connect(form->hostNameEdit, &QLineEdit::textChanged,
            this, &SslClient::updateEnabledState);
    connect(form->connectButton, &QPushButton::clicked,
            this, &SslClient::secureConnect);
    connect(form->sendButton, &QPushButton::clicked,
            this, &SslClient::sendData);

    padLock = new QToolButton;
    padLock->setIcon(QIcon(":/encrypted.png"));
    connect(padLock, &QToolButton::clicked,
            this, &SslClient::displayCertificateInfo);

#if QT_CONFIG(cursor)
    padLock->setCursor(Qt::ArrowCursor);
#endif
    padLock->setToolTip(tr("Display encryption details."));

    const int extent = form->hostNameEdit->height() - 2;
    padLock->resize(extent, extent);
    padLock->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);

    QHBoxLayout *layout = new QHBoxLayout(form->hostNameEdit);
    const int margin = form->hostNameEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(padLock);

    form->hostNameEdit->setLayout(layout);
    padLock->hide();
}

void SslClient::setupSecureSocket()
{
    if (socket)
        return;

    socket = new QSslSocket(this);

    connect(socket, &QSslSocket::stateChanged,
            this, &SslClient::socketStateChanged);
    connect(socket, &QSslSocket::encrypted,
            this, &SslClient::socketEncrypted);
    connect(socket, &QSslSocket::errorOccurred,
            this, &SslClient::socketError);
    connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
            this, &SslClient::sslErrors);
    connect(socket, &QSslSocket::readyRead,
            this, &SslClient::socketReadyRead);

}

void SslClient::appendString(const QString &line)
{
    QTextCursor cursor(form->sessionOutput->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line);
    form->sessionOutput->verticalScrollBar()->setValue(form->sessionOutput->verticalScrollBar()->maximum());
}
