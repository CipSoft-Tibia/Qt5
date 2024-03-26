// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtGraphs API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef QABSTRACTDATAPROXY_P_H
#define QABSTRACTDATAPROXY_P_H

#include "qabstractdataproxy.h"

QT_BEGIN_NAMESPACE

class QAbstract3DSeries;

class QAbstractDataProxyPrivate
{
    Q_DECLARE_PUBLIC(QAbstractDataProxy)

public:
    QAbstractDataProxyPrivate(QAbstractDataProxy *q, QAbstractDataProxy::DataType type);
    virtual ~QAbstractDataProxyPrivate();

    inline QAbstract3DSeries *series() const { return m_series; }
    virtual void setSeries(QAbstract3DSeries *series);

protected:
    QAbstractDataProxy *q_ptr;
    QAbstractDataProxy::DataType m_type;
    QAbstract3DSeries *m_series;
};

QT_END_NAMESPACE

#endif
