// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q3DOBJECT_H
#define Q3DOBJECT_H

#include <QtGraphs/qgraphsglobal.h>
#include <QtCore/QObject>
#include <QtGui/QVector3D>

Q_MOC_INCLUDE(<QtGraphs/q3dscene.h>)

QT_BEGIN_NAMESPACE

class Q3DObjectPrivate;
class Q3DScene;

class Q_GRAPHS_EXPORT Q3DObject : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Q3DObject)
    Q_PROPERTY(Q3DScene* parentScene READ parentScene CONSTANT)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)

protected:
    explicit Q3DObject(Q3DObjectPrivate *d, QObject *parent = nullptr);

public:
    explicit Q3DObject(QObject *parent = nullptr);
    virtual ~Q3DObject();

    virtual void copyValuesFrom(const Q3DObject &source);

    Q3DScene *parentScene();

    QVector3D position() const;
    void setPosition(const QVector3D &position);

Q_SIGNALS:
    void positionChanged(const QVector3D &position);

protected:
    QScopedPointer<Q3DObjectPrivate> d_ptr;

    void setDirty(bool dirty);
    bool isDirty() const;

private:
    Q_DISABLE_COPY(Q3DObject)
};

QT_END_NAMESPACE

#endif
