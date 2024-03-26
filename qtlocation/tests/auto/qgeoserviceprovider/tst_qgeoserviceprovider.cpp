// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>
#include <QDebug>
#include <QTest>
#include <QtLocation/QGeoServiceProvider>

QT_USE_NAMESPACE

class tst_QGeoServiceProvider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void tst_availableServiceProvider();
    void tst_features_data();
    void tst_features();
    void tst_misc();
    void tst_nokiaRename();

private:
    QStringList providerList;
};

void tst_QGeoServiceProvider::initTestCase()
{
#if QT_CONFIG(library)
    /*
     * Set custom path since CI doesn't install test plugins
     */
#ifdef Q_OS_WIN
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath() +
                                     QStringLiteral("/../../../../plugins"));
#else
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath()
                                     + QStringLiteral("/../../../plugins"));
#endif
#endif
    providerList = QGeoServiceProvider::availableServiceProviders();
    qInfo() << "Supported geoservice providers:" << providerList;
}

void tst_QGeoServiceProvider::tst_availableServiceProvider()
{
    // Currently provided plugins
    QVERIFY(providerList.count() >= 5);
    // these providers are deployed
    QVERIFY(providerList.contains(QStringLiteral("osm")));
    // these providers exist for unit tests only
    QVERIFY(providerList.contains(QStringLiteral("geocode.test.plugin")));
    QVERIFY(providerList.contains(QStringLiteral("georoute.test.plugin")));
    QVERIFY(providerList.contains(QStringLiteral("qmlgeo.test.plugin")));
    QVERIFY(providerList.contains(QStringLiteral("test.places.unsupported")));

}

Q_DECLARE_METATYPE(QGeoServiceProvider::MappingFeatures)
Q_DECLARE_METATYPE(QGeoServiceProvider::GeocodingFeatures)
Q_DECLARE_METATYPE(QGeoServiceProvider::RoutingFeatures)
Q_DECLARE_METATYPE(QGeoServiceProvider::PlacesFeatures)

void tst_QGeoServiceProvider::tst_features_data()
{
    QTest::addColumn<QString>("providerName");
    QTest::addColumn<QGeoServiceProvider::MappingFeatures>("mappingFeatures");
    QTest::addColumn<QGeoServiceProvider::GeocodingFeatures>("codingFeatures");
    QTest::addColumn<QGeoServiceProvider::RoutingFeatures>("routingFeatures");
    QTest::addColumn<QGeoServiceProvider::PlacesFeatures>("placeFeatures");

    QTest::newRow("invalid") << QString("non-existing-provider-name")
                             << QGeoServiceProvider::MappingFeatures(QGeoServiceProvider::NoMappingFeatures)
                             << QGeoServiceProvider::GeocodingFeatures(QGeoServiceProvider::NoGeocodingFeatures)
                             << QGeoServiceProvider::RoutingFeatures(QGeoServiceProvider::NoRoutingFeatures)
                             << QGeoServiceProvider::PlacesFeatures(QGeoServiceProvider::NoPlacesFeatures);

    if (providerList.contains("mapbox")) {
        QTest::newRow("mapbox") << QString("mapbox")
                                << QGeoServiceProvider::MappingFeatures(QGeoServiceProvider::OnlineMappingFeature)
                                << QGeoServiceProvider::GeocodingFeatures(QGeoServiceProvider::OnlineGeocodingFeature
                                                                        | QGeoServiceProvider::ReverseGeocodingFeature
                                                                        | QGeoServiceProvider::LocalizedGeocodingFeature)
                                << QGeoServiceProvider::RoutingFeatures(QGeoServiceProvider::OnlineRoutingFeature)
                                << QGeoServiceProvider::PlacesFeatures(QGeoServiceProvider::OnlinePlacesFeature
                                                                    | QGeoServiceProvider::PlaceRecommendationsFeature
                                                                    | QGeoServiceProvider::SearchSuggestionsFeature
                                                                    | QGeoServiceProvider::LocalizedPlacesFeature);
    }

    if (providerList.contains("here")) {
        QTest::newRow("here")   << QString("here")
                                << QGeoServiceProvider::MappingFeatures(QGeoServiceProvider::OnlineMappingFeature)
                                << QGeoServiceProvider::GeocodingFeatures(QGeoServiceProvider::OnlineGeocodingFeature
                                                                        | QGeoServiceProvider::ReverseGeocodingFeature)
                                << QGeoServiceProvider::RoutingFeatures(QGeoServiceProvider::OnlineRoutingFeature
                                                                        | QGeoServiceProvider::RouteUpdatesFeature
                                                                        | QGeoServiceProvider::AlternativeRoutesFeature
                                                                        | QGeoServiceProvider::ExcludeAreasRoutingFeature)
                                << QGeoServiceProvider::PlacesFeatures(QGeoServiceProvider::OnlinePlacesFeature
                                                                    | QGeoServiceProvider::PlaceRecommendationsFeature
                                                                    | QGeoServiceProvider::SearchSuggestionsFeature
                                                                    | QGeoServiceProvider::LocalizedPlacesFeature);
    }

    if (providerList.contains("osm")) {
        QTest::newRow("osm")    << QString("osm")
                                << QGeoServiceProvider::MappingFeatures(QGeoServiceProvider::OnlineMappingFeature)
                                << QGeoServiceProvider::GeocodingFeatures(QGeoServiceProvider::OnlineGeocodingFeature
                                                                        | QGeoServiceProvider::ReverseGeocodingFeature)
                                << QGeoServiceProvider::RoutingFeatures(QGeoServiceProvider::OnlineRoutingFeature)
                                << QGeoServiceProvider::PlacesFeatures(QGeoServiceProvider::OnlinePlacesFeature);
    }

    if (providerList.contains("esri")) {
        QTest::newRow("esri")   << QString("esri")
                                << QGeoServiceProvider::MappingFeatures(QGeoServiceProvider::OnlineMappingFeature)
                                << QGeoServiceProvider::GeocodingFeatures(QGeoServiceProvider::OnlineGeocodingFeature
                                                                        | QGeoServiceProvider::ReverseGeocodingFeature)
                                << QGeoServiceProvider::RoutingFeatures(QGeoServiceProvider::OnlineRoutingFeature)
                                << QGeoServiceProvider::PlacesFeatures(QGeoServiceProvider::OnlinePlacesFeature);
    }
}

