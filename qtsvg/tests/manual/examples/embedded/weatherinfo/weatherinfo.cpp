// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>
#include <QtSvgWidgets>

#define GET_DATA_ATTR(val) xml.attributes().value(val).toString()
#define GET_DATETIME(val) QDateTime::fromString(val, "yyyy-MM-ddThh:mm:ss")
#define FORMAT_TEMPERATURE(val) val + QChar(176) + "C"
#define TEXTCOLOR palette().color(QPalette::WindowText)

class WeatherInfo: public QMainWindow
{
    Q_OBJECT

private:

    QGraphicsView *m_view;
    QGraphicsScene m_scene;
    QString city;
    QGraphicsRectItem *m_statusItem;
    QGraphicsTextItem *m_temperatureItem;
    QGraphicsTextItem *m_conditionItem;
    QGraphicsTextItem *m_cityItem;
    QGraphicsTextItem *m_copyright;
    QGraphicsSvgItem *m_iconItem;
    QList<QGraphicsRectItem*> m_forecastItems;
    QList<QGraphicsTextItem*> m_dayItems;
    QList<QGraphicsSvgItem*> m_conditionItems;
    QList<QGraphicsTextItem*> m_rangeItems;
    QTimeLine m_timeLine;
    QHash<QString, QString> m_icons;
    QNetworkAccessManager m_manager;

public:
    WeatherInfo(QWidget *parent = 0): QMainWindow(parent) {

        m_view = new QGraphicsView(this);
        setCentralWidget(m_view);

        setupScene();
        m_view->setScene(&m_scene);
        m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        m_view->setFrameShape(QFrame::NoFrame);
        setWindowTitle("Weather Info");

        QStringList cities;
        cities << "Oslo";
        cities << "Berlin";
        cities << "Moscow";
        cities << "Helsinki";
        cities << "Santa Clara";
        for (int i = 0; i < cities.count(); ++i) {
            QAction *action = new QAction(cities[i], this);
            connect(action, SIGNAL(triggered()), SLOT(chooseCity()));
            addAction(action);
        }
        setContextMenuPolicy(Qt::ActionsContextMenu);

        connect(&m_manager, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(handleNetworkData(QNetworkReply*)));

        QTimer::singleShot(0, this, SLOT(delayedInit()));
    }

private slots:
    void delayedInit() {
        request("http://www.yr.no/place/Norge/Oslo/Oslo/Oslo/varsel.xml");
    }

private slots:

    void chooseCity() {
        QAction *action = qobject_cast<QAction*>(sender());
        if (action) {
            if (action->text() == "Oslo") {
                request("http://www.yr.no/place/Norge/Oslo/Oslo/Oslo/varsel.xml");
            } else if (action->text() == "Berlin") {
                request("http://www.yr.no/place/Germany/Berlin/Berlin/varsel.xml");
            } else if (action->text() == "Moscow") {
                request("http://www.yr.no/place/Russia/Moscow/Moscow/varsel.xml");
            } else if (action->text() == "Helsinki") {
                request("http://www.yr.no/place/Finland/Southern_Finland/Helsinki/varsel.xml");
            } else if (action->text() == "Santa Clara") {
                request("http://www.yr.no/place/United_States/California/Santa_Clara/varsel.xml");
            }
        }
    }

    void handleNetworkData(QNetworkReply *networkReply) {
        QUrl url = networkReply->url();
        if (!networkReply->error())
            digest(QString::fromUtf8(networkReply->readAll()));
        networkReply->deleteLater();
    }

    void animate(int frame) {
        qreal progress = static_cast<qreal>(frame) / 100;
        m_iconItem->setOpacity(progress);
        qreal hw = width() / 2.0;
        m_statusItem->setPos(-hw + hw * progress, 0);
        for (int i = 0; i < m_forecastItems.count(); ++i) {
            qreal ofs = i * 0.5 / m_forecastItems.count();
            qreal alpha = qBound(qreal(0), 2 * (progress - ofs), qreal(1));
            m_conditionItems[i]->setOpacity(alpha);
            QPointF pos = m_forecastItems[i]->pos();
            if (width() > height()) {
                qreal fx = width() - width() * 0.4 * alpha;
                m_forecastItems[i]->setPos(fx, pos.y());
            } else {
                qreal fx = height() - height() * 0.5 * alpha;
                m_forecastItems[i]->setPos(pos.x(), fx);
            }
        }
    }

private:

