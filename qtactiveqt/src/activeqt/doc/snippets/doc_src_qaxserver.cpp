// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [3]
#include <QWidget>

class MyActiveX : public QWidget
{
    Q_OBJECT
//! [3]


//! [4]
Q_CLASSINFO("ClassID", "{1D9928BD-4453-4bdd-903D-E525ED17FDE5}")
Q_CLASSINFO("InterfaceID", "{99F6860E-2C5A-42ec-87F2-43396F4BE389}")
Q_CLASSINFO("EventsID", "{0A3E9F27-E4F1-45bb-9E47-63099BCCD0E3}")
//! [4]


//! [5]
Q_PROPERTY(int value READ value WRITE setValue)
//! [5]


//! [6]
public:
    MyActiveX(QWidget *parent = 0)
    ...

    int value() const;

public slots:
    void setValue(int v);
    ...

signals:
    void valueChange(int v);
    ...

};
//! [6]


//! [7]
#include <QAxBindable>
#include <QWidget>

class MyActiveX : public QWidget, public QAxBindable
{
    Q_OBJECT
//! [7]


//! [8]
QAXFACTORY_BEGIN("{ad90301a-849e-4e8b-9a91-0a6dc5f6461f}",
                 "{a8f21901-7ff7-4f6a-b939-789620c03d83}")
    QAXCLASS(MyWidget)
    QAXCLASS(MyWidget2)
    QAXTYPE(MySubType)
QAXFACTORY_END()
//! [8]


//! [9]
#include <QApplication>
#include <QAxFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (!QAxFactory::isServer()) {
        // create and show main window
    }
    return app.exec();
}
//! [9]


//! [10]
MyFactory(const QUuid &, const QUuid &);
//! [10]


//! [11]
HMODULE dll = LoadLibrary("myserver.dll");
typedef HRESULT(__stdcall *DllRegisterServerProc)();
DllRegisterServerProc DllRegisterServer =
    (DllRegisterServerProc)GetProcAddress(dll, "DllRegisterServer");

HRESULT res = E_FAIL;
if (DllRegisterServer)
    res = DllRegisterServer();
if (res != S_OK)
    // error handling
//! [11]


//! [15]
class MyActiveX : public QWidget
{
    Q_OBJECT
    Q_CLASSINFO("Version", "2.0")
    Q_CLASSINFO("ClassID", "{7a4cffd8-cbcd-4ae9-ae7e-343e1e5710df}")
    Q_CLASSINFO("InterfaceID", "{6fb035bf-8019-48d8-be51-ef05427d8994}")
    Q_CLASSINFO("EventsID", "{c42fffdf-6557-47c9-817a-2da2228bc29c}")
    Q_CLASSINFO("Insertable", "yes")
    Q_CLASSINFO("ToSuperClass", "MyActiveX")
    Q_PROPERTY(...)

public:
    MyActiveX(QWidget *parent = 0);

    ...
};
//! [15]


//! [16]
class MyLicensedControl : public QWidget
{
    Q_OBJECT
    Q_CLASSINFO("LicenseKey", "<key string>")
    ...
};
//! [16]


//! [17]
class AxImpl : public QAxAggregated, public ISomeCOMInterface
{
public:
    AxImpl() {}

    long queryInterface(const QUuid &iid, void **iface);

    // IUnknown
    QAXAGG_IUNKNOWN

    // ISomeCOMInterface
    ...
}
//! [17]


//! [18]
long AxImpl::queryInterface(const QUuid &iid, void **iface)
{
    *iface = 0;
    if (iid == IID_ISomeCOMInterface)
        *iface = (ISomeCOMInterface *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}
//! [18]


//! [19]
HRESULT AxImpl::QueryInterface(REFIID iid, void **iface)
{
    return controllingUnknown()->QueryInterface(iid, iface);
}
//! [19]


//! [20]
class MyActiveX : public QWidget, public QAxBindable
{
    Q_OBJECT

public:
    MyActiveX(QWidget *parent);

    QAxAggregated *createAggregate()
    {
        return new AxImpl();
    }
};
//! [20]
