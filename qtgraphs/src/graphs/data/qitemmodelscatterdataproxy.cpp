// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qitemmodelscatterdataproxy_p.h"
#include "scatteritemmodelhandler_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QItemModelScatterDataProxy
 * \inmodule QtGraphs
 * \brief Proxy class for presenting data in item models with Q3DScatter.
 *
 * QItemModelScatterDataProxy allows you to use QAbstractItemModel derived models as a data source
 * for Q3DScatter. It maps roles of QAbstractItemModel to the XYZ-values of Q3DScatter points.
 *
 * The data is resolved asynchronously whenever the mapping or the model changes.
 * QScatterDataProxy::arrayReset() is emitted when the data has been resolved. However, inserts,
 * removes, and single data item changes after the model initialization are resolved synchronously,
 * unless the same frame also contains a change that causes the whole model to be resolved.
 *
 * Mapping ignores rows and columns of the QAbstractItemModel and treats
 * all items equally. It requires the model to provide roles for the data items
 * that can be mapped to X, Y, and Z-values for the scatter points.
 *
 * For example, assume that you have a custom QAbstractItemModel for storing various measurements
 * done on material samples, providing data for roles such as "density", "hardness", and
 * "conductivity". You could visualize these properties on a scatter graph using this proxy:
 *
 * \snippet doc_src_qtgraphs.cpp 4
 *
 * If the fields of the model do not contain the data in the exact format you need, you can specify
 * a search pattern regular expression and a replace rule for each role to get the value in a
 * format you need. For more information how the replace using regular expressions works, see
 * QString::replace(const QRegularExpression &rx, const QString &after) function documentation. Note that
 * using regular expressions has an impact on the performance, so it's more efficient to utilize
 * item models where doing search and replace is not necessary to get the desired values.
 *
 * For example about using the search patterns in conjunction with the roles, see
 * ItemModelBarDataProxy usage in \l{Simple Bar Graph}.
 *
 * \sa {Qt Graphs Data Handling}
 */

/*!
 * \qmltype ItemModelScatterDataProxy
 * \inqmlmodule QtGraphs
 * \ingroup graphs_qml
 * \instantiates QItemModelScatterDataProxy
 * \inherits ScatterDataProxy
 * \brief Proxy class for presenting data in item models with Scatter3D.
 *
 * This type allows you to use AbstractItemModel derived models as a data source for Scatter3D.
 *
 * The data is resolved asynchronously whenever the mapping or the model changes.
 * QScatterDataProxy::arrayReset() is emitted when the data has been resolved.
 *
 * For more details, see QItemModelScatterDataProxy documentation.
 *
 * Usage example:
 *
 * \snippet doc_src_qmlgraphs.cpp 8
 *
 * \sa ScatterDataProxy, {Qt Graphs Data Handling}
 */

/*!
 * \qmlproperty model ItemModelScatterDataProxy::itemModel
 * The item model to use as a data source for Scatter3D.
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::xPosRole
 * The item model role to map into the X position.
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::yPosRole
 * The item model role to map into the Y position.
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::zPosRole
 * The item model role to map into the Z position.
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::rotationRole
 *
 * The item model role to map into item rotation.
 * The model may supply the value for rotation as either variant that is directly convertible
 * to \l [QtQuick] quaternion, or as one of the string representations: \c{"scalar,x,y,z"} or
 * \c{"@angle,x,y,z"}. The first format will construct the \l [QtQuick] quaternion directly with
 * given values, and the second one will construct the \l [QtQuick] quaternion using
 * QQuaternion::fromAxisAndAngle() method.
 */

/*!
 * \qmlproperty regExp ItemModelScatterDataProxy::xPosRolePattern
 *
 * When set, a search and replace is done on the value mapped by the x position
 * role before it is used as
 * an item position value. This property specifies the regular expression to find the portion of the
 * mapped value to replace and xPosRoleReplace property contains the replacement string.
 *
 * \sa xPosRole, xPosRoleReplace
 */

/*!
 * \qmlproperty regExp ItemModelScatterDataProxy::yPosRolePattern
 *
 * When set, a search and replace is done on the value mapped by the y position
 * role before it is used as
 * an item position value. This property specifies the regular expression to find the portion of the
 * mapped value to replace and yPosRoleReplace property contains the replacement string.
 *
 * \sa yPosRole, yPosRoleReplace
 */

/*!
 * \qmlproperty regExp ItemModelScatterDataProxy::zPosRolePattern
 *
 * When set, a search and replace is done on the value mapped by the z position
 * role before it is used as
 * an item position value. This property specifies the regular expression to find the portion of the
 * mapped value to replace and zPosRoleReplace property contains the replacement string.
 *
 * \sa zPosRole, zPosRoleReplace
 */

