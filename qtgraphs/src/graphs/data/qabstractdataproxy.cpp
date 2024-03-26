// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qabstractdataproxy_p.h"
#include "qabstract3dseries_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QAbstractDataProxy
 * \inmodule QtGraphs
 * \brief The QAbstractDataProxy class is a base class for all graphs proxies.
 *
 * The following graphs type specific inherited classes are used instead
 * of the base class: QBarDataProxy, QScatterDataProxy, and QSurfaceDataProxy.
 *
 * For more information, see \l{Qt Graphs Data Handling}.
 */

/*!
 * \qmltype AbstractDataProxy
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates QAbstractDataProxy
 * \brief Base type for all QtGraphs data proxies.
 *
 * This type is uncreatable, but contains properties that are exposed via the
 * following subtypes: BarDataProxy, ScatterDataProxy, SurfaceDataProxy.
 *
 * For more information, see \l {Qt Graphs Data Handling}.
 */

/*!
 * \qmlproperty AbstractDataProxy.DataType AbstractDataProxy::type
 * The type of the proxy. One of the QAbstractDataProxy::DataType values.
 */

/*!
 * \enum QAbstractDataProxy::DataType
 *
 * This enum type specifies the data type of the proxy.
 *
 * \value DataTypeNone
 *        No data type.
 * \value DataTypeBar
 *        Data type for Q3DBars.
 * \value DataTypeScatter
 *        Data type for Q3DScatter.
 * \value DataTypeSurface
 *        Data type for Q3DSurface.
 */

/*!
 * \internal
 */
QAbstractDataProxy::QAbstractDataProxy(QAbstractDataProxyPrivate *d, QObject *parent) :
    QObject(parent),
    d_ptr(d)
{
}

/*!
 * Deletes the abstract data proxy.
 */
QAbstractDataProxy::~QAbstractDataProxy()
{
}

/*!
 * \property QAbstractDataProxy::type
 *
 * \brief The data type of the proxy.
 */
QAbstractDataProxy::DataType QAbstractDataProxy::type() const
{
    const Q_D(QAbstractDataProxy);
    return d->m_type;
}

// QAbstractDataProxyPrivate

QAbstractDataProxyPrivate::QAbstractDataProxyPrivate(QAbstractDataProxy *q,
                                                     QAbstractDataProxy::DataType type)
    : q_ptr(q),
      m_type(type),
      m_series(0)
{
}

QAbstractDataProxyPrivate::~QAbstractDataProxyPrivate()
{
}

void QAbstractDataProxyPrivate::setSeries(QAbstract3DSeries *series)
{
    q_ptr->setParent(series);
    m_series = series;
}

QT_END_NAMESPACE