    void setupScene() {
        QFont textFont = font();
        textFont.setBold(true);
        textFont.setPointSize(static_cast<int>(textFont.pointSize() * 1.5));

        m_temperatureItem = m_scene.addText(QString(), textFont);
        m_temperatureItem->setDefaultTextColor(TEXTCOLOR);

        m_conditionItem = m_scene.addText(QString(), textFont);
        m_conditionItem->setDefaultTextColor(TEXTCOLOR);

        m_cityItem = m_scene.addText(QString(), textFont);
        m_cityItem->setDefaultTextColor(TEXTCOLOR);

        m_copyright = m_scene.addText(QString());
        m_copyright->setDefaultTextColor(TEXTCOLOR);
        m_copyright->setOpenExternalLinks(true);
        m_copyright->setTextInteractionFlags(Qt::TextBrowserInteraction);

        m_iconItem = new QGraphicsSvgItem;
        m_scene.addItem(m_iconItem);

        m_statusItem = m_scene.addRect(0, 0, 10, 10);
        m_statusItem->setPen(Qt::NoPen);
        m_statusItem->setBrush(Qt::NoBrush);
        m_temperatureItem->setParentItem(m_statusItem);
        m_conditionItem->setParentItem(m_statusItem);
        m_iconItem->setParentItem(m_statusItem);
        m_copyright->setParentItem(m_statusItem);

        connect(&m_timeLine, SIGNAL(frameChanged(int)), SLOT(animate(int)));
        m_timeLine.setDuration(1100);
        m_timeLine.setFrameRange(0, 100);
        m_timeLine.setEasingCurve(QEasingCurve::InCurve);
    }

    void request(const QString &location) {
        QUrl url(location);
        m_manager.get(QNetworkRequest(url));

        city = QString();
        setWindowTitle("Loading...");
    }

    QString extractIcon(const QString &data) {
        if (m_icons.isEmpty()) {
            m_icons["Partly cloudy"]    = "weather-few-clouds";
            m_icons["Cloudy"]           = "weather-overcast";
            m_icons["Fair"]             = "weather-sunny-very-few-clouds";
            m_icons["Sun"]              = "weather-sunny";
            m_icons["Sun/clear sky"]    = "weather-sunny";
            m_icons["Clear sky"]    = "weather-sunny";
            m_icons["Snow showers"]     = "weather-snow";
            m_icons["Snow"]             = "weather-snow";
            m_icons["Fog"]              = "weather-fog";
            m_icons["Sleet"]            = "weather-sleet";
            m_icons["Sleet showers"]    = "weather-sleet";
            m_icons["Rain showers"]     = "weather-showers";
            m_icons["Rain"]             = "weather-showers";
            m_icons["Heavy rain"]       = "weather-showers";
            m_icons["Rain showers with thunder"]  = "weather-thundershower";
            m_icons["Rain and thunder"]           = "weather-thundershower";
            m_icons["Sleet and thunder"]          = "weather-thundershower";
            m_icons["Heavy rain and thunder"]     = "weather-thundershower";
            m_icons["Snow and thunder"]           = "weather-thundershower";
            m_icons["Sleet showers and thunder"]  = "weather-thundershower";
            m_icons["Snow showers and thunder"]   = "weather-thundershower";
        }
        QString name = m_icons.value(data);
        if (!name.isEmpty()) {
            name.prepend(":/icons/");
            name.append(".svg");
            return name;
        }
        return QString();
    }