void tst_QGeoServiceProvider::tst_features()
{
    QFETCH(QString, providerName);
    QFETCH(QGeoServiceProvider::MappingFeatures, mappingFeatures);
    QFETCH(QGeoServiceProvider::GeocodingFeatures, codingFeatures);
    QFETCH(QGeoServiceProvider::RoutingFeatures, routingFeatures);
    QFETCH(QGeoServiceProvider::PlacesFeatures, placeFeatures);

    QGeoServiceProvider provider(providerName);
    QCOMPARE(provider.mappingFeatures(), mappingFeatures);
    QCOMPARE(provider.geocodingFeatures(), codingFeatures);
    QCOMPARE(provider.routingFeatures(), routingFeatures);
    QCOMPARE(provider.placesFeatures(), placeFeatures);

    if (provider.mappingFeatures() == QGeoServiceProvider::NoMappingFeatures) {
        QVERIFY(provider.mappingManager() == nullptr);
    } else {
        // some plugins require token/access parameter
        // they return 0 but set QGeoServiceProvider::MissingRequiredParameterError
        if (provider.mappingManager() != nullptr)
            QCOMPARE(provider.error(), QGeoServiceProvider::NoError);
        else
            QCOMPARE(provider.error(), QGeoServiceProvider::MissingRequiredParameterError);
    }

    if (provider.geocodingFeatures() == QGeoServiceProvider::NoGeocodingFeatures) {
        QVERIFY(provider.geocodingManager() == nullptr);
    } else {
        if (provider.geocodingManager() != nullptr)
            QVERIFY(provider.geocodingManager() != nullptr); //pointless but we want a VERIFY here
        else
            QCOMPARE(provider.error(), QGeoServiceProvider::MissingRequiredParameterError);
    }

    if (provider.routingFeatures() == QGeoServiceProvider::NoRoutingFeatures) {
        QVERIFY(provider.routingManager() == nullptr);
    } else {
        if (provider.routingManager() != nullptr)
            QCOMPARE(provider.error(), QGeoServiceProvider::NoError);
        else
            QCOMPARE(provider.error(), QGeoServiceProvider::MissingRequiredParameterError);
    }

    if (provider.placesFeatures() == QGeoServiceProvider::NoPlacesFeatures) {
        QVERIFY(provider.placeManager() == nullptr);
    } else {
        if (provider.placeManager() != nullptr)
            QCOMPARE(provider.error(), QGeoServiceProvider::NoError);
        else
            QCOMPARE(provider.error(), QGeoServiceProvider::MissingRequiredParameterError);
    }
}

void tst_QGeoServiceProvider::tst_misc()
{
    const QStringList provider = QGeoServiceProvider::availableServiceProviders();
    QVERIFY(provider.contains(QStringLiteral("osm")));
    QVERIFY(provider.contains(QStringLiteral("geocode.test.plugin")));

    QGeoServiceProvider test_experimental(
                QStringLiteral("geocode.test.plugin"), QVariantMap(), true);
    QGeoServiceProvider test_noexperimental(
                QStringLiteral("geocode.test.plugin"), QVariantMap(), false);
    QCOMPARE(test_experimental.error(), QGeoServiceProvider::NoError);
    QCOMPARE(test_noexperimental.error(), QGeoServiceProvider::NotSupportedError);

    QGeoServiceProvider osm_experimental(
                QStringLiteral("osm"), QVariantMap(), true);
    QGeoServiceProvider osm_noexperimental(
                QStringLiteral("osm"), QVariantMap(), false);
    QCOMPARE(osm_experimental.error(), QGeoServiceProvider::NoError);
    QCOMPARE(osm_noexperimental.error(), QGeoServiceProvider::NoError);
}

void tst_QGeoServiceProvider::tst_nokiaRename()
{
    // The "nokia" plugin was renamed to "here".
    // It remains available under the name "nokia" for now
    // but is not advertised via QGeoServiceProvider::availableServiceProviders()

    if (providerList.contains("here")) {
        QVERIFY(!QGeoServiceProvider::availableServiceProviders().contains("nokia"));
        QGeoServiceProvider provider(QStringLiteral("nokia"));
        QCOMPARE(provider.error(), QGeoServiceProvider::NoError);
    }
}

QTEST_GUILESS_MAIN(tst_QGeoServiceProvider)

#include "tst_qgeoserviceprovider.moc"