/*!
 * \qmlproperty regExp ItemModelScatterDataProxy::rotationRolePattern
 * When set, a search and replace is done on the value mapped by the rotation
 * role before it is used
 * as item rotation. This property specifies the regular expression to find the portion
 * of the mapped value to replace and rotationRoleReplace property contains the replacement string.
 *
 * \sa rotationRole, rotationRoleReplace
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::xPosRoleReplace
 *
 * This property defines the replace content to be used in conjunction with xPosRolePattern.
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa xPosRole, xPosRolePattern
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::yPosRoleReplace
 *
 * This property defines the replace content to be used in conjunction with yPosRolePattern.
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa yPosRole, yPosRolePattern
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::zPosRoleReplace
 *
 * This property defines the replace content to be used in conjunction with zPosRolePattern.
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa zPosRole, zPosRolePattern
 */

/*!
 * \qmlproperty string ItemModelScatterDataProxy::rotationRoleReplace
 * This property defines the replace content to be used in conjunction with rotationRolePattern.
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa rotationRole, rotationRolePattern
 */

/*!
 * Constructs QItemModelScatterDataProxy with optional \a parent.
 */
QItemModelScatterDataProxy::QItemModelScatterDataProxy(QObject *parent)
    : QScatterDataProxy(new QItemModelScatterDataProxyPrivate(this), parent)
{
    Q_D(QItemModelScatterDataProxy);
    d->connectItemModelHandler();
}

/*!
 * Constructs QItemModelScatterDataProxy with \a itemModel and optional \a parent. Proxy doesn't take
 * ownership of the \a itemModel, as typically item models are owned by other controls.
 */
QItemModelScatterDataProxy::QItemModelScatterDataProxy(QAbstractItemModel *itemModel,
                                                       QObject *parent)
    : QScatterDataProxy(new QItemModelScatterDataProxyPrivate(this), parent)
{
    Q_D(QItemModelScatterDataProxy);
    d->m_itemModelHandler->setItemModel(itemModel);
    d->connectItemModelHandler();
}

/*!
 * Constructs QItemModelScatterDataProxy with \a itemModel and optional \a parent. Proxy doesn't take
 * ownership of the \a itemModel, as typically item models are owned by other controls.
 * The xPosRole property is set to \a xPosRole, yPosRole property to \a yPosRole, and zPosRole property
 * to \a zPosRole.
 */
QItemModelScatterDataProxy::QItemModelScatterDataProxy(QAbstractItemModel *itemModel,
                                                       const QString &xPosRole,
                                                       const QString &yPosRole,
                                                       const QString &zPosRole,
                                                       QObject *parent)
    : QScatterDataProxy(new QItemModelScatterDataProxyPrivate(this), parent)
{
    Q_D(QItemModelScatterDataProxy);
    d->m_itemModelHandler->setItemModel(itemModel);
    d->m_xPosRole = xPosRole;
    d->m_yPosRole = yPosRole;
    d->m_zPosRole = zPosRole;
    d->connectItemModelHandler();
}

/*!
 * Constructs QItemModelScatterDataProxy with \a itemModel and optional \a parent. Proxy doesn't take
 * ownership of the \a itemModel, as typically item models are owned by other controls.
 * The xPosRole property is set to \a xPosRole, yPosRole property to \a yPosRole, zPosRole property
 * to \a zPosRole, and rotationRole property to \a rotationRole.
 */
QItemModelScatterDataProxy::QItemModelScatterDataProxy(QAbstractItemModel *itemModel,
                                                       const QString &xPosRole,
                                                       const QString &yPosRole,
                                                       const QString &zPosRole,
                                                       const QString &rotationRole,
                                                       QObject *parent)
    : QScatterDataProxy(new QItemModelScatterDataProxyPrivate(this), parent)
{
    Q_D(QItemModelScatterDataProxy);
    d->m_itemModelHandler->setItemModel(itemModel);
    d->m_xPosRole = xPosRole;
    d->m_yPosRole = yPosRole;
    d->m_zPosRole = zPosRole;
    d->m_rotationRole = rotationRole;
    d->connectItemModelHandler();
}

/*!
 * Destroys QItemModelScatterDataProxy.
 */
QItemModelScatterDataProxy::~QItemModelScatterDataProxy()
{
}

/*!
 * \property QItemModelScatterDataProxy::itemModel
 *
 * \brief The item model to use as a data source for a 3D scatter series.
 */

/*!
 * Sets \a itemModel as the item model for Q3DScatter. Does not take
 * ownership of the model, but does connect to it to listen for changes.
 */
void QItemModelScatterDataProxy::setItemModel(QAbstractItemModel *itemModel)
{
    Q_D(QItemModelScatterDataProxy);
    d->m_itemModelHandler->setItemModel(itemModel);
}

