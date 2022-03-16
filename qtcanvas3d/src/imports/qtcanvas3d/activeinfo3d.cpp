/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCanvas3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "activeinfo3d_p.h"

QT_BEGIN_NAMESPACE
QT_CANVAS3D_BEGIN_NAMESPACE

/*!
 * \qmltype Canvas3DActiveInfo
 * \since QtCanvas3D 1.0
 * \inqmlmodule QtCanvas3D
 * \brief Active attribute or uniform information.
 * \deprecated
 *
 * \b{Deprecated in Qt 5.12.} An uncreatable QML type that contains the
 * information returned from the Context3D::getActiveAttrib() and
 * Context3D::getActiveUniform() calls.
 *
 * \sa Context3D, Canvas3D
 */

CanvasActiveInfo::CanvasActiveInfo(int size, CanvasContext::glEnums type,
                                   QString name, QObject *parent) :
    QObject(parent),
    m_size(size),
    m_type(type),
    m_name(name)
{
}

/*!
 * \qmlproperty string Canvas3DActiveInfo::name
 * \b{Deprecated in Qt 5.12.} Name of the attribute or the uniform.
 */
const QString CanvasActiveInfo::name() const
{
    return m_name;
}

/*!
 * \qmlproperty string Canvas3DActiveInfo::size
 * \b{Deprecated in Qt 5.12.} Size of the attribute or the uniform, in units of
 * its type.
 *
 * \sa type
 */
int CanvasActiveInfo::size() const
{
    return m_size;
}

/*!
 * \qmlproperty string Canvas3DActiveInfo::type
 * \b{Deprecated in Qt 5.12.} Type of the attribute or the uniform.
 */
CanvasContext::glEnums CanvasActiveInfo::type() const
{
    return m_type;
}

QT_CANVAS3D_END_NAMESPACE
QT_END_NAMESPACE
