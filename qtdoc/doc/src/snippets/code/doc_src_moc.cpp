// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [3]
#include "foo.moc"
//! [3]


//! [4]
#ifndef Q_MOC_RUN
    ...
#endif
//! [4]


//! [5]
class SomeTemplate<int> : public QFrame
{
    Q_OBJECT
    ...

signals:
    void mySignal(int);
};
//! [5]


//! [6]
// correct
class SomeClass : public QObject, public OtherClass
{
    ...
};
//! [6]


//! [7]
class SomeClass : public QObject
{
    Q_OBJECT

public slots:
    void apply(void (*apply)(List *, void *), char *); // WRONG
};
//! [7]


//! [8]
typedef void (*ApplyFunction)(List *, void *);

class SomeClass : public QObject
{
    Q_OBJECT

public slots:
    void apply(ApplyFunction, char *);
};
//! [8]


//! [9]
class MyClass : public QObject
{
    Q_OBJECT

    enum Error {
        ConnectionRefused,
        RemoteHostClosed,
        UnknownError
    };

signals:
    void stateChanged(MyClass::Error error);
};
//! [9]


//! [10]
#ifdef ultrix
#define SIGNEDNESS(a) unsigned a
#else
#define SIGNEDNESS(a) a
#endif

class Whatever : public QObject
{
    Q_OBJECT

signals:
    void someSignal(SIGNEDNESS(int));
};
//! [10]


//! [11]
class A
{
public:
    class B
    {
        Q_OBJECT

    public slots:   // WRONG
        void b();
    };
};
//! [11]
