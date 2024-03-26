// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QTest>
#include <private/qtestlog_p.h>

class tst_Signaldumper : public QObject
{
    Q_OBJECT

    void addConnectionTypeData();

private slots:
    void noConnections();
    void oneSlot_data();
    void oneSlot();
    void oneSlotOldSyntax_data();
    void oneSlotOldSyntax();
    void twoSlots_data();
    void twoSlots();
    void twoSlotsOldSyntax_data();
    void twoSlotsOldSyntax();
    void signalForwarding_data();
    void signalForwarding();
    void signalForwardingOldSyntax_data();
    void signalForwardingOldSyntax();
    void slotEmittingSignal_data();
    void slotEmittingSignal();
    void slotEmittingSignalOldSyntax_data();
    void slotEmittingSignalOldSyntax();

    void variousTypes();

    void deletingSender();
};

void tst_Signaldumper::addConnectionTypeData()
{
    QTest::addColumn<Qt::ConnectionType>("connectionType");
    QTest::newRow("direct") << Qt::ConnectionType::DirectConnection;
    QTest::newRow("queued") << Qt::ConnectionType::QueuedConnection;
}

/*
    Simple class to keep the slots and signals separate from the test
*/
class SignalSlotClass : public QObject
{
    Q_OBJECT

public:
    SignalSlotClass();

public slots:
    void slotWithoutParameters() {}
    void slotWithParameters(int, char) {}
    void emitSecondSignal() { emit nestedSignal(); }

signals:
    void signalWithoutParameters();
    void signalWithParameters(int i, char c);

    void nestedSignal();
    void nestedSignalWithParameters(int i, char c);

    // For the "variousTypes" test
    void qStringSignal(QString string);
    void qStringRefSignal(QString &string);
    void qStringConstRefSignal(const QString &string);
    void qByteArraySignal(QByteArray byteArray);
    void qListSignal(QList<int> list);
    void qVectorSignal(QList<int> vector);
    void qVectorRefSignal(QList<int> &vector);
    void qVectorConstRefSignal(const QList<int> &vector);
    void qVectorConstPointerSignal(const QList<int> *vector);
    void qVectorPointerConstSignal(QList<int> *const vector);
    void qVariantSignal(QVariant variant);
};

SignalSlotClass::SignalSlotClass()
{
    // For printing signal argument in "variousTypes" test
    qRegisterMetaType<QList<int>>();
    qRegisterMetaType<QList<int>>();
}

void tst_Signaldumper::noConnections()
{
    SignalSlotClass signalSlotOwner;

    emit signalSlotOwner.signalWithoutParameters();
    emit signalSlotOwner.signalWithParameters(242, 'm');
}

void tst_Signaldumper::oneSlot_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::oneSlot()
{
    QFETCH(Qt::ConnectionType, connectionType);

    SignalSlotClass signalSlotOwner;
    // parameterless to parameterless
    auto connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithoutParameters,
                              &signalSlotOwner, &SignalSlotClass::slotWithoutParameters, connectionType);
    emit signalSlotOwner.signalWithoutParameters();

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameters to parameters
    connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::slotWithParameters, connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameters to no parameters
    connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::slotWithoutParameters, connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
}

void tst_Signaldumper::oneSlotOldSyntax_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::oneSlotOldSyntax()
{
    QFETCH(Qt::ConnectionType, connectionType);

    SignalSlotClass signalSlotOwner;
    // parameterless to parameterless
    auto connection = connect(&signalSlotOwner, SIGNAL(signalWithoutParameters()),
                              &signalSlotOwner, SLOT(slotWithoutParameters()), connectionType);
    emit signalSlotOwner.signalWithoutParameters();

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameters to parameters
    connection = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SLOT(slotWithParameters(int, char)), connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameters to no parameters
    connection = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SLOT(slotWithoutParameters()), connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
}

void tst_Signaldumper::twoSlots_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::twoSlots()
{
    QFETCH(Qt::ConnectionType, connectionType);

    // Now, instead of creating two slots or two objects, we will just do the same connection twice.
    // The same slot will then be invoked twice.

    SignalSlotClass signalSlotOwner;
    // parameterless to parameterless
    auto connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithoutParameters,
                              &signalSlotOwner, &SignalSlotClass::slotWithoutParameters, connectionType);
    auto connection2 = connect(&signalSlotOwner, &SignalSlotClass::signalWithoutParameters,
                              &signalSlotOwner, &SignalSlotClass::slotWithoutParameters, connectionType);
    emit signalSlotOwner.signalWithoutParameters();

    QCoreApplication::processEvents();
    disconnect(connection);
    disconnect(connection2);

    // parameters to parameters
    connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::slotWithParameters, connectionType);
    connection2 = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::slotWithParameters, connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
    disconnect(connection2);

    // parameters to no parameters
    connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::slotWithoutParameters, connectionType);
    connection2 = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::slotWithoutParameters, connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
    disconnect(connection2);
}