QAbstractItemModel *QItemModelScatterDataProxy::itemModel() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_itemModelHandler->itemModel();
}

/*!
 * \property QItemModelScatterDataProxy::xPosRole
 *
 * \brief The item model role to map into the X position.
 */
void QItemModelScatterDataProxy::setXPosRole(const QString &role)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_xPosRole != role) {
        d->m_xPosRole = role;
        emit xPosRoleChanged(role);
    }
}

QString QItemModelScatterDataProxy::xPosRole() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_xPosRole;
}

/*!
 * \property QItemModelScatterDataProxy::yPosRole
 *
 * \brief The item model role to map into the Y position.
 */
void QItemModelScatterDataProxy::setYPosRole(const QString &role)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_yPosRole != role) {
        d->m_yPosRole = role;
        emit yPosRoleChanged(role);
    }
}

QString QItemModelScatterDataProxy::yPosRole() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_yPosRole;
}

/*!
 * \property QItemModelScatterDataProxy::zPosRole
 *
 * \brief The item model role to map into the Z position.
 */
void QItemModelScatterDataProxy::setZPosRole(const QString &role)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_zPosRole != role) {
        d->m_zPosRole = role;
        emit zPosRoleChanged(role);
    }
}

QString QItemModelScatterDataProxy::zPosRole() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_zPosRole;
}

/*!
 * \property QItemModelScatterDataProxy::rotationRole
 *
 * \brief The item model role to map into item rotation.
 *
 * The model may supply the value for rotation as either variant that is directly convertible
 * to QQuaternion, or as one of the string representations: \c{"scalar,x,y,z"} or \c{"@angle,x,y,z"}.
 * The first will construct the quaternion directly with given values, and the second one will
 * construct the quaternion using QQuaternion::fromAxisAndAngle() method.
 */
void QItemModelScatterDataProxy::setRotationRole(const QString &role)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_rotationRole != role) {
        d->m_rotationRole = role;
        emit rotationRoleChanged(role);
    }
}

QString QItemModelScatterDataProxy::rotationRole() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_rotationRole;
}

/*!
 * \property QItemModelScatterDataProxy::xPosRolePattern
 *
 * \brief Whether search and replace is done on the value mapped by the x
 * position role before it is used as an item position value.
 *
 * This property specifies the regular expression to find the portion of the
 * mapped value to replace and xPosRoleReplace property contains the replacement string.
 *
 * \sa xPosRole, xPosRoleReplace
 */
void QItemModelScatterDataProxy::setXPosRolePattern(const QRegularExpression &pattern)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_xPosRolePattern != pattern) {
        d->m_xPosRolePattern = pattern;
        emit xPosRolePatternChanged(pattern);
    }
}

QRegularExpression QItemModelScatterDataProxy::xPosRolePattern() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_xPosRolePattern;
}

/*!
 * \property QItemModelScatterDataProxy::yPosRolePattern
 *
 * \brief Whether a search and replace is done on the value mapped by the
 * y position role before it is used as an item position value.
 *
 * This property specifies the regular expression to find the portion of the
 * mapped value to replace and yPosRoleReplace property contains the replacement string.
 *
 * \sa yPosRole, yPosRoleReplace
 */
void QItemModelScatterDataProxy::setYPosRolePattern(const QRegularExpression &pattern)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_yPosRolePattern != pattern) {
        d->m_yPosRolePattern = pattern;
        emit yPosRolePatternChanged(pattern);
    }
}

QRegularExpression QItemModelScatterDataProxy::yPosRolePattern() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_yPosRolePattern;
}

/*!
 * \property QItemModelScatterDataProxy::zPosRolePattern
 *
 * \brief Whether a search and replace is done on the value mapped by the z
 * position role before it is used as an item position value.
 *
 * This property specifies the regular expression to find the portion of the
 * mapped value to replace and zPosRoleReplace property contains the replacement string.
 *
 * \sa zPosRole, zPosRoleReplace
 */
void QItemModelScatterDataProxy::setZPosRolePattern(const QRegularExpression &pattern)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_zPosRolePattern != pattern) {
        d->m_zPosRolePattern = pattern;
        emit zPosRolePatternChanged(pattern);
    }
}

QRegularExpression QItemModelScatterDataProxy::zPosRolePattern() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_zPosRolePattern;
}

/*!
 * \property QItemModelScatterDataProxy::rotationRolePattern
 *
 * \brief Whether a search and replace is done on the value mapped by the
 * rotation role before it is used as item rotation.
 *
 * This property specifies the regular expression to find the portion
 * of the mapped value to replace and rotationRoleReplace property contains the replacement string.
 *
 * \sa rotationRole, rotationRoleReplace
 */