    void digest(const QString &data) {
        delete m_iconItem;
        m_iconItem = new QGraphicsSvgItem();
        m_scene.addItem(m_iconItem);
        m_iconItem->setParentItem(m_statusItem);
        m_conditionItem->setPlainText(QString());

        qDeleteAll(m_dayItems);
        qDeleteAll(m_conditionItems);
        qDeleteAll(m_rangeItems);
        qDeleteAll(m_forecastItems);
        m_dayItems.clear();
        m_conditionItems.clear();
        m_rangeItems.clear();
        m_forecastItems.clear();

        QXmlStreamReader xml(data);

        bool foundCurrentForecast = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement) {
                if (xml.name() == u"location") {
                    while (!xml.atEnd()) {
                        xml.readNext();
                        if (xml.tokenType() == QXmlStreamReader::StartElement) {
                            if (xml.name() == u"name") {
                                city = xml.readElementText();
                                m_cityItem->setPlainText(city);
                                setWindowTitle(city);
                                xml.skipCurrentElement();
                                break;
                            }
                        }
                    }
                } else if (xml.name() == u"credit") {
                    while (!xml.atEnd()) {
                        xml.readNext();
                        if (xml.tokenType() == QXmlStreamReader::StartElement) {
                            if (xml.name() == u"link") {
                                m_copyright->setHtml(QString("<td align=\"center\">%1 <a href=\"%2\">(source)</a></td>").arg(GET_DATA_ATTR("text")).arg(GET_DATA_ATTR("url")));
                                xml.skipCurrentElement();
                                break;
                            }
                        }
                    }
                } else if (xml.name() == u"tabular") {
                    while (!xml.atEnd()) {
                        xml.readNext();
                        if (xml.tokenType() == QXmlStreamReader::StartElement) {
                            if (xml.name() == u"time") {
                                if (!foundCurrentForecast) {
                                    QString temperature;
                                    QString symbol;
                                    getSymbolTemp(xml, symbol, temperature);
                                    if (!symbol.isEmpty()) {
                                        delete m_iconItem;
                                        m_iconItem = new QGraphicsSvgItem(symbol);
                                        m_scene.addItem(m_iconItem);
                                        m_iconItem->setParentItem(m_statusItem);
                                    }
                                    QString s = FORMAT_TEMPERATURE(temperature);
                                    m_temperatureItem->setPlainText(s);
                                    foundCurrentForecast = true;
                                } else {
                                    createNewDay(xml);
                                }

                            }
                        }
                    }
                } else if (xml.name() != u"weatherdata" && xml.name() != u"forecast" && xml.name() != u"credit"){
                    xml.skipCurrentElement();
                }
            }
        }





        m_timeLine.stop();
        layoutItems();
        animate(0);
        m_timeLine.start();
    }

    void createNewDay(QXmlStreamReader &xml) {
        QGraphicsTextItem *dayItem  = nullptr;
        QString lowT;
        QString highT;
        QString period = GET_DATA_ATTR("period");
        QString datetime;
        if (period == "0")
            datetime = GET_DATA_ATTR("to");
        else
            datetime = GET_DATA_ATTR("from");
        QString temperature;
        QString symbol;
        getSymbolTemp(xml, symbol, temperature);
        lowT = highT = temperature;
        QDateTime date = GET_DATETIME(datetime);
        dayItem = m_scene.addText(date.date().toString("ddd"));
        dayItem->setDefaultTextColor(TEXTCOLOR);

        // check for other info same day
        bool saved = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement) {
                if (xml.name() == u"time") {
                    QString period = GET_DATA_ATTR("period");
                    // save data if new day starts
                    if (period == "0") {
                        saveDayItem(dayItem, lowT, highT, symbol);
                        createNewDay(xml);
                        saved = true;
                    } else {
                        updateDay(xml, lowT, highT, symbol, period == "2");
                    }
                }
            }
        }
        if (!saved)// last Item
            saveDayItem(dayItem, lowT, highT, symbol);
    }

    void updateDay(QXmlStreamReader &xml, QString &lowT, QString &highT, QString &symbolToShow, bool updateSymbol) {
        QString temperature;
        QString symbol;
        getSymbolTemp(xml, symbol, temperature);
        if (lowT.toFloat() > temperature.toFloat())
            lowT = temperature;
        if (highT.toFloat() < temperature.toFloat())
            highT = temperature;
        if (updateSymbol)
            symbolToShow = symbol;
    }

    void saveDayItem(QGraphicsTextItem *dayItem, QString lowT, QString highT, QString symbolToShow) {
        QGraphicsSvgItem *statusItem = nullptr;
        if (!symbolToShow.isEmpty()) {
            statusItem = new QGraphicsSvgItem(symbolToShow);
            m_scene.addItem(statusItem);
        }
        if (m_dayItems.count() < 4 && dayItem && statusItem &&  // Show 4 days
                !lowT.isEmpty() && !highT.isEmpty()) {
            m_dayItems << dayItem;
            m_conditionItems << statusItem;
            QString txt = FORMAT_TEMPERATURE(lowT) + '/' + FORMAT_TEMPERATURE(highT);
            QGraphicsTextItem* rangeItem;
            rangeItem = m_scene.addText(txt);
            rangeItem->setDefaultTextColor(TEXTCOLOR);
            m_rangeItems << rangeItem;
            QGraphicsRectItem *box;
            box = m_scene.addRect(0, 0, 10, 10);
            box->setPen(Qt::NoPen);
            box->setBrush(Qt::NoBrush);
            m_forecastItems << box;
            dayItem->setParentItem(box);
            statusItem->setParentItem(box);
            rangeItem->setParentItem(box);
        } else {
            delete dayItem;
            delete statusItem;
        }
    }

    void getSymbolTemp(QXmlStreamReader &xml, QString &symbol, QString &temp) {
        bool foundIcon = false;
        bool foundTemp = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement) {
                if (xml.name() == u"symbol") {
                    QString condition = GET_DATA_ATTR("name");
                    symbol = extractIcon(condition);
                    if (m_conditionItem->toPlainText().isEmpty())
                        m_conditionItem->setPlainText(condition);
                    foundIcon = true;
                }
                if (xml.name() == u"temperature") {
                    temp = GET_DATA_ATTR("value");
                    foundTemp = true;
                }
                if (foundIcon && foundTemp)
                    break;
            }
        }
    }


    void layoutItems() {
        m_scene.setSceneRect(0, 0, width() - 1, height() - 1);
        m_view->centerOn(width() / 2, height() / 2);
        if (width() > height())
            layoutItemsLandscape();
        else
            layoutItemsPortrait();
    }

    void layoutItemsLandscape() {
        qreal statusItemWidth = width() / 2 - 1;
        m_statusItem->setRect(0, 0, statusItemWidth, height() - 1);

        m_temperatureItem->setPos(10, 2);
        qreal wtemp = m_temperatureItem->boundingRect().width();
        qreal h1 = m_conditionItem->boundingRect().height();
        m_conditionItem->setPos(wtemp + 20, 2);

        m_copyright->setTextWidth(statusItemWidth);

        qreal wcity = m_cityItem->boundingRect().width();
        m_cityItem->setPos(statusItemWidth - wcity - 1, 2);;

        qreal h2 = m_copyright->boundingRect().height();
        m_copyright->setPos(0, height() - h2);

        if (!m_iconItem->boundingRect().isEmpty()) {
            qreal sizeLeft = qMin(statusItemWidth, height() - h2 - h1 - 10);
            qreal sw = sizeLeft / m_iconItem->boundingRect().width();
            qreal sh = sizeLeft / m_iconItem->boundingRect().height();
            m_iconItem->setTransform(QTransform().scale(sw, sh));
            m_iconItem->setPos(statusItemWidth/2 - sizeLeft/2, h1 + 5);
        }

        if (m_dayItems.count()) {
            qreal left = width() * 0.6;
            qreal statusWidth = 0;
            qreal rangeWidth = 0;
            qreal h = height() / m_dayItems.count();
            for (int i = 0; i < m_dayItems.count(); ++i) {
                QRectF brect = m_dayItems[i]->boundingRect();
                statusWidth = qMax(statusWidth, brect.width());
                brect = m_rangeItems[i]->boundingRect();
                rangeWidth = qMax(rangeWidth, brect.width());
            }
            qreal space = width() - left - statusWidth - rangeWidth;
            qreal dim = qMin(h, space);
            qreal pad = statusWidth + (space  - dim) / 2;
            for (int i = 0; i < m_dayItems.count(); ++i) {
                qreal base = h * i;
                m_forecastItems[i]->setPos(left, base);
                m_forecastItems[i]->setRect(0, 0, width() - left, h);
                QRectF brect = m_dayItems[i]->boundingRect();
                qreal ofs = (h - brect.height()) / 2;
                m_dayItems[i]->setPos(0, ofs);
                brect = m_rangeItems[i]->boundingRect();
                ofs = (h - brect.height()) / 2;
                m_rangeItems[i]->setPos(width() - rangeWidth - left, ofs);
                brect = m_conditionItems[i]->boundingRect();
                ofs = (h - dim) / 2;
                m_conditionItems[i]->setPos(pad, ofs);
                if (brect.isEmpty())
                    continue;
                qreal sw = dim / brect.width();
                qreal sh = dim / brect.height();
                m_conditionItems[i]->setTransform(QTransform().scale(sw, sh));
            }
        }
    }

    void layoutItemsPortrait() {
        qreal statusItemWidth = width() - 1;
        m_statusItem->setRect(0, 0, statusItemWidth, height() / 2 - 1);

        m_temperatureItem->setPos(10, 2);
        qreal wtemp = m_temperatureItem->boundingRect().width();
        qreal h1 = m_conditionItem->boundingRect().height();
        m_conditionItem->setPos(wtemp + 20, 2);

        m_copyright->setTextWidth(statusItemWidth);

        qreal wcity = m_cityItem->boundingRect().width();
        m_cityItem->setPos(statusItemWidth - wcity - 1, 2);;

        m_copyright->setTextWidth(statusItemWidth);
        qreal h2 = m_copyright->boundingRect().height();
        m_copyright->setPos(0, height() - h2);

        if (m_dayItems.count()) {
            qreal top = height() * 0.5;
            qreal w = width() / m_dayItems.count();
            qreal statusHeight = 0;
            qreal rangeHeight = 0;
            for (int i = 0; i < m_dayItems.count(); ++i) {
                m_dayItems[i]->setFont(font());
                QRectF brect = m_dayItems[i]->boundingRect();
                statusHeight = qMax(statusHeight, brect.height());
                brect = m_rangeItems[i]->boundingRect();
                rangeHeight = qMax(rangeHeight, brect.height());
            }
            qreal space = height() - top - statusHeight - rangeHeight;
            qreal dim = qMin(w, space);

            qreal boxh = statusHeight + rangeHeight + dim;
            qreal pad = (height() - top - boxh) / 2;

            if (!m_iconItem->boundingRect().isEmpty()) {
                qreal sizeLeft = qMin(statusItemWidth - 10, height() - top - 10);
                qreal sw = sizeLeft / m_iconItem->boundingRect().width();
                qreal sh = sizeLeft / m_iconItem->boundingRect().height();
                m_iconItem->setTransform(QTransform().scale(sw, sh));
                m_iconItem->setPos(statusItemWidth/2 - sizeLeft/2, h1 + 5);
            }

            for (int i = 0; i < m_dayItems.count(); ++i) {
                qreal base = w * i;
                m_forecastItems[i]->setPos(base, top);
                m_forecastItems[i]->setRect(0, 0, w, boxh);
                QRectF brect = m_dayItems[i]->boundingRect();
                qreal ofs = (w - brect.width()) / 2;
                m_dayItems[i]->setPos(ofs, pad);

                brect = m_rangeItems[i]->boundingRect();
                ofs = (w - brect.width()) / 2;
                m_rangeItems[i]->setPos(ofs, pad + statusHeight + dim);

                brect = m_conditionItems[i]->boundingRect();
                ofs = (w - dim) / 2;
                m_conditionItems[i]->setPos(ofs, pad + statusHeight);
                if (brect.isEmpty())
                    continue;
                qreal sw = dim / brect.width();
                qreal sh = dim / brect.height();
                m_conditionItems[i]->setTransform(QTransform().scale(sw, sh));
            }
        }
    }


    void resizeEvent(QResizeEvent *event) override {
        Q_UNUSED(event);
        layoutItems();
    }

};

#include "weatherinfo.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    WeatherInfo w;
    w.resize(520, 288);
    w.show();

    return app.exec();
}
