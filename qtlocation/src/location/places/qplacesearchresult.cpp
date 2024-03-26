// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplacesearchresult.h"
#include "qplacesearchresult_p.h"
#include "qplaceresult.h"
#include <QtCore/qnumeric.h>

QT_USE_NAMESPACE

template<> QPlaceSearchResultPrivate *QSharedDataPointer<QPlaceSearchResultPrivate>::clone()
{
    return d->clone();
}

inline QPlaceSearchResultPrivate *QPlaceSearchResult::d_func()
{
    return static_cast<QPlaceSearchResultPrivate *>(d_ptr.data());
}

inline const QPlaceSearchResultPrivate *QPlaceSearchResult::d_func() const
{
    return static_cast<const QPlaceSearchResultPrivate *>(d_ptr.constData());
}

bool QPlaceSearchResultPrivate::compare(const QPlaceSearchResultPrivate *other) const
{
    return title == other->title
            && icon == other->icon;
}

/*!
    \class QPlaceSearchResult
    \inmodule QtLocation
    \ingroup QtLocation-places
    \ingroup QtLocation-places-data
    \since 5.6

    \brief The QPlaceSearchResult class is the base class for search results.

    A list of search results can be retrieved from the QPlaceSearchReply after it has
    successfully completed the request.  Common to all search results are the
    \l {QPlaceSearchResult::title()} {title} and \l {QPlaceSearchResult::icon()}{icon},
    which can be used to present the search result to the user.

    The intended usage is that depending  on the \l {QPlaceSearchResult::type()} {type},
    the search result can be converted to a more detailed subclass like so:

    \snippet places/requesthandler.h Convert search result

    The implementation is handled in such a way that object slicing is not an issue.
    It is not expected that client applications or backend plugins instantiate
    a QPlaceSearchResult directly, but rather client applications simply convert
    to search result subclasses and backend plugins only instantiate subclasses.

    \sa QPlaceResult
*/

/*!
    \enum QPlaceSearchResult::SearchResultType

    Defines the type of search result

    \value UnknownSearchResult The contents of the search result are unknown.
    \value PlaceResult The search result contains a place.
    \value ProposedSearchResult The search result contains a proposed search which may be relevant.
*/

/*!
    Constructs a new search result.
*/
QPlaceSearchResult::QPlaceSearchResult()
    : d_ptr(new QPlaceSearchResultPrivate)
{
}

/*!
    Constructs a copy of \a other
*/
QPlaceSearchResult::QPlaceSearchResult(const QPlaceSearchResult &other)
    :d_ptr(other.d_ptr)
{
}

/*!
    Destroys the search result.
*/
QPlaceSearchResult::~QPlaceSearchResult()
{
}

/*!
    Assigns \a other to this search result and returns a reference to this
    search result.
*/
QPlaceSearchResult &QPlaceSearchResult::operator =(const QPlaceSearchResult &other)
{
    if (this == &other)
        return *this;

    d_ptr = other.d_ptr;
    return *this;
}

/*!
    Returns true if \a other is equal to this search result, otherwise
    returns false.
*/
bool QPlaceSearchResult::operator==(const QPlaceSearchResult &other) const
{
    // An unknown object is only equal to another unknown search result
    if (!d_ptr)
        return !other.d_ptr;

    if (type() != other.type())
        return false;

    return d_ptr->compare(other.d_ptr);
}

/*!
    \fn bool QPlaceSearchResult::operator!=(const QPlaceSearchResult &other) const
    Returns true if \a other not equal to this search result, otherwise
    returns false.
*/

/*!
    Returns the result type.
*/
QPlaceSearchResult::SearchResultType QPlaceSearchResult::type() const
{
    if (!d_ptr)
        return UnknownSearchResult;
    return d_ptr->type();
}

/*!
    Returns the title of the search result.  This string can be used to display the search result
    to the user.
*/
QString QPlaceSearchResult::title() const
{
    Q_D(const QPlaceSearchResult);
    return d->title;
}

/*!
    Sets the title of the search result to \a title.
*/
void QPlaceSearchResult::setTitle(const QString &title)
{
    Q_D(QPlaceSearchResult);
    d->title = title;
}

/*!
    Returns an icon that can be used to represent the search result.
*/
QPlaceIcon QPlaceSearchResult::icon() const
{
    Q_D(const QPlaceSearchResult);
    return d->icon;
}

/*!
    Sets the icon of the search result to \a icon.
*/
void QPlaceSearchResult::setIcon(const QPlaceIcon &icon)
{
    Q_D(QPlaceSearchResult);
    d->icon = icon;
}

/*!
    \internal
    Constructs a new search result from the given pointer \a d.
*/
QPlaceSearchResult::QPlaceSearchResult(QPlaceSearchResultPrivate *d)
    :d_ptr(d)
{
}
