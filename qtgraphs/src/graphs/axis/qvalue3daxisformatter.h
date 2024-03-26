// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QVALUE3DAXISFORMATTER_H
#define QVALUE3DAXISFORMATTER_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtCore/QList>
#include <QtCore/QLocale>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

class QValue3DAxisFormatterPrivate;
class QValue3DAxis;

class Q_GRAPHS_EXPORT QValue3DAxisFormatter : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QValue3DAxisFormatter)

protected:
    explicit QValue3DAxisFormatter(QValue3DAxisFormatterPrivate *d, QObject *parent = nullptr);
public:
    explicit QValue3DAxisFormatter(QObject *parent = nullptr);
    virtual ~QValue3DAxisFormatter();

private Q_SLOTS:
    void markDirtyNoLabelChange();

protected:
    void setAllowNegatives(bool allow);
    bool allowNegatives() const;
    void setAllowZero(bool allow);
    bool allowZero() const;
    void setAxis(QValue3DAxis *axis);

    virtual QValue3DAxisFormatter *createNewInstance() const;
    virtual void recalculate();
    virtual QString stringForValue(qreal value, const QString &format);
    virtual float positionAt(float value) const;
    virtual float valueAt(float position) const;
    virtual void populateCopy(QValue3DAxisFormatter &copy);

    void markDirty(bool labelsChange = false);
    QValue3DAxis *axis() const;

    QList<float> &gridPositions();
    QList<float> &subGridPositions();
    QList<float> &labelPositions();
    QStringList &labelStrings();

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    QScopedPointer<QValue3DAxisFormatterPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QValue3DAxisFormatter)

    friend class Abstract3DController;
    friend class QValue3DAxisFormatterPrivate;
    friend class QValue3DAxis;
    friend class QValue3DAxisPrivate;
    friend class AxisHelper;
    friend class QBar3DSeriesPrivate;
    friend class QScatter3DSeriesPrivate;
    friend class QSurface3DSeriesPrivate;
    friend class QQuickGraphsItem;
    friend class QQuickGraphsScatter;
    friend class QQuickGraphsBars;
};

QT_END_NAMESPACE

#endif
