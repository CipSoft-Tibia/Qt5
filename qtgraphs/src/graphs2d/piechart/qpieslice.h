// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTGRAPHS_QPIESLICE_H
#define QTGRAPHS_QPIESLICE_H

#include <QtCore/qobject.h>
#include <QtGraphs/qgraphsglobal.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

class PieRenderer;
class QPieSlicePrivate;
class QPieSeries;

class Q_GRAPHS_EXPORT QPieSlice : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    Q_PROPERTY(bool labelVisible READ isLabelVisible WRITE setLabelVisible NOTIFY
                   labelVisibleChanged FINAL)
    Q_PROPERTY(LabelPosition labelPosition READ labelPosition WRITE setLabelPosition NOTIFY
                   labelPositionChanged FINAL)
    Q_PROPERTY(QColor labelColor READ labelColor WRITE setLabelColor NOTIFY labelColorChanged FINAL)
    Q_PROPERTY(QFont labelFont READ labelFont WRITE setLabelFont NOTIFY labelFontChanged FINAL)
    Q_PROPERTY(qreal labelArmLengthFactor READ labelArmLengthFactor WRITE setLabelArmLengthFactor
                   NOTIFY labelArmLengthFactorChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(
        QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged FINAL)
    Q_PROPERTY(
        qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(bool exploded READ isExploded WRITE setExploded NOTIFY explodedChanged FINAL)
    Q_PROPERTY(qreal explodeDistanceFactor READ explodeDistanceFactor WRITE setExplodeDistanceFactor
                   NOTIFY explodeDistanceFactorChanged FINAL)
    Q_PROPERTY(qreal percentage READ percentage NOTIFY percentageChanged FINAL)
    Q_PROPERTY(qreal startAngle READ startAngle NOTIFY startAngleChanged FINAL)
    Q_PROPERTY(qreal angleSpan READ angleSpan NOTIFY angleSpanChanged FINAL)
    QML_NAMED_ELEMENT(PieSlice)

public:
    enum class LabelPosition {
        Outside,
        InsideHorizontal,
        InsideTangential,
        InsideNormal,
    };
    Q_ENUM(LabelPosition)

    explicit QPieSlice(QObject *parent = nullptr);
    QPieSlice(const QString &label, qreal value, QObject *parent = nullptr);
    ~QPieSlice() override;

    void setLabel(const QString &label);
    QString label() const;
    void setLabelVisible(bool visible = true);
    bool isLabelVisible() const;
    void setLabelPosition(LabelPosition position);
    LabelPosition labelPosition();
    void setLabelColor(QColor color);
    QColor labelColor() const;
    void setLabelFont(const QFont &font);
    QFont labelFont() const;
    void setLabelArmLengthFactor(qreal factor);
    qreal labelArmLengthFactor() const;

    void setColor(QColor color);
    QColor color() const;

    void setBorderColor(QColor color);
    QColor borderColor() const;
    void setBorderWidth(qreal borderWidth);
    qreal borderWidth() const;

    void setValue(qreal value);
    qreal value() const;

    void setExploded(bool exploded);
    bool isExploded() const;
    void setExplodeDistanceFactor(qreal factor);
    qreal explodeDistanceFactor() const;

    qreal percentage() const;
    qreal startAngle() const;
    qreal angleSpan() const;

    QPieSeries *series() const;

Q_SIGNALS:
    void labelChanged();
    void labelVisibleChanged();
    void labelFontChanged();
    void labelColorChanged();
    void valueChanged();
    void explodedChanged();
    void explodeDistanceFactorChanged();
    void percentageChanged();
    void startAngleChanged();
    void angleSpanChanged();
    void sliceChanged();
    void labelPositionChanged();
    void labelArmLengthFactorChanged();
    void colorChanged();
    void borderColorChanged();
    void borderWidthChanged();

private:
    friend class PieRenderer;
    friend class QPieSeries;
    friend class QPieSeriesPrivate;

    Q_DECLARE_PRIVATE(QPieSlice)
    Q_DISABLE_COPY(QPieSlice)
};

QT_END_NAMESPACE

#endif // QTGRAPHS_QPIESLICE_H
