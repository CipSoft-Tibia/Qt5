// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "q3dobject_p.h"
#include "q3dscene_p.h"

QT_BEGIN_NAMESPACE

/*!
   \class Q3DObject
   \inmodule QtGraphs
   \brief The Q3DObject class is a simple base class for all the objects in a
   3D scene.

    Contains position information for an object in a 3D scene.
    The object is considered to be a single point in the coordinate space without dimensions.
*/

/*!
   \qmltype Object3D
   \inqmlmodule QtGraphs
   \ingroup graphs_qml
   \instantiates Q3DObject
   \brief A base type for all the objects in a 3D scene.

    An uncreatable base type that contains position information for an object in
    a 3D scene. The object is considered to be a single point in the coordinate space without
    dimensions.
*/

/*!
 * \qmlproperty vector3d Object3D::position
 *
 * The 3D position of the object.
 *
 * \note Currently setting this property has no effect for Camera3D, as the position is handled
 * internally.
 */

/*!
 * \internal
 */
Q3DObject::Q3DObject(Q3DObjectPrivate *d, QObject *parent) :
    QObject(parent),
    d_ptr(d)
{
}

/*!
 * Constructs a new 3D object with the position set to origin by default. An
 * optional \a parent parameter can be given and is then passed to the QObject
 * constructor.
 */
Q3DObject::Q3DObject(QObject *parent) :
    QObject(parent)
{
}

/*!
 *  Destroys the 3D object.
 */
Q3DObject::~Q3DObject()
{
}

/*!
 * Copies the 3D object position from the given \a source 3D object to this 3D object instance.
 */
void Q3DObject::copyValuesFrom(const Q3DObject &source)
{
    Q_D(Q3DObject);
    d->m_position = source.d_func()->m_position;
    setDirty(true);
}

/*!
 * \property Q3DObject::parentScene
 *
 * \brief The parent scene as a read only value.
 *
 * If the object has no parent scene, the value is 0.
 */
Q3DScene *Q3DObject::parentScene()
{
    return qobject_cast<Q3DScene *>(parent());
}

/*!
 * \property Q3DObject::position
 *
 * \brief The 3D position of the object.
 *
 * \note Currently setting this property has no effect for Q3DCamera, as the position is handled
 * internally.
 */
QVector3D Q3DObject::position() const
{
    const Q_D(Q3DObject);
    return d->m_position;
}

void Q3DObject::setPosition(const QVector3D &position)
{
    Q_D(Q3DObject);
    if (d->m_position != position) {
        d->m_position = position;
        setDirty(true);
        emit positionChanged(d->m_position);
    }
}

/*!
 * Sets \a dirty to \c true if the 3D object has changed since the last update.
 */
void Q3DObject::setDirty(bool dirty)
{
    Q_D(Q3DObject);
    d->m_isDirty = dirty;
    if (parentScene())
        parentScene()->d_func()->markDirty();
}

/*!
 * Returns whether the 3D object has changed.
 */
bool Q3DObject::isDirty() const
{
    const Q_D(Q3DObject);
    return d->m_isDirty;
}

Q3DObjectPrivate::Q3DObjectPrivate(Q3DObject *q) :
    m_isDirty(true),
    q_ptr(q)
{
}

Q3DObjectPrivate::~Q3DObjectPrivate()
{

}

QT_END_NAMESPACE