void QItemModelScatterDataProxy::setRotationRolePattern(const QRegularExpression &pattern)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_rotationRolePattern != pattern) {
        d->m_rotationRolePattern = pattern;
        emit rotationRolePatternChanged(pattern);
    }
}

QRegularExpression QItemModelScatterDataProxy::rotationRolePattern() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_rotationRolePattern;
}

/*!
 * \property QItemModelScatterDataProxy::xPosRoleReplace
 *
 * \brief The replace content to be used in conjunction with the x position role
 * pattern.
 *
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa xPosRole, xPosRolePattern
 */
void QItemModelScatterDataProxy::setXPosRoleReplace(const QString &replace)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_xPosRoleReplace != replace) {
        d->m_xPosRoleReplace = replace;
        emit xPosRoleReplaceChanged(replace);
    }
}

QString QItemModelScatterDataProxy::xPosRoleReplace() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_xPosRoleReplace;
}

/*!
 * \property QItemModelScatterDataProxy::yPosRoleReplace
 *
 * \brief The replace content to be used in conjunction with the y position role
 * pattern.
 *
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa yPosRole, yPosRolePattern
 */
void QItemModelScatterDataProxy::setYPosRoleReplace(const QString &replace)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_yPosRoleReplace != replace) {
        d->m_yPosRoleReplace = replace;
        emit yPosRoleReplaceChanged(replace);
    }
}

QString QItemModelScatterDataProxy::yPosRoleReplace() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_yPosRoleReplace;
}

/*!
 * \property QItemModelScatterDataProxy::zPosRoleReplace
 *
 * \brief The replace content to be used in conjunction with the z position role
 * pattern.
 *
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa zPosRole, zPosRolePattern
 */
void QItemModelScatterDataProxy::setZPosRoleReplace(const QString &replace)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_zPosRoleReplace != replace) {
        d->m_zPosRoleReplace = replace;
        emit zPosRoleReplaceChanged(replace);
    }
}

QString QItemModelScatterDataProxy::zPosRoleReplace() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_zPosRoleReplace;
}

/*!
 * \property QItemModelScatterDataProxy::rotationRoleReplace
 *
 * \brief The replace content to be used in conjunction with the rotation role
 * pattern.
 *
 * Defaults to an empty string. For more information on how the search and replace using regular
 * expressions works, see QString::replace(const QRegularExpression &rx, const QString &after)
 * function documentation.
 *
 * \sa rotationRole, rotationRolePattern
 */
void QItemModelScatterDataProxy::setRotationRoleReplace(const QString &replace)
{
    Q_D(QItemModelScatterDataProxy);
    if (d->m_rotationRoleReplace != replace) {
        d->m_rotationRoleReplace = replace;
        emit rotationRoleReplaceChanged(replace);
    }
}

QString QItemModelScatterDataProxy::rotationRoleReplace() const
{
    const Q_D(QItemModelScatterDataProxy);
    return d->m_rotationRoleReplace;
}

/*!
 * Changes \a xPosRole, \a yPosRole, \a zPosRole, and \a rotationRole mapping.
 */
void QItemModelScatterDataProxy::remap(const QString &xPosRole, const QString &yPosRole,
                                       const QString &zPosRole, const QString &rotationRole)
{
    setXPosRole(xPosRole);
    setYPosRole(yPosRole);
    setZPosRole(zPosRole);
    setRotationRole(rotationRole);
}

// QItemModelScatterDataProxyPrivate

QItemModelScatterDataProxyPrivate::QItemModelScatterDataProxyPrivate(QItemModelScatterDataProxy *q)
    : QScatterDataProxyPrivate(q),
      m_itemModelHandler(new ScatterItemModelHandler(q))
{
}

QItemModelScatterDataProxyPrivate::~QItemModelScatterDataProxyPrivate()
{
    delete m_itemModelHandler;
}

void QItemModelScatterDataProxyPrivate::connectItemModelHandler()
{
    Q_Q(QItemModelScatterDataProxy);
    QObject::connect(m_itemModelHandler, &ScatterItemModelHandler::itemModelChanged,
                     q, &QItemModelScatterDataProxy::itemModelChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::xPosRoleChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::yPosRoleChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::zPosRoleChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::rotationRoleChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::xPosRolePatternChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::yPosRolePatternChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::zPosRolePatternChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::rotationRolePatternChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::xPosRoleReplaceChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::yPosRoleReplaceChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::zPosRoleReplaceChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
    QObject::connect(q, &QItemModelScatterDataProxy::rotationRoleReplaceChanged,
                     m_itemModelHandler, &AbstractItemModelHandler::handleMappingChanged);
}

QT_END_NAMESPACE
