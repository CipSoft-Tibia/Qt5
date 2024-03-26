// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QPLACEMANAGERENGINE_TEST_H
#define QPLACEMANAGERENGINE_TEST_H

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QUuid>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoLocation>
#include <QtLocation/QPlaceContentReply>
#include <QtLocation/QPlaceContentRequest>
#include <QtLocation/QPlaceManager>
#include <QtLocation/QPlaceManagerEngine>
#include <QtLocation/QPlaceReply>
#include <QtLocation/QPlaceDetailsReply>
#include <QtLocation/QPlaceIdReply>
#include <QtLocation/QPlaceSearchSuggestionReply>
#include <QtLocation/QPlaceSearchReply>
#include <QtLocation/QPlaceSearchRequest>
#include <QtLocation/QPlaceResult>
#include <QtLocation/QPlaceCategory>
#include <QtLocation/QPlace>
#include <QtLocation/private/qplace_p.h>
#include <QtTest/QTest>

QT_BEGIN_NAMESPACE

inline size_t qHash(const QPlaceCategory &category)
{
    return qHash(QUuid(category.categoryId().toLatin1()));
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

class QPlacePrivateDefaultAlt : public QPlacePrivateDefault
{
public:
    QPlacePrivateDefaultAlt() {}
    QPlacePrivateDefaultAlt(const QPlacePrivateDefaultAlt &other)
        : QPlacePrivateDefault(other)
    {
    }
    ~QPlacePrivateDefaultAlt() {}

    QPlaceAttribute extendedAttribute(const QString &attributeType) const override
    {
        if (attributeType == QStringLiteral("x_provider")) {
            QPlaceAttribute a;
            a.setLabel(QStringLiteral("x_provider"));
            a.setText(QStringLiteral("QPlacePrivateDefaultAlt"));
            return a;
        } else {
            return QPlacePrivateDefault::extendedAttribute(attributeType);
        }
    }
};

class QPlaceAlt : public QPlace
{
public:
    QPlaceAlt() : QPlace(QSharedDataPointer<QPlacePrivate>(new QPlacePrivateDefaultAlt()))
    {
    }
};

class PlaceReply : public QPlaceReply
{
    Q_OBJECT

    friend class QPlaceManagerEngineTest;

public:
    PlaceReply(QObject *parent = nullptr)
    :   QPlaceReply(parent)
    { }

    Q_INVOKABLE void emitFinished()
    {
        emit finished();
    }
};

class ContentReply : public QPlaceContentReply
{
    Q_OBJECT

    friend class QPlaceManagerEngineTest;

public:
    ContentReply(QObject *parent = nullptr)
    : QPlaceContentReply(parent)
    {}

    Q_INVOKABLE void emitError()
    {
        emit errorOccurred(error(), errorString());
    }

    Q_INVOKABLE void emitFinished()
    {
        emit finished();
    }
};

class DetailsReply : public QPlaceDetailsReply
{
    Q_OBJECT

    friend class QPlaceManagerEngineTest;

public:
    DetailsReply(QObject *parent = nullptr)
    :   QPlaceDetailsReply(parent)
    { }

    Q_INVOKABLE void emitError()
    {
        emit errorOccurred(error(), errorString());
    }

    Q_INVOKABLE void emitFinished()
    {
        emit finished();
    }
};

class IdReply : public QPlaceIdReply
{
    Q_OBJECT

    friend class QPlaceManagerEngineTest;

public:
    IdReply(QPlaceIdReply::OperationType type, QObject *parent = nullptr)
    :   QPlaceIdReply(type, parent)
    { }

    Q_INVOKABLE void emitError()
    {
        emit errorOccurred(error(), errorString());
    }

    Q_INVOKABLE void emitFinished()
    {
        emit finished();
    }
};

class PlaceSearchReply : public QPlaceSearchReply
{
    Q_OBJECT

public:
    PlaceSearchReply(const QList<QPlaceSearchResult> &results, QObject *parent = nullptr)
    :   QPlaceSearchReply(parent)
    {
        setResults(results);
    }

    Q_INVOKABLE void emitError()
    {
        emit errorOccurred(error(), errorString());
    }

    Q_INVOKABLE void emitFinished()
    {
        emit finished();
    }
};

class SuggestionReply : public QPlaceSearchSuggestionReply
{
    Q_OBJECT

public:
    SuggestionReply(const QStringList &suggestions, QObject *parent = nullptr)
    :   QPlaceSearchSuggestionReply(parent)
    {
        setSuggestions(suggestions);
    }

    Q_INVOKABLE void emitError()
    {
        emit errorOccurred(error(), errorString());
    }

    Q_INVOKABLE void emitFinished()
    {
        emit finished();
    }
};

class QPlaceManagerEngineTest : public QPlaceManagerEngine
{
    Q_OBJECT
public:
    QPlaceManagerEngineTest(const QVariantMap &parameters)
        : QPlaceManagerEngine(parameters)
    {
        m_locales << QLocale();
        if (parameters.value(QStringLiteral("initializePlaceData"), false).toBool()) {
            QFile placeData(QFINDTESTDATA("place_data.json"));
            QVERIFY(placeData.exists());
            if (placeData.open(QIODevice::ReadOnly)) {
                QJsonDocument document = QJsonDocument::fromJson(placeData.readAll());

                if (document.isObject()) {
                    QJsonObject o = document.object();

                    if (o.contains(QStringLiteral("categories"))) {
                        QJsonArray categories = o.value(QStringLiteral("categories")).toArray();

                        for (int i = 0; i < categories.count(); ++i) {
                            QJsonObject c = categories.at(i).toObject();

                            QPlaceCategory category;

                            category.setName(c.value(QStringLiteral("name")).toString());
                            category.setCategoryId(c.value(QStringLiteral("id")).toString());

                            m_categories.insert(category.categoryId(), category);

                            const QString parentId = c.value(QStringLiteral("parentId")).toString();
                            m_childCategories[parentId].append(category.categoryId());
                        }
                    }

                    if (o.contains(QStringLiteral("places"))) {
                        QJsonArray places = o.value(QStringLiteral("places")).toArray();

                        for (int i = 0; i < places.count(); ++i) {
                            QJsonObject p = places.at(i).toObject();

                            QPlace place;
                            if (p.value(QStringLiteral("alternateImplementation")).toBool(false)) {
                                place = QPlaceAlt();
                                QPlaceAttribute att;
                                att.setLabel(QStringLiteral("x_provider"));
                                att.setText(QStringLiteral("42")); // Doesn't matter, wont be used.
                                place.setExtendedAttribute(QStringLiteral("x_provider"), att);
                            }

                            place.setName(p.value(QStringLiteral("name")).toString());
                            place.setPlaceId(p.value(QStringLiteral("id")).toString());

                            QList<QPlaceCategory> categories;
                            QJsonArray ca = p.value(QStringLiteral("categories")).toArray();
                            for (int j = 0; j < ca.count(); ++j) {
                                QPlaceCategory c = m_categories.value(ca.at(j).toString());
                                if (!c.isEmpty())
                                    categories.append(c);
                            }
                            place.setCategories(categories);

                            QGeoCoordinate coordinate;
                            QJsonObject lo = p.value(QStringLiteral("location")).toObject();
                            coordinate.setLatitude(lo.value(QStringLiteral("latitude")).toDouble());
                            coordinate.setLongitude(lo.value(QStringLiteral("longitude")).toDouble());

                            QGeoLocation location;
                            location.setCoordinate(coordinate);

                            place.setLocation(location);

                            m_places.insert(place.placeId(), place);

                            QStringList recommendations;
                            QJsonArray ra = p.value(QStringLiteral("recommendations")).toArray();
                            for (int j = 0; j < ra.count(); ++j)
                                recommendations.append(ra.at(j).toString());
                            m_placeRecommendations.insert(place.placeId(), recommendations);

                            QJsonArray revArray = p.value(QStringLiteral("reviews")).toArray();
                            QList<QPlaceContent> reviews;
                            for (int j = 0; j < revArray.count(); ++j) {
                                QJsonObject ro = revArray.at(j).toObject();
                                QPlaceContent review(QPlaceContent::ReviewType);
                                if (ro.contains(QStringLiteral("title"))) {
                                    review.setValue(QPlaceContent::ReviewTitle,
                                                    ro.value(QStringLiteral("title")).toString());
                                }
                                if (ro.contains(QStringLiteral("text"))) {
                                    review.setValue(QPlaceContent::ReviewText,
                                                    ro.value(QStringLiteral("text")).toString());
                                }

                                if (ro.contains(QStringLiteral("language"))) {
                                    review.setValue(QPlaceContent::ReviewLanguage,
                                                    ro.value("language").toString());
                                }

                                if (ro.contains(QStringLiteral("rating"))) {
                                    review.setValue(QPlaceContent::ReviewRating,
                                                    ro.value("rating").toDouble());
                                }

                                if (ro.contains(QStringLiteral("dateTime"))) {
                                    const QString dtString =
                                        ro.value(QStringLiteral("dateTime")).toString();
                                    review.setValue(QPlaceContent::ReviewDateTime,
                                                    QDateTime::fromString(dtString,
                                                        QStringLiteral("hh:mm dd-MM-yyyy")));
                                }
                                if (ro.contains(QStringLiteral("reviewId"))) {
                                    review.setValue(QPlaceContent::ReviewId,
                                                    ro.value("reviewId").toString());
                                }

                                reviews << review;
                            }
                            m_placeReviews.insert(place.placeId(), reviews);

                            QJsonArray imgArray = p.value(QStringLiteral("images")).toArray();
                            QList<QPlaceContent> images;
                            for (int j = 0; j < imgArray.count(); ++j) {
                                QJsonObject imgo = imgArray.at(j).toObject();
                                QPlaceContent image(QPlaceContent::ImageType);
                                if (imgo.contains(QStringLiteral("url"))) {
                                    image.setValue(QPlaceContent::ImageUrl,
                                                  imgo.value(QStringLiteral("url")).toString());
                                }

                                if (imgo.contains("imageId")) {
                                    image.setValue(QPlaceContent::ImageId,
                                                imgo.value(QStringLiteral("imageId")).toString());
                                }

                                if (imgo.contains("mimeType")) {
                                    image.setValue(QPlaceContent::ImageMimeType,
                                                imgo.value(QStringLiteral("mimeType")).toString());
                                }

                                images << image;
                            }

                            m_placeImages.insert(place.placeId(), images);

                            QJsonArray edArray = p.value(QStringLiteral("editorials")).toArray();
                            QList<QPlaceContent> editorials;
                            for (int j = 0; j < edArray.count(); ++j) {
                                QJsonObject edo = edArray.at(j).toObject();
                                QPlaceContent editorial(QPlaceContent::EditorialType);
                                if (edo.contains(QStringLiteral("title"))) {
                                    editorial.setValue(QPlaceContent::EditorialTitle,
                                                    edo.value(QStringLiteral("title")).toString());
                                }

                                if (edo.contains(QStringLiteral("text"))) {
                                    editorial.setValue(QPlaceContent::EditorialText,
                                                    edo.value(QStringLiteral("text")).toString());
                                }

                                if (edo.contains(QStringLiteral("language"))) {
                                    editorial.setValue(QPlaceContent::EditorialLanguage,
                                                edo.value(QStringLiteral("language")).toString());
                                }

                                editorials << editorial;
                            }

                            m_placeEditorials.insert(place.placeId(), editorials);
                        }
                    }
                }
            }
        }
    }

    QPlaceDetailsReply *getPlaceDetails(const QString &placeId) override
    {
        DetailsReply *reply = new DetailsReply(this);

        if (placeId.isEmpty() || !m_places.contains(placeId)) {
            reply->setError(QPlaceReply::PlaceDoesNotExistError, tr("Place does not exist"));
            QMetaObject::invokeMethod(reply, "emitError", Qt::QueuedConnection);
        } else {
            reply->setPlace(m_places.value(placeId));
        }

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceContentReply *getPlaceContent(const QPlaceContentRequest &query) override
    {
        ContentReply *reply = new ContentReply(this);
        if (query.placeId().isEmpty() || !m_places.contains(query.placeId())) {
            reply->setError(QPlaceReply::PlaceDoesNotExistError, tr("Place does not exist"));
            QMetaObject::invokeMethod(reply, "emitError", Qt::QueuedConnection);

        } else {
                QPlaceContent::Collection collection;
                int totalCount = 0;
                switch (query.contentType()) {
                case QPlaceContent::ReviewType:
                    totalCount = m_placeReviews.value(query.placeId()).count();
                    break;
                case QPlaceContent::ImageType:
                    totalCount = m_placeImages.value(query.placeId()).count();
                    break;
                case QPlaceContent::EditorialType:
                    totalCount = m_placeEditorials.value(query.placeId()).count();
                default:
                    //do nothing
                    break;
                }

                QVariantMap context = query.contentContext().toMap();

                int offset = context.value(QStringLiteral("offset"), 0).toInt();
                int max = (query.limit() == -1) ? totalCount
                                                : qMin(offset + query.limit(), totalCount);
                for (int i = offset; i < max; ++i) {
                    switch (query.contentType()) {
                    case QPlaceContent::ReviewType:
                        collection.insert(i, m_placeReviews.value(query.placeId()).at(i));
                        break;
                    case QPlaceContent::ImageType:
                        collection.insert(i, m_placeImages.value(query.placeId()).at(i));
                        break;
                    case QPlaceContent::EditorialType:
                        collection.insert(i, m_placeEditorials.value(query.placeId()).at(i));
                    default:
                        //do nothing
                        break;
                    }
                }

                reply->setContent(collection);
                reply->setTotalCount(totalCount);

                if (max != totalCount) {
                    context.clear();
                    context.insert(QStringLiteral("offset"), offset + query.limit());
                    QPlaceContentRequest request = query;
                    request.setContentContext(context);
                    reply->setNextPageRequest(request);
                }
                if (offset > 0) {
                    context.clear();
                    context.insert(QStringLiteral("offset"), qMin(0, offset - query.limit()));
                    QPlaceContentRequest request = query;
                    request.setContentContext(context);
                    reply->setPreviousPageRequest(request);
                }
        }

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);
        return reply;
    }

    QPlaceSearchReply *search(const QPlaceSearchRequest &query) override
    {
        QList<QPlaceSearchResult> results;

        if (!query.searchTerm().isEmpty()) {
            for (const QPlace &place : m_places) {
                if (!place.name().contains(query.searchTerm(), Qt::CaseInsensitive))
                    continue;

                QPlaceResult r;
                r.setPlace(place);
                r.setTitle(place.name());

                results.append(r);
            }
        } else if (!query.categories().isEmpty()) {
            const auto &categoryList = query.categories();
            const QSet<QPlaceCategory> categories(categoryList.cbegin(), categoryList.cend());
            for (const QPlace &place : std::as_const(m_places)) {
                const auto &placeCategoryList = place.categories();
                const QSet<QPlaceCategory> placeCategories(placeCategoryList.cbegin(), placeCategoryList.cend());
                if (!placeCategories.intersects(categories))
                    continue;

                QPlaceResult r;
                r.setPlace(place);
                r.setTitle(place.name());

                results.append(r);
            }
        } else if (!query.recommendationId().isEmpty()) {
            const QStringList recommendations = m_placeRecommendations.value(query.recommendationId());
            for (const QString &id : recommendations) {
                QPlaceResult r;
                r.setPlace(m_places.value(id));
                r.setTitle(r.place().name());

                results.append(r);
            }
        }

        PlaceSearchReply *reply = new PlaceSearchReply(results, this);

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceSearchSuggestionReply *searchSuggestions(const QPlaceSearchRequest &query) override
    {
        QStringList suggestions;
        if (query.searchTerm() == QLatin1String("test")) {
            suggestions << QStringLiteral("test1");
            suggestions << QStringLiteral("test2");
            suggestions << QStringLiteral("test3");
        }

        SuggestionReply *reply = new SuggestionReply(suggestions, this);

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceIdReply *savePlace(const QPlace &place) override
    {
        IdReply *reply = new IdReply(QPlaceIdReply::SavePlace, this);

        if (!place.placeId().isEmpty() && !m_places.contains(place.placeId())) {
            reply->setError(QPlaceReply::PlaceDoesNotExistError, tr("Place does not exist"));
            QMetaObject::invokeMethod(reply, "emitError", Qt::QueuedConnection);
        } else if (!place.placeId().isEmpty()) {
            m_places.insert(place.placeId(), place);
            reply->setId(place.placeId());
        } else {
            QPlace p = place;
            p.setPlaceId(QUuid::createUuid().toString());
            m_places.insert(p.placeId(), p);

            reply->setId(p.placeId());
        }

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceIdReply *removePlace(const QString &placeId) override
    {
        IdReply *reply = new IdReply(QPlaceIdReply::RemovePlace, this);
        reply->setId(placeId);

        if (!m_places.contains(placeId)) {
            reply->setError(QPlaceReply::PlaceDoesNotExistError, tr("Place does not exist"));
            QMetaObject::invokeMethod(reply, "emitError", Qt::QueuedConnection);
        } else {
            m_places.remove(placeId);
        }

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceIdReply *saveCategory(const QPlaceCategory &category, const QString &parentId) override
    {
        IdReply *reply = new IdReply(QPlaceIdReply::SaveCategory, this);

        if ((!category.categoryId().isEmpty() && !m_categories.contains(category.categoryId())) ||
            (!parentId.isEmpty() && !m_categories.contains(parentId))) {
            reply->setError(QPlaceReply::CategoryDoesNotExistError, tr("Category does not exist"));
            QMetaObject::invokeMethod(reply, "emitError", Qt::QueuedConnection);
        } else if (!category.categoryId().isEmpty()) {
            m_categories.insert(category.categoryId(), category);
            QStringList children = m_childCategories.value(parentId);

            for (QStringList &c : m_childCategories)
                c.removeAll(category.categoryId());

            if (!children.contains(category.categoryId())) {
                children.append(category.categoryId());
                m_childCategories.insert(parentId, children);
            }
            reply->setId(category.categoryId());
        } else {
            QPlaceCategory c = category;
            c.setCategoryId(QUuid::createUuid().toString());
            m_categories.insert(c.categoryId(), c);
            QStringList children = m_childCategories.value(parentId);
            if (!children.contains(c.categoryId())) {
                children.append(c.categoryId());
                m_childCategories.insert(parentId, children);
            }

            reply->setId(c.categoryId());
        }

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceIdReply *removeCategory(const QString &categoryId) override
    {
        IdReply *reply = new IdReply(QPlaceIdReply::RemoveCategory, this);
        reply->setId(categoryId);

        if (!m_categories.contains(categoryId)) {
            reply->setError(QPlaceReply::CategoryDoesNotExistError, tr("Category does not exist"));
            QMetaObject::invokeMethod(reply, "emitError", Qt::QueuedConnection);
        } else {
            m_categories.remove(categoryId);

            for (auto &c : m_childCategories)
                c.removeAll(categoryId);
        }

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QPlaceReply *initializeCategories() override
    {
        QPlaceReply *reply = new PlaceReply(this);

        QMetaObject::invokeMethod(reply, "emitFinished", Qt::QueuedConnection);

        return reply;
    }

    QString parentCategoryId(const QString &categoryId) const override
    {
        for (auto i = m_childCategories.cbegin(), end = m_childCategories.cend(); i != end; ++i) {
            if (i.value().contains(categoryId))
                return i.key();
        }

        return QString();
    }

    QStringList childCategoryIds(const QString &categoryId) const override
    {
        return m_childCategories.value(categoryId);
    }

    QPlaceCategory category(const QString &categoryId) const override
    {
        return m_categories.value(categoryId);
    }

    QList<QPlaceCategory> childCategories(const QString &parentId) const override
    {
        QList<QPlaceCategory> categories;

        for (const QString &id : m_childCategories.value(parentId))
            categories.append(m_categories.value(id));

        return categories;
    }

    QList<QLocale> locales() const override
    {
        return m_locales;
    }

    void setLocales(const QList<QLocale> &locales) override
    {
        m_locales = locales;
    }

    QUrl constructIconUrl(const QPlaceIcon &icon, const QSize &size) const override
    {
        QList<QPair<int, QUrl> > candidates;

        QMap<QString, int> sizeDictionary;
        sizeDictionary.insert(QStringLiteral("s"), 20);
        sizeDictionary.insert(QStringLiteral("m"), 30);
        sizeDictionary.insert(QStringLiteral("l"), 50);

        const QStringList sizeKeys = { QStringLiteral("s"), QStringLiteral("m"), QStringLiteral("l") };
        for (const QString &sizeKey : sizeKeys) {
            if (icon.parameters().contains(sizeKey))
                candidates.append(QPair<int, QUrl>(sizeDictionary.value(sizeKey),
                                  icon.parameters().value(sizeKey).toUrl()));
        }

        if (candidates.isEmpty())
            return QUrl();
        else if (candidates.count() == 1) {
            return candidates.first().second;
        } else {
            //we assume icons are squarish so we can use height to
            //determine which particular icon to return
            int requestedHeight = size.height();

            for (int i = 0; i < candidates.count() - 1; ++i) {
                int thresholdHeight = (candidates.at(i).first + candidates.at(i+1).first) / 2;
                if (requestedHeight < thresholdHeight)
                    return candidates.at(i).second;
            }
            return candidates.last().second;
        }
    }

    QPlace compatiblePlace(const QPlace &original) const override
    {
        QPlace place;
        place.setName(original.name());
        return place;
    }

private:
    QList<QLocale> m_locales;
    QHash<QString, QPlace> m_places;
    QHash<QString, QPlaceCategory> m_categories;
    QHash<QString, QStringList> m_childCategories;
    QHash<QString, QStringList> m_placeRecommendations;
    QHash<QString, QList<QPlaceContent>> m_placeReviews;
    QHash<QString, QList<QPlaceContent>> m_placeImages;
    QHash<QString, QList<QPlaceContent>> m_placeEditorials;
};

#endif
