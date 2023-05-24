// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QCLASSFACTORY_P_H
#define QCLASSFACTORY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcoreapplication.h>
#include <ocidl.h>

QT_BEGIN_NAMESPACE

// COM Factory class, mapping COM requests to ActiveQt requests.
// One instance of this class for each ActiveX the server can provide.
class QClassFactory : public IClassFactory2
{
    Q_DISABLE_COPY_MOVE(QClassFactory)
public:
    QClassFactory(CLSID clsid);

    virtual ~QClassFactory();

    // IUnknown
    unsigned long WINAPI AddRef() override;
    unsigned long WINAPI Release() override;
    HRESULT WINAPI QueryInterface(REFIID iid, LPVOID *iface) override;

    HRESULT WINAPI CreateInstanceHelper(IUnknown *pUnkOuter, REFIID iid, void **ppObject);

    // IClassFactory
    HRESULT WINAPI CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppObject) override;

    HRESULT WINAPI LockServer(BOOL fLock) override;
    // IClassFactory2
    HRESULT WINAPI RequestLicKey(DWORD, BSTR *pKey) override;

    HRESULT WINAPI GetLicInfo(LICINFO *pLicInfo) override;

    HRESULT WINAPI CreateInstanceLic(IUnknown *pUnkOuter, IUnknown *pUnkReserved,
                                     REFIID iid, BSTR bKey, PVOID *ppObject) override;

    static void cleanupCreatedApplication(QCoreApplication &app);

    QString className;

protected:
    CRITICAL_SECTION refCountSection;
    LONG ref = 0;
    bool licensed = false;
    QString classKey;
};

QT_END_NAMESPACE

#endif // QCLASSFACTORY_P_H
