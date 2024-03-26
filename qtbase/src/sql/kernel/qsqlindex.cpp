// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsqlindex.h"

#include "qsqlfield.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QSqlIndex
    \brief The QSqlIndex class provides functions to manipulate and
    describe database indexes.

    \ingroup database
    \inmodule QtSql

    An \e index refers to a single table or view in a database.
    Information about the fields that comprise the index can be used
    to generate SQL statements.
*/

/*!
    Constructs an empty index using the cursor name \a cursorname and
    index name \a name.
*/

QSqlIndex::QSqlIndex(const QString& cursorname, const QString& name)
    : cursor(cursorname), nm(name)
{
}

/*!
    Constructs a copy of \a other.
*/

QSqlIndex::QSqlIndex(const QSqlIndex& other)
    : QSqlRecord(other), cursor(other.cursor), nm(other.nm), sorts(other.sorts)
{
}

/*! \fn QSqlIndex::QSqlIndex(QSqlIndex &&other)
    Move-constructs a new QSqlIndex from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 6.6
*/
/*! \fn QSqlIndex& QSqlIndex::operator=(QSqlIndex &&other)
    Move-assigns \a other to this QSqlIndex instance.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.

    \since 6.6
*/

/*!
    Sets the index equal to \a other.
*/

QSqlIndex& QSqlIndex::operator=(const QSqlIndex& other)
{
    cursor = other.cursor;
    nm = other.nm;
    sorts = other.sorts;
    QSqlRecord::operator=(other);
    return *this;
}


/*!
    Destroys the object and frees any allocated resources.
*/

QSqlIndex::~QSqlIndex()
{

}

/*!
    Sets the name of the index to \a name.
*/

void QSqlIndex::setName(const QString& name)
{
    nm = name;
}

/*!
    \fn QString QSqlIndex::name() const

    Returns the name of the index.
*/

/*!
    Appends the field \a field to the list of indexed fields. The
    field is appended with an ascending sort order.
*/

void QSqlIndex::append(const QSqlField& field)
{
    append(field, false);
}

/*!
    \overload

    Appends the field \a field to the list of indexed fields. The
    field is appended with an ascending sort order, unless \a desc is
    true.
*/

void QSqlIndex::append(const QSqlField& field, bool desc)
{
    sorts.append(desc);
    QSqlRecord::append(field);
}


/*!
    Returns \c true if field \a i in the index is sorted in descending
    order; otherwise returns \c false.
*/

bool QSqlIndex::isDescending(int i) const
{
    if (i >= 0 && i < sorts.size())
        return sorts[i];
    return false;
}

/*!
    If \a desc is true, field \a i is sorted in descending order.
    Otherwise, field \a i is sorted in ascending order (the default).
    If the field does not exist, nothing happens.
*/

void QSqlIndex::setDescending(int i, bool desc)
{
    if (i >= 0 && i < sorts.size())
        sorts[i] = desc;
}

/*! \internal

  Creates a string representing the field number \a i using prefix \a
  prefix. If \a verbose is true, ASC or DESC is included in the field
  description if the field is sorted in ASCending or DESCending order.
*/

QString QSqlIndex::createField(int i, const QString& prefix, bool verbose) const
{
    QString f;
    if (!prefix.isEmpty())
        f += prefix + u'.';
    f += field(i).name();
    if (verbose)
        f += u' ' + QString((isDescending(i) ? "DESC"_L1 : "ASC"_L1));
    return f;
}

/*!
    \fn QString QSqlIndex::cursorName() const

    Returns the name of the cursor which the index is associated with.
*/


/*!
    Sets the name of the cursor that the index is associated with to
    \a cursorName.
*/
void QSqlIndex::setCursorName(const QString& cursorName)
{
    cursor = cursorName;
}

QT_END_NAMESPACE