void tst_Signaldumper::twoSlotsOldSyntax_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::twoSlotsOldSyntax()
{
    QFETCH(Qt::ConnectionType, connectionType);

    // Now, instead of creating two slots or two objects, we will just do the same connection twice.
    // The same slot will then be invoked twice.

    SignalSlotClass signalSlotOwner;
    // parameterless to parameterless
    auto connection = connect(&signalSlotOwner, SIGNAL(signalWithoutParameters()),
                              &signalSlotOwner, SLOT(slotWithoutParameters()), connectionType);
    auto connection2 = connect(&signalSlotOwner, SIGNAL(signalWithoutParameters()),
                              &signalSlotOwner, SLOT(slotWithoutParameters()), connectionType);
    emit signalSlotOwner.signalWithoutParameters();

    QCoreApplication::processEvents();
    disconnect(connection);
    disconnect(connection2);

    // parameters to parameters
    connection = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SLOT(slotWithParameters(int, char)), connectionType);
    connection2 = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SLOT(slotWithParameters(int, char)), connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
    disconnect(connection2);

    // parameters to no parameters
    connection = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SLOT(slotWithoutParameters()), connectionType);
    connection2 = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SLOT(slotWithoutParameters()), connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
    disconnect(connection2);
}

void tst_Signaldumper::signalForwarding_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::signalForwarding()
{
    QFETCH(Qt::ConnectionType, connectionType);

    SignalSlotClass signalSlotOwner;

    // parameterless signal to parameterless signal
    auto connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithoutParameters,
                              &signalSlotOwner, &SignalSlotClass::nestedSignal, connectionType);
    emit signalSlotOwner.signalWithoutParameters();

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameter(full) signal to parameter(full) signal
    connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::nestedSignalWithParameters, connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameter(full) signal to parameterless signal
    connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithParameters,
                         &signalSlotOwner, &SignalSlotClass::nestedSignal, connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
}

void tst_Signaldumper::signalForwardingOldSyntax_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::signalForwardingOldSyntax()
{
    QFETCH(Qt::ConnectionType, connectionType);

    SignalSlotClass signalSlotOwner;

    // parameterless signal to parameterless signal
    auto connection = connect(&signalSlotOwner, SIGNAL(signalWithoutParameters()),
                              &signalSlotOwner, SIGNAL(nestedSignal()), connectionType);
    emit signalSlotOwner.signalWithoutParameters();

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameter(full) signal to parameter(full) signal
    connection = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SIGNAL(nestedSignalWithParameters(int, char)), connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);

    // parameter(full) signal to parameterless signal
    connection = connect(&signalSlotOwner, SIGNAL(signalWithParameters(int, char)),
                         &signalSlotOwner, SIGNAL(nestedSignal()), connectionType);
    emit signalSlotOwner.signalWithParameters(242, 'm');

    QCoreApplication::processEvents();
    disconnect(connection);
}

void tst_Signaldumper::slotEmittingSignal_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::slotEmittingSignal()
{
    QFETCH(Qt::ConnectionType, connectionType);

    SignalSlotClass signalSlotOwner;

    auto connection = connect(&signalSlotOwner, &SignalSlotClass::signalWithoutParameters,
                              &signalSlotOwner, &SignalSlotClass::emitSecondSignal, connectionType);
    emit signalSlotOwner.signalWithoutParameters();
    QCoreApplication::processEvents();
    disconnect(connection);
}

void tst_Signaldumper::slotEmittingSignalOldSyntax_data()
{
    addConnectionTypeData();
}

void tst_Signaldumper::slotEmittingSignalOldSyntax()
{
    QFETCH(Qt::ConnectionType, connectionType);

    SignalSlotClass signalSlotOwner;

    auto connection = connect(&signalSlotOwner, SIGNAL(signalWithoutParameters()),
                              &signalSlotOwner, SLOT(emitSecondSignal()), connectionType);
    emit signalSlotOwner.signalWithoutParameters();
    QCoreApplication::processEvents();
    disconnect(connection);
}

void tst_Signaldumper::variousTypes()
{
    SignalSlotClass signalSlotOwner;
    QString string = QString::fromLatin1("Test string");
    emit signalSlotOwner.qStringSignal(string);
    emit signalSlotOwner.qStringRefSignal(string);
    emit signalSlotOwner.qStringConstRefSignal(string);
    emit signalSlotOwner.qByteArraySignal(QByteArray("Test bytearray"));

    QList<int> list{1, 2, 3, 242};
    emit signalSlotOwner.qListSignal(list);

    QList<int> vector { 1, 2, 3, 242 };
    emit signalSlotOwner.qVectorSignal(vector);
    emit signalSlotOwner.qVectorRefSignal(vector);
    emit signalSlotOwner.qVectorConstRefSignal(vector);
    emit signalSlotOwner.qVectorConstPointerSignal(&vector);
    emit signalSlotOwner.qVectorPointerConstSignal(&vector);

    QVariant variant = 24;
    emit signalSlotOwner.qVariantSignal(variant);
    variant = QVariant(string);
    emit signalSlotOwner.qVariantSignal(variant);
}

void tst_Signaldumper::deletingSender()
{
    SignalSlotClass *signalSlotOwner = new SignalSlotClass();
    connect(signalSlotOwner, &SignalSlotClass::signalWithoutParameters, [signalSlotOwner]() {
        delete signalSlotOwner;
    });
    emit signalSlotOwner->signalWithoutParameters();
}

QTEST_MAIN_WRAPPER(tst_Signaldumper,
    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-vs");
    argc = int(args.size());
    argv = const_cast<char**>(&args[0]);
    QTEST_MAIN_SETUP())

#include "tst_signaldumper.moc"
