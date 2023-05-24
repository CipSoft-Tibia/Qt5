// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QAxBindable>
#include <QAxFactory>
#include <QMainWindow>
#include <QQuickWidget>
#include <QQmlContext>

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QColor color READ color NOTIFY valueChanged)
public:
    explicit Controller(QWidget *parent = nullptr) :
        QObject(parent)
    { }

    qreal value() const { return m_value; }

    void setValue(qreal value)
    {
        m_value = qBound(qreal(0.0), value, qreal(1.0));
        valueChanged();
    }

    QColor color() const
    {
        QColor start = Qt::yellow;
        QColor end = Qt::magenta;

        // Linear interpolation between two colors in HSV space
        return QColor::fromHsvF(
            start.hueF()        * (1.0f - m_value) + end.hueF()        * m_value,
            start.saturationF() * (1.0f - m_value) + end.saturationF() * m_value,
            start.valueF()      * (1.0f - m_value) + end.valueF()      * m_value,
            start.alphaF()      * (1.0f - m_value) + end.alphaF()      * m_value
        );
    }

signals:
    void valueChanged();

private:
    qreal m_value = 0;
};

class QSimpleQmlAx : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("ClassID", "{50477337-58FE-4898-8FFC-6F6199CEAE08}")
    Q_CLASSINFO("InterfaceID", "{A5EC7D99-CEC9-4BD1-8336-ED15A579B185}")
    Q_CLASSINFO("EventsID", "{5BBFBCFD-20FD-48A3-96C7-1F6649CD1F52}")
public:
    explicit QSimpleQmlAx(QWidget *parent = nullptr) :
        QMainWindow(parent)
    {
        auto ui = new QQuickWidget(this);

        // Register our type to qml
        qmlRegisterType<Controller>("app", 1, 0, "Controller");

        // Initialize view
        ui->rootContext()->setContextProperty(QStringLiteral("context"), QVariant::fromValue(new Controller(this)));
        ui->setMinimumSize(200, 200);
        ui->setResizeMode(QQuickWidget::SizeRootObjectToView);
        ui->setSource(QUrl(QStringLiteral("qrc:/main.qml")));
        setCentralWidget(ui);
    }
};

#include "main.moc"

QAXFACTORY_BEGIN(
    "{E544E321-EF8B-4CD4-91F6-DB55A59DBADB}", // type library ID
    "{E37E3131-DEA2-44EB-97A2-01CDD09A5A4D}") // application ID
    QAXCLASS(QSimpleQmlAx)
QAXFACTORY_END()
