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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QtCanvas3D API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef CONTEXTATTRIBUTES_P_H
#define CONTEXTATTRIBUTES_P_H

#include "abstractobject3d_p.h"

#include <QObject>

QT_BEGIN_NAMESPACE
QT_CANVAS3D_BEGIN_NAMESPACE

class CanvasContextAttributes : public CanvasAbstractObject
{
    Q_OBJECT

    Q_PROPERTY(bool alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)
    Q_PROPERTY(bool depth READ depth WRITE setDepth NOTIFY depthChanged)
    Q_PROPERTY(bool stencil READ stencil WRITE setStencil NOTIFY stencilChanged)
    Q_PROPERTY(bool antialias READ antialias WRITE setAntialias NOTIFY antialiasChanged)
    Q_PROPERTY(bool premultipliedAlpha READ premultipliedAlpha WRITE setPremultipliedAlpha NOTIFY premultipliedAlphaChanged)
    Q_PROPERTY(bool preserveDrawingBuffer READ preserveDrawingBuffer WRITE setPreserveDrawingBuffer NOTIFY preserveDrawingBufferChanged)
    Q_PROPERTY(bool preferLowPowerToHighPerformance READ preferLowPowerToHighPerformance WRITE setPreferLowPowerToHighPerformance NOTIFY preferLowPowerToHighPerformanceChanged)
    Q_PROPERTY(bool failIfMajorPerformanceCaveat READ failIfMajorPerformanceCaveat WRITE setFailIfMajorPerformanceCaveat NOTIFY failIfMajorPerformanceCaveatChanged)

public:
    explicit CanvasContextAttributes(QObject *parent = 0);
    virtual ~CanvasContextAttributes();

    void setFrom(const QVariantMap &options);
    void setFrom(const CanvasContextAttributes &source);
    bool alpha() const;
    void setAlpha(bool value);
    bool depth() const;
    void setDepth(bool value);
    bool stencil() const;
    void setStencil(bool value);
    bool antialias() const;
    void setAntialias(bool value);
    bool premultipliedAlpha() const;
    void setPremultipliedAlpha(bool value);
    bool preserveDrawingBuffer() const;
    void setPreserveDrawingBuffer(bool value);
    bool preferLowPowerToHighPerformance() const;
    void setPreferLowPowerToHighPerformance(bool value);
    bool failIfMajorPerformanceCaveat() const;
    void setFailIfMajorPerformanceCaveat(bool value);

    friend QDebug operator<< (QDebug d, const CanvasContextAttributes &attribs);

signals:
    void alphaChanged(bool newValue);
    void depthChanged(bool newValue);
    void stencilChanged(bool newValue);
    void antialiasChanged(bool newValue);
    void premultipliedAlphaChanged(bool newValue);
    void preserveDrawingBufferChanged(bool newValue);
    void preferLowPowerToHighPerformanceChanged(bool newValue);
    void failIfMajorPerformanceCaveatChanged(bool newValue);

private:
    bool m_alpha;
    bool m_depth;
    bool m_stencil;
    bool m_antialias;
    bool m_premultipliedAlpha;
    bool m_preserveDrawingBuffer;
    bool m_preferLowPowerToHighPerformance;
    bool m_failIfMajorPerformanceCaveat;
};

QT_CANVAS3D_END_NAMESPACE
QT_END_NAMESPACE

#endif // QCONTEXTATTRIBUTES_P_H
