// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QBARSET_H
#define QBARSET_H

#if 0
#  pragma qt_class(QBarSet)
#endif

#include <QtCore/qobject.h>
#include <QtGraphs/qgraphsglobal.h>
#include <QtGui/qbrush.h>
#include <QtGui/qfont.h>
#include <QtGui/qpen.h>
#include <QtQml/QQmlEngine>

QT_BEGIN_NAMESPACE

class QBarSetPrivate;

class QT_TECH_PREVIEW_API Q_GRAPHS_EXPORT QBarSet : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)
    Q_PROPERTY(QColor labelColor READ labelColor WRITE setLabelColor NOTIFY labelColorChanged)
    Q_PROPERTY(QVariantList values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QList<int> selectedBars READ selectedBars NOTIFY selectedBarsChanged)
    QML_NAMED_ELEMENT(BarSet)

public:
    explicit QBarSet(QObject *parent = nullptr);
    explicit QBarSet(const QString label, QObject *parent = nullptr);
    virtual ~QBarSet();

    void setLabel(const QString label);
    QString label() const;

    void append(const qreal value);
    void append(const QList<qreal> &values);

    QBarSet &operator << (const qreal &value);

    // TODO: Consider making these slots, available from QML.
    void insert(const int index, const qreal value);
    void remove(const int index, const int count = 1);
    void replace(const int index, const qreal value);
    qreal at(const int index) const;
    qreal operator [](const int index) const;
    int count() const;
    qreal sum() const;

    QColor color();
    void setColor(QColor color);

    QColor borderColor();
    void setBorderColor(QColor color);

    QColor labelColor();
    void setLabelColor(QColor color);

    QColor selectedColor() const;
    void setSelectedColor(const QColor &color);

    QVariantList values();
    void setValues(QVariantList values);
    qreal borderWidth() const;
    void setBorderWidth(qreal borderWidth);

    bool isBarSelected(int index) const;
    QList<int> selectedBars() const;

public Q_SLOTS:
    void selectBar(int index);
    void deselectBar(int index);
    void setBarSelected(int index, bool selected);
    void selectAllBars();
    void deselectAllBars();
    void selectBars(const QList<int> &indexes);
    void deselectBars(const QList<int> &indexes);
    void toggleSelection(const QList<int> &indexes);

Q_SIGNALS:
    void update();
    void labelChanged();
    void colorChanged(QColor color);
    void borderColorChanged(QColor color);
    void labelColorChanged(QColor color);
    void valuesChanged();
    void selectedColorChanged(const QColor &color);
    void countChanged();

    void borderWidthChanged(qreal width);

    void valuesAdded(int index, int count);
    void valuesRemoved(int index, int count);
    void valueChanged(int index);

    void selectedBarsChanged(const QList<int> &indexes);

private:
    QScopedPointer<QBarSetPrivate> d_ptr;
    Q_DISABLE_COPY(QBarSet)
    friend class QAbstractBarSeries;
    friend class QAbstractBarSeriesPrivate;
};

QT_END_NAMESPACE

#endif // QBARSET_H
