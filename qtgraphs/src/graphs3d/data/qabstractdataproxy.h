// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QABSTRACTDATAPROXY_H
#define QABSTRACTDATAPROXY_H

#if 0
#  pragma qt_class(QAbstractDataProxy)
#endif

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtGraphs/qgraphsglobal.h>

QT_BEGIN_NAMESPACE

class QAbstractDataProxyPrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QAbstractDataProxy : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractDataProxy)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(QAbstractDataProxy::DataType type READ type CONSTANT)

public:
    enum class DataType { None, Bar, Scatter, Surface };
    Q_ENUM(DataType)

protected:
    explicit QAbstractDataProxy(QAbstractDataProxyPrivate &d, QObject *parent = nullptr);

public:
    ~QAbstractDataProxy() override;

    QAbstractDataProxy::DataType type() const;

private:
    Q_DISABLE_COPY(QAbstractDataProxy)

    friend class QAbstract3DSeriesPrivate;
};

QT_END_NAMESPACE

#endif
