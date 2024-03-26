// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSQLINDEX_H
#define QSQLINDEX_H

#include <QtSql/qtsqlglobal.h>
#include <QtSql/qsqlrecord.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE


class Q_SQL_EXPORT QSqlIndex : public QSqlRecord
{
public:
    explicit QSqlIndex(const QString &cursorName = QString(), const QString &name = QString());
    QSqlIndex(const QSqlIndex &other);
    QSqlIndex(QSqlIndex &&other) noexcept = default;
    ~QSqlIndex();
    QSqlIndex &operator=(const QSqlIndex &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QSqlIndex)

    void swap(QSqlIndex &other) noexcept {
        QSqlRecord::swap(other);
        cursor.swap(other.cursor);
        nm.swap(other.nm);
        sorts.swap(other.sorts);
    };

    void setCursorName(const QString &cursorName);
    inline QString cursorName() const { return cursor; }
    void setName(const QString& name);
    inline QString name() const { return nm; }

    void append(const QSqlField &field);
    void append(const QSqlField &field, bool desc);

    bool isDescending(int i) const;
    void setDescending(int i, bool desc);

private:
    QString createField(int i, const QString& prefix, bool verbose) const;
    // ### Qt7: move to d-ptr
    QString cursor;
    QString nm;
    QList<bool> sorts;
};

Q_DECLARE_SHARED(QSqlIndex)

QT_END_NAMESPACE

#endif // QSQLINDEX_H
