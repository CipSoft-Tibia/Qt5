// Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT3DRENDER_QSHADERGRAPHLOADER_P_H
#define QT3DRENDER_QSHADERGRAPHLOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DRender/private/qt3drender_global_p.h>

#include <Qt3DRender/private/qshadergraph_p.h>

QT_BEGIN_NAMESPACE

class QIODevice;
namespace Qt3DRender
{

class QShaderGraphLoader
{
public:
    enum Status : char {
        Null,
        Waiting,
        Ready,
        Error
    };

    Q_3DRENDERSHARED_PRIVATE_EXPORT QShaderGraphLoader() noexcept;

    Q_3DRENDERSHARED_PRIVATE_EXPORT Status status() const noexcept;
    Q_3DRENDERSHARED_PRIVATE_EXPORT QShaderGraph graph() const noexcept;

    Q_3DRENDERSHARED_PRIVATE_EXPORT QIODevice *device() const noexcept;
    Q_3DRENDERSHARED_PRIVATE_EXPORT void setDevice(QIODevice *device) noexcept;

    Q_3DRENDERSHARED_PRIVATE_EXPORT QHash<QString, QShaderNode> prototypes() const noexcept;
    Q_3DRENDERSHARED_PRIVATE_EXPORT void setPrototypes(const QHash<QString, QShaderNode> &prototypes) noexcept;

    Q_3DRENDERSHARED_PRIVATE_EXPORT void load();

private:
    Status m_status;
    QIODevice *m_device;
    QHash<QString, QShaderNode> m_prototypes;
    QShaderGraph m_graph;
};


}
Q_DECLARE_TYPEINFO(Qt3DRender::QShaderGraphLoader, Q_RELOCATABLE_TYPE);
QT_END_NAMESPACE

Q_DECLARE_METATYPE(Qt3DRender::QShaderGraphLoader)
Q_DECLARE_METATYPE(Qt3DRender::QShaderGraphLoader::Status)

#endif // QT3DRENDER_QSHADERGRAPHLOADER_P_H
