// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TESTUTIL_P_H
#define TESTUTIL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtcore/qt_windows.h>
#include <qtcore/qtglobal>
#include <wrl/client.h>
#include <atomic>

using Microsoft::WRL::ComPtr;

template<typename T>
struct ComBase : T
{
    ~ComBase() { Q_ASSERT(m_refCount == 0); }

    ULONG AddRef() final
    {
        return m_refCount++;
    }

    ULONG Release() final
    {
        const int refCount = --m_refCount;
        if (refCount == 0)
            delete this;
        return refCount;
    }

    std::atomic_int m_refCount = 1u;
};

struct IUnknownStub : ComBase<IUnknown>
{
    HRESULT QueryInterface(const IID &riid, void **ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        if (riid == IID_IUnknown) {
            *ppvObject = this;
            return S_OK;
        }

        return E_NOINTERFACE;
    }
};

struct IDispatchStub : ComBase<IDispatch>
{
    HRESULT QueryInterface(const IID &riid, void **ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        if (riid == IID_IUnknown || riid == IID_IDispatch) {
            *ppvObject = this;
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    HRESULT GetTypeInfoCount(UINT * /*pctinfo*/) override { return E_NOTIMPL; }

    HRESULT GetTypeInfo(UINT /*iTInfo*/, LCID /*lcid*/, ITypeInfo ** /*ppTInfo*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT GetIDsOfNames(const IID & /*riid*/, LPOLESTR * /*rgszNames*/, UINT /*cNames*/,
                          LCID /*lcid*/, DISPID * /*rgDispId*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT Invoke(DISPID /*dispIdMember*/, const IID & /*riid*/, LCID /*lcid*/, WORD /*wFlags*/,
                   DISPPARAMS * /*pDispParams*/, VARIANT * /*pVarResult*/,
                   EXCEPINFO * /*pExcepInfo*/, UINT * /*puArgErr*/) override
    {
        return E_NOTIMPL;
    }
};

#endif
